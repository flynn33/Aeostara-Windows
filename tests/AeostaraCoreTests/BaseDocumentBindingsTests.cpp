#include "AeostaraCore/BaseDocumentBindings.h"
#include "CppUnitTest.h"
#include "DocumentBindingTestInputs.inc"
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace Aeostara;
using namespace Aeostara::BaseBindings;
namespace {
std::size_t assertions = 0;
void check(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
Json rules() { return Json::array({Json{{"ruleID", "ASH-STATE-STRUCTURE-001"}, {"ruleFamily", "ASH-STATE"}, {"sourceReference", "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md"}}}); }
std::vector<std::string> sources() { return {"https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/algorithms/semantic_projection.pseudo.md", "supplement-sha256:98539d83ad6199cecc26eb7b9fcb92993073a2e511ebf70191330cd46ba35dc7"}; }
BaseContracts::JsonDocumentReference reference(const Json& snapshot, const char* role) { return BaseContracts::JsonDocumentReference(Json{{"documentID", snapshot.at("revision")}, {"role", role}, {"canonicalHash", snapshot.at("proposedCanonicalSha256")}, {"sourceReference", snapshot.at("sourceName")}}); }
std::vector<CapturedDocument> captures(const Json& input) { return {{reference(input.at("observedSnapshot"), "OBSERVED"), input.at("observedSnapshot").at("rawUtf8").get<std::string>()}, {reference(input.at("desiredSnapshot"), "DESIRED"), input.at("desiredSnapshot").at("rawUtf8").get<std::string>()}}; }
SemanticEnvironment environment(const Json&) { return SemanticEnvironment::adoptedDirect(); }
template<class T> T success(OperationAttempt<T> value) { if (auto failure = std::get_if<OperationFailure>(&value)) throw std::runtime_error(failure->message); return std::get<T>(std::move(value)); }
BaseContracts::JsonPointer pointer(const std::string& text, const Json* value) { return BaseContracts::JsonPointer(Json{{"pointer", text}, {"tokens", Json(Json::pointer_tokens(text))}, {"pointsToMissing", value == nullptr}, {"pointsToNull", value && value->is_null()}}); }
void resetSnapshot(Json& input, const char* field, const Json& document) { auto& snapshot = input[field]; snapshot["document"] = document; snapshot["rawUtf8"] = document.canonical(); snapshot["rawByteSha256"] = sha256(document.canonical()); snapshot["proposedCanonicalSha256"] = sha256(document.canonical()); }
}
int exerciseDocumentBindings() {

    try {
        const auto fixture = Json::parse(documentBindingInputs); const auto input = fixture.at("vectors").at(0).at("input");
        BaseDocumentBindings bridge(captures(input), environment(input), input, rules(), sources());
        const auto observedRef = reference(input.at("observedSnapshot"), "OBSERVED"); const auto desiredRef = reference(input.at("desiredSnapshot"), "DESIRED");
        const auto observed = success(bridge.observe(observedRef)); const auto desired = success(bridge.desiredIntent(desiredRef, false)); const auto spec = success(bridge.projectionSpecification());
        const auto mapped = success(bridge.project(observed, desired, spec));
        check(mapped.value().at("projectedState").at("coordinates") == input.at("observedSnapshot").at("document").at("state_vector"), "Actual carrier projection differs");
        check(mapped.value().at("dimensionBindings").size() == 9, "Coordinate evidence count differs");
        check(success(bridge.project(observed, desired, spec)) == mapped, "Identical complete inputs are not deterministic");
        check(success(bridge.canonicalize(observedRef)).value().at("role") == Json("CANONICALIZED"), "Canonical reference role differs");
        check(!bridge.diagnosticChains().empty(), "Material diagnostics absent");
        check(std::holds_alternative<BaseContracts::DiagnosticChain>(bridge.diagnosticChain(BaseContracts::DiagnosticReference(mapped.value().at("diagnosticReferences").at(0)))), "Mapping diagnostic chain unresolved");
        auto wrong = observedRef.value(); wrong["canonicalHash"] = std::string(64, '0'); check(std::holds_alternative<OperationFailure>(bridge.observe(BaseContracts::JsonDocumentReference(wrong))), "Foreign hash accepted");
        auto wrongSpec = spec.value(); wrongSpec["dimensions"][0]["mappingRule"] = "unadopted"; check(std::holds_alternative<OperationFailure>(bridge.project(observed, desired, BaseContracts::SemanticProjectionSpec(wrongSpec))), "Foreign projection rule accepted");
        auto wrongObserved = observed.value(); wrongObserved["surfaceDifferences"] = Json::array(); wrongObserved["observedSystemStateID"] = "foreign"; check(std::holds_alternative<OperationFailure>(bridge.project(BaseContracts::ObservedSystemState(wrongObserved), desired, spec)), "Foreign observed identity accepted");
        auto pointerInput = input; auto document = input.at("observedSnapshot").at("document"); document["feature.flag"] = "enabled"; document["nullable"] = nullptr; document["a/b~c"] = 12;
        resetSnapshot(pointerInput, "observedSnapshot", document);
        BaseDocumentBindings pointers(captures(pointerInput), environment(pointerInput), pointerInput, rules(), sources()); const auto pointerRef = reference(pointerInput.at("observedSnapshot"), "OBSERVED");
        check(std::get<Json>(success(pointers.resolvePointer(pointerRef, pointer("/feature.flag", document.resolve_pointer("/feature.flag")))).value) == Json("enabled"), "Dot-containing key was not literal");
        check(std::get<Json>(success(pointers.resolvePointer(pointerRef, pointer("/a~1b~0c", document.resolve_pointer("/a~1b~0c")))).value) == Json(12), "Escaped pointer failed");
        check(std::holds_alternative<Absent>(success(pointers.resolvePointer(pointerRef, pointer("/missing", nullptr))).value), "Missing became null");
        check(std::get<Json>(success(pointers.resolvePointer(pointerRef, pointer("/nullable", &document.at("nullable")))).value).is_null(), "Null became missing");
        check(std::holds_alternative<OperationFailure>(pointers.resolvePointer(pointerRef, pointer("/nullable", nullptr))), "False presence flags accepted");
        auto differenceInput = pointerInput; auto target = pointerInput.at("desiredSnapshot").at("document"); target["feature.flag"] = "disabled"; target["new"] = nullptr; resetSnapshot(differenceInput, "desiredSnapshot", target);
        BaseDocumentBindings differences(captures(differenceInput), environment(differenceInput), differenceInput, rules(), sources()); const auto diffs = success(differences.generateSurfaceDifferences(reference(differenceInput.at("observedSnapshot"), "OBSERVED"), reference(differenceInput.at("desiredSnapshot"), "DESIRED")));
        check(!diffs.empty(), "Surface evidence absent"); check(success(differences.generateSurfaceDifferences(reference(differenceInput.at("observedSnapshot"), "OBSERVED"), reference(differenceInput.at("desiredSnapshot"), "DESIRED"))) == diffs, "Surface ordering/identity nondeterministic");
        for (const auto& diff : diffs) check(std::holds_alternative<BaseContracts::DiagnosticChain>(differences.diagnosticChain(BaseContracts::DiagnosticReference(diff.value().at("diagnosticReference")))), "Surface chain unresolved");
        auto missingInput = input; auto missing = input.at("observedSnapshot").at("document"); missing["state_vector"].array_items().pop_back(); resetSnapshot(missingInput, "observedSnapshot", missing);
        BaseDocumentBindings missingBridge(captures(missingInput), environment(missingInput), missingInput, rules(), sources()); const auto missingResult = missingBridge.project(success(missingBridge.observe(reference(missingInput.at("observedSnapshot"), "OBSERVED"))), success(missingBridge.desiredIntent(reference(missingInput.at("desiredSnapshot"), "DESIRED"), false)), success(missingBridge.projectionSpecification()));
        check(std::holds_alternative<OperationFailure>(missingResult), "Unresolved coordinate fabricated complete DTO"); const auto& error = std::get<OperationFailure>(missingResult); check(error.mappingStatus == MappingStatus::Blocked && error.evidence.at("dimensions").size() == 9, "Missing projection evidence lost"); check(!error.diagnosticChain.is_null(), "Missing projection diagnostic chain absent"); check(!error.mutationMayHaveOccurred(), "Pure projection claims mutation");
        check(input == fixture.at("vectors").at(0).at("input"), "Input mutation during pure bridge operations");
        bool testedPlan = false;
        for (const auto& vector : fixture.at("vectors").array_items()) {
            const auto& planInput = vector.at("input");
            if (!planInput.is_object() || !planInput.contains("mappingProfile")) continue;
            auto env = environment(planInput); auto planned = env.plan(planInput);
            if (!std::holds_alternative<SemanticPlan>(planned) || std::get<SemanticPlan>(planned).operations.empty()) continue;
            const auto& plan = std::get<SemanticPlan>(planned); BaseDocumentBindings plannedBridge(captures(planInput), env, planInput, rules(), sources());
            const auto planRef = reference(planInput.at("observedSnapshot"), "OBSERVED"); const auto observation = success(plannedBridge.observe(planRef)); const auto& first = plan.operations.at(0); const auto path = first.at("pointer").get<std::string>();
            Json operation{{"operationID", "operation-" + sha256(first.canonical())}, {"op", first.at("operation")}, {"path", pointer(path, planInput.at("observedSnapshot").at("document").resolve_pointer(path)).value()}, {"value", first.at("value")}, {"preconditionHash", plan.rawPreimageHash}, {"diagnosticReference", observation.value().at("diagnosticReference")}};
            check(std::holds_alternative<BaseContracts::DiagnosticReference>(plannedBridge.validateMutationPreconditions(planRef, BaseContracts::JsonMutationOperation(operation))), "Actual adopted planned operation precondition rejected");
            auto stale = operation; stale["preconditionHash"] = std::string(64, '0'); auto staleResult = plannedBridge.validateMutationPreconditions(planRef, BaseContracts::JsonMutationOperation(stale)); check(std::holds_alternative<OperationFailure>(staleResult) && std::get<OperationFailure>(staleResult).code == OperationCode::StaleState, "Stale precondition accepted");
            auto unsafe = operation; unsafe["path"] = pointer("", &planInput.at("observedSnapshot").at("document")).value(); check(std::holds_alternative<OperationFailure>(plannedBridge.validateMutationPreconditions(planRef, BaseContracts::JsonMutationOperation(unsafe))), "Whole document mutation accepted outside adopted inverse");
            auto foreign = operation; foreign["diagnosticReference"]["diagnosticID"] = "foreign"; check(std::holds_alternative<OperationFailure>(plannedBridge.validateMutationPreconditions(planRef, BaseContracts::JsonMutationOperation(foreign))), "Foreign operation diagnostic accepted");
            testedPlan = true; break;
        }
        check(testedPlan, "No actual independently computable adopted plan exercised");
        std::cout << "BaseDocumentAssertions=" << assertions << " Failed=0 PureCapturedSnapshots=true\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

namespace AeostaraCoreTests {
TEST_CLASS(BaseDocumentBindingsTests) {
public:
    TEST_METHOD(CapturedProjectionPointerSurfaceAndPlanGuards) { Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(0, exerciseDocumentBindings()); }
};
}
