// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/NativeExecutionBindings.h"
#include <chrono>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace Aeostara {
namespace {
class NativeClock final : public IDiagnosticClock {
public:
    std::string utcNow() const override {
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::tm value{}; if (gmtime_s(&value, &now) != 0) throw std::runtime_error("Actual UTC unavailable");
        std::ostringstream text; text << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ"); return text.str();
    }
    std::uint64_t monotonicMilliseconds() const override {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
    }
};
class RejectedOperation final : public std::runtime_error {
public:
    explicit RejectedOperation(OperationFailure value) : std::runtime_error(value.message), failure(std::move(value)) {}
    OperationFailure failure;
};
template<class T> T actual(OperationAttempt<T> result) {
    if (const auto* failure = std::get_if<OperationFailure>(&result)) throw RejectedOperation(*failure);
    return std::get<T>(std::move(result));
}
Json reference(const Json& envelope) {
    return Json{{"diagnosticID", envelope.at("diagnosticID")}, {"chainRootReference", envelope.at("chainRootReference")},
        {"diagnosticKind", envelope.at("diagnosticKind")}, {"ruleReferences", envelope.at("ruleReferences")}};
}
OperationFailure pureFailure(OperationPhase phase, OperationCode code, const std::exception& cause, const Json& input) {
    auto result = operationFailureFromException(phase, code, cause);
    try {
        const auto hash = hashCompleteSemanticInput(input);
        const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md";
        const auto bundle = makeAshDiagnosticBundle(AshDiagnosticContext{"STATE_VALIDITY", "DETECTION", "BLOCKED", hash, {cause.what()}}, "ERROR", cause.what(),
            Json{{"actualPhase", to_string(phase)}, {"inputHash", hash}, {"failureEvidence", result.evidence}},
            Json::array({Json{{"ruleID", "ASH-STATE-STRUCTURE-001"}, {"ruleFamily", "ASH-STATE"}, {"sourceReference", source}}}), {source});
        result.diagnosticChain = bundle.chain; result.unavailableEvidence.clear(); result.diagnosticPersistence = DiagnosticPersistence::NotRequested;
    } catch (const std::exception& error) { result.addSecondary(OperationPhase::Export, OperationCode::DiagnosticUnavailable, error.what()); }
    return result;
}
DiagnosticBundle planningDiagnostic(const SemanticPlan& plan, const Json& input) {
    DiagnosticBundle result;
    for (const std::string phase : {"OBSERVE", "NORMALIZE", "PROJECT", "DIAGNOSE", "CLASSIFY", "PLAN"}) {
        const bool classified = phase == "CLASSIFY" || phase == "PLAN";
        const bool diagnosed = phase == "DIAGNOSE";
        const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/" +
            std::string(classified ? "system-state-classification.pseudo.md" : diagnosed ? "state-validity-diagnostics.pseudo.md" : "ash-state-space.pseudo.md");
        const std::string rule = classified ? "ASH-CLASSIFICATION-MAPPING-001" : diagnosed ? "ASH-STATE-VALIDITY-001" : "ASH-STATE-STRUCTURE-001";
        const std::string message = "Actual immutable semantic calculation completed: " + phase;
        AshDiagnosticContext context{"STATE_VALIDITY", classified ? "CLASSIFICATION" : "DETECTION", "RESOLVED", plan.planHash, {message}};
        result = makeAshDiagnosticBundle(context, "INFO", message, Json{{"actualPhase", phase}, {"planHash", plan.planHash},
            {"inputHash", hashCompleteSemanticInput(input)}, {"rawHash", plan.rawPreimageHash}, {"projectedCoordinates", stateJson(plan.analysis.projection.state)},
            {"stateClass", to_string(plan.analysis.stateClass)}, {"recoveryCategory", to_string(plan.analysis.recovery)},
            {"witnessID", plan.analysis.witnessID}, {"operationCount", plan.operations.size()}},
            Json::array({Json{{"ruleID", rule}, {"ruleFamily", classified ? "ASH-CLASSIFICATION" : "ASH-STATE"}, {"sourceReference", source}}}),
            {source}, result.chain.is_null() ? nullptr : &result.chain);
    }
    return result;
}
Json frozenState(const AshState& state, const std::string& source) {
    const auto coordinates = stateJson(state); const auto hash = sha256(coordinates.dump());
    return Json{{"stateID", "state-" + hash}, {"coordinates", coordinates}, {"sourceReference", source}, {"canonicalHash", hash}};
}
BaseContracts::VerificationPlan verificationPlan(const SemanticPlan& plan, const DiagnosticBundle& diagnostic) {
    const auto expected = plan.analysis.selectedTarget.value_or(plan.analysis.projection.state);
    Json checks = Json::array({Json{{"check", "EXACT_AUTHORIZED_DOCUMENT"}, {"canonicalHash", sha256(plan.expectedDocument.dump())}},
        Json{{"check", "SAME_PROFILE_NINE_COORDINATE_REPROJECTION"}, {"completeInputHash", plan.analysis.projection.completeSemanticInputHash}},
        Json{{"check", "VALID_STABLE_TARGET_AND_MANDATORY_POSTCONDITIONS"}, {"semanticDependencyHash", plan.semanticDependencyHash}}});
    return BaseContracts::VerificationPlan(Json{{"verificationPlanID", "verification-" + sha256(checks.dump())}, {"checks", checks},
        {"expectedSemanticState", frozenState(expected, "native-plan:" + plan.planHash)},
        {"generatedBeforeMutation", true}, {"diagnosticReference", diagnostic.reference}});
}
bool resolves(const Json& chain, const Json& ref) {
    if (chain.is_null() || chain.at("chainID") != ref.at("chainRootReference")) return false;
    for (const auto& envelope : chain.at("diagnostics").array_items()) if (reference(envelope) == ref) return true;
    return false;
}
DiagnosticPhaseType phaseType(OperationPhase phase) {
    switch (phase) {
    case OperationPhase::Normalize: return DiagnosticPhaseType::Normalization;
    case OperationPhase::Project: return DiagnosticPhaseType::Projection;
    case OperationPhase::Diagnose: return DiagnosticPhaseType::Diagnosis;
    case OperationPhase::Classify: return DiagnosticPhaseType::Recoverability;
    case OperationPhase::Plan: return DiagnosticPhaseType::Planning;
    case OperationPhase::Policy: return DiagnosticPhaseType::Policy;
    case OperationPhase::Backup: return DiagnosticPhaseType::Backup;
    case OperationPhase::Execute: return DiagnosticPhaseType::Execution;
    case OperationPhase::Verify: return DiagnosticPhaseType::Verification;
    case OperationPhase::Rollback: return DiagnosticPhaseType::Recovery;
    case OperationPhase::Contain: return DiagnosticPhaseType::Containment;
    case OperationPhase::Audit: return DiagnosticPhaseType::Completion;
    case OperationPhase::Export: return DiagnosticPhaseType::Export;
    default: return DiagnosticPhaseType::Observation;
    }
}
DiagnosticError retainedError(OperationPhase phase, OperationCode code, const Json& evidence) {
    DiagnosticError result;
    if (code == OperationCode::InvalidInput) result.kind = DiagnosticErrorType::Input;
    else if (code == OperationCode::Ambiguous || code == OperationCode::UnsupportedEvidence) result.kind = DiagnosticErrorType::Mapping;
    else if (code == OperationCode::PolicyRejected) result.kind = DiagnosticErrorType::Policy;
    else if (code == OperationCode::StorageFailure) result.kind = DiagnosticErrorType::Storage;
    else if (code == OperationCode::StaleState) result.kind = DiagnosticErrorType::Stale;
    else if (code == OperationCode::VerificationFailed) result.kind = DiagnosticErrorType::Verification;
    else if (code == OperationCode::RestorationFailed || phase == OperationPhase::Rollback) result.kind = DiagnosticErrorType::Recovery;
    else if (phase == OperationPhase::Audit) result.kind = DiagnosticErrorType::Audit;
    else if (phase == OperationPhase::Policy) result.kind = DiagnosticErrorType::Policy;
    else if (phase == OperationPhase::Project) result.kind = DiagnosticErrorType::Mapping;
    else result.kind = DiagnosticErrorType::Execution;
    if (evidence.is_object() && evidence.contains("nativeError") && evidence.at("nativeError").contains("value")) {
        result.nativeCode = evidence.at("nativeError").at("value").get<std::int64_t>();
        if (code == OperationCode::Failed && (phase == OperationPhase::Parse || phase == OperationPhase::Backup)) result.kind = DiagnosticErrorType::Storage;
    }
    return result;
}
}

