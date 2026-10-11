// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include "CppUnitTest.h"
#include "AeostaraCore/HealingEngine.h"
#include "AeostaraCore/JsonConfigAdapter.h"
#include "AeostaraCore/BackupManager.h"
#include "TestHelpers.h"
#include "SemanticTestInput.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;
namespace AeostaraCoreTests {
namespace {
void seedSemantic(MockFileSystem& fs, const Json& input) {
    fs.files["config.json"] = input.at("observedSnapshot").at("rawUtf8").get<std::string>();
    fs.files["desired.json"] = Json{{"semanticRequest", input}}.dump();
}
HealingEngine engineFor(const std::shared_ptr<MockFileSystem>& fs) {
    return HealingEngine(std::make_shared<JsonConfigAdapter>(fs), std::make_shared<BackupManager>(fs), fs, std::make_shared<SemanticEnvironment>(SemanticEnvironment::adoptedDirect()));
}
class LosingDiagnostics final : public IDiagnostics {
public:
    std::shared_ptr<IDiagnostics> actual = createDefaultDiagnostics();
    std::uint64_t losses = 0;
    void capture(const DiagnosticEvent&) noexcept override { ++losses; }
    DiagnosticSnapshot snapshot() const override { return actual->snapshot(); }
    DiagnosticProfileType profile() const noexcept override { return actual->profile(); }
    DiagnosticHealth health() const noexcept override { auto result = actual->health(); result.captureFailures += losses; return result; }
};
}
TEST_CLASS(HealingEngineTests) { public:
TEST_METHOD(Validate_ValidConfig_NoDrift) {
    auto fs = std::make_shared<MockFileSystem>(); fs->files["config.json"] = SAMPLE_CONFIG; fs->files["desired.json"] = SAMPLE_DESIRED; fs->files["invariants.json"] = SAMPLE_INVARIANTS;
    auto engine = engineFor(fs); const auto result = engine.validate("config.json", "desired.json", "invariants.json"); Assert::IsTrue(result.valid); Assert::AreEqual(size_t{0}, result.drifts.size()); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Validate_InvalidJSON_ReportsError) {
    auto fs = std::make_shared<MockFileSystem>(); fs->files["config.json"] = "{ invalid JSON }"; fs->files["desired.json"] = SAMPLE_DESIRED;
    auto engine = engineFor(fs); const auto result = engine.validate("config.json", "desired.json", ""); Assert::IsFalse(result.valid); Assert::IsFalse(result.errors.empty()); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Heal_PolicyBlocked_NoRepair) {
    auto fs = std::make_shared<MockFileSystem>(); auto input = repairInput(); input["policyContext"]["bundle"]["rules"].at(size_t{0})["allowedPointers"] = Json::array(); input["policyContext"]["bundleSha256"] = sha256(input.at("policyContext").at("bundle").dump()); seedSemantic(*fs, input); const auto original = fs->readFile("config.json");
    auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl"); Assert::IsFalse(result.success); Assert::IsTrue(result.failure.has_value()); Assert::IsTrue(result.failure->phase == OperationPhase::Policy); Assert::AreEqual(original, fs->readFile("config.json")); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Heal_RepairableConfig_FixesDrift) {
    auto fs = std::make_shared<MockFileSystem>(); const auto input = repairInput(); seedSemantic(*fs, input); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsTrue(result.success); Assert::IsTrue(result.verification.success); Assert::IsFalse(result.auditEvents.empty()); Assert::IsTrue(Json::parse(fs->readFile("config.json")) == input.at("desiredSnapshot").at("document")); Assert::IsTrue(result.semanticAnalysis->stateClass == SystemClass::Correctable);
}
TEST_METHOD(Heal_RepairableConfig_CreatesBackup) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, repairInput()); const auto original = fs->readFile("config.json"); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsTrue(result.success); Assert::IsTrue(result.backupPath.has_value()); Assert::IsTrue(fs->fileExists(*result.backupPath)); Assert::AreEqual(original, fs->readFile(*result.backupPath));
}
TEST_METHOD(Heal_RepairableConfig_WritesAuditTrail) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, repairInput()); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsTrue(result.success); Assert::IsTrue(fs->fileExists("audit.jsonl")); Assert::IsFalse(fs->readFile("audit.jsonl").empty());
}
TEST_METHOD(Heal_VerificationFails_ExecutesRollback) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, repairInput()); const auto original = fs->readFile("config.json"); bool corrupted = false;
    fs->afterWrite = [fs, &corrupted](const std::string& path, const std::string& bytes) { if (path == "config.json" && !corrupted) { corrupted = true; auto value = Json::parse(bytes); value.replace_pointer("/state_vector/0", Json(1)); fs->files[path] = value.dump(); } };
    auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsTrue(corrupted); Assert::IsFalse(result.success); Assert::IsFalse(result.verification.success); Assert::IsTrue(result.rollback.has_value()); Assert::IsTrue(result.restoration == RestorationStatus::Verified); Assert::IsTrue(result.failure->mutationState == MutationState::VerifiedRestored); Assert::AreEqual(original, fs->readFile("config.json")); Assert::IsFalse(result.failure->diagnosticChain.is_null());
}
TEST_METHOD(Diff_DetectsDrift) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, repairInput()); auto engine = engineFor(fs); const auto result = engine.diff("config.json", "desired.json", "");
    Assert::IsTrue(result.semanticPlan.has_value()); Assert::AreEqual(size_t{4}, result.proposedPlan.actions.size()); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Diff_NoDrift_EmptyResult) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, healthyInput()); auto engine = engineFor(fs); const auto result = engine.diff("config.json", "desired.json", "");
    Assert::IsTrue(result.semanticPlan.has_value()); Assert::AreEqual(size_t{0}, result.proposedPlan.actions.size()); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Heal_NoDrift_SucceedsWithNoDriftMessage) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, healthyInput()); const auto original = fs->readFile("config.json"); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsTrue(result.success); Assert::AreEqual(size_t{0}, fs->configurationWrites); Assert::AreEqual(original, fs->readFile("config.json"));
}
TEST_METHOD(Heal_GenericDifferenceHasNoMutationAuthority) {
    auto fs = std::make_shared<MockFileSystem>(); fs->files["config.json"] = DRIFTED_CONFIG; fs->files["desired.json"] = SAMPLE_DESIRED; const auto original = fs->readFile("config.json"); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "", "audit.jsonl");
    Assert::IsFalse(result.success); Assert::IsTrue(result.failure->code == OperationCode::Blocked); Assert::AreEqual(size_t{0}, fs->configurationWrites); Assert::AreEqual(original, fs->readFile("config.json"));
}
TEST_METHOD(Validate_NoDriftInvariantViolationIsInvalid) {
    auto fs = std::make_shared<MockFileSystem>(); fs->files["config.json"] = SAMPLE_CONFIG; fs->files["desired.json"] = SAMPLE_DESIRED; fs->files["invariants.json"] = BLOCKING_INVARIANTS; auto engine = engineFor(fs); const auto result = engine.validate("config.json", "desired.json", "invariants.json");
    Assert::IsFalse(result.valid); Assert::IsFalse(result.violations.empty()); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Heal_UnavailableInvariantFileFailsBeforeMutation) {
    auto fs = std::make_shared<MockFileSystem>(); seedSemantic(*fs, repairInput()); auto engine = engineFor(fs); const auto result = engine.heal("config.json", "desired.json", "missing-invariants.json", "audit.jsonl"); Assert::IsFalse(result.success); Assert::AreEqual(size_t{0}, fs->configurationWrites);
}
TEST_METHOD(Heal_CollectorLossPreservesVerifiedNoActionAndMutationOutcomes) {
    for (const bool repair : {false, true}) {
        auto fs = std::make_shared<MockFileSystem>(); const auto input = repair ? repairInput() : healthyInput(); seedSemantic(*fs, input); const auto before = fs->readFile("config.json");
        auto sink = std::make_shared<LosingDiagnostics>();
        HealingEngine engine(std::make_shared<JsonConfigAdapter>(fs), std::make_shared<BackupManager>(fs), fs, std::make_shared<SemanticEnvironment>(SemanticEnvironment::adoptedDirect()), sink);
        const auto result = engine.healSemantic("config.json", input, "audit.jsonl");
        Assert::IsTrue(result.success); Assert::IsTrue(result.verification.success); Assert::IsFalse(result.failure.has_value()); Assert::IsTrue(sink->losses > 0); Assert::IsFalse(result.diagnosticErrors.empty());
        Assert::IsTrue(result.diagnosticHealth.has_value()); Assert::AreEqual(sink->losses, result.diagnosticHealth->captureFailures); Assert::AreEqual(std::uint64_t{0}, result.diagnosticErrorsDropped);
        Assert::IsTrue(result.restoration == RestorationStatus::NotNeeded); Assert::IsFalse(result.diagnosticChain.is_null()); Assert::IsTrue(fs->fileExists("audit.jsonl"));
        if (repair) Assert::IsTrue(Json::parse(fs->readFile("config.json")) == input.at("desiredSnapshot").at("document"));
        else { Assert::AreEqual(before, fs->readFile("config.json")); Assert::AreEqual(size_t{0}, fs->configurationWrites); }
    }
}
};
}
