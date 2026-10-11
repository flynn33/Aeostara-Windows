// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include "AeostaraCore/HealingEngine.h"
#include "AeostaraCore/AuditTrail.h"
#include "AeostaraCore/NativeExecutionBindings.h"
#include "AeostaraCore/DriftAnalyzer.h"
#include "AeostaraCore/InvariantParser.h"
#include "AeostaraCore/PolicyEvaluator.h"
#include "AeostaraCore/RepairPlanner.h"
#include "AeostaraCore/JsonPath.h"
#include <set>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <limits>
namespace Aeostara {
namespace {
struct RejectedOperation final : std::runtime_error { OperationFailure failure; explicit RejectedOperation(OperationFailure value) : std::runtime_error(value.message), failure(std::move(value)) {} };
OperationFailure failure(std::string phase, std::string code, std::string message) { return OperationFailure(operationPhase(phase), operationCode(code), std::move(message)); }
void actualObservation(IFileSystem& fs, IConfigAdapter& adapter, const std::string& path, const Json& input, const std::string& bytes) {
    if (input.at("observedSnapshot").at("rawUtf8").get<std::string>() != bytes || input.at("observedSnapshot").at("rawByteSha256") != Json(sha256(bytes))) throw std::runtime_error("Stale actual observation differs from complete input preimage");
    if (adapter.observe(path).data != Json::parse(bytes) || fs.readFile(path) != bytes) throw std::runtime_error("Adapter observation is inconsistent with captured raw preimage");
}
RepairPlan compatibilityPlan(const SemanticPlan& semantic) {
    RepairPlan result; result.planID = semantic.planHash; result.requiresBackup = !semantic.operations.empty();
    for (const auto& operation : semantic.operations.array_items()) result.actions.push_back({.keyPath = operation.at("pointer").get<std::string>(), .actionType = RepairActionType::Set, .fromValue = operation.at("expectedPreimage"), .toValue = operation.at("value"), .rationale = semantic.inverseRuleID});
    return result;
}
}
HealingEngine::HealingEngine(std::shared_ptr<IConfigAdapter> adapter, std::shared_ptr<IBackupProvider> backup, std::shared_ptr<IFileSystem> fs, std::shared_ptr<const SemanticEnvironment> environment, std::shared_ptr<IDiagnostics> diagnostics, std::shared_ptr<BaseBindings::IPolicyGate> policyGate)
    : adapter_(std::move(adapter)), backup_(std::move(backup)), fs_(std::move(fs)), environment_(std::move(environment)), diagnostics_(std::move(diagnostics)), policyGate_(std::move(policyGate)) {
    if (!adapter_ || !backup_ || !fs_) throw std::invalid_argument("Healing engine host dependencies unavailable");
    if (!diagnostics_) diagnostics_ = createDefaultDiagnostics();
}
DesiredState HealingEngine::loadDesired(const std::string& path) const { return {.data = Json::parse(fs_->readFile(path)), .source = path}; }
std::vector<Invariant> HealingEngine::loadInvariants(const std::string& path) const { if (path.empty()) return {}; return InvariantParser::parseFile(path, *fs_); }
IHealingEngine::ValidationResult HealingEngine::validate(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath) {
    ValidationResult result;
    try {
        auto observed = adapter_->observe(configPath); auto desired = loadDesired(desiredPath); const auto invariants = loadInvariants(invariantsPath);
        if (desired.data.contains("semanticRequest")) return validateSemantic(configPath, desired.data.at("semanticRequest"));
        const auto encoded = adapter_->encode(observed, desired); DriftAnalyzer analyzer(encoded); result.drifts = analyzer.analyze();
        for (const auto& inv : invariants) if (!PolicyEvaluator::evaluateExpression(inv.expression, encoded.observed)) {
            const auto parsed = PolicyEvaluator::parseExpression(inv.expression); const auto found = encoded.observed.find(parsed.keyPath);
            result.violations.push_back({.invariantID = inv.invariantID, .keyPath = parsed.keyPath, .actualValue = found == encoded.observed.end() ? Json(nullptr) : found->second, .expectedValue = parsed.expectedValue, .severity = inv.severity});
        }
        result.valid = !analyzer.hasDrift() && result.violations.empty();
    } catch (const std::exception& error) { result.errors.push_back(error.what()); result.failure = failure("VALIDATE", "FAILED", error.what()); }
    return result;
}
IHealingEngine::DiffResult HealingEngine::diff(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath) {
    auto observed = adapter_->observe(configPath); auto desired = loadDesired(desiredPath); (void)loadInvariants(invariantsPath);
    if (desired.data.contains("semanticRequest")) return diffSemantic(configPath, desired.data.at("semanticRequest"));
    const auto encoded = adapter_->encode(observed, desired); DriftAnalyzer analyzer(encoded); DiffResult result; result.drifts = analyzer.analyze();
    // Surface differences remain informational. They never authorize a repair.
    result.failure = failure("PLAN", "BLOCKED", "Explicit adopted semantic request required for recovery planning"); return result;
}
IHealingEngine::HealResult HealingEngine::heal(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath, const std::string& auditPath) {
    HealResult result;
    try {
        (void)loadInvariants(invariantsPath); auto desired = loadDesired(desiredPath);
        if (desired.data.contains("semanticRequest")) return healSemantic(configPath, desired.data.at("semanticRequest"), auditPath);
        result.failure = failure("PROJECT", "BLOCKED", "Explicit adopted semantic request required; surface differences do not grant mutation authority");
    } catch (const std::exception& error) { result.failure = failure("POLICY", "FAILED", error.what()); }
    result.message = result.failure->message; return result;
}
IHealingEngine::ValidationResult HealingEngine::validateSemantic(const std::string& path, const Json& input) {
    ValidationResult result;
    if (!environment_) { result.failure = failure("PROJECT", "BLOCKED", "Adopted semantic environment unavailable"); return result; }
    try {
        const auto bytes = fs_->readFile(path); actualObservation(*fs_, *adapter_, path, input, bytes); auto computed = environment_->analyze(input);
        if (auto error = std::get_if<OperationFailure>(&computed)) { result.failure = *error; result.errors.push_back(error->message); return result; }
        result.semanticAnalysis = std::get<SemanticAnalysis>(std::move(computed));
        const auto invariants = environment_->evaluateInvariants(input, Json::parse(bytes), "LIFECYCLE_SUCCESS_POSTCONDITION", Json::array());
        result.valid = result.semanticAnalysis->stateClass == SystemClass::Stable && invariants.passed;
        if (!invariants.passed) { result.failure = failure("VALIDATE", "BLOCKED", "Mandatory lifecycle invariant violation or unavailable evidence"); result.failure->evidence = invariants.results; }
    } catch (const std::exception& error) { result.failure = failure("VALIDATE", "FAILED", error.what()); result.errors.push_back(error.what()); }
    return result;
}
IHealingEngine::DiffResult HealingEngine::diffSemantic(const std::string& path, const Json& input) {
    DiffResult result; if (!environment_) { result.failure = failure("PLAN", "BLOCKED", "Adopted semantic environment unavailable"); return result; }
    try {
        actualObservation(*fs_, *adapter_, path, input, fs_->readFile(path)); auto computed = environment_->plan(input);
        if (auto error = std::get_if<OperationFailure>(&computed)) result.failure = *error;
        else { result.semanticPlan = std::get<SemanticPlan>(std::move(computed)); result.proposedPlan = compatibilityPlan(*result.semanticPlan); }
    } catch (const std::exception& error) { result.failure = failure("PLAN", "FAILED", error.what()); }
    return result;
}
IHealingEngine::HealResult HealingEngine::healSemantic(const std::string& path, const Json& input, const std::string& auditPath) {
    HealResult result;
    const auto captureRejected = [&](OperationFailure& rejected) noexcept {
        try {
            DiagnosticEvent event; event.operationID = "rejected-" + sha256(Json{{"phase", to_string(rejected.phase)}, {"code", to_string(rejected.code)}, {"message", rejected.message}}.dump());
            event.phase = rejected.phase == OperationPhase::Policy ? DiagnosticPhaseType::Policy : rejected.phase == OperationPhase::Plan ? DiagnosticPhaseType::Planning : DiagnosticPhaseType::Projection;
            event.outcome = rejected.code == OperationCode::Blocked || rejected.code == OperationCode::Ambiguous || rejected.code == OperationCode::PolicyRejected ? DiagnosticOutcomeType::Blocked : DiagnosticOutcomeType::Failed;
            event.primaryError = DiagnosticError{rejected.phase == OperationPhase::Policy ? DiagnosticErrorType::Policy : DiagnosticErrorType::Mapping, {}, {}};
            try { event.objectHash = hashCompleteSemanticInput(input); } catch (...) { event.evidence["inputHashAvailability"] = "UNAVAILABLE"; }
            event.evidence["actualFailurePhase"] = to_string(rejected.phase); event.evidence["actualFailureCode"] = to_string(rejected.code);
            const auto before = diagnostics_->health(); diagnostics_->capture(event); const auto after = diagnostics_->health(); result.diagnosticHealth = after;
            if (after.captureFailures > before.captureFailures) {
                const OperationFault fault{OperationPhase::Export, OperationCode::DiagnosticUnavailable, "Rejected operation diagnostic capture failed", Json{{"captureFailuresBefore", before.captureFailures}, {"captureFailuresAfter", after.captureFailures}}};
                if (result.diagnosticErrors.size() < 32) result.diagnosticErrors.push_back(fault); else if (result.diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++result.diagnosticErrorsDropped;
                if (rejected.secondaryFailures.size() < 32) rejected.secondaryFailures.push_back(fault);
            }
        } catch (...) { if (result.diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++result.diagnosticErrorsDropped; }
    };
    if (!environment_) { result.failure = failure("PROJECT", "BLOCKED", "Adopted semantic environment unavailable"); result.message = result.failure->message; captureRejected(*result.failure); return result; }
    try {
        if (auditPath.empty()) throw std::invalid_argument("Actual audit storage path is required");
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); std::tm utc{};
        if (gmtime_s(&utc, &now) != 0) throw std::runtime_error("Actual lifecycle time unavailable");
        std::ostringstream timestamp; timestamp << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
        auto computed = environment_->plan(input);
        if (auto error = std::get_if<OperationFailure>(&computed)) { result.failure = *error; result.message = error->message; result.diagnosticChain = error->diagnosticChain; captureRejected(*result.failure); return result; }
        const auto& planned = std::get<SemanticPlan>(computed); result.semanticAnalysis = planned.analysis;
        if (planned.operations.empty()) {
            // STABLE/NO_ACTION has a real read-only lifecycle; it cannot invent
            // the required frozen BackupResult or acquire a mutation capability.
            const auto identity = fs_->pathIdentity(path), auditIdentity = fs_->pathIdentity(auditPath);
            if (identity.empty() || auditIdentity.empty() || identity == auditIdentity) throw std::runtime_error("Target/audit identity unavailable or aliased");
            const auto original = fs_->readFile(path); actualObservation(*fs_, *adapter_, path, input, original);
            result.rawHashBefore = sha256(original);
            const auto invariants = environment_->evaluateInvariants(input, Json::parse(original), "LIFECYCLE_SUCCESS_POSTCONDITION", Json::array());
            if (!invariants.passed) throw std::runtime_error("Mandatory read-only lifecycle predicate unavailable or violated");
            const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/system-state-classification.pseudo.md";
            auto bundle = makeAshDiagnosticBundle({"STATE_VALIDITY", "CLASSIFICATION", "RESOLVED", planned.planHash,
                {"Actual captured observation remains unchanged; all mandatory predicates passed; STABLE maps to NO_ACTION"}}, "INFO", "Read-only semantic lifecycle verified",
                Json{{"inputHash", hashCompleteSemanticInput(input)}, {"stateClass", to_string(planned.analysis.stateClass)}, {"recoveryCategory", to_string(planned.analysis.recovery)}, {"configurationWriteCalls", 0}},
                Json::array({Json{{"ruleID", "ASH-CLASSIFICATION-MAPPING-001"}, {"ruleFamily", "ASH-CLASSIFICATION"}, {"sourceReference", source}}}), {source}, &planned.analysis.diagnosticChain);
            result.diagnosticChain = bundle.chain;
            DiagnosticEvent event; event.operationID = planned.planHash; event.phase = DiagnosticPhaseType::Completion; event.outcome = DiagnosticOutcomeType::Passed; event.objectHash = hashCompleteSemanticInput(input); event.evidence = Json{{"stateClass", "STABLE"}, {"recoveryCategory", "NO_ACTION"}};
            const auto before = diagnostics_->health(); diagnostics_->capture(event); const auto after = diagnostics_->health(); result.diagnosticHealth = after;
            if (after.captureFailures > before.captureFailures) {
                try { if (result.diagnosticErrors.size() < 32) result.diagnosticErrors.push_back({OperationPhase::Export, OperationCode::DiagnosticUnavailable, "Read-only completion collector reported capture loss", Json{{"captureFailuresBefore", before.captureFailures}, {"captureFailuresAfter", after.captureFailures}}});
                    else if (result.diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++result.diagnosticErrorsDropped;
                } catch (...) { if (result.diagnosticErrorsDropped < std::numeric_limits<std::uint64_t>::max()) ++result.diagnosticErrorsDropped; }
            }
            auto audit = std::make_shared<BaseBindings::BaseAuditSink>(auditPath, fs_);
            auto retained = audit->registerDiagnosticChain(BaseContracts::DiagnosticChain(bundle.chain));
            if (auto error = std::get_if<OperationFailure>(&retained)) throw RejectedOperation(*error);
            const Json frozenEvent{{"auditEventID", "event-" + sha256(Json{{"planHash", planned.planHash}, {"time", timestamp.str()}, {"phase", "RESULT"}}.dump())},
                {"eventSequence", 1}, {"eventType", "RESULT"}, {"diagnosticReference", bundle.reference}, {"occurredAt", timestamp.str()}, {"details", Json{{"stateClass", "STABLE"}, {"recoveryCategory", "NO_ACTION"}, {"configurationWriteCalls", 0}}}};
            const BaseContracts::AuditChain chain(Json{{"auditChainID", "audit-" + sha256(Json{{"planHash", planned.planHash}, {"time", timestamp.str()}, {"targetIdentity", identity}}.dump())},
                {"diagnosticChainReference", bundle.chain.at("chainID")}, {"events", Json::array({frozenEvent})}});
            auto persisted = audit->persistChain(chain); if (auto error = std::get_if<OperationFailure>(&persisted)) throw RejectedOperation(*error);
            auto readback = audit->readChain(chain.value().at("auditChainID").get<std::string>()); if (auto error = std::get_if<OperationFailure>(&readback)) throw RejectedOperation(*error);
            if (std::get<BaseContracts::AuditChain>(readback) != chain || fs_->readFile(path) != original || fs_->pathIdentity(path) != identity) throw std::runtime_error("Read-only lifecycle audit/preimage verification failed");
            result.auditChain = chain.value(); result.auditEvents.push_back({frozenEvent.at("auditEventID").get<std::string>(), AuditEventType::NoDrift, timestamp.str(), path, frozenEvent.at("details")});
            result.success = true; result.verification.success = true; result.rawHashAfter = result.rawHashBefore; result.message = "STABLE/NO_ACTION with verified mandatory lifecycle predicates"; return result;
        }
        auto audit = std::make_shared<BaseBindings::BaseAuditSink>(auditPath, fs_);
        NativeExecutionSession session(*environment_, input, path, timestamp.str(), fs_, adapter_, backup_, audit, diagnostics_, policyGate_);
        const auto outcome = session.run();
        result.success = outcome.success; result.verification.success = outcome.success; result.failure = outcome.failure;
        result.rawHashBefore = outcome.rawHashBefore; result.rawHashAfter = outcome.rawHashAfter; result.backupPath = outcome.backupPath;
        result.diagnosticChain = outcome.diagnosticChain; result.auditChain = outcome.auditChain;
        result.diagnosticErrors = outcome.diagnosticErrors;
        result.diagnosticErrorsDropped = outcome.diagnosticErrorsDropped; result.diagnosticHealth = outcome.diagnosticHealth;
        if (outcome.semanticPlan) {
            result.semanticAnalysis = outcome.semanticPlan->analysis;
            if (outcome.completedOperationCount) { auto executed = *outcome.semanticPlan; executed.operations = Json::array();
                for (std::size_t index = 0; index < outcome.completedOperationCount; ++index) executed.operations.array_items().push_back(outcome.semanticPlan->operations.at(index));
                result.executedPlan = compatibilityPlan(executed);
            }
        }
        result.mutationMayHaveOccurred = outcome.mutationState == MutationState::MayHaveMutated || outcome.mutationState == MutationState::Mutated || outcome.mutationState == MutationState::RestoreFailed || outcome.mutationState == MutationState::Unknown;
        if (outcome.mutationState == MutationState::VerifiedRestored) result.restoration = RestorationStatus::Verified;
        else if (outcome.mutationState == MutationState::RestoreFailed) result.restoration = RestorationStatus::Failed;
        if (result.restoration != RestorationStatus::NotNeeded && result.backupPath && outcome.semanticPlan) result.rollback = RollbackPlan{.planID = outcome.semanticPlan->planHash, .backupFilePath = *result.backupPath, .originalFilePath = path};
        if (!outcome.auditChain.is_null()) for (const auto& event : outcome.auditChain.at("events").array_items()) {
            const auto phase = event.at("eventType").get<std::string>(); AuditEventType type = AuditEventType::ValidationPerformed;
            if (phase == "PLAN") type = AuditEventType::DiffGenerated; else if (phase == "POLICY_GATE") type = AuditEventType::HealStarted; else if (phase == "BACKUP") type = AuditEventType::BackupCreated;
            else if (phase == "EXECUTE") type = event.at("details").value("stage", std::string{}) == "SUCCEEDED" ? AuditEventType::RepairApplied : AuditEventType::HealStarted;
            else if (phase == "VERIFY" || phase == "RESULT") type = event.at("details").value("status", std::string{}) == "FAILED" ? AuditEventType::VerificationFailed : AuditEventType::VerificationSucceeded;
            else if (phase == "ROLLBACK") type = AuditEventType::RollbackExecuted;
            auto details = event.at("details"); details["sourceEventType"] = phase;
            result.auditEvents.push_back({event.at("auditEventID").get<std::string>(), type, event.at("occurredAt").get<std::string>(), path, details});
        }
        result.message = outcome.success ? "Authorized semantic repair applied and independently verified" : outcome.failure ? outcome.failure->message : "Native execution did not produce a verified result";
        return result;
    } catch (const RejectedOperation& error) { result.failure = error.failure; }
    catch (const std::exception& error) { result.failure = operationFailureFromException(OperationPhase::Project, OperationCode::Failed, error); }
    catch (...) { result.failure = failure("PROJECT", "FAILED", "Nonstandard host failure before execution authority"); }
    result.message = result.failure->message;
    captureRejected(*result.failure);
    for (const auto& fault : result.diagnosticErrors) if (result.failure->secondaryFailures.size() < 32) {
        bool present = false; for (const auto& prior : result.failure->secondaryFailures) if (prior.phase == fault.phase && prior.code == fault.code && prior.evidence == fault.evidence) present = true;
        if (!present) result.failure->secondaryFailures.push_back(fault);
    }
    if (!result.diagnosticChain.is_null()) { result.failure->diagnosticChain = result.diagnosticChain; result.failure->diagnosticPersistence = DiagnosticPersistence::NotRequested; std::erase_if(result.failure->unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; }); }
    return result;
}
} // namespace Aeostara
