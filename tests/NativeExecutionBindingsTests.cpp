// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "CppUnitTest.h"
#include "AeostaraCore/NativeExecutionBindings.h"
#include "AeostaraCore/BaseDocumentBindings.h"
#include "AeostaraCore/JsonConfigAdapter.h"
#include "AeostaraCore/BackupManager.h"
#include "AeostaraCore/WindowsAtomicFileSystem.h"
#include <filesystem>
#include <atomic>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <thread>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
namespace {
std::string read(const std::string& path) {
    const auto owned = std::filesystem::path(AEOSTARA_EXECUTION_FIXTURES).parent_path() / std::filesystem::path(path).filename();
    std::ifstream file(owned, std::ios::binary); if (!file) throw std::runtime_error("Owned product fixture definition unavailable: " + owned.string()); std::ostringstream text; text << file.rdbuf(); return text.str();
}
template<class T> T success(OperationAttempt<T> result) { if (auto error = std::get_if<OperationFailure>(&result)) throw std::runtime_error(error->message); return std::get<T>(std::move(result)); }
struct TestLease final : ITargetWriteLease {};
class TestFS final : public Aeostara::IFileSystem {
public:
    std::map<std::string, std::string> files;
    std::string version;
    int reads = 0, targetWrites = 0;
    bool auditFail = false;
    std::string readFile(const std::string& path) override { ++reads; if (!files.contains(path)) throw std::runtime_error("Actual file absent"); return files.at(path); }
    void writeFile(const std::string& path, const std::string& bytes) override { if (path == "audit" && auditFail) throw std::runtime_error("Actual audit write rejected"); files[path] = bytes; if (path == "configuration") ++targetWrites; }
    bool fileExists(const std::string& path) override { return files.contains(path); }
    bool copyFile(const std::string& source, const std::string& destination) override { writeFile(destination, readFile(source)); return readFile(source) == readFile(destination); }
    bool writeFileExclusive(const std::string& path, const std::string& bytes) override { if (files.contains(path)) return false; writeFile(path, bytes); return true; }
    std::string pathIdentity(const std::string& path) override { return path == "configuration" ? "fixture:configuration:demo-state-carrier" : "identity:" + path; }
    std::string versionToken(const std::string&) override { return version; }
    std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string&) override { return std::make_unique<TestLease>(); }
};
class TestBackup final : public Aeostara::IBackupProvider {
public:
    std::shared_ptr<TestFS> fs;
    bool corrupt = false, restoreFails = false, attempted = false, capability = true;
    int restorationCalls = 0;
    explicit TestBackup(std::shared_ptr<TestFS> value) : fs(std::move(value)) {}
    bool supportsBackup() const noexcept override { return capability; }
    std::string createBackup(const std::string& path) override { fs->files["backup"] = corrupt ? "{}" : fs->readFile(path); return "backup"; }
    bool restoreBackup(const std::string& backup, const std::string& target) override { attempted = true; ++restorationCalls; if (restoreFails) throw std::runtime_error("Actual injected restore failure"); fs->writeFile(target, fs->readFile(backup)); return true; }
};
class TestAdapter final : public Aeostara::IConfigAdapter {
public:
    JsonConfigAdapter actual;
    bool partialThrow = false;
    explicit TestAdapter(std::shared_ptr<TestFS> fs) : actual(std::move(fs)) {}
    ObservedState observe(const std::string& path) override { return actual.observe(path); }
    EncodedState encode(const ObservedState& a, const DesiredState& b) override { return actual.encode(a,b); }
    bool applyRepair(const std::string&, const RepairPlan&) override { throw std::runtime_error("Legacy repair must not execute"); }
    bool applyOperations(const std::string& path, const Json& operations, const std::string& hash) override { if (partialThrow) { actual.applyOperations(path, Json::array({operations.at(std::size_t{0})}), hash); throw std::invalid_argument("Actual partial-write adapter exception"); } return actual.applyOperations(path, operations, hash); }
};
class TestClock final : public IDiagnosticClock {
public:
    std::string utcNow() const override { return "2026-10-11T18:00:00Z"; }
    std::uint64_t monotonicMilliseconds() const override { return 100; }
};
class FailingDiagnostics final : public IDiagnostics {
public:
    mutable DiagnosticHealth loss;
    std::shared_ptr<IDiagnostics> actual = createDefaultDiagnostics();
    void capture(const DiagnosticEvent&) noexcept override { ++loss.captureFailures; }
    DiagnosticSnapshot snapshot() const override { auto snapshot = actual->snapshot(); snapshot.document["loss"]["captureFailures"] = loss.captureFailures; return snapshot; }
    DiagnosticProfileType profile() const noexcept override { return actual->profile(); }
    DiagnosticHealth health() const noexcept override { return loss; }
};
Json actualInput() {
    const auto fixtures = Json::parse(read(AEOSTARA_EXECUTION_FIXTURES));
    for (const auto& vector : fixtures.at("vectors").array_items()) if (vector.at("phaseUnderTest") == Json("CONTROLLED_EXECUTION")) return vector.at("input");
    throw std::runtime_error("Complete actual input absent");
}
SemanticEnvironment environment(const Json& input) { return {read(input.at("mappingProfile").at("path").get<std::string>()), read(input.at("registryReferences").at("recognizedValidStates").at("path").get<std::string>()), read(input.at("registryReferences").at("fallback").at("path").get<std::string>()), read(AEOSTARA_EXECUTION_AUTHORITY)}; }
BaseContracts::JsonDocumentReference document(const Json& snapshot, const char* role) { return BaseContracts::JsonDocumentReference(Json{{"documentID", snapshot.at("revision")}, {"sourceReference", snapshot.at("sourceName")}, {"role", role}, {"canonicalHash", snapshot.at("proposedCanonicalSha256")}}); }
struct Harness final {
    Json input = actualInput();
    std::shared_ptr<TestFS> fs = std::make_shared<TestFS>();
    std::shared_ptr<TestBackup> backup = std::make_shared<TestBackup>(fs);
    std::shared_ptr<TestAdapter> adapter = std::make_shared<TestAdapter>(fs);
    std::shared_ptr<BaseBindings::BaseAuditSink> audit = std::make_shared<BaseBindings::BaseAuditSink>("audit", fs);
    std::shared_ptr<NativeExecutionSession> session;
    std::unique_ptr<BaseBindings::NativeExecutionBindings> bridges;
    std::optional<BaseContracts::ObservedSystemState> observed;
    std::optional<BaseContracts::DesiredSystemIntent> desired;
    std::optional<BaseContracts::SemanticProjectionSpec> spec;
    Harness() {
        fs->files["configuration"] = input.at("observedSnapshot").at("rawUtf8").get<std::string>(); fs->version = input.at("observedSnapshot").at("revision").get<std::string>();
        session = std::make_shared<NativeExecutionSession>(environment(input), input, "configuration", "2026-10-11T18:00:00Z", fs, adapter, backup, audit, createDefaultDiagnostics(), std::shared_ptr<BaseBindings::IPolicyGate>{}, std::make_shared<TestClock>());
        const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md";
        BaseBindings::BaseDocumentBindings docs({{document(input.at("observedSnapshot"), "OBSERVED"), input.at("observedSnapshot").at("rawUtf8").get<std::string>()}, {document(input.at("desiredSnapshot"), "DESIRED"), input.at("desiredSnapshot").at("rawUtf8").get<std::string>()}},
            environment(input), input, Json::array({Json{{"ruleID", "ASH-STATE-STRUCTURE-001"}, {"ruleFamily", "ASH-STATE"}, {"sourceReference", source}}}), {source});
        observed = success(docs.observe(document(input.at("observedSnapshot"), "OBSERVED"))); desired = success(docs.desiredIntent(document(input.at("desiredSnapshot"), "DESIRED"), false)); spec = success(docs.projectionSpecification());
        bridges = std::make_unique<BaseBindings::NativeExecutionBindings>(session, *observed, *desired, *spec, docs.diagnosticChains());
    }
};
BaseBindings::ByteSequence bytes(const std::string& text) { BaseBindings::ByteSequence result; for (const auto c : text) result.push_back(static_cast<std::byte>(static_cast<unsigned char>(c))); return result; }
struct RealHarness final {
    Json input = actualInput();
    std::filesystem::path directory;
    std::string target, auditPath;
    std::shared_ptr<DefaultFileSystem> fs = std::make_shared<DefaultFileSystem>();
    std::shared_ptr<JsonConfigAdapter> adapter = std::make_shared<JsonConfigAdapter>(fs);
    std::shared_ptr<BackupManager> backup = std::make_shared<BackupManager>(fs);
    std::shared_ptr<BaseBindings::BaseAuditSink> audit;
    std::shared_ptr<IDiagnostics> diagnostics = createDefaultDiagnostics();
    std::shared_ptr<NativeExecutionSession> session;
    RealHarness() {
        static std::atomic<unsigned> sequence{0};
        directory = std::filesystem::temp_directory_path() / ("aeostara-native-session-" + sha256(std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ":" + std::to_string(sequence++)));
        if (!std::filesystem::create_directory(directory)) throw std::runtime_error("Actual unique test directory unavailable");
        target = (directory / "configuration.json").string(); auditPath = (directory / "audit.jsonl").string();
        fs->writeFile(target, input.at("observedSnapshot").at("rawUtf8").get<std::string>());
        input["observedSnapshot"]["revision"] = fs->versionToken(target);
        input["proposedPlan"]["expectedObservedVersion"] = fs->versionToken(target);
        input["proposedPlan"]["expectedObservedInputSha256"] = sha256(input.at("observedSnapshot").dump());
        input["proposedPlan"]["adapterTargetIdentity"] = fs->pathIdentity(target);
        input["policyContext"]["evaluationRequest"]["adapterTargetIdentity"] = fs->pathIdentity(target);
        input["policyContext"]["evaluationRequest"]["planContentSha256"] = sha256(input.at("proposedPlan").dump());
        audit = std::make_shared<BaseBindings::BaseAuditSink>(auditPath, fs);
        session = std::make_shared<NativeExecutionSession>(environment(input), input, target, "2026-10-11T18:00:00Z", fs, adapter, backup, audit, diagnostics, std::shared_ptr<BaseBindings::IPolicyGate>{}, std::make_shared<TestClock>(), true);
    }
    ~RealHarness() { session.reset(); audit.reset(); backup.reset(); adapter.reset(); fs.reset(); std::error_code ignored; std::filesystem::remove_all(directory, ignored); }
};
}