struct NativeExecutionSession::State final {
    std::optional<NativePlanningResult> planning;
    std::optional<BaseContracts::PolicyDecision> policy;
    std::optional<BaseContracts::BackupResult> backup;
    std::optional<BaseContracts::BackupRecord> copySourceRecord;
    std::optional<BaseContracts::ExecutionContext> context;
    std::optional<BaseContracts::VerificationResult> verification;
    std::optional<BaseContracts::RollbackResult> rollback;
    std::optional<BaseContracts::ExecutionStepResult> lastExecution;
    std::optional<BaseContracts::HealResult> healResult;
    std::optional<OperationFailure> failure;
    std::vector<OperationFault> diagnosticErrors;
    std::uint64_t diagnosticErrorsDropped = 0;
    std::unique_ptr<ITargetWriteLease> lease;
    std::thread::id owner;
    std::string identity, auditIdentity, version, original, backupPath, backupIdentity, rawAfter, copySourcePath, copySourceIdentity;
    Json chain, auditChain, intermediate;
    std::size_t executed = 0;
    bool armed = false, complete = false, auditPersisted = false, auditExisted = false;
    MutationState mutation = MutationState::NotStarted;
};

NativeExecutionSession::NativeExecutionSession(SemanticEnvironment environment, Json input,
    std::string path, std::string createdAt, std::shared_ptr<Aeostara::IFileSystem> fs,
    std::shared_ptr<Aeostara::IConfigAdapter> adapter, std::shared_ptr<Aeostara::IBackupProvider> backup,
    std::shared_ptr<BaseBindings::BaseAuditSink> audit, std::shared_ptr<IDiagnostics> diagnostics,
    std::shared_ptr<BaseBindings::IPolicyGate> policy, std::shared_ptr<IDiagnosticClock> clock, bool retainPersistenceCopySource)
    : state_(std::make_unique<State>()), environment_(std::move(environment)), input_(std::move(input)),
      path_(std::move(path)), createdAt_(std::move(createdAt)), fs_(std::move(fs)), adapter_(std::move(adapter)),
      backup_(std::move(backup)), audit_(std::move(audit)), diagnostics_(std::move(diagnostics)),
      hostPolicy_(std::move(policy)), clock_(std::move(clock)), retainPersistenceCopySource_(retainPersistenceCopySource) {
    if (path_.empty() || createdAt_.empty() || !fs_ || !adapter_ || !backup_ || !audit_) throw std::invalid_argument("Actual execution dependency/path/time unavailable");
    validateCompleteSemanticInput(input_); if (!diagnostics_) diagnostics_ = createDefaultDiagnostics(); if (!clock_) clock_ = std::make_shared<NativeClock>();
}
NativeExecutionSession::~NativeExecutionSession() {
    if (!state_->armed || state_->complete || state_->failure) return;
    try { std::lock_guard lock(mutex_); (void)failAndRestore(OperationPhase::Execute, OperationCode::Failed, "Execution session abandoned before verified completion"); }
    catch (...) { /* Caller uses outcome/complete to observe failures before destruction. */ }
}

OperationAttempt<NativePlanningResult> NativeExecutionSession::planCaptured() const {
    try {
        auto computed = environment_.plan(input_); if (const auto* error = std::get_if<OperationFailure>(&computed)) return *error;
        auto plan = std::get<SemanticPlan>(std::move(computed));
        const auto diagnostic = planningDiagnostic(plan, input_);
        auto bound = makeBoundPolicyContracts(plan, input_, diagnostic, createdAt_);
        auto verification = verificationPlan(plan, diagnostic);
        return NativePlanningResult{std::move(plan), std::move(bound.plan), std::move(bound.bundle),
            std::move(verification), BaseContracts::DiagnosticChain(diagnostic.chain)};
    } catch (const std::exception& error) { return pureFailure(OperationPhase::Plan, OperationCode::Failed, error, input_); }
}

