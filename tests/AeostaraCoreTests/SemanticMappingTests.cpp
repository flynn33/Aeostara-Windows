// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include "CppUnitTest.h"
#include "AeostaraCore/IAshBindingProvider.h"
#include "AeostaraCore/NativeRequestBinding.h"
#include "AeostaraCore/HealingEngine.h"
#include "AeostaraCore/JsonConfigAdapter.h"
#include "AeostaraCore/BackupManager.h"
#include "AeostaraCore/PolicyEvaluator.h"
#include "SemanticTestInput.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <filesystem>
#include <atomic>
#include <chrono>
#include <thread>
#include <system_error>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
namespace AeostaraCoreTests {
namespace {
template<class T> T actual(OperationAttempt<T> result) {
    if (const auto* error = std::get_if<OperationFailure>(&result)) Assert::Fail(std::wstring(error->message.begin(), error->message.end()).c_str());
    return std::get<T>(std::move(result));
}
Json stateValue(const AshState& state, const std::string& source = "test:explicit-generation-context") {
    const auto coordinates = stateJson(state); const auto hash = sha256(coordinates.dump());
    return {{"stateID", "ASH-STATE-" + hash}, {"coordinates", coordinates}, {"canonicalHash", hash}, {"sourceReference", source}};
}
Json bindContent(Json object) { object["contentHash"] = sha256(object.dump()); return object; }
Json axiomSubject(const BaseContracts::AshState& state) {
    return {{"stateReference", state.value().at("stateID")}, {"sourceReference", "test:explicit-measured-subject"},
        {"relation_count", 1}, {"compressed_measure", 2}, {"raw_measure", 3}, {"scale_count", 2},
        {"erasure_cost", 1}, {"self_reference_capable", true}, {"self_model_available", true}};
}
Json generationBindings() {
    const auto env = SemanticEnvironment::adoptedDirect(); const AshState zero{};
    const BaseContracts::AshState state(stateValue(zero));
    const Json topologyContext{{"depth", 1}, {"seed", "test-root"}, {"sourceReference", "test:explicit-topology-request"}};
    AshBindingProvider topologyProducer(env, Json{{"topology", topologyContext}});
    const auto topology = actual(topologyProducer.generateTopology(state)).value();
    Json roles = Json::array(), descriptions = Json::array();
    const auto target = bindContent(Json{{"sourceReference", "test:explicit-abstract-target"}, {"revision", "1"}, {"emissionTargetKind", "ABSTRACT"}});
    for (const auto& node : topology.at("nodes").array_items()) {
        const auto id = node.at("node_id").get<std::string>();
        roles.array_items().push_back(Json{{"nodeID", id}, {"roleID", "explicit-role:" + id}, {"profileID", "TEST-PROFILE"}, {"sourceReference", "test:registered-role-assignment"}});
        descriptions.array_items().push_back(Json{{"nodeID", id}, {"roleID", "explicit-role:" + id}, {"elementID", "element:" + id},
            {"namingPolicyID", "TEST-NAMING"}, {"projectName", "TestProject"}, {"emissionTargetKind", "ABSTRACT"}, {"targetConstraintsHash", target.at("contentHash")},
            {"artifact", Json{{"artifactID", "artifact:" + id}, {"kind", "abstract-description"}, {"name", id}, {"contentDescription", "Explicit host-supplied description"}}}});
    }
    auto subject = axiomSubject(state); subject["revision"] = "1"; subject["topologyHash"] = sha256(topology.dump()); subject["roleAssignmentsHash"] = sha256(roles.dump()); subject = bindContent(subject);
    Json context{{"sourceReference", "test:explicit-generation-context"},
        {"request", Json{{"seed_state", stateJson(zero)}, {"project_name", "TestProject"}, {"profile_id", "TEST-PROFILE"}, {"naming_policy_id", "TEST-NAMING"}, {"emission_target_kind", "ABSTRACT"}, {"transition_sequence", Json::array()}, {"topology_depth", 1}}},
        {"registeredProfile", bindContent(Json{{"sourceReference", "test:registered-profile"}, {"revision", "1"}, {"profileID", "TEST-PROFILE"}, {"roleAssignments", roles}})},
        {"registeredNamingPolicy", bindContent(Json{{"sourceReference", "test:registered-naming"}, {"revision", "1"}, {"namingPolicyID", "TEST-NAMING"}, {"artifactDescriptions", descriptions}})},
        {"targetConstraints", target}, {"transitionRegistry", bindContent(Json{{"sourceReference", "test:registered-transitions"}, {"revision", "1"}, {"entries", Json::array()}})},
        {"topology", topologyContext}, {"axiomSubject", subject}, {"artifactDescriptions", descriptions}};
    return {{"generation", context}};
}
std::string utf8(const std::filesystem::path& path) { const auto text = path.u8string(); return {reinterpret_cast<const char*>(text.data()), text.size()}; }
class TemporaryDirectory final {
public:
    TemporaryDirectory() {
        static std::atomic<unsigned> sequence = 0;
        const auto token = std::to_string(GetCurrentProcessId()) + "-" + std::to_string(GetTickCount64()) + "-" + std::to_string(sequence.fetch_add(1));
        root = std::filesystem::temp_directory_path() / ("AeostaraNative-" + token);
        if (!std::filesystem::create_directory(root)) throw std::runtime_error("Cannot create unique native test directory");
    }
    ~TemporaryDirectory() { std::error_code error; std::filesystem::remove_all(root, error); }
    std::string path(const char* name) const { return utf8(root / name); }
    std::filesystem::path root;
};
}
TEST_CLASS(SemanticMappingTests) { public:
TEST_METHOD(AllPublicStateConsumersRejectNonbinaryCoordinates) {
    auto env = SemanticEnvironment::adoptedDirect();
    for (std::size_t coordinate = 0; coordinate < 9; ++coordinate) {
        AshState malformed{}; malformed[coordinate] = 2;
        Assert::ExpectException<std::invalid_argument>([&] { (void)env.diagnose(malformed); });
        Assert::ExpectException<std::invalid_argument>([&] { (void)stateJson(malformed); });
        StateDiagnostic diagnostic; diagnostic.state = malformed;
        Assert::ExpectException<std::invalid_argument>([&] { (void)classifySystemState(diagnostic, false, false, false, false); });
    }
}
TEST_METHOD(FullCodewordOrbitIndependentlyDiagnosesAll512States) {
    auto env = SemanticEnvironment::adoptedDirect(); std::size_t valid = 0, compatible = 0, incompatible = 0;
    for (unsigned number = 0; number < 512; ++number) {
        AshState state{}; for (unsigned bit = 0; bit < 9; ++bit) state[bit] = static_cast<unsigned char>((number >> (8 - bit)) & 1);
        const auto diagnosis = env.diagnose(state); Assert::AreEqual(size_t{16}, diagnosis.orbit.size());
        if (diagnosis.valid) ++valid; else if (diagnosis.admissibility == "TRANSFORMATION_COMPATIBLE") ++compatible; else ++incompatible;
    }
    Assert::AreEqual(size_t{2}, valid); Assert::AreEqual(size_t{30}, compatible); Assert::AreEqual(size_t{480}, incompatible);
}
TEST_METHOD(RealmEncoderRejectsUnrecognizedStateWithoutNormalization) {
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), Json::object()); AshState compatible = canonicalCodewords().at(1);
    const auto rejected = provider.resolveRealmIdentity(BaseContracts::AshState(stateValue(compatible)));
    Assert::IsTrue(std::holds_alternative<OperationFailure>(rejected)); Assert::IsFalse(std::get<OperationFailure>(rejected).diagnosticChain.is_null());
    const auto realm = actual(provider.resolveRealmIdentity(BaseContracts::AshState(stateValue(AshState{}))));
    Assert::AreEqual(std::string("000000000"), realm.value().at("encoding").get<std::string>());
    Assert::AreEqual(std::string("ASH-REALM-000000000"), realm.value().at("realmID").get<std::string>());
}
TEST_METHOD(AxiomDiagnosticsExplainEveryPredicateAndComputeOverallConjunction) {
    const BaseContracts::AshState state(stateValue(AshState{})); auto subject = axiomSubject(state); subject["self_model_available"] = false;
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), Json{{"axiomSubject", subject}});
    const auto diagnostics = actual(provider.evaluateAxioms(state)); Assert::AreEqual(size_t{5}, diagnostics.size());
    for (std::size_t i = 0; i < diagnostics.size(); ++i) { const auto& value = diagnostics[i].value(); Assert::IsFalse(value.at("evidence").at("overall_pass").get<bool>()); Assert::IsFalse(value.at("evidence").at("explanation").get<std::string>().empty()); Assert::IsTrue(value.at("diagnosticEnvelope").at("evidence").contains("ashDiagnostic")); Assert::IsTrue(value.at("evaluationResult") == Json(i == 4 ? "FAIL" : "PASS")); }
}
TEST_METHOD(CompleteGenerationBindingsProduceDeterministicPureEmission) {
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), generationBindings()); const BaseContracts::AshState state(stateValue(AshState{}));
    const auto first = actual(provider.generateMaterializationPlan(state)); const auto second = actual(provider.generateMaterializationPlan(state)); Assert::IsTrue(first == second);
    Assert::AreEqual(size_t{3}, first.value().at("planElements").size()); Assert::AreEqual(size_t{5}, first.value().at("semanticInputs").at("axiomDiagnostics").size());
    const auto emission = actual(provider.planArtifactEmission(first)); Assert::AreEqual(size_t{3}, emission.value().at("artifacts").size()); Assert::AreEqual(size_t{3}, emission.value().at("traceability").size());
    AshBindingProvider planOnly(SemanticEnvironment::adoptedDirect(), Json::object());
    const auto independentEmission = actual(planOnly.planArtifactEmission(first)); Assert::IsTrue(independentEmission == emission);
    auto missing = first.value(); missing["semanticInputs"].object_items().erase("diagnosticChains"); auto content = missing; content.object_items().erase("planID"); content.object_items().erase("planHash"); missing["planHash"] = sha256(content.dump()); missing["planID"] = "generation-" + missing.at("planHash").get<std::string>();
    Assert::IsTrue(std::holds_alternative<OperationFailure>(planOnly.planArtifactEmission(BaseContracts::GenerationPlan(missing))));
}
TEST_METHOD(SuccessfulProviderReferencesResolveThroughPublicDiagnosticTransport) {
    const BaseContracts::AshState state(stateValue(AshState{})); auto bindings = generationBindings(); bindings["topology"] = bindings.at("generation").at("topology");
    const Json definition{{"transitionID", "test:zero-transition"}, {"codewordReference", "test:canonical-zero"}, {"preconditions", Json::array({Json("FULL_NINE_BINARY_STATE"), Json("CANONICAL_CODEWORD")})},
        {"diagnosticRules", Json::array({Json{{"ruleID", "ASH-CODEWORD-STRUCTURE-001"}, {"ruleFamily", "ASH-CODEWORD"}, {"sourceReference", "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/codeword-set.pseudo.md"}}})}};
    bindings["transitionRegistry"] = Json::array({Json{{"definition", definition}, {"codeword", stateJson(AshState{})}, {"sourceReference", "test:registered-zero-transition"}}});
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), bindings); const IAshBindingProvider& publicProvider = provider;
    const auto resolves = [&](const Json& reference) {
        const auto chain = actual(publicProvider.resolveDiagnosticChain(BaseContracts::DiagnosticReference(reference))).value(); validateDiagnosticChain(chain, true);
        Assert::IsTrue(chain.at("chainID") == reference.at("chainRootReference")); bool found = false;
        for (const auto& envelope : chain.at("diagnostics").array_items()) if (envelope.at("diagnosticID") == reference.at("diagnosticID")) found = envelope.at("diagnosticKind") == reference.at("diagnosticKind") && envelope.at("ruleReferences") == reference.at("ruleReferences");
        Assert::IsTrue(found);
    };
    const auto admissibility = actual(publicProvider.evaluateAdmissibility(state)); resolves(admissibility.value().at("diagnosticReference"));
    resolves(actual(publicProvider.resolveRealmIdentity(state)).value().at("diagnosticReference"));
    resolves(actual(publicProvider.resolveTransition(state, BaseContracts::TransitionDefinition(definition))).value().at("diagnosticReference"));
    const BaseContracts::CodewordTransformation transformation(Json{{"transformationID", "test:explicit-zero-xor"}, {"inputState", state.value()}, {"outputState", state.value()}, {"codeword", stateJson(AshState{})}, {"diagnosticReference", admissibility.value().at("diagnosticReference")}});
    resolves(actual(publicProvider.applyCodewordTransformation(transformation)).value().at("diagnosticReference"));
    for (const auto& reference : actual(publicProvider.generateTopology(state)).value().at("diagnosticReferences").array_items()) resolves(reference);
    const auto plan = actual(publicProvider.generateMaterializationPlan(state)); for (const auto& reference : plan.value().at("diagnosticReferences").array_items()) resolves(reference);
    const auto emission = actual(publicProvider.planArtifactEmission(plan)); for (const auto& entry : emission.value().at("traceability").array_items()) resolves(entry.at("diagnosticReference"));
    auto foreign = admissibility.value().at("diagnosticReference"); foreign["diagnosticID"] = "foreign-envelope";
    Assert::IsTrue(std::holds_alternative<OperationFailure>(publicProvider.resolveDiagnosticChain(BaseContracts::DiagnosticReference(foreign))));
}
TEST_METHOD(DiagnosticTransportBudgetRejectsNewChainsWithoutEvictingSuccessReferences) {
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), Json::object()); Json first;
    for (unsigned index = 0; index < 64; ++index) {
        const auto result = actual(provider.evaluateAdmissibility(BaseContracts::AshState(stateValue(AshState{}, "test:actual-source-" + std::to_string(index)))));
        if (index == 0) first = result.value().at("diagnosticReference");
    }
    const auto exhausted = provider.evaluateAdmissibility(BaseContracts::AshState(stateValue(AshState{}, "test:actual-source-64")));
    Assert::IsTrue(std::holds_alternative<OperationFailure>(exhausted)); Assert::IsTrue(std::get<OperationFailure>(exhausted).message.find("budget") != std::string::npos);
    Assert::IsTrue(std::holds_alternative<BaseContracts::DiagnosticChain>(provider.resolveDiagnosticChain(BaseContracts::DiagnosticReference(first))));
}
TEST_METHOD(GenerationRejectsUnboundRolesAndRehashedForeignEmission) {
    auto bindings = generationBindings(); auto damaged = bindings; damaged["generation"]["registeredProfile"]["roleAssignments"].at(size_t{0})["nodeID"] = "foreign-node";
    auto profile = damaged.at("generation").at("registeredProfile"); profile.object_items().erase("contentHash"); damaged["generation"]["registeredProfile"] = bindContent(profile);
    const BaseContracts::AshState state(stateValue(AshState{})); AshBindingProvider malformed(SemanticEnvironment::adoptedDirect(), damaged);
    Assert::IsTrue(std::holds_alternative<OperationFailure>(malformed.generateMaterializationPlan(state)));
    AshBindingProvider provider(SemanticEnvironment::adoptedDirect(), bindings); auto foreign = actual(provider.generateMaterializationPlan(state)).value(); foreign["planElements"].at(size_t{0})["artifact"]["name"] = "foreign";
    auto content = foreign; content.object_items().erase("planID"); content.object_items().erase("planHash"); foreign["planHash"] = sha256(content.dump()); foreign["planID"] = "generation-" + foreign.at("planHash").get<std::string>();
    Assert::IsTrue(std::holds_alternative<OperationFailure>(provider.planArtifactEmission(BaseContracts::GenerationPlan(foreign))));
}
TEST_METHOD(FailureSerializationPreservesPrimarySecondaryAndTruthfulRestoration) {
    OperationFailure failure(OperationPhase::Execute, OperationCode::Failed, "Original partial write error"); failure.mutationState = MutationState::VerifiedRestored;
    failure.addSecondary(OperationPhase::Audit, OperationCode::StorageFailure, "Actual restoration audit failed"); const auto value = operationFailureJson(failure);
    Assert::IsTrue(value.at("mutationState") == Json("VERIFIED_RESTORED")); Assert::IsTrue(value.at("primaryFailure").at("message") == Json("Original partial write error")); Assert::AreEqual(size_t{1}, value.at("secondaryFailures").size());
    failure.diagnosticPersistence = DiagnosticPersistence::Persisted; Assert::ExpectException<std::invalid_argument>([&] { (void)operationFailureJson(failure); });
    failure.diagnosticPersistence = DiagnosticPersistence::NotRequested; failure.diagnosticChain = SemanticEnvironment::adoptedDirect().diagnose(AshState{}).diagnosticChain; failure.unavailableEvidence.clear();
    failure.diagnosticChain["diagnostics"].at(size_t{0})["chainRootReference"] = "foreign-root";
    Assert::ExpectException<std::invalid_argument>([&] { (void)operationFailureJson(failure); });
}
TEST_METHOD(ExactPolicyOrderingDoesNotRoundLargeIntegers) {
    std::map<std::string, Json> state{{"measure", Json::parse("9007199254740993")}};
    Assert::IsTrue(PolicyEvaluator::evaluateExpression("measure > 9007199254740992", state)); Assert::IsFalse(PolicyEvaluator::evaluateExpression("measure == 9007199254740992", state));
    state["measure"] = Json::parse("1e-1000"); Assert::IsTrue(PolicyEvaluator::evaluateExpression("measure > 0", state));
}
TEST_METHOD(CapturedAggregateOver16MiBRetainsIndependentSourceLimits) {
    auto input = Testing::healthyInput(); const auto document = input.at("observedSnapshot").at("document");
    const auto raw = std::string(2 * 1024 * 1024, ' ') + document.dump();
    const auto snapshot = [&](const std::string& name) { return Json{{"sourceName", name}, {"revision", "actual-large-source-r1"}, {"document", document},
        {"rawUtf8", Json::captured_raw_utf8(raw)}, {"rawByteSha256", sha256(raw)}, {"proposedCanonicalSha256", sha256(document.dump())}}; };
    input["observedSnapshot"] = snapshot("observed"); input["desiredSnapshot"] = snapshot("desired");
    input["evidenceSnapshots"] = Json::array(); for (unsigned i = 0; i < 7; ++i) input["evidenceSnapshots"].array_items().push_back(snapshot("unused-explicit-source-" + std::to_string(i)));
    Assert::ExpectException<JsonError>([&] { (void)input.dump(); });
    const auto env = SemanticEnvironment::adoptedDirect(); const auto projected = actual(env.project(input));
    Assert::IsTrue(projected.state == AshState{}); Assert::AreEqual(size_t{64}, projected.completeSemanticInputHash.size());
    Assert::AreEqual(hashCompleteSemanticInput(input), hashCompleteSemanticInput(input));
}
TEST_METHOD(SemanticSourceCountAndAggregateRejectBeforeSuccessfulProjection) {
    auto input = Testing::healthyInput(); const auto source = input.at("desiredSnapshot");
    input["evidenceSnapshots"] = Json::array(); for (unsigned i = 0; i < 27; ++i) { auto evidence = source; evidence["sourceName"] = "source-" + std::to_string(i); input["evidenceSnapshots"].array_items().push_back(std::move(evidence)); }
    const auto result = SemanticEnvironment::adoptedDirect().project(input); Assert::IsTrue(std::holds_alternative<OperationFailure>(result));
    Assert::IsTrue(std::get<OperationFailure>(result).message.find("count") != std::string::npos);
}
TEST_METHOD(NativeExclusiveLeaseUsesOwnedHandleAndBlocksOtherWriters) {
    TemporaryDirectory directory; const auto path = directory.path("target.json"); DefaultFileSystem fs, other; Assert::IsTrue(fs.writeFileExclusive(path, "before"));
    Assert::ExpectException<std::invalid_argument>([&] { (void)fs.readFile(path + std::string("\0suffix", 7)); });
    auto lease = fs.acquireWriteLease(path); Assert::IsTrue(static_cast<bool>(lease)); Assert::ExpectException<std::system_error>([&] { (void)other.acquireWriteLease(path); });
    Assert::ExpectException<std::system_error>([&] { other.writeFile(path, "foreign"); }); fs.writeFile(path, "owned"); Assert::AreEqual(std::string("owned"), fs.readFile(path));
    bool threadRejected = false; std::thread foreign([&] { try { fs.writeFile(path, "foreign-thread"); } catch (const std::runtime_error&) { threadRejected = true; } }); foreign.join(); Assert::IsTrue(threadRejected);
    lease.reset(); other.writeFile(path, "released"); Assert::AreEqual(std::string("released"), fs.readFile(path));
}
TEST_METHOD(NativeHardlinkIdentityAndUniqueBinaryBackupAreActual) {
    TemporaryDirectory directory; auto fs = std::make_shared<DefaultFileSystem>(); const auto path = directory.path("target.bin"), alias = directory.path("alias.bin");
    const std::string original("\xEF\xBB\xBF{ }\r\n\0tail", 14); Assert::IsTrue(fs->writeFileExclusive(path, original)); Assert::IsFalse(fs->writeFileExclusive(path, "overwrite"));
    std::filesystem::create_hard_link(directory.root / "target.bin", directory.root / "alias.bin"); Assert::AreEqual(fs->pathIdentity(path), fs->pathIdentity(alias));
    BackupManager backup(fs); const auto first = backup.createBackup(path), second = backup.createBackup(path); Assert::IsTrue(first != second); Assert::AreEqual(original, fs->readFile(first));
    Assert::IsTrue(fs->pathIdentity(first) != fs->pathIdentity(path)); auto lease = fs->acquireWriteLease(path); fs->writeFile(path, "changed"); Assert::IsTrue(backup.restoreBackup(first, path)); Assert::AreEqual(original, fs->readFile(path));
}
TEST_METHOD(NativeCaptureAndSemanticRepairPreserveUnapprovedMetadataAndRawBackup) {
    TemporaryDirectory directory; auto fs = std::make_shared<DefaultFileSystem>(); const auto path = directory.path("target.json"), audit = directory.path("audit.json");
    const std::string raw = " \r\n{\"protected\":{\"mode\":\"unchanged\"},\"state_vector\":[1,1,1,1,0,0,0,0,0]}\r\n"; Assert::IsTrue(fs->writeFileExclusive(path, raw));
    auto adapter = std::make_shared<JsonConfigAdapter>(fs); const auto input = actual(bindCapturedNativeTarget(Testing::repairInput(), path, *fs, *adapter));
    auto env = std::make_shared<SemanticEnvironment>(SemanticEnvironment::adoptedDirect()); const auto planned = actual(env->plan(input)); Assert::IsTrue(planned.expectedDocument.at("protected") == Json::parse(raw).at("protected"));
    HealingEngine engine(adapter, std::make_shared<BackupManager>(fs), fs, env); const auto result = engine.healSemantic(path, input, audit);
    if (!result.success) Assert::Fail(std::wstring(result.message.begin(), result.message.end()).c_str());
    Assert::IsTrue(result.backupPath.has_value()); Assert::AreEqual(raw, fs->readFile(*result.backupPath)); Assert::IsTrue(Json::parse(fs->readFile(path)).at("protected") == Json::parse(raw).at("protected"));
}
TEST_METHOD(NativeErrorCodeIsRecordedOnlyFromActualSystemError) {
    TemporaryDirectory directory; DefaultFileSystem fs;
    try { (void)fs.readFile(directory.path("missing.json")); Assert::Fail(L"Expected actual missing-file error"); }
    catch (const std::system_error& error) { const auto result = operationFailureFromException(OperationPhase::Project, OperationCode::StorageFailure, error); Assert::IsTrue(result.evidence.at("nativeError").at("value") == Json(error.code().value())); Assert::IsTrue(error.code().value() != 0); }
}
};
} // namespace AeostaraCoreTests