namespace AeostaraExecutionTests {
TEST_CLASS(NativeExecutionBindingsTests) {
public:
    TEST_METHOD(CapturedDryRunIsDeterministicAndHasNoHostCalls) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto reads = h.fs->reads;
        const auto a = success(h.bridges->dryRun(*h.observed, *h.desired, *h.spec, plan.policyBundle)); const auto b = success(h.bridges->dryRun(*h.observed, *h.desired, *h.spec, plan.policyBundle));
        Assert::IsTrue(a == b); Assert::IsTrue(a.value().at("wouldMutate").get<bool>()); Assert::AreEqual(reads, h.fs->reads); Assert::AreEqual(0, h.fs->targetWrites); Assert::IsFalse(h.fs->files.contains("backup")); Assert::IsFalse(h.fs->files.contains("audit"));
    }
    TEST_METHOD(ActualPhaseDiagnosticsPreserveDetectionClassificationAndExactSourceRules) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto& diagnostics = plan.diagnosticChain.value().at("diagnostics"); const char* phases[] = {"OBSERVE", "NORMALIZE", "PROJECT", "DIAGNOSE", "CLASSIFY", "PLAN"};
        Assert::AreEqual(std::size_t{6}, diagnostics.size());
        for (std::size_t i = 0; i < diagnostics.size(); ++i) {
            const auto& envelope = diagnostics.at(i); Assert::IsTrue(envelope.at("evidence").at("actualPhase") == Json(phases[i])); Assert::IsTrue(envelope.at("evidence").at("ashDiagnostic").at("stage") == Json(i < 4 ? "DETECTION" : "CLASSIFICATION"));
            Assert::IsTrue(envelope.at("evidence").at("inputHash") == Json(hashCompleteSemanticInput(h.input))); Assert::IsFalse(envelope.at("ruleReferences").empty()); Assert::IsFalse(envelope.at("sourceReferences").empty());
        }
    }
    TEST_METHOD(CompletePhaseInputsProduceMeasuredBackupExecutionVerificationAndHeal) {
        Harness h; const auto planned = success(h.session->planCaptured()); const auto context = success(h.bridges->prepareExecution());
        const auto backup = success(h.bridges->prepareBackup(context, planned.recoveryPlan)); Assert::IsTrue(backup.value().at("backupRecord").at("sourceHash") == Json(sha256(h.fs->files.at("backup")))); Assert::AreEqual(0, h.fs->targetWrites);
        const auto step = BaseContracts::RecoveryStep(planned.recoveryPlan.value().at("recoverySteps").at(std::size_t{0})); const auto executed = success(h.bridges->executeStep(context, step));
        Assert::IsTrue(executed.value().at("observedHashAfterStep") == Json(sha256(h.fs->files.at("configuration"))));
        Assert::IsTrue(success(h.bridges->verify(context, planned.verificationPlan)).value().at("status") == Json("PASSED"));
        const auto done = success(h.session->complete(context)); Assert::IsTrue(done.value().at("status") == Json("HEALED")); Assert::IsTrue(h.session->outcome().success); Assert::AreEqual(1, h.fs->targetWrites);
        Assert::IsTrue(success(h.audit->readChain(done.value().at("auditChain").at("auditChainID").get<std::string>())).value() == done.value().at("auditChain"));
    }
    TEST_METHOD(FrozenHealingFacadeDelegatesActualCompleteLifecycle) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto context = success(h.bridges->prepareExecution());
        const auto healed = success(h.bridges->heal(*h.observed, *h.desired, *h.spec, plan.policyBundle, context)); Assert::IsTrue(healed.value().at("status") == Json("HEALED")); Assert::IsFalse(healed.value().at("auditChain").at("events").empty()); Assert::IsFalse(healed.value().at("diagnosticChain").at("diagnostics").empty());
    }
    TEST_METHOD(TamperedContextAndForeignStepDoNotMutate) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto context = success(h.bridges->prepareExecution()); auto bad = context.value(); bad["recoveryPlanID"] = "foreign";
        const auto step = BaseContracts::RecoveryStep(plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0})); Assert::IsTrue(std::holds_alternative<OperationFailure>(h.bridges->executeStep(BaseContracts::ExecutionContext(bad), step)));
        auto wrong = step.value(); wrong["jsonMutationOperations"][0]["value"] = 1; Assert::IsTrue(std::holds_alternative<OperationFailure>(h.bridges->executeStep(context, BaseContracts::RecoveryStep(wrong)))); Assert::AreEqual(0, h.fs->targetWrites);
        Assert::IsTrue(std::holds_alternative<OperationFailure>(h.bridges->verify(context, plan.verificationPlan))); Assert::AreEqual(0, h.fs->targetWrites);
    }
    TEST_METHOD(IndividualMutationBoundaryEnforcesOrderScopeAndWholePlanVerification) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto context = success(h.bridges->prepareExecution()); const auto& ops = plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0}).at("jsonMutationOperations");
        Assert::IsTrue(std::holds_alternative<OperationFailure>(h.bridges->applyMutation(context, BaseContracts::JsonMutationOperation(ops.at(std::size_t{1}))))); Assert::AreEqual(0, h.fs->targetWrites);
        for (const auto& op : ops.array_items()) (void)success(h.bridges->applyMutation(context, BaseContracts::JsonMutationOperation(op)));
        Assert::AreEqual(static_cast<int>(ops.size()), h.fs->targetWrites); Assert::IsTrue(success(h.bridges->verify(context, plan.verificationPlan)).value().at("status") == Json("PASSED")); (void)success(h.session->complete(context));
    }
    TEST_METHOD(StaleObservationAndInvalidBackupRejectBeforeTargetMutation) {
        Harness stale; stale.fs->files["configuration"] = "{}"; Assert::IsTrue(std::holds_alternative<OperationFailure>(stale.session->prepareExecution())); Assert::AreEqual(0, stale.fs->targetWrites);
        Harness corrupt; corrupt.backup->corrupt = true; Assert::IsTrue(std::holds_alternative<OperationFailure>(corrupt.session->prepareExecution())); Assert::AreEqual(0, corrupt.fs->targetWrites);
        Harness unavailable; unavailable.backup->capability = false; Assert::IsTrue(std::holds_alternative<OperationFailure>(unavailable.session->prepareExecution())); Assert::AreEqual(0, unavailable.fs->targetWrites);
    }
    TEST_METHOD(PartialInvalidArgumentMutationRestoresExactOriginalAndRetainsFailure) {
        Harness h; const auto original = h.fs->files.at("configuration"); h.adapter->partialThrow = true; const auto result = h.session->run();
        Assert::IsFalse(result.success); Assert::IsTrue(result.failure.has_value()); Assert::IsTrue(h.backup->attempted); Assert::IsTrue(h.fs->files.at("configuration") == original); Assert::IsTrue(result.mutationState == MutationState::VerifiedRestored); Assert::AreEqual(2, h.fs->targetWrites); Assert::IsFalse(result.diagnosticChain.is_null());
        Assert::IsTrue(result.failure->diagnosticPersistence == DiagnosticPersistence::Persisted); (void)operationFailureJson(*result.failure);
    }
    TEST_METHOD(RestorationFailureRetainsMeasuredMutationAndBothFailures) {
        Harness h; const auto original = h.fs->files.at("configuration"); const auto plan = success(h.session->planCaptured()); const auto context = success(h.session->prepareExecution());
        const BaseContracts::RecoveryStep step(plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0}));
        h.adapter->partialThrow = true; h.backup->restoreFails = true; Assert::IsTrue(std::holds_alternative<OperationFailure>(h.session->executeStep(context, step))); const auto result = h.session->outcome();
        Assert::IsFalse(result.success); Assert::IsTrue(result.failure.has_value()); Assert::IsTrue(result.mutationState == MutationState::RestoreFailed); Assert::IsTrue(h.fs->files.at("configuration") != original); Assert::IsFalse(result.failure->secondaryFailures.empty()); Assert::IsTrue(result.rawHashAfter.has_value()); Assert::IsTrue(*result.rawHashAfter == sha256(h.fs->files.at("configuration")));
        const auto failure = operationFailureJson(*result.failure); const auto writes = h.fs->targetWrites; const auto restoreCalls = h.backup->restorationCalls; const auto actualPartial = h.fs->files.at("configuration");
        h.adapter->partialThrow = false; h.backup->restoreFails = false;
        const auto remainsStopped = [&](const auto& attempted) { Assert::IsTrue(std::holds_alternative<OperationFailure>(attempted)); Assert::IsTrue(operationFailureJson(std::get<OperationFailure>(attempted)) == failure); };
        remainsStopped(h.session->prepareExecution()); remainsStopped(h.session->prepareBackup(context, plan.recoveryPlan)); remainsStopped(h.session->executeStep(context, step));
        remainsStopped(h.session->applyMutation(context, BaseContracts::JsonMutationOperation(step.value().at("jsonMutationOperations").at(std::size_t{0}))));
        remainsStopped(h.session->verify(context, plan.verificationPlan)); remainsStopped(h.session->complete(context)); remainsStopped(h.session->rollbackPlan());
        remainsStopped(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::BackupSnapshot, BaseBindings::AtomicOperation::Write));
        const auto rerun = h.session->run(); Assert::IsFalse(rerun.success); Assert::IsTrue(operationFailureJson(*rerun.failure) == failure);
        Assert::AreEqual(writes, h.fs->targetWrites); Assert::AreEqual(restoreCalls, h.backup->restorationCalls); Assert::AreEqual(actualPartial, h.fs->files.at("configuration"));
        h.bridges.reset(); h.session.reset(); Assert::AreEqual(writes, h.fs->targetWrites); Assert::AreEqual(restoreCalls, h.backup->restorationCalls);
    }
    TEST_METHOD(AuditPersistenceFailureBlocksTargetMutation) {
        Harness h; h.fs->auditFail = true; const auto result = h.session->run(); Assert::IsFalse(result.success); Assert::IsTrue(result.failure.has_value()); Assert::AreEqual(0, h.fs->targetWrites); Assert::IsFalse(h.fs->files.contains("backup"));
        Assert::IsTrue(result.failure->diagnosticPersistence == DiagnosticPersistence::Failed); (void)operationFailureJson(*result.failure);
    }
    TEST_METHOD(ForeignThreadCannotExerciseOwnedExecutionLease) {
        Harness h; const auto plan = success(h.session->planCaptured()); const auto context = success(h.session->prepareExecution()); const auto step = BaseContracts::RecoveryStep(plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0})); bool rejected = false;
        std::thread other([&] { rejected = std::holds_alternative<OperationFailure>(h.bridges->executeStep(context, step)); }); other.join(); Assert::IsTrue(rejected); Assert::AreEqual(0, h.fs->targetWrites);
    }
    TEST_METHOD(ActualFailedVerificationAuthorizesFrozenRollbackReceiptWithoutHealingSuccess) {
        Harness h; const auto original = h.fs->files.at("configuration"); const auto plan = success(h.session->planCaptured()); const auto context = success(h.bridges->prepareExecution());
        (void)success(h.bridges->executeStep(context, BaseContracts::RecoveryStep(plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0}))));
        h.fs->files["configuration"] = original;
        Assert::IsTrue(std::holds_alternative<OperationFailure>(h.bridges->verify(context, plan.verificationPlan)));
        const auto rollback = success(h.session->rollbackPlan()); const auto result = success(h.bridges->executeRollback(context, rollback));
        Assert::IsTrue(result.value().at("status") == Json("SUCCEEDED")); Assert::IsTrue(result.value().at("restoredHash") == Json(sha256(original))); Assert::IsFalse(h.session->outcome().success); Assert::IsTrue(h.session->outcome().failure.has_value());
    }
    TEST_METHOD(RealWindowsAuditCreationBindsActualFileIdentityAndCompletesHeldLeaseRepair) {
        RealHarness h; const auto future = h.fs->pathIdentity(h.auditPath); const auto context = success(h.session->prepareExecution()); const auto plan = success(h.session->planCaptured());
        Assert::IsTrue(future != h.fs->pathIdentity(h.auditPath));
        (void)success(h.session->executeStep(context, BaseContracts::RecoveryStep(plan.recoveryPlan.value().at("recoverySteps").at(std::size_t{0}))));
        (void)success(h.session->complete(context)); Assert::IsTrue(h.session->outcome().success); Assert::IsTrue(Json::parse(h.fs->readFile(h.target)) == plan.semanticPlan.expectedDocument);
    }
    TEST_METHOD(CompletedSessionAuthorizesAtomicBackupWriteWithActualReadExistsAndHash) {
        RealHarness h; const auto context = success(h.session->prepareExecution()); const auto result = h.session->run(); Assert::IsTrue(result.success);
        const auto cap = success(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::BackupSnapshot, BaseBindings::AtomicOperation::Write));
        BaseBindings::WindowsAtomicFileSystem atomic(h.directory.string(), cap);
        Assert::IsTrue(success(atomic.exists(cap->targetPath()))); Assert::IsTrue(success(atomic.computeSHA256(cap->targetPath())) == sha256(cap->exactResult())); Assert::IsTrue(success(atomic.read(cap->targetPath())) == bytes(cap->exactResult()));
        const auto receipt = success(atomic.writeAtomically(cap->targetPath(), bytes(cap->operationInput()))); Assert::IsTrue(receipt.bytesTransferred == cap->exactResult().size()); Assert::IsTrue(h.fs->readFile(cap->targetPath()) == cap->exactResult()); Assert::IsTrue(sha256(h.fs->readFile(h.target)) == cap->carrierRawHash());
    }
    TEST_METHOD(CompletedSessionAuthorizesActualAuditAppendWithoutInventingAnEvent) {
        RealHarness h; const auto context = success(h.session->prepareExecution()); Assert::IsTrue(h.session->run().success);
        const auto cap = success(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::AuditSnapshot, BaseBindings::AtomicOperation::Append));
        BaseBindings::WindowsAtomicFileSystem atomic(h.directory.string(), cap); const auto receipt = success(atomic.appendAtomically(cap->targetPath(), bytes(cap->operationInput())));
        Assert::IsTrue(receipt.bytesTransferred == cap->exactResult().size()); Assert::IsTrue(h.fs->readFile(h.auditPath) == cap->exactResult());
        Assert::IsTrue(success(h.audit->readChain(cap->auditChain().value().at("auditChainID").get<std::string>())) == cap->auditChain()); Assert::IsTrue(sha256(h.fs->readFile(h.target)) == cap->carrierRawHash());
    }
    TEST_METHOD(CompletedSessionAuthorizesAtomicCopyOnlyFromDistinctMeasuredOwnedBackup) {
        RealHarness h; const auto context = success(h.session->prepareExecution()); Assert::IsTrue(h.session->run().success);
        const auto cap = success(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::BackupSnapshot, BaseBindings::AtomicOperation::Copy));
        Assert::IsTrue(cap->copySourceRecord().has_value()); Assert::IsTrue(cap->sourceIdentity() != cap->targetIdentity()); Assert::IsTrue(cap->sourcePath() != h.target);
        BaseBindings::WindowsAtomicFileSystem atomic(h.directory.string(), cap); const auto receipt = success(atomic.copyAtomically(cap->sourcePath(), cap->targetPath()));
        Assert::IsTrue(receipt.bytesTransferred == cap->exactResult().size()); Assert::IsTrue(h.fs->readFile(cap->targetPath()) == cap->exactResult()); Assert::IsTrue(sha256(h.fs->readFile(h.target)) == cap->carrierRawHash());
    }
    TEST_METHOD(PersistenceCapabilityRejectsIncompleteForeignAndChangedCarrierWithoutWrites) {
        RealHarness h; const auto context = success(h.session->prepareExecution()); const auto original = h.fs->readFile(h.target);
        Assert::IsTrue(std::holds_alternative<OperationFailure>(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::BackupSnapshot, BaseBindings::AtomicOperation::Write))); Assert::IsTrue(h.fs->readFile(h.target) == original);
        Assert::IsTrue(h.session->run().success); auto foreign = context.value(); foreign["executionContextID"] = "foreign";
        Assert::IsTrue(std::holds_alternative<OperationFailure>(h.session->authorizeAtomicPersistence(BaseContracts::ExecutionContext(foreign), BaseBindings::NativePersistencePurpose::AuditSnapshot, BaseBindings::AtomicOperation::Write)));
        const auto cap = success(h.session->authorizeAtomicPersistence(context, BaseBindings::NativePersistencePurpose::BackupSnapshot, BaseBindings::AtomicOperation::Write)); const auto backupBefore = h.fs->readFile(cap->targetPath()); h.fs->writeFile(h.target, original);
        BaseBindings::WindowsAtomicFileSystem atomic(h.directory.string(), cap); Assert::IsTrue(std::holds_alternative<OperationFailure>(atomic.writeAtomically(cap->targetPath(), bytes(cap->operationInput())))); Assert::IsTrue(h.fs->readFile(cap->targetPath()) == backupBefore);
    }
    TEST_METHOD(ActualWindowsNativeFailurePreservesErrorCategoryCodeAndOwningPhase) {
        RealHarness h; std::filesystem::remove(h.target); const auto result = h.session->prepareExecution(); Assert::IsTrue(std::holds_alternative<OperationFailure>(result)); const auto& failure = std::get<OperationFailure>(result);
        Assert::IsTrue(failure.phase == OperationPhase::Parse); Assert::IsTrue(failure.evidence.contains("nativeError")); Assert::IsTrue(failure.evidence.at("nativeError").at("value").get<int>() != 0); Assert::IsFalse(failure.evidence.at("nativeError").at("category").get<std::string>().empty()); Assert::IsTrue(failure.mutationState == MutationState::NotStarted);
        const auto snapshot = h.diagnostics->snapshot(); bool retained = false;
        for (const auto& event : snapshot.document.at("events").array_items()) if (!event.at("primaryError").is_null() && event.at("primaryError").at("nativeCode") == failure.evidence.at("nativeError").at("value")) retained = true;
        Assert::IsTrue(retained);
    }
    TEST_METHOD(FailingDeveloperCollectorPreservesSuccessfulRepairAndRequiredBusinessAudit) {
        Harness h; auto failing = std::make_shared<FailingDiagnostics>();
        NativeExecutionSession session(environment(h.input), h.input, "configuration", "2026-10-11T18:00:00Z", h.fs, h.adapter, h.backup, h.audit, failing, {}, std::make_shared<TestClock>());
        const auto result = session.run(); Assert::IsTrue(result.success); Assert::IsFalse(result.failure.has_value()); Assert::IsTrue(result.diagnosticHealth.captureFailures > 0); Assert::IsFalse(result.diagnosticErrors.empty()); Assert::IsTrue(result.diagnosticErrors.size() <= 32);
        Assert::IsTrue(Json::parse(h.fs->files.at("configuration")) == result.semanticPlan->expectedDocument);
        Assert::IsTrue(success(h.audit->readChain(result.auditChain.at("auditChainID").get<std::string>())).value() == result.auditChain);
        const auto exported = makeDiagnosticExport(failing->snapshot()); Assert::IsTrue(Json::parse(exported.json).at("loss").at("captureFailures").get<std::uint64_t>() > 0);
    }
    TEST_METHOD(FailingDeveloperCollectorKeepsOriginalExecutionFailureAndVerifiedRestoration) {
        Harness h; h.adapter->partialThrow = true; auto failing = std::make_shared<FailingDiagnostics>(); const auto original = h.fs->files.at("configuration");
        NativeExecutionSession session(environment(h.input), h.input, "configuration", "2026-10-11T18:00:00Z", h.fs, h.adapter, h.backup, h.audit, failing, {}, std::make_shared<TestClock>());
        const auto result = session.run(); Assert::IsFalse(result.success); Assert::IsTrue(result.failure.has_value()); Assert::IsTrue(result.failure->phase == OperationPhase::Execute); Assert::IsTrue(result.failure->code == OperationCode::InvalidInput); Assert::IsTrue(result.mutationState == MutationState::VerifiedRestored); Assert::IsTrue(h.fs->files.at("configuration") == original);
        bool loss = false; for (const auto& secondary : result.failure->secondaryFailures) if (secondary.code == OperationCode::DiagnosticUnavailable) loss = true;
        Assert::IsTrue(loss); Assert::IsTrue(result.diagnosticHealth.captureFailures > 0); (void)operationFailureJson(*result.failure);
    }
};
} // namespace AeostaraExecutionTests