DiagnosticBundle NativeExecutionSession::diagnostic(OperationPhase phase, const std::string& message, const Json& evidence, bool failed, OperationCode code) const {
    const bool recovery = phase == OperationPhase::Execute || phase == OperationPhase::Verify || phase == OperationPhase::Rollback || phase == OperationPhase::Backup;
    const std::string source = std::string("https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/") +
        (recovery ? "recoverability-semantics.pseudo.md" : "system-state-classification.pseudo.md");
    const bool recoveryStage = recovery || (!state_->chain.is_null() && state_->chain.at("diagnostics").array_items().back().at("evidence").at("ashDiagnostic").at("stage") == Json("RECOVERY"));
    AshDiagnosticContext context{recovery ? "RECOVERY" : "STATE_VALIDITY", recoveryStage ? "RECOVERY" : "CLASSIFICATION", failed ? "BLOCKED" : "RESOLVED",
        state_->planning ? state_->planning->semanticPlan.planHash : hashCompleteSemanticInput(input_), {message}};
    auto bundle = makeAshDiagnosticBundle(context, failed ? "ERROR" : "INFO", message, evidence,
        Json::array({Json{{"ruleID", recovery ? "ASH-RECOVERY-ACTION-001" : "ASH-CLASSIFICATION-MAPPING-001"},
            {"ruleFamily", recovery ? "ASH-RECOVERY" : "ASH-CLASSIFICATION"}, {"sourceReference", source}}}), {source}, state_->chain.is_null() ? nullptr : &state_->chain);
    state_->chain = bundle.chain;
    DiagnosticEvent event; event.operationID = context.subjectReference;
    event.phase = phaseType(phase); event.outcome = failed ? DiagnosticOutcomeType::Failed : DiagnosticOutcomeType::Passed;
    event.objectHash = hashCompleteSemanticInput(input_); event.evidence = Json{{"actualPhase", to_string(phase)}, {"inputHash", event.objectHash}};
    if (failed) event.primaryError = retainedError(phase, code, evidence);
    captureTelemetry(phase, event);
    return bundle;
}
void NativeExecutionSession::auditPhase(const std::string& phase, const DiagnosticBundle& diagnosticValue, const Json& details) const {
    (void)actual(audit_->registerDiagnosticChain(BaseContracts::DiagnosticChain(diagnosticValue.chain)));
    const auto time = clock_->utcNow(); if (time.empty()) throw std::runtime_error("Actual audit time unavailable");
    const std::size_t sequence = state_->auditChain.is_null() ? 1 : state_->auditChain.at("events").size() + 1;
    Json event{{"auditEventID", "event-" + sha256(Json{{"phase", phase}, {"sequence", sequence}, {"diagnostic", diagnosticValue.reference}, {"details", details}, {"time", time}}.dump())},
        {"eventSequence", sequence}, {"eventType", phase}, {"diagnosticReference", diagnosticValue.reference}, {"occurredAt", time}, {"details", details}};
    const BaseContracts::AuditEvent checked(event);
    Json expected;
    if (state_->auditChain.is_null()) expected = Json{{"auditChainID", "audit-" + sha256(Json{{"inputHash", hashCompleteSemanticInput(input_)}, {"path", path_}, {"createdAt", createdAt_}}.dump())},
        {"diagnosticChainReference", diagnosticValue.chain.at("chainID")}, {"events", Json::array({event})}};
    else { expected = state_->auditChain; expected["events"].array_items().push_back(event);
        auto appended = actual(audit_->appendEvent(BaseContracts::AuditChain(state_->auditChain), checked));
        if (appended.value() != expected) throw std::runtime_error("Audit append changed the exact ordered event chain"); }
    const BaseContracts::AuditChain chain(expected); (void)actual(audit_->persistChain(chain));
    const auto stored = actual(audit_->readChain(expected.at("auditChainID").get<std::string>()));
    if (stored != chain) throw std::runtime_error("Audit persistence readback differs from exact actual event chain");
    const auto identity = fs_->pathIdentity(audit_->storagePath());
    if (identity.empty() || identity == state_->identity || (!state_->backupIdentity.empty() && identity == state_->backupIdentity)) throw std::runtime_error("Actual persisted audit identity unavailable or aliases target/backup");
    if (state_->auditExisted && identity != state_->auditIdentity) throw std::runtime_error("Existing audit identity changed during persistence");
    state_->auditIdentity = identity; state_->auditExisted = true;
    state_->auditChain = expected; state_->auditPersisted = true;
}
OperationFailure NativeExecutionSession::reject(OperationPhase phase, OperationCode code, const std::string& message, const Json& evidence) const {
    OperationFailure result(phase, code, message, evidence); result.mutationState = state_->mutation;
    try { const auto bundle = diagnostic(phase, message, evidence, true, code); result.diagnosticChain = bundle.chain;
        result.diagnosticPersistence = DiagnosticPersistence::NotRequested;
        std::erase_if(result.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; }); }
    catch (const std::exception& error) { result.addSecondary(phase, OperationCode::DiagnosticUnavailable, error.what()); result.diagnosticChain = state_->chain;
        if (!result.diagnosticChain.is_null()) { result.diagnosticPersistence = DiagnosticPersistence::NotRequested;
            std::erase_if(result.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; }); } }
    attachTelemetryErrors(result); return result;
}
void NativeExecutionSession::captureTelemetry(OperationPhase phase, const DiagnosticEvent& event) const noexcept {
    try {
        const auto before = diagnostics_->health(); diagnostics_->capture(event); const auto after = diagnostics_->health();
        if (after.captureFailures > before.captureFailures) {
            if (state_->diagnosticErrors.size() < 32) state_->diagnosticErrors.push_back(OperationFault{phase, OperationCode::DiagnosticUnavailable,
                "Best-effort diagnostic collector capture failed", Json{{"captureFailuresBefore", before.captureFailures}, {"captureFailuresAfter", after.captureFailures}}});
            else if (state_->diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++state_->diagnosticErrorsDropped;
        }
    } catch (...) { if (state_->diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++state_->diagnosticErrorsDropped; }
}
void NativeExecutionSession::attachTelemetryErrors(OperationFailure& failure) const {
    for (const auto& loss : state_->diagnosticErrors) {
        bool present = false; for (const auto& prior : failure.secondaryFailures) if (prior.phase == loss.phase && prior.code == loss.code && prior.evidence == loss.evidence) present = true;
        if (!present) failure.addSecondary(loss.phase, loss.code, loss.message, loss.evidence);
    }
}
void NativeExecutionSession::checkContext(const BaseContracts::ExecutionContext& context) const {
    if (!state_->context || context != *state_->context) throw std::invalid_argument("Foreign, stale or tampered execution context");
    if (state_->owner != std::this_thread::get_id()) throw std::invalid_argument("Execution lease belongs to another thread");
    if (!resolves(state_->chain, context.value().at("diagnosticReference"))) throw std::invalid_argument("Execution diagnostic no longer resolves");
}
void NativeExecutionSession::checkTarget(bool originalRequired) const {
    if (!state_->lease || fs_->pathIdentity(path_) != state_->identity) throw std::runtime_error("Target identity/lease changed");
    if (fs_->pathIdentity(audit_->storagePath()) != state_->auditIdentity || state_->auditIdentity.empty() || state_->auditIdentity == state_->identity) throw std::runtime_error("Audit identity changed or aliases protected target");
    if (originalRequired && (fs_->readFile(path_) != state_->original || fs_->versionToken(path_) != state_->version)) throw std::runtime_error("Stale actual target preimage/version");
    if (!state_->backupPath.empty() && (fs_->pathIdentity(state_->backupPath) != state_->backupIdentity || state_->backupIdentity == state_->auditIdentity || fs_->readFile(state_->backupPath) != state_->original)) throw std::runtime_error("Backup identity/bytes changed or aliases audit");
}

OperationAttempt<BaseContracts::ExecutionContext> NativeExecutionSession::prepareExecution() {
    std::lock_guard lock(mutex_);
    if (state_->failure) return *state_->failure;
    OperationPhase activePhase = OperationPhase::Plan;
    try {
        if (state_->context) { checkContext(*state_->context); if (!state_->complete) checkTarget(state_->executed == 0); return *state_->context; }
        if (state_->failure || state_->complete) return reject(OperationPhase::Plan, OperationCode::Blocked, "Session already failed or completed");
        auto computed = planCaptured(); if (auto error = std::get_if<OperationFailure>(&computed)) { state_->failure = *error; return *error; }
        state_->planning = std::get<NativePlanningResult>(std::move(computed)); state_->chain = state_->planning->diagnosticChain.value();
        if (state_->planning->semanticPlan.operations.empty()) return failAndRestore(OperationPhase::Plan, OperationCode::Blocked, "Read-only NO_ACTION uses the native healing path; frozen execution context requires a real backup and is inapplicable");
        activePhase = OperationPhase::Parse;
        state_->owner = std::this_thread::get_id(); state_->identity = fs_->pathIdentity(path_);
        state_->auditIdentity = fs_->pathIdentity(audit_->storagePath());
        state_->auditExisted = fs_->fileExists(audit_->storagePath());
        if (state_->identity.empty()) throw std::runtime_error("Target identity unavailable");
        if (state_->auditIdentity.empty() || state_->auditIdentity == state_->identity) throw std::runtime_error("Audit storage identity unavailable or aliases target");
        state_->lease = fs_->acquireWriteLease(path_); if (!state_->lease || fs_->pathIdentity(path_) != state_->identity) throw std::runtime_error("Exclusive identity lease unavailable");
        state_->original = fs_->readFile(path_); state_->version = fs_->versionToken(path_);
        const auto& captured = input_.at("observedSnapshot");
        if (captured.at("rawUtf8").get<std::string>() != state_->original || captured.at("rawByteSha256") != Json(sha256(state_->original)) ||
            adapter_->observe(path_).data != Json::parse(state_->original) || fs_->readFile(path_) != state_->original) return failAndRestore(OperationPhase::Project, OperationCode::StaleState, "Actual observation differs from exact captured semantic snapshot");
        const auto& plan = state_->planning->semanticPlan;
        if (plan.adapterTargetIdentity != state_->identity || state_->version.empty() || input_.at("proposedPlan").at("expectedObservedVersion") != Json(state_->version)) return failAndRestore(OperationPhase::Policy, OperationCode::StaleState, "Actual target identity/version differs from approved plan");
        state_->intermediate = Json::parse(state_->original);
        const OperationPhase nativePhases[] = {OperationPhase::Parse, OperationPhase::Normalize, OperationPhase::Project, OperationPhase::Diagnose, OperationPhase::Classify, OperationPhase::Plan};
        std::size_t index = 0;
        for (const auto& envelope : state_->planning->diagnosticChain.value().at("diagnostics").array_items()) {
            DiagnosticEvent event; event.operationID = plan.planHash; event.phase = phaseType(nativePhases[index++]); event.outcome = DiagnosticOutcomeType::Passed;
            event.objectHash = hashCompleteSemanticInput(input_); event.evidence = envelope.at("evidence");
            captureTelemetry(nativePhases[index - 1], event);
            const DiagnosticBundle bundle{envelope, reference(envelope), state_->chain};
            activePhase = OperationPhase::Audit;
            auditPhase(envelope.at("evidence").at("actualPhase").get<std::string>(), bundle, envelope.at("evidence"));
        }
        activePhase = OperationPhase::Policy;
        BasePolicyGate gate(environment_, input_, BoundPolicyContracts{state_->planning->recoveryPlan, state_->planning->policyBundle}, state_->chain);
        auto decision = gate.evaluateWithChain(state_->planning->recoveryPlan, state_->planning->policyBundle);
        if (auto error = std::get_if<OperationFailure>(&decision)) { state_->failure = *error; return *error; }
        auto accepted = std::get<BoundPolicyDecision>(std::move(decision)); state_->chain = accepted.diagnosticChain; state_->policy = accepted.decision;
        if (hostPolicy_) {
            const auto supplied = hostPolicy_->evaluate(state_->planning->recoveryPlan, state_->planning->policyBundle);
            if (const auto* error = std::get_if<OperationFailure>(&supplied)) return failAndRestore(OperationPhase::Policy, OperationCode::PolicyRejected, error->message);
            if (std::get<BaseContracts::PolicyDecision>(supplied) != *state_->policy) return failAndRestore(OperationPhase::Policy, OperationCode::PolicyRejected, "Host policy receipt differs from independently evaluated exact decision");
        }
        const DiagnosticBundle policyDiagnostic{state_->chain.at("diagnostics").array_items().back(), state_->policy->value().at("diagnosticReference"), state_->chain};
        auditPhase("POLICY_GATE", policyDiagnostic, Json{{"decision", state_->policy->value().at("decision")}, {"planHash", plan.planHash}});
        checkTarget(true);
        activePhase = OperationPhase::Backup;
        if (!backup_->supportsBackup()) return failAndRestore(OperationPhase::Backup, OperationCode::Blocked, "Real backup capability unavailable");
        state_->backupPath = backup_->createBackup(path_); state_->backupIdentity = fs_->pathIdentity(state_->backupPath);
        if (state_->backupPath.empty() || state_->backupIdentity.empty() || state_->backupIdentity == state_->identity || state_->backupIdentity == state_->auditIdentity || !fs_->fileExists(state_->backupPath) || fs_->readFile(state_->backupPath) != state_->original) throw std::runtime_error("Backup missing, aliased or not exact original bytes");
        checkTarget(true);
        const auto bundle = diagnostic(OperationPhase::Backup, "Unique real backup persisted and independently read back", Json{{"sourceHash", sha256(state_->original)}, {"backupHash", sha256(fs_->readFile(state_->backupPath))}});
        const Json document{{"documentID", "document-" + sha256(Json{{"path", path_}, {"rawHash", sha256(state_->original)}, {"version", state_->version}}.dump())}, {"sourceReference", path_}, {"role", "OBSERVED"}, {"canonicalHash", sha256(Json::parse(state_->original).dump())}};
        Json record{{"backupRecordID", "backup-" + sha256(Json{{"identity", state_->backupIdentity}, {"hash", sha256(state_->original)}}.dump())}, {"sourceDocument", document},
            {"sourceHash", sha256(state_->original)}, {"storageReference", state_->backupPath}, {"diagnosticReference", bundle.reference}};
        state_->backup = BaseContracts::BackupResult(Json{{"backupResultID", "backup-result-" + sha256(record.dump())}, {"status", "SUCCEEDED"}, {"backupRecord", record}, {"diagnosticReference", bundle.reference}});
        if (retainPersistenceCopySource_) {
            state_->copySourcePath = backup_->createBackup(path_); state_->copySourceIdentity = fs_->pathIdentity(state_->copySourcePath);
            if (state_->copySourcePath.empty() || state_->copySourceIdentity.empty() || state_->copySourceIdentity == state_->identity ||
                state_->copySourceIdentity == state_->auditIdentity || state_->copySourceIdentity == state_->backupIdentity || fs_->readFile(state_->copySourcePath) != state_->original) throw std::runtime_error("Independent copy-source backup missing, aliased or not exact original bytes");
            auto second = record; second["backupRecordID"] = "backup-" + sha256(Json{{"identity", state_->copySourceIdentity}, {"hash", sha256(state_->original)}}.dump()); second["storageReference"] = state_->copySourcePath;
            state_->copySourceRecord = BaseContracts::BackupRecord(second);
        }
        auditPhase("BACKUP", bundle, Json{{"backupRecordID", record.at("backupRecordID")}, {"sourceHash", record.at("sourceHash")}});
        Json context{{"executionContextID", "execution-" + sha256(Json{{"planHash", plan.planHash}, {"backupID", record.at("backupRecordID")}, {"version", state_->version}, {"identity", state_->identity}}.dump())},
            {"recoveryPlanID", state_->planning->recoveryPlan.value().at("recoveryPlanID")}, {"policyDecision", state_->policy->value()},
            {"backupResult", state_->backup->value()}, {"verificationPlan", state_->planning->verificationPlan.value()}, {"diagnosticReference", bundle.reference}};
        state_->context = BaseContracts::ExecutionContext(context); return *state_->context;
    } catch (const RejectedOperation& error) { return failAndRestore(error.failure); }
    catch (const std::exception& error) { return failAndRestore(operationFailureFromException(activePhase, OperationCode::Failed, error)); }
    catch (...) { return failAndRestore(activePhase, OperationCode::Failed, "Nonstandard execution preparation exception"); }
}

OperationAttempt<BaseContracts::BackupResult> NativeExecutionSession::prepareBackup(const BaseContracts::ExecutionContext& context, const BaseContracts::RecoveryPlan& plan) {
    std::lock_guard lock(mutex_); if (state_->failure) return *state_->failure;
    try { checkContext(context); if (plan != state_->planning->recoveryPlan) throw std::invalid_argument("Foreign recovery plan");
        checkTarget(state_->executed == 0); if (!state_->backup) throw std::runtime_error("No actual verified backup receipt"); return *state_->backup;
    } catch (const std::exception& error) { return reject(OperationPhase::Backup, OperationCode::InvalidInput, error.what()); }
}

OperationAttempt<BaseContracts::ExecutionStepResult> NativeExecutionSession::executeOperations(const BaseContracts::ExecutionContext& context, const Json& requested, const std::string& stepID) {
    if (state_->failure) return *state_->failure;
    bool authorizedPhase = false;
    try {
        checkContext(context); if (state_->failure || state_->complete) return reject(OperationPhase::Execute, OperationCode::Blocked, "Execution already failed/completed");
        const auto& allSteps = state_->planning->recoveryPlan.value().at("recoverySteps");
        if (allSteps.size() != 1 || allSteps.at(std::size_t{0}).at("stepID") != Json(stepID)) throw std::invalid_argument("Unknown source recovery step");
        const auto& all = allSteps.at(std::size_t{0}).at("jsonMutationOperations");
        if (requested.empty() || state_->executed + requested.size() > all.size()) throw std::invalid_argument("No remaining approved operation");
        for (std::size_t i = 0; i < requested.size(); ++i) if (requested.at(i) != all.at(state_->executed + i)) throw std::invalid_argument("Foreign, reordered, replayed or tampered mutation");
        authorizedPhase = true;
        checkTarget(state_->executed == 0);
        if (Json::parse(fs_->readFile(path_)) != state_->intermediate) return failAndRestore(OperationPhase::Execute, OperationCode::StaleState, "Actual intermediate document changed");
        Json selected = Json::array(); auto expected = state_->intermediate;
        for (std::size_t i = 0; i < requested.size(); ++i) {
            const auto& native = state_->planning->semanticPlan.operations.at(state_->executed + i);
            const auto* before = expected.resolve_pointer(native.at("pointer").get<std::string>());
            if (!before || *before != native.at("expectedPreimage")) throw std::runtime_error("Actual authorized slot preimage differs");
            selected.array_items().push_back(native); expected.replace_pointer(native.at("pointer").get<std::string>(), native.at("value"));
        }
        const auto beforeBytes = fs_->readFile(path_);
        const auto start = diagnostic(OperationPhase::Execute, "Exact checked source recovery operations authorized for execution", Json{{"stepID", stepID}, {"firstOperation", state_->executed}, {"count", selected.size()}});
        auditPhase("EXECUTE", start, Json{{"stage", "STARTED"}, {"stepID", stepID}, {"count", selected.size()}});
        state_->armed = true; state_->mutation = MutationState::MayHaveMutated;
        if (!adapter_->applyOperations(path_, selected, sha256(beforeBytes))) return failAndRestore(OperationPhase::Execute, OperationCode::Failed, "Adapter returned false after possible mutation");
        const auto bytes = fs_->readFile(path_); state_->rawAfter = sha256(bytes);
        if (Json::parse(bytes) != expected) return failAndRestore(OperationPhase::Execute, OperationCode::VerificationFailed, "Actual post-step document differs from exact allowed intermediate target");
        state_->intermediate = expected; state_->executed += selected.size(); state_->mutation = MutationState::Mutated;
        const auto done = diagnostic(OperationPhase::Execute, "Actual source recovery operations completed and hashes measured", Json{{"stepID", stepID}, {"observedHashAfterStep", state_->rawAfter}, {"completedOperationCount", state_->executed}});
        auditPhase("EXECUTE", done, Json{{"stage", "SUCCEEDED"}, {"stepID", stepID}, {"rawHash", state_->rawAfter}});
        state_->lastExecution = BaseContracts::ExecutionStepResult(Json{{"executionStepResultID", "execution-result-" + sha256(Json{{"stepID", stepID}, {"hash", state_->rawAfter}, {"count", state_->executed}}.dump())}, {"stepID", stepID},
            {"status", "SUCCEEDED"}, {"observedHashAfterStep", state_->rawAfter}, {"diagnosticReference", done.reference}});
        return *state_->lastExecution;
    } catch (const std::invalid_argument& error) { return authorizedPhase ? failAndRestore(OperationPhase::Execute, OperationCode::InvalidInput, error.what()) : reject(OperationPhase::Execute, OperationCode::InvalidInput, error.what()); }
    catch (const RejectedOperation& error) { return failAndRestore(error.failure); }
    catch (const std::exception& error) { return failAndRestore(operationFailureFromException(OperationPhase::Execute, OperationCode::Failed, error)); }
    catch (...) { return failAndRestore(OperationPhase::Execute, OperationCode::Failed, "Nonstandard execution/host exception"); }
}
OperationAttempt<BaseContracts::ExecutionStepResult> NativeExecutionSession::executeStep(const BaseContracts::ExecutionContext& context, const BaseContracts::RecoveryStep& step) {
    std::lock_guard lock(mutex_);
    if (state_->failure) return *state_->failure;
    try { checkContext(context); const auto& steps = state_->planning->recoveryPlan.value().at("recoverySteps");
        if (steps.size() != 1 || steps.at(std::size_t{0}) != step.value()) throw std::invalid_argument("Step differs from exact owned recovery plan");
        if (state_->executed != 0) return reject(OperationPhase::Execute, OperationCode::InvalidInput, "Recovery step already started; use remaining exact mutation operations");
        return executeOperations(context, step.value().at("jsonMutationOperations"), step.value().at("stepID").get<std::string>());
    } catch (const std::exception& error) { return reject(OperationPhase::Execute, OperationCode::InvalidInput, error.what()); }
}
OperationAttempt<BaseContracts::ExecutionStepResult> NativeExecutionSession::applyMutation(const BaseContracts::ExecutionContext& context, const BaseContracts::JsonMutationOperation& operation) {
    std::lock_guard lock(mutex_); if (state_->failure) return *state_->failure;
    try { checkContext(context); const auto& steps = state_->planning->recoveryPlan.value().at("recoverySteps");
        if (steps.size() != 1) throw std::invalid_argument("Mutation has no exact owning recovery step");
        return executeOperations(context, Json::array({operation.value()}), steps.at(std::size_t{0}).at("stepID").get<std::string>());
    } catch (const std::exception& error) { return reject(OperationPhase::Execute, OperationCode::InvalidInput, error.what()); }
}

OperationAttempt<BaseContracts::VerificationResult> NativeExecutionSession::verify(const BaseContracts::ExecutionContext& context, const BaseContracts::VerificationPlan& requested) const {
    std::lock_guard lock(mutex_);
    if (state_->failure) return *state_->failure;
    bool authorizedPhase = false;
    try {
        checkContext(context); if (requested != state_->planning->verificationPlan || !requested.value().at("generatedBeforeMutation").get<bool>()) throw std::invalid_argument("Foreign or late-generated verification plan");
        if (state_->complete && state_->verification) return *state_->verification;
        if (state_->failure || state_->rollback) return reject(OperationPhase::Verify, OperationCode::Blocked, "Recovery failed or was restored; restoration is not healing success");
        if (state_->executed != state_->planning->semanticPlan.operations.size()) return reject(OperationPhase::Verify, OperationCode::Blocked, "Approved operations remain unexecuted");
        authorizedPhase = true;
        checkTarget(false); const auto bytes = fs_->readFile(path_); const auto document = Json::parse(bytes); state_->rawAfter = sha256(bytes);
        auto observed = input_; auto& snapshot = observed["observedSnapshot"]; snapshot["document"] = document; snapshot["rawUtf8"] = Json::captured_raw_utf8(bytes);
        snapshot["rawByteSha256"] = state_->rawAfter; snapshot["proposedCanonicalSha256"] = sha256(document.dump()); snapshot["revision"] = fs_->versionToken(path_);
        if (snapshot.at("revision").get<std::string>().empty()) throw std::runtime_error("Actual post-execution version unavailable");
        auto projected = environment_.project(observed); if (const auto* error = std::get_if<OperationFailure>(&projected)) return failAndRestore(*error);
        const auto state = std::get<Projection>(projected).state; const auto expected = parseAshState(requested.value().at("expectedSemanticState").at("coordinates"));
        const auto diagnosis = environment_.diagnose(state); const auto invariants = environment_.evaluateInvariants(input_, document, "LIFECYCLE_SUCCESS_POSTCONDITION", state_->planning->semanticPlan.operations);
        const bool passed = document == state_->planning->semanticPlan.expectedDocument && state == expected && diagnosis.valid &&
            classifySystemState(diagnosis, false, false, false, false) == SystemClass::Stable && invariants.passed;
        const auto bundle = diagnostic(OperationPhase::Verify, passed ? "Actual complete repair target verified" : "Actual repair verification failed", Json{{"rawHash", state_->rawAfter}, {"allPostconditionsPassed", passed}, {"invariantResults", invariants.results}}, !passed);
        state_->verification = BaseContracts::VerificationResult(Json{{"verificationResultID", "verification-result-" + sha256(Json{{"planID", requested.value().at("verificationPlanID")}, {"rawHash", state_->rawAfter}, {"passed", passed}}.dump())},
            {"verificationPlanID", requested.value().at("verificationPlanID")}, {"status", passed ? "PASSED" : "FAILED"},
            {"expectedSemanticState", requested.value().at("expectedSemanticState")}, {"actualSemanticState", frozenState(state, "actual-target:" + state_->identity + ":" + state_->rawAfter)}, {"diagnosticReference", bundle.reference}});
        auditPhase("VERIFY", bundle, Json{{"status", passed ? "PASSED" : "FAILED"}, {"rawHash", state_->rawAfter}});
        if (!passed) return failAndRestore(OperationPhase::Verify, OperationCode::VerificationFailed, "Exact target, ASH state or mandatory postcondition failed");
        return *state_->verification;
    } catch (const std::invalid_argument& error) { return authorizedPhase ? failAndRestore(OperationPhase::Verify, OperationCode::InvalidInput, error.what()) : reject(OperationPhase::Verify, OperationCode::InvalidInput, error.what()); }
    catch (const RejectedOperation& error) { return failAndRestore(error.failure); }
    catch (const std::exception& error) { return failAndRestore(operationFailureFromException(OperationPhase::Verify, OperationCode::VerificationFailed, error)); }
    catch (...) { return failAndRestore(OperationPhase::Verify, OperationCode::VerificationFailed, "Nonstandard verification/host exception"); }
}

void NativeExecutionSession::restoreAuthorized(OperationFailure& failure) const {
    if (!state_->armed) return;
    try {
        if (!state_->context || !state_->backup || state_->owner != std::this_thread::get_id()) throw std::runtime_error("Actual same-owner restoration authority unavailable");
        checkTarget(false);
        if (!backup_->restoreBackup(state_->backupPath, path_)) throw std::runtime_error("Actual restore returned false");
        const auto bytes = fs_->readFile(path_); state_->rawAfter = sha256(bytes);
        if (bytes != state_->original || state_->rawAfter != sha256(state_->original)) throw std::runtime_error("Restore did not reproduce exact original bytes");
        const auto invariants = environment_.evaluateInvariants(input_, Json::parse(bytes), "RESTORATION_SAFETY", state_->planning->semanticPlan.operations);
        if (!invariants.passed) throw std::runtime_error("Actual restoration safety predicates unavailable or failed");
        state_->mutation = MutationState::VerifiedRestored; failure.mutationState = state_->mutation; state_->armed = false;
        const auto bundle = diagnostic(OperationPhase::Rollback, "Authorized original bytes independently verified after restoration; healing remains failed", Json{{"restoredHash", state_->rawAfter}, {"healingSucceeded", false}});
        const auto backupID = state_->backup->value().at("backupRecord").at("backupRecordID");
        state_->rollback = BaseContracts::RollbackResult(Json{{"rollbackResultID", "rollback-result-" + sha256(Json{{"backupID", backupID}, {"rawHash", state_->rawAfter}}.dump())},
            {"status", "SUCCEEDED"}, {"backupRecordID", backupID}, {"restoredHash", state_->rawAfter}, {"diagnosticReference", bundle.reference}});
        auditPhase("ROLLBACK", bundle, Json{{"restoredHash", state_->rawAfter}, {"healingSucceeded", false}});
    } catch (const std::exception& error) { if (state_->mutation != MutationState::VerifiedRestored) state_->mutation = MutationState::RestoreFailed;
        const auto code = state_->mutation == MutationState::VerifiedRestored ? OperationCode::StorageFailure : OperationCode::RestorationFailed;
        const auto actualError = operationFailureFromException(OperationPhase::Rollback, code, error);
        failure.mutationState = state_->mutation; failure.addSecondary(OperationPhase::Rollback, code, error.what(), actualError.evidence);
        try { (void)diagnostic(OperationPhase::Rollback, error.what(), Json{{"mutationState", to_string(state_->mutation)}}, true); } catch (...) {} }
    catch (...) { state_->mutation = MutationState::RestoreFailed; failure.mutationState = state_->mutation; failure.addSecondary(OperationPhase::Rollback, OperationCode::RestorationFailed, "Nonstandard restoration exception"); }
}
OperationFailure NativeExecutionSession::failAndRestore(OperationPhase phase, OperationCode code, const std::string& message) const {
    return failAndRestore(OperationFailure(phase, code, message));
}
OperationFailure NativeExecutionSession::failAndRestore(OperationFailure error) const {
    if (state_->failure) return *state_->failure;
    auto captured = reject(error.phase, error.code, error.message, error.evidence);
    captured.mappingStatus = error.mappingStatus;
    for (const auto& unavailable : error.unavailableEvidence) if (unavailable.field != "diagnosticChain") captured.unavailableEvidence.push_back(unavailable);
    for (const auto& secondary : error.secondaryFailures) captured.secondaryFailures.push_back(secondary);
    error = std::move(captured);
    if (state_->lease && state_->owner == std::this_thread::get_id()) {
        try { state_->rawAfter = sha256(fs_->readFile(path_)); }
        catch (const std::exception& cause) { error.addSecondary(OperationPhase::Verify, OperationCode::StorageFailure, std::string("Actual failure-state hash unavailable: ") + cause.what(), operationFailureFromException(OperationPhase::Verify, OperationCode::StorageFailure, cause).evidence); }
    }
    restoreAuthorized(error);
    if (state_->lease && state_->owner == std::this_thread::get_id()) {
        try { state_->rawAfter = sha256(fs_->readFile(path_)); }
        catch (const std::exception& cause) { error.addSecondary(OperationPhase::Verify, OperationCode::StorageFailure, std::string("Actual restoration-state hash unavailable: ") + cause.what(), operationFailureFromException(OperationPhase::Verify, OperationCode::StorageFailure, cause).evidence); }
    }
    if (!state_->chain.is_null() && !state_->auditIdentity.empty() && state_->auditIdentity != state_->identity) {
        error.diagnosticPersistence = DiagnosticPersistence::Failed;
        try { const DiagnosticBundle failureDiagnostic{state_->chain.at("diagnostics").array_items().back(), reference(state_->chain.at("diagnostics").array_items().back()), state_->chain};
            auditPhase("RESULT", failureDiagnostic, Json{{"status", "FAILED"}, {"failurePhase", to_string(error.phase)}, {"mutationState", to_string(state_->mutation)}});
            error.diagnosticPersistence = DiagnosticPersistence::Persisted; }
        catch (const RejectedOperation& cause) { error.addSecondary(cause.failure.phase, cause.failure.code, cause.failure.message, cause.failure.evidence); }
        catch (const std::exception& cause) { error.addSecondary(OperationPhase::Audit, OperationCode::StorageFailure, std::string("Terminal failure audit unavailable: ") + cause.what(), operationFailureFromException(OperationPhase::Audit, OperationCode::StorageFailure, cause).evidence); }
    }
    error.diagnosticChain = state_->chain;
    if (!error.diagnosticChain.is_null()) std::erase_if(error.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
    DiagnosticEvent event; event.operationID = state_->planning ? state_->planning->semanticPlan.planHash : hashCompleteSemanticInput(input_);
    event.phase = phaseType(error.phase); event.outcome = state_->mutation == MutationState::RestoreFailed ? DiagnosticOutcomeType::RecoveryFailed : DiagnosticOutcomeType::Failed;
    event.objectHash = hashCompleteSemanticInput(input_); event.primaryError = retainedError(error.phase, error.code, error.evidence);
    for (const auto& secondary : error.secondaryFailures) event.secondaryErrors.push_back(retainedError(secondary.phase, secondary.code, secondary.evidence));
    event.evidence = Json{{"actualPhase", to_string(error.phase)}, {"inputHash", event.objectHash}};
    captureTelemetry(error.phase, event); attachTelemetryErrors(error);
    state_->failure = error; return error;
}
OperationAttempt<BaseContracts::RollbackPlan> NativeExecutionSession::rollbackPlan() const {
    std::lock_guard lock(mutex_); if (state_->failure && state_->mutation == MutationState::RestoreFailed) return *state_->failure;
    try { if (!state_->verification || state_->verification->value().at("status") != Json("FAILED") || !state_->backup) throw std::invalid_argument("Actual failed verification and verified backup required");
        const auto ref = state_->verification->value().at("diagnosticReference");
        return BaseContracts::RollbackPlan(Json{{"rollbackPlanID", "rollback-" + sha256(Json{{"backupID", state_->backup->value().at("backupRecord").at("backupRecordID")}, {"verificationID", state_->verification->value().at("verificationResultID")}}.dump())},
            {"backupRecordID", state_->backup->value().at("backupRecord").at("backupRecordID")}, {"triggerVerificationResultID", state_->verification->value().at("verificationResultID")}, {"diagnosticReference", ref}});
    } catch (const std::exception& error) { return reject(OperationPhase::Rollback, OperationCode::Blocked, error.what()); }
}
OperationAttempt<BaseContracts::RollbackResult> NativeExecutionSession::executeRollback(const BaseContracts::ExecutionContext& context, const BaseContracts::RollbackPlan& requested) {
    std::lock_guard lock(mutex_); if (state_->failure && state_->mutation == MutationState::RestoreFailed) return *state_->failure;
    try { checkContext(context); const auto genuine = rollbackPlan();
        if (const auto* error = std::get_if<OperationFailure>(&genuine)) return *error;
        if (requested != std::get<BaseContracts::RollbackPlan>(genuine)) throw std::invalid_argument("Foreign rollback evidence");
        if (state_->rollback) return *state_->rollback;
        if (!state_->failure) throw std::invalid_argument("No actual lifecycle failure authorized restoration");
        auto error = *state_->failure; restoreAuthorized(error); state_->failure = error;
        if (!state_->rollback) return error; return *state_->rollback;
    } catch (const std::exception& error) { return reject(OperationPhase::Rollback, OperationCode::InvalidInput, error.what()); }
}

OperationAttempt<BaseContracts::HealResult> NativeExecutionSession::complete(const BaseContracts::ExecutionContext& context) {
    std::lock_guard lock(mutex_); if (state_->failure) return *state_->failure;
    try {
        checkContext(context); if (state_->failure) return *state_->failure;
        if (state_->complete && state_->healResult) return *state_->healResult;
        if (!state_->verification) { auto checked = verify(context, state_->planning->verificationPlan); if (const auto* error = std::get_if<OperationFailure>(&checked)) return *error; }
        if (state_->verification->value().at("status") != Json("PASSED")) throw std::runtime_error("No actual successful verification");
        const auto bundle = diagnostic(OperationPhase::Verify, "Actual lifecycle completed with verified target and persisted audit", Json{{"planHash", state_->planning->semanticPlan.planHash}, {"rawHash", state_->rawAfter}});
        auditPhase("RESULT", bundle, Json{{"status", state_->planning->semanticPlan.operations.empty() ? "NO_ACTION" : "HEALED"}});
        BaseContracts::HealResult result(Json{{"healResultID", "heal-" + sha256(Json{{"planHash", state_->planning->semanticPlan.planHash}, {"rawHash", state_->rawAfter}, {"auditChainID", state_->auditChain.at("auditChainID")}}.dump())},
            {"status", state_->planning->semanticPlan.operations.empty() ? "NO_ACTION" : "HEALED"}, {"recoveryPlan", state_->planning->recoveryPlan.value()}, {"auditChain", state_->auditChain}, {"diagnosticChain", state_->chain}});
        state_->healResult = result; state_->complete = true; state_->armed = false; state_->lease.reset(); return result;
    } catch (const RejectedOperation& error) { return failAndRestore(error.failure); }
    catch (const std::exception& error) { return failAndRestore(operationFailureFromException(OperationPhase::Verify, OperationCode::Failed, error)); }
}
OperationAttempt<std::shared_ptr<const BaseBindings::AtomicPersistenceContext>> NativeExecutionSession::authorizeAtomicPersistence(
    const BaseContracts::ExecutionContext& context, BaseBindings::NativePersistencePurpose purpose, BaseBindings::AtomicOperation operation) const {
    std::lock_guard lock(mutex_);
    if (state_->failure) return *state_->failure;
    try {
        checkContext(context);
        if (!state_->complete || state_->failure || state_->armed || state_->lease || !state_->verification ||
            state_->verification->value().at("status") != Json("PASSED") || !state_->backup || !state_->policy || !state_->auditPersisted) throw std::invalid_argument("Verified completed lifecycle and released target lease required for persistence capability");
        const auto carrier = fs_->readFile(path_);
        if (fs_->pathIdentity(path_) != state_->identity || sha256(carrier) != state_->rawAfter || Json::parse(carrier) != state_->planning->semanticPlan.expectedDocument) throw std::runtime_error("Completed carrier identity/hash/target changed");
        if (fs_->pathIdentity(state_->backupPath) != state_->backupIdentity || fs_->readFile(state_->backupPath) != state_->original) throw std::runtime_error("Measured backup changed before capability issuance");
        if (fs_->pathIdentity(audit_->storagePath()) != state_->auditIdentity || state_->auditIdentity == state_->identity || state_->auditIdentity == state_->backupIdentity) throw std::runtime_error("Measured audit identity changed before capability issuance");
        const auto stored = actual(audit_->readChain(state_->auditChain.at("auditChainID").get<std::string>()));
        if (stored.value() != state_->auditChain) throw std::runtime_error("Latest stored audit chain differs from completed actual receipts");
        std::string source, sourceIdentity, target, targetIdentity, preimage, operationInput, result;
        std::optional<BaseContracts::BackupRecord> copyRecord;
        if (purpose == BaseBindings::NativePersistencePurpose::BackupSnapshot) {
            target = state_->backupPath; targetIdentity = state_->backupIdentity; preimage = state_->original; operationInput = state_->original; result = state_->original;
            if (operation == BaseBindings::AtomicOperation::Copy) {
                if (!state_->copySourceRecord) throw std::invalid_argument("Opt-in actual independently verified copy-source snapshot required");
                source = state_->copySourcePath; sourceIdentity = state_->copySourceIdentity; copyRecord = state_->copySourceRecord;
                if (sourceIdentity == targetIdentity || sourceIdentity == state_->identity || sourceIdentity == state_->auditIdentity || fs_->pathIdentity(source) != sourceIdentity || fs_->readFile(source) != state_->original) throw std::runtime_error("Actual copy-source snapshot changed or aliases another carrier");
            } else if (operation != BaseBindings::AtomicOperation::Write) throw std::invalid_argument("Backup append is outside exact original snapshot authorization");
        } else if (purpose == BaseBindings::NativePersistencePurpose::AuditSnapshot) {
            target = audit_->storagePath(); targetIdentity = state_->auditIdentity; preimage = fs_->readFile(target);
            if (operation == BaseBindings::AtomicOperation::Write) { operationInput = preimage; result = preimage; }
            else if (operation == BaseBindings::AtomicOperation::Append) {
                operationInput = Json{{"storageSchema", "aeostara.audit-snapshot/1"}, {"auditChain", state_->auditChain}, {"diagnosticChain", state_->chain}}.dump() + "\n";
                result = preimage + operationInput;
            } else throw std::invalid_argument("Audit copy is outside owned measured persistence scope");
        } else throw std::invalid_argument("Unknown persistence purpose");
        return std::shared_ptr<const BaseBindings::AtomicPersistenceContext>(new BaseBindings::AtomicPersistenceContext(purpose, operation,
            source, target, sourceIdentity, targetIdentity, preimage, operationInput, result, path_, state_->identity, state_->rawAfter,
            state_->backupPath, state_->backupIdentity, sha256(state_->original), audit_->storagePath(), state_->auditIdentity,
            state_->planning->recoveryPlan, *state_->policy, *state_->backup, context, BaseContracts::DiagnosticChain(state_->chain),
            BaseContracts::AuditChain(state_->auditChain), copyRecord));
    } catch (const std::exception& error) { return reject(OperationPhase::Audit, OperationCode::Blocked, error.what()); }
    catch (...) { return reject(OperationPhase::Audit, OperationCode::Blocked, "Nonstandard persistence capability measurement exception"); }
}
NativeExecutionOutcome NativeExecutionSession::run() {
    auto prepared = prepareExecution(); if (std::holds_alternative<OperationFailure>(prepared)) return outcome();
    const auto context = std::get<BaseContracts::ExecutionContext>(prepared);
    const auto plan = state_->planning->recoveryPlan.value();
    if (!plan.at("recoverySteps").empty()) {
        auto executed = executeStep(context, BaseContracts::RecoveryStep(plan.at("recoverySteps").at(std::size_t{0})));
        if (std::holds_alternative<OperationFailure>(executed)) return outcome();
    }
    auto result = complete(context); (void)result; return outcome();
}
NativeExecutionOutcome NativeExecutionSession::outcome() const {
    std::lock_guard lock(mutex_); NativeExecutionOutcome result; result.success = state_->complete && !state_->failure;
    if (state_->planning) result.semanticPlan = state_->planning->semanticPlan;
    result.failure = state_->failure; if (!state_->original.empty()) result.rawHashBefore = sha256(state_->original);
    if (!state_->rawAfter.empty()) result.rawHashAfter = state_->rawAfter; if (!state_->backupPath.empty()) result.backupPath = state_->backupPath;
    result.mutationState = state_->mutation; result.completedOperationCount = state_->executed; result.diagnosticErrors = state_->diagnosticErrors;
    result.diagnosticErrorsDropped = state_->diagnosticErrorsDropped; result.diagnosticHealth = diagnostics_->health();
    result.diagnosticChain = state_->chain; result.auditChain = state_->auditChain; return result;
}
} // namespace Aeostara

namespace Aeostara::BaseBindings {
NativeExecutionBindings::NativeExecutionBindings(std::shared_ptr<NativeExecutionSession> session,
    BaseContracts::ObservedSystemState observed, BaseContracts::DesiredSystemIntent desired,
    BaseContracts::SemanticProjectionSpec projection, std::vector<BaseContracts::DiagnosticChain> chains)
    : session_(std::move(session)), observed_(std::move(observed)), desired_(std::move(desired)), projection_(std::move(projection)) {
    if (!session_) throw std::invalid_argument("Actual shared execution session required");
    const auto& input = session_->capturedInput();
    for (const auto& pair : {std::pair{&observed_.value(), "observedSnapshot"}, std::pair{&desired_.value(), "desiredSnapshot"}}) {
        const auto& value = *pair.first; const auto& snapshot = input.at(pair.second);
        if (value.at("documentReference").at("canonicalHash") != snapshot.at("proposedCanonicalSha256")) throw std::invalid_argument("Frozen document reference differs from exact captured snapshot");
        bool found = false; for (const auto& chain : chains) if (resolves(chain.value(), value.at("diagnosticReference"))) found = true;
        if (!found) throw std::invalid_argument("Captured frozen input diagnostic is foreign or unresolvable");
    }
    if (observed_.value().at("documentReference").at("role") != Json("OBSERVED") || desired_.value().at("documentReference").at("role") != Json("DESIRED")) throw std::invalid_argument("Wrong captured document roles");
    if (observed_.value().at("observedSystemStateID") != Json("observation-" + sha256(observed_.value().at("documentReference").dump())) || !observed_.value().at("surfaceDifferences").empty()) throw std::invalid_argument("Observed DTO differs from owned capture binding");
    if (desired_.value().at("desiredSystemIntentID") != Json("intent-" + sha256(Json{{"documentReference", desired_.value().at("documentReference")}, {"partialIntent", desired_.value().at("partialIntent")}}.dump()))) throw std::invalid_argument("Desired DTO identity differs from explicit captured intent");
    const auto& profile = session_->capturedProfile();
    if (projection_.value().at("projectionSpecID") != profile.at("profileID") || projection_.value().at("version") != Json(profile.at("revision").number_text()) || projection_.value().at("dimensions").size() != 9) throw std::invalid_argument("Frozen projection identity/revision differs from captured actual profile");
    for (std::size_t i = 0; i < 9; ++i) {
        const auto& declared = projection_.value().at("dimensions").at(i); const auto& actual = profile.at("dimensions").at(i);
        if (declared.at("dimensionID") != actual.at("dimension") || declared.at("mappingRule") != profile.at("evaluatorID") || declared.at("evidence") != actual.at("sourceBindings")) throw std::invalid_argument("Frozen dimension meaning/rule/evidence differs from adopted profile");
        Json pointers = Json::array();
        for (const auto& binding : actual.at("sourceBindings").array_items()) {
            const Json* snapshot = nullptr; if (binding.at("sourceName") == Json("observed")) snapshot = &input.at("observedSnapshot");
            else for (const auto& item : input.at("evidenceSnapshots").array_items()) if (item.at("sourceName") == binding.at("sourceName")) { if (snapshot) throw std::invalid_argument("Ambiguous captured dimension source"); snapshot = &item; }
            const auto text = binding.at("pointer").get<std::string>(); const auto* value = snapshot ? snapshot->at("document").resolve_pointer(text) : nullptr;
            pointers.push_back(Json{{"pointer", text}, {"tokens", Json(Json::pointer_tokens(text))}, {"pointsToMissing", value == nullptr}, {"pointsToNull", value && value->is_null()}});
        }
        if (declared.at("sourcePointers") != pointers) throw std::invalid_argument("Frozen source pointers/presence differ from actual captured evidence");
        bool found = false; for (const auto& chain : chains) if (resolves(chain.value(), declared.at("diagnosticReference"))) found = true;
        if (!found) throw std::invalid_argument("Dimension diagnostic does not resolve to captured actual envelope");
    }
}
void NativeExecutionBindings::checkInputs(const BaseContracts::ObservedSystemState& observed, const BaseContracts::DesiredSystemIntent& desired,
    const BaseContracts::SemanticProjectionSpec& projection, const BaseContracts::PolicyBundle& policy, const NativePlanningResult& plan) const {
    if (observed != observed_ || desired != desired_ || projection != projection_ || policy != plan.policyBundle) throw std::invalid_argument("Foreign or tampered immutable healing input");
}
OperationAttempt<BaseContracts::ExecutionContext> NativeExecutionBindings::prepareExecution() { return session_->prepareExecution(); }
OperationAttempt<BaseContracts::BackupResult> NativeExecutionBindings::prepareBackup(const BaseContracts::ExecutionContext& c, const BaseContracts::RecoveryPlan& p) { return session_->prepareBackup(c,p); }
OperationAttempt<BaseContracts::RollbackResult> NativeExecutionBindings::executeRollback(const BaseContracts::ExecutionContext& c, const BaseContracts::RollbackPlan& p) { return session_->executeRollback(c,p); }
OperationAttempt<BaseContracts::ExecutionStepResult> NativeExecutionBindings::executeStep(const BaseContracts::ExecutionContext& c, const BaseContracts::RecoveryStep& p) { return session_->executeStep(c,p); }
OperationAttempt<BaseContracts::ExecutionStepResult> NativeExecutionBindings::applyMutation(const BaseContracts::ExecutionContext& c, const BaseContracts::JsonMutationOperation& p) { return session_->applyMutation(c,p); }
OperationAttempt<BaseContracts::VerificationResult> NativeExecutionBindings::verify(const BaseContracts::ExecutionContext& c, const BaseContracts::VerificationPlan& p) const { return session_->verify(c,p); }
OperationAttempt<BaseContracts::DryRunResult> NativeExecutionBindings::dryRun(const BaseContracts::ObservedSystemState& observed, const BaseContracts::DesiredSystemIntent& desired,
    const BaseContracts::SemanticProjectionSpec& projection, const BaseContracts::PolicyBundle& policy) const {
    try { auto planned = session_->planCaptured(); if (const auto* error = std::get_if<OperationFailure>(&planned)) return *error;
        const auto& plan = std::get<NativePlanningResult>(planned); checkInputs(observed, desired, projection, policy, plan);
        return BaseContracts::DryRunResult(Json{{"dryRunResultID", "dry-run-" + sha256(Json{{"planHash", plan.semanticPlan.planHash}, {"inputHash", hashCompleteSemanticInput(session_->capturedInput())}}.dump())},
            {"wouldMutate", !plan.semanticPlan.operations.empty()}, {"recoveryPlan", plan.recoveryPlan.value()}, {"verificationPlan", plan.verificationPlan.value()}, {"diagnosticChain", plan.diagnosticChain.value()}});
    } catch (const std::exception& error) { return pureFailure(OperationPhase::Plan, OperationCode::InvalidInput, error, session_->capturedInput()); }
}
OperationAttempt<BaseContracts::HealResult> NativeExecutionBindings::heal(const BaseContracts::ObservedSystemState& observed, const BaseContracts::DesiredSystemIntent& desired,
    const BaseContracts::SemanticProjectionSpec& projection, const BaseContracts::PolicyBundle& policy, const BaseContracts::ExecutionContext& context) {
    try { auto planned = session_->planCaptured(); if (const auto* error = std::get_if<OperationFailure>(&planned)) return *error;
        const auto& plan = std::get<NativePlanningResult>(planned); checkInputs(observed, desired, projection, policy, plan);
        for (const auto& step : plan.recoveryPlan.value().at("recoverySteps").array_items()) {
            auto executed = session_->executeStep(context, BaseContracts::RecoveryStep(step)); if (const auto* error = std::get_if<OperationFailure>(&executed)) return *error;
        }
        return session_->complete(context);
    } catch (const std::exception& error) { return pureFailure(OperationPhase::Execute, OperationCode::InvalidInput, error, session_->capturedInput()); }
}
} // namespace Aeostara::BaseBindings
