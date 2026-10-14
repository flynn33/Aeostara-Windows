// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "AeostaraCore/IAshBindingProvider.h"
#include "AeostaraCore/Diagnostics.h"
#include <algorithm>
#include <set>
#include <stdexcept>
namespace Aeostara {
namespace {
constexpr const char* sourceRoot = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/";
Json rule(const char* id, const char* family, const char* source) { return {{"ruleID", id}, {"ruleFamily", family}, {"sourceReference", std::string(sourceRoot) + source}}; }
DiagnosticBundle diagnosticBundle(const Json& evidence, const std::string& source, const char* id, const char* family, const std::string& explanation) {
    const char* ruleSource = "specs/core/ash-state-space.pseudo.md";
    if (std::string_view(id) == "ASH-STATE-VALIDITY-001") ruleSource = "specs/core/state-validity-diagnostics.pseudo.md";
    else if (std::string_view(id) == "ASH-ADMISSIBILITY-CLASSIFICATION-001") ruleSource = "specs/core/state-admissibility.pseudo.md";
    else if (std::string_view(id) == "ASH-CODEWORD-STRUCTURE-001") ruleSource = "specs/core/codeword-set.pseudo.md";
    AshDiagnosticContext context{"STATE_VALIDITY", "DETECTION", "RESOLVED", "actual-input-" + sha256(evidence.dump()), {explanation}};
    std::vector<std::string> sources{std::string(sourceRoot) + ruleSource};
    if (std::string_view(source) != ruleSource) sources.push_back(std::string(sourceRoot) + source);
    return makeAshDiagnosticBundle(context, "INFO", explanation, evidence, Json::array({rule(id, family, ruleSource)}), sources);
}
OperationFailure linkedFailure(OperationFailure error) {
    try {
        AshDiagnosticContext context{"STATE_VALIDITY", "DETECTION", "BLOCKED", "actual-failure-" + sha256(error.evidence.dump()), {error.message, "Failure concerns an actual attempted source-bound operation; unavailable input remains absent"}};
        const auto bundle = makeAshDiagnosticBundle(context, "ERROR", error.message, error.evidence, Json::array({rule("ASH-STATE-STRUCTURE-001", "ASH-STATE", "specs/core/ash-state-space.pseudo.md")}), {std::string(sourceRoot) + "specs/core/ash-state-space.pseudo.md", std::string(sourceRoot) + "specs/interfaces/diagnostic-schema.md"});
        error.diagnosticChain = bundle.chain; error.diagnosticPersistence = DiagnosticPersistence::NotRequested;
        std::erase_if(error.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
    } catch (const std::exception& diagnosticError) { error.addSecondary(OperationPhase::Export, OperationCode::DiagnosticUnavailable, diagnosticError.what()); }
    return error;
}
OperationFailure blocked(std::string phase, std::string message, const Json& evidence) { return linkedFailure(OperationFailure(operationPhase(phase), OperationCode::Blocked, std::move(message), evidence)); }
OperationFailure failed(std::string phase, const std::exception& error) { return linkedFailure(OperationFailure(operationPhase(phase), OperationCode::Failed, error.what())); }
std::optional<OperationFailure> terminalContextFailure(const Json& bindings, const char* operation, const Json& input) {
    if (!bindings.contains("completeSemanticInput")) return std::nullopt;
    const auto& complete = bindings.at("completeSemanticInput"); validateCompleteSemanticInput(complete);
    const auto& lifecycle = complete.at("lifecycleContext");
    if (!lifecycle.at("is_in_safe_halt").get<bool>()) return std::nullopt;
    Json evidence{{"attemptedOperation", operation}, {"input", input}, {"completeSemanticInputHash", hashCompleteSemanticInput(complete)},
        {"lifecycleContext", lifecycle}, {"systemStateClass", "SAFE_HALT"}, {"recoveryCategory", "TERMINAL_NO_RECOVERY"}};
    OperationFailure error(OperationPhase::Halt, OperationCode::Blocked, "Supplied SAFE_HALT lifecycle context prohibits further state transitions or generation", evidence);
    error.unavailableEvidence.push_back({"priorSafeHaltDiagnosticChain", "The complete triggering safe-halt history was not supplied; this rejection does not manufacture or resume that history"});
    AshDiagnosticContext context{"RECOVERY", "DETECTION", "TERMINAL", "terminal-attempt-" + sha256(evidence.dump()),
        {"Detected an attempted operation in an explicitly supplied terminal lifecycle", "This diagnostic records rejection, not entry into SAFE_HALT; supplied lifecycle evidence remains unchanged"}};
    const auto bundle = makeAshDiagnosticBundle(context, "ERROR", error.message, evidence,
        Json::array({rule("ASH-RECOVERY-ACTION-001", "ASH-RECOVERY", "specs/core/recoverability-semantics.pseudo.md")}),
        {std::string(sourceRoot) + "specs/core/recoverability-semantics.pseudo.md", std::string(sourceRoot) + "specs/algorithms/containment-safe-failure-semantics.pseudo.md"});
    error.diagnosticChain = bundle.chain; error.diagnosticPersistence = DiagnosticPersistence::NotRequested;
    std::erase_if(error.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
    return error;
}
AshState validatedState(const BaseContracts::AshState& dto) {
    const auto& value = dto.value(); const auto state = parseAshState(value.at("coordinates"));
    if (value.at("sourceReference").get<std::string>().empty() || value.at("stateID").get<std::string>().empty() || value.at("canonicalHash") != Json(sha256(value.at("coordinates").dump()))) throw std::invalid_argument("State identity/source/hash is not genuine"); return state;
}
OperationFailure unsuitableNormalizedState(const BaseContracts::AshState& state, const StateDiagnostic& diagnosis, const char* operation, const char* contract) {
    Json evidence{{"state", state.value()}, {"attemptedOperation", operation}, {"admissibility", diagnosis.admissibility}, {"normalizationStatus", diagnosis.normalizationStatus}};
    OperationFailure error(OperationPhase::Plan, OperationCode::Blocked, "Operation requires a recognized valid normalized state; input was not implicitly normalized", evidence);
    AshDiagnosticContext context{"STATE_VALIDITY", "DETECTION", "BLOCKED", "unsuitable-state-" + sha256(evidence.dump()), {error.message}};
    const auto bundle = makeAshDiagnosticBundle(context, "ERROR", error.message, evidence,
        Json::array({rule("ASH-STATE-VALIDITY-001", "ASH-STATE", "specs/core/state-validity-diagnostics.pseudo.md")}),
        {std::string(sourceRoot) + "specs/core/state-validity-diagnostics.pseudo.md", std::string(sourceRoot) + contract});
    error.diagnosticChain = bundle.chain; error.diagnosticPersistence = DiagnosticPersistence::NotRequested;
    std::erase_if(error.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
    return error;
}
Json makeState(const AshState& bits, const std::string& source) {
    auto coordinates = stateJson(bits); const auto hash = sha256(coordinates.dump());
    return {{"stateID", "ASH-STATE-" + hash}, {"coordinates", coordinates}, {"sourceReference", source}, {"canonicalHash", hash}};
}
AshState transformed(const AshState& state, const AshState& codeword) {
    const auto& words = canonicalCodewords(); if (std::find(words.begin(), words.end(), codeword) == words.end()) throw std::invalid_argument("Codeword not in canonical C");
    AshState result{}; for (std::size_t i = 0; i < 9; ++i) result[i] = static_cast<unsigned char>(state[i] ^ codeword[i]); return result;
}
bool referenceResolves(const Json& chain, const Json& reference) {
    if (chain.at("chainID") != reference.at("chainRootReference")) return false;
    for (const auto& envelope : chain.at("diagnostics").array_items())
        if (envelope.at("diagnosticID") == reference.at("diagnosticID")) return envelope.at("diagnosticKind") == reference.at("diagnosticKind") && envelope.at("ruleReferences") == reference.at("ruleReferences");
    return false;
}
Json envelopeReference(const Json& envelope) {
    return {{"diagnosticID", envelope.at("diagnosticID")}, {"chainRootReference", envelope.at("chainRootReference")}, {"diagnosticKind", envelope.at("diagnosticKind")}, {"ruleReferences", envelope.at("ruleReferences")}};
}
}
AshBindingProvider::AshBindingProvider(SemanticEnvironment environment, Json bindings) : environment_(std::move(environment)), bindings_(std::move(bindings)) { if (!bindings_.is_object()) throw std::invalid_argument("ASH bindings must be immutable object"); }
void AshBindingProvider::retainDiagnosticChain(const Json& chain) const {
    validateDiagnosticChain(chain, true); const auto id = chain.at("chainID").get<std::string>(); const auto bytes = chain.dump().size();
    std::lock_guard lock(diagnosticMutex_); const auto existing = diagnosticChains_.find(id);
    if (existing != diagnosticChains_.end()) { if (existing->second != chain) throw std::invalid_argument("Conflicting diagnostic chain identity"); return; }
    constexpr std::size_t budget = 16 * 1024 * 1024;
    if (diagnosticChains_.size() >= 64 || bytes > budget - diagnosticBytes_) throw std::runtime_error("ASH diagnostic retention budget exhausted; existing references remain resolvable");
    diagnosticChains_.emplace(id, chain); diagnosticBytes_ += bytes;
}
DiagnosticBundle AshBindingProvider::diagnostic(const Json& evidence, const std::string& source, const char* id, const char* family, const std::string& explanation) const {
    auto bundle = diagnosticBundle(evidence, source, id, family, explanation); retainDiagnosticChain(bundle.chain); return bundle;
}
OperationAttempt<BaseContracts::DiagnosticChain> AshBindingProvider::resolveDiagnosticChain(const BaseContracts::DiagnosticReference& reference) const {
    try {
        std::lock_guard lock(diagnosticMutex_); const auto found = diagnosticChains_.find(reference.value().at("chainRootReference").get<std::string>());
        if (found == diagnosticChains_.end() || !referenceResolves(found->second, reference.value())) return blocked("EXPORT", "Diagnostic reference does not resolve to an actual retained chain", reference.value());
        return BaseContracts::DiagnosticChain(found->second);
    } catch (const std::exception& error) { return failed("EXPORT", error); }
}
OperationAttempt<BaseContracts::AdmissibilityResult> AshBindingProvider::evaluateAdmissibility(const BaseContracts::AshState& state) const {
    try { const auto actual = validatedState(state); const auto diagnosis = environment_.diagnose(actual); const auto bundle = diagnostic(state.value(), "specs/core/state-admissibility.pseudo.md", "ASH-ADMISSIBILITY-CLASSIFICATION-001", "ASH-ADMISSIBILITY");
        Json orbit = Json::array(); for (const auto& member : diagnosis.orbit) orbit.array_items().push_back(stateJson(member));
        return BaseContracts::AdmissibilityResult(Json{{"resultID", "admissibility-" + sha256(state.value().dump())}, {"stateReference", state.value().at("stateID")}, {"status", diagnosis.admissibility}, {"orbitReference", "orbit-" + sha256(orbit.dump())}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failed("ADMISSIBILITY", error); }
}
OperationAttempt<BaseContracts::StateValidityDiagnostic> AshBindingProvider::diagnoseState(const BaseContracts::AshState& state) const {
    try { const auto actual = validatedState(state); const auto diagnosis = environment_.diagnose(actual); Json evidence = Json::object(); evidence["state"] = state.value(); evidence["transformationCompatibility"] = diagnosis.transformationCompatibility; evidence["fixedCatalog"] = environment_.catalog(); evidence["orbit"] = Json::array(); for (const auto& member : diagnosis.orbit) evidence["orbit"].array_items().push_back(stateJson(member));
        const auto bundle = diagnostic(evidence, "specs/core/state-validity-diagnostics.pseudo.md", "ASH-STATE-VALIDITY-001", "ASH-STATE");
        // The frozen base field is RecoveryCategory, unlike ASH's relevance
        // vocabulary. Obtain a real category from full lifecycle context.
        if (!bindings_.contains("completeSemanticInput")) return blocked("DIAGNOSE", "Frozen diagnostic requires recovery category; complete classification context unavailable", evidence);
        auto analyzed = environment_.analyze(bindings_.at("completeSemanticInput")); if (auto error = std::get_if<OperationFailure>(&analyzed)) return linkedFailure(*error);
        const auto& analysis = std::get<SemanticAnalysis>(analyzed); if (analysis.projection.state != actual) throw std::invalid_argument("Diagnostic lifecycle context state differs from actual input");
        return BaseContracts::StateValidityDiagnostic(Json{{"stateValidityDiagnosticID", "diagnosis-" + sha256(evidence.dump())}, {"diagnosticEnvelope", bundle.envelope}, {"admissibilityStatus", diagnosis.admissibility}, {"normalizationStatus", diagnosis.normalizationStatus}, {"recoverabilityRelevance", to_string(analysis.recovery)}});
    } catch (const std::exception& error) { return failed("DIAGNOSE", error); }
}
OperationAttempt<BaseContracts::SystemStateClass> AshBindingProvider::classifyState(const BaseContracts::StateValidityDiagnostic& dto) const {
    try {
        if (!bindings_.contains("completeSemanticInput")) return blocked("CLASSIFY", "Complete immutable classification context unavailable", dto.value());
        const auto& value = dto.value(); const auto& evidence = value.at("diagnosticEnvelope").at("evidence");
        if (!evidence.contains("state")) return blocked("CLASSIFY", "Full state evidence absent from supplied diagnostic", value);
        const auto state = validatedState(BaseContracts::AshState(evidence.at("state"))); const auto computed = environment_.diagnose(state);
        if (value.at("admissibilityStatus") != Json(computed.admissibility) || value.at("normalizationStatus") != Json(computed.normalizationStatus)) throw std::invalid_argument("Supplied diagnostic differs from actual canonical diagnosis");
        auto analyzed = environment_.analyze(bindings_.at("completeSemanticInput")); if (auto error = std::get_if<OperationFailure>(&analyzed)) return linkedFailure(*error);
        const auto& analysis = std::get<SemanticAnalysis>(analyzed); if (analysis.projection.state != state) throw std::invalid_argument("Classifier lifecycle context state differs from diagnostic");
        return BaseContracts::SystemStateClass(Json(to_string(analysis.stateClass)));
    } catch (const std::exception& error) { return failed("CLASSIFY", error); }
}
OperationAttempt<BaseContracts::TransitionResult> AshBindingProvider::applyCodewordTransformation(const BaseContracts::CodewordTransformation& transformation) const {
    try { const auto& value = transformation.value(); if (auto terminal = terminalContextFailure(bindings_, "applyCodewordTransformation", value)) return *terminal;
        auto resolved = resolveDiagnosticChain(BaseContracts::DiagnosticReference(value.at("diagnosticReference"))); if (auto error = std::get_if<OperationFailure>(&resolved)) return *error;
        const auto input = validatedState(BaseContracts::AshState(value.at("inputState"))); const auto output = transformed(input, parseAshState(value.at("codeword")));
        if (validatedState(BaseContracts::AshState(value.at("outputState"))) != output) throw std::invalid_argument("Requested transformation output differs from actual XOR");
        Json evidence = Json::object(); evidence["transformation"] = value; const auto bundle = diagnostic(evidence, "specs/algorithms/transition-system.pseudo.md", "ASH-CODEWORD-STRUCTURE-001", "ASH-CODEWORD");
        return BaseContracts::TransitionResult(Json{{"resultID", "transition-result-" + sha256(value.dump())}, {"transitionID", value.at("transformationID")}, {"inputState", value.at("inputState")}, {"outputState", makeState(output, value.at("inputState").at("sourceReference").get<std::string>())}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failed("TRANSFORM", error); }
}
OperationAttempt<BaseContracts::RealmIdentity> AshBindingProvider::resolveRealmIdentity(const BaseContracts::AshState& state) const {
    try { const auto actual = validatedState(state); const auto actualDiagnosis = environment_.diagnose(actual); if (!actualDiagnosis.valid) return blocked("REALM", "Realm encoder requires a recognized valid normalized state; input was not normalized or encoded", Json{{"state", state.value()}, {"admissibility", actualDiagnosis.admissibility}, {"normalizationStatus", actualDiagnosis.normalizationStatus}}); std::string encoding; for (auto bit : actual) encoding.push_back(static_cast<char>('0' + bit)); const auto bundle = diagnostic(state.value(), "specs/core/realm-identity.pseudo.md");
        return BaseContracts::RealmIdentity(Json{{"realmID", "ASH-REALM-" + encoding}, {"stateReference", state.value().at("stateID")}, {"encoding", encoding}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failed("REALM", error); }
}
OperationAttempt<BaseContracts::TransitionResult> AshBindingProvider::resolveTransition(const BaseContracts::AshState& state, const BaseContracts::TransitionDefinition& definition) const {
    try {
        if (auto terminal = terminalContextFailure(bindings_, "resolveTransition", Json{{"state", state.value()}, {"definition", definition.value()}})) return *terminal;
        const auto actual = validatedState(state); const auto diagnosis = environment_.diagnose(actual);
        if (!diagnosis.valid) return unsuitableNormalizedState(state, diagnosis, "resolveTransition", "specs/interfaces/contracts/transition-registry-contract.md");
        if (!bindings_.contains("transitionRegistry")) return blocked("TRANSITION", "Actual transition registry unavailable", definition.value());
        const auto& requested = definition.value(); const Json* selected = nullptr;
        for (const auto& entry : bindings_.at("transitionRegistry").array_items()) if (entry.at("definition") == requested) { if (selected) throw std::invalid_argument("Duplicate transition definition"); selected = &entry; }
        if (!selected) return blocked("TRANSITION", "Requested transition not registered", requested);
        if (selected->at("sourceReference").get<std::string>().empty()) throw std::invalid_argument("Transition applicability source absent");
        for (const auto& condition : requested.at("preconditions").array_items()) if (condition != Json("FULL_NINE_BINARY_STATE") && condition != Json("CANONICAL_CODEWORD")) return blocked("TRANSITION", "Undefined transition applicability predicate", requested);
        const auto codeword = parseAshState(selected->at("codeword")); const auto output = transformed(actual, codeword); Json evidence = Json::object(); evidence["inputState"] = state.value(); evidence["definition"] = requested; evidence["registeredEntry"] = *selected; const auto bundle = diagnostic(evidence, "specs/algorithms/transition-system.pseudo.md", "ASH-CODEWORD-STRUCTURE-001", "ASH-CODEWORD");
        return BaseContracts::TransitionResult(Json{{"resultID", "transition-result-" + sha256(evidence.dump())}, {"transitionID", requested.at("transitionID")}, {"inputState", state.value()}, {"outputState", makeState(output, selected->at("sourceReference").get<std::string>())}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failed("TRANSITION", error); }
}
OperationAttempt<BaseContracts::TopologyPlan> AshBindingProvider::generateTopology(const BaseContracts::AshState& state) const {
    try {
        const auto actual = validatedState(state); const auto diagnosis = environment_.diagnose(actual);
        if (!diagnosis.valid) return unsuitableNormalizedState(state, diagnosis, "generateTopology", "specs/interfaces/contracts/topology-generator-contract.md");
        if (!bindings_.contains("topology")) return blocked("TOPOLOGY", "Explicit topology depth/seed/source unavailable", state.value());
        const auto& context = bindings_.at("topology"); const auto depth = context.at("depth").get<int>(); const auto seed = context.at("seed").get<std::string>();
        if (depth < 0 || depth > 8 || seed.empty() || context.at("sourceReference").get<std::string>().empty()) throw std::invalid_argument("Invalid/unbounded topology request");
        if (depth == 0) return blocked("TOPOLOGY", "Depth-zero topology has no edges; frozen TopologyPlan requires at least one genuine edge", context);
        Json allNodes = Json::array(); Json edges = Json::array(); Json current = Json::array();
        Json root{{"node_id", seed}, {"parent_id", nullptr}, {"depth", 0}, {"ordinal", 0}, {"branch_kind", "root"}, {"path_token", seed}}; current.array_items().push_back(root); allNodes.array_items().push_back(root);
        for (int generation = 1; generation <= depth; ++generation) {
            Json next = Json::array(); int ordinal = 0;
            for (const auto& parent : current.array_items()) for (const char* kind : {"continuation", "positive", "negative"}) {
                const auto id = parent.at("node_id").get<std::string>() + "/" + kind; Json child{{"node_id", id}, {"parent_id", parent.at("node_id")}, {"depth", generation}, {"ordinal", ordinal++}, {"branch_kind", kind}, {"path_token", id}};
                next.array_items().push_back(child); allNodes.array_items().push_back(child); edges.array_items().push_back(Json{{"parent_id", parent.at("node_id")}, {"child_id", id}});
            } current = std::move(next);
        }
        Json evidence{{"state", state.value()}, {"topologyContext", context}}; const auto bundle = diagnostic(evidence, "specs/algorithms/topology-expansion.pseudo.md");
        return BaseContracts::TopologyPlan(Json{{"topologyID", "topology-" + sha256(evidence.dump())}, {"rootState", state.value()}, {"nodes", current}, {"edges", edges}, {"lineage", Json{{"allNodes", allNodes}, {"depth", depth}, {"seed", seed}}}, {"diagnosticReferences", Json::array({bundle.reference})}});
    } catch (const std::exception& error) { return failed("TOPOLOGY", error); }
}
OperationAttempt<std::vector<BaseContracts::AxiomDiagnostic>> AshBindingProvider::evaluateAxioms(const BaseContracts::AshState& state) const {
    try {
        (void)validatedState(state); if (!bindings_.contains("axiomSubject")) return blocked("AXIOM", "Actual axiom subject measures/source unavailable; bits do not define them", state.value());
        const auto& subject = bindings_.at("axiomSubject"); if (subject.at("stateReference") != state.value().at("stateID") || subject.at("sourceReference").get<std::string>().empty()) throw std::invalid_argument("Axiom subject state/source binding mismatch");
        const auto positive = [&](const char* name) { return subject.at(name).compare_number(Json(0)) > 0; };
        const std::array<bool, 5> results{positive("relation_count"), subject.at("compressed_measure").compare_number(subject.at("raw_measure")) < 0, subject.at("scale_count").compare_number(Json(1)) > 0, positive("erasure_cost"), subject.at("self_reference_capable").get<bool>() && subject.at("self_model_available").get<bool>()};
        static constexpr const char* expressions[]{"relation_count > 0", "compressed_measure < raw_measure", "scale_count > 1", "erasure_cost > 0", "self_reference_capable AND self_model_available"};
        static constexpr const char* explanations[]{"The supplied subject has at least one relation", "The supplied compressed measure is smaller than its raw measure", "The supplied subject spans more than one scale", "The supplied subject has positive erasure cost", "The supplied subject can reference itself and has an available self model"};
        const bool overallPass = std::all_of(results.begin(), results.end(), [](bool value) { return value; });
        std::vector<BaseContracts::AxiomDiagnostic> diagnostics;
        for (std::size_t i = 0; i < results.size(); ++i) {
            const auto explanation = std::string(results[i] ? "PASS: " : "FAIL: ") + expressions[i] + "; " + explanations[i] + (results[i] ? " (predicate satisfied)" : " (predicate not satisfied)");
            Json evidence{{"subject", subject}, {"axiomOrdinal", static_cast<int>(i + 1)}, {"predicate", expressions[i]}, {"explanation", explanation}, {"actualPredicatePassed", results[i]}, {"overall_pass", overallPass}, {"sourceReference", std::string(sourceRoot) + "specs/algorithms/axiom-evaluation.pseudo.md"}};
            const auto bundle = diagnostic(evidence, "specs/algorithms/axiom-evaluation.pseudo.md", "ASH-STATE-STRUCTURE-001", "ASH-STATE", explanation);
            diagnostics.emplace_back(Json{{"axiomID", "axiom-" + std::to_string(i + 1)}, {"diagnosticEnvelope", bundle.envelope}, {"evaluationResult", results[i] ? "PASS" : "FAIL"}, {"evidence", evidence}});
        }
        return diagnostics;
    } catch (const std::exception& error) { return failed("AXIOM", error); }
}
OperationAttempt<BaseContracts::GenerationPlan> AshBindingProvider::generateMaterializationPlan(const BaseContracts::AshState& state) const {
    try {
        if (auto terminal = terminalContextFailure(bindings_, "generateMaterializationPlan", state.value())) return *terminal;
        const auto actual = validatedState(state);
        if (!bindings_.contains("generation")) return blocked("GENERATION", "Complete immutable generation helper context unavailable", state.value());
        const auto& context = bindings_.at("generation");
        for (const char* field : {"request", "sourceReference", "registeredProfile", "registeredNamingPolicy", "targetConstraints", "transitionRegistry", "topology", "axiomSubject", "artifactDescriptions"})
            if (!context.contains(field)) return blocked("GENERATION", std::string("Missing actual generation helper input: ") + field, context);
        if (context.at("sourceReference").get<std::string>().empty()) throw std::invalid_argument("Generation source binding absent");
        const auto& request = context.at("request");
        if (parseAshState(request.at("seed_state")) != actual) throw std::invalid_argument("Generation request seed differs from argument");
        for (const char* field : {"project_name", "profile_id", "naming_policy_id", "emission_target_kind"}) if (request.at(field).get<std::string>().empty()) throw std::invalid_argument("Empty generation request identity");
        auto verifyBound = [](const Json& object) {
            if (!object.is_object() || object.at("sourceReference").get<std::string>().empty() || object.at("revision").get<std::string>().empty()) throw std::invalid_argument("Unbound generation helper authority");
            Json content = object; content.object_items().erase("contentHash");
            if (object.at("contentHash") != Json(sha256(content.dump()))) throw std::invalid_argument("Generation helper content hash mismatch");
        };
        verifyBound(context.at("registeredProfile")); verifyBound(context.at("registeredNamingPolicy")); verifyBound(context.at("targetConstraints")); verifyBound(context.at("transitionRegistry"));
        const auto& profile = context.at("registeredProfile"); const auto& naming = context.at("registeredNamingPolicy"); const auto& target = context.at("targetConstraints");
        if (profile.at("profileID") != request.at("profile_id") || naming.at("namingPolicyID") != request.at("naming_policy_id") || target.at("emissionTargetKind") != request.at("emission_target_kind")) throw std::invalid_argument("Generation profile/naming/target identity mismatch");
        AshState normalized = actual;
        if (!environment_.diagnose(actual).valid) {
            if (!context.contains("normalizationWitness")) return blocked("GENERATION", "Actual selected normalization witness unavailable", state.value());
            const auto& witness = context.at("normalizationWitness"); verifyBound(witness);
            if (parseAshState(witness.at("seedState")) != actual || witness.at("catalogContentHash") != Json(sha256(environment_.catalog().dump()))) throw std::invalid_argument("Generation normalization witness seed/catalog mismatch");
            for (const auto& word : witness.at("transformationChain").array_items()) normalized = transformed(normalized, parseAshState(word));
            if (parseAshState(witness.at("targetState")) != normalized || !environment_.diagnose(normalized).valid) throw std::invalid_argument("Generation normalization target/path not recognized valid");
        }
        const auto normalizedDto = BaseContracts::AshState(makeState(normalized, context.at("sourceReference").get<std::string>()));
        auto sourceRealm = resolveRealmIdentity(normalizedDto); if (auto error = std::get_if<OperationFailure>(&sourceRealm)) return linkedFailure(*error);
        Json helperBindings = bindings_; helperBindings["transitionRegistry"] = context.at("transitionRegistry").at("entries"); helperBindings["topology"] = context.at("topology");
        AshBindingProvider helper(environment_, helperBindings);
        Json transitionResults = Json::array(); auto transformedDto = normalizedDto;
        for (const auto& transitionID : request.at("transition_sequence").array_items()) {
            const Json* selected = nullptr;
            for (const auto& entry : helperBindings.at("transitionRegistry").array_items()) if (entry.at("definition").at("transitionID") == transitionID) { if (selected) throw std::invalid_argument("Duplicate generation transition identity"); selected = &entry; }
            if (!selected) return blocked("GENERATION", "Requested generation transition is absent from actual registry", request);
            auto step = helper.resolveTransition(transformedDto, BaseContracts::TransitionDefinition(selected->at("definition"))); if (auto error = std::get_if<OperationFailure>(&step)) return linkedFailure(*error);
            const auto& output = std::get<BaseContracts::TransitionResult>(step); transitionResults.array_items().push_back(output.value()); transformedDto = BaseContracts::AshState(output.value().at("outputState"));
        }
        auto destinationRealm = resolveRealmIdentity(transformedDto); if (auto error = std::get_if<OperationFailure>(&destinationRealm)) return linkedFailure(*error);
        if (context.at("topology").at("depth") != request.at("topology_depth")) throw std::invalid_argument("Generation topology depth differs from request");
        auto topologyResult = helper.generateTopology(transformedDto); if (auto error = std::get_if<OperationFailure>(&topologyResult)) return linkedFailure(*error);
        const auto& topology = std::get<BaseContracts::TopologyPlan>(topologyResult).value();
        const auto& roles = profile.at("roleAssignments"); if (!roles.is_array() || roles.size() != topology.at("nodes").size()) throw std::invalid_argument("Generation roles do not cover actual topology exactly");
        std::set<std::string> nodes, assignedNodes, roleIDs;
        for (const auto& node : topology.at("nodes").array_items()) nodes.insert(node.at("node_id").get<std::string>());
        for (const auto& role : roles.array_items()) {
            const auto node = role.at("nodeID").get<std::string>(); const auto id = role.at("roleID").get<std::string>();
            if (id.empty() || !nodes.contains(node) || !assignedNodes.insert(node).second || role.at("profileID") != profile.at("profileID") || role.at("sourceReference").get<std::string>().empty()) throw std::invalid_argument("Missing/foreign/duplicate generation role binding"); roleIDs.insert(id);
        }
        const auto& subject = context.at("axiomSubject"); verifyBound(subject);
        if (subject.at("stateReference") != transformedDto.value().at("stateID") || subject.at("topologyHash") != Json(sha256(topology.dump())) || subject.at("roleAssignmentsHash") != Json(sha256(roles.dump()))) throw std::invalid_argument("Generation axiom subject does not bind actual computed inputs");
        helperBindings["axiomSubject"] = subject; AshBindingProvider axiomHelper(environment_, helperBindings);
        auto axiomResult = axiomHelper.evaluateAxioms(transformedDto); if (auto error = std::get_if<OperationFailure>(&axiomResult)) return linkedFailure(*error);
        Json axioms = Json::array(), warnings = Json::array();
        for (const auto& result : std::get<std::vector<BaseContracts::AxiomDiagnostic>>(axiomResult)) { axioms.array_items().push_back(result.value()); if (result.value().at("evaluationResult") == Json("FAIL")) warnings.array_items().push_back(result.value().at("evidence").at("explanation")); }
        const auto& descriptions = context.at("artifactDescriptions"); if (descriptions != naming.at("artifactDescriptions") || !descriptions.is_array() || descriptions.empty()) throw std::invalid_argument("Artifact descriptions differ from registered naming rules or are incomplete");
        std::set<std::string> elementIDs, artifactIDs, describedNodes; Json elements = Json::array();
        for (const auto& description : descriptions.array_items()) {
            const auto node = description.at("nodeID").get<std::string>(); const auto id = description.at("elementID").get<std::string>(); const auto role = description.at("roleID").get<std::string>(); const auto& artifact = description.at("artifact");
            const auto artifactID = artifact.at("artifactID").get<std::string>();
            if (id.empty() || artifactID.empty() || !elementIDs.insert(id).second || !artifactIDs.insert(artifactID).second || !nodes.contains(node) || !roleIDs.contains(role)) throw std::invalid_argument("Invalid artifact/node/role identity");
            bool boundRole = false; for (const auto& assignment : roles.array_items()) if (assignment.at("nodeID") == Json(node) && assignment.at("roleID") == Json(role)) boundRole = true;
            if (!boundRole || description.at("namingPolicyID") != naming.at("namingPolicyID") || description.at("projectName") != request.at("project_name") || description.at("emissionTargetKind") != request.at("emission_target_kind") || description.at("targetConstraintsHash") != target.at("contentHash")) throw std::invalid_argument("Artifact semantic helper binding mismatch");
            for (const char* field : {"kind", "name", "contentDescription"}) if (artifact.at(field).get<std::string>().empty()) throw std::invalid_argument("Incomplete abstract artifact description");
            describedNodes.insert(node); elements.array_items().push_back(description);
        }
        if (describedNodes != nodes) throw std::invalid_argument("Abstract artifacts do not cover assigned topology");
        Json semantic{{"request", request}, {"normalizedState", normalizedDto.value()}, {"sourceRealm", std::get<BaseContracts::RealmIdentity>(sourceRealm).value()}, {"transformedState", transformedDto.value()}, {"destinationRealm", std::get<BaseContracts::RealmIdentity>(destinationRealm).value()}, {"transitionResults", transitionResults}, {"topology", topology}, {"roleAssignments", roles}, {"axiomDiagnostics", axioms}, {"warnings", warnings}, {"registeredProfile", profile}, {"registeredNamingPolicy", naming}, {"targetConstraints", target}, {"sourceReference", context.at("sourceReference")}};
        const auto bundle = diagnostic(semantic, "specs/algorithms/generation-planning.pseudo.md", "ASH-STATE-STRUCTURE-001", "ASH-STATE", "Verified normalized state and registered helper inputs; calculated transitions, topology and all five axioms; bound complete abstract artifacts without materialization");
        Json chains = Json::array(); std::set<std::string> chainIDs;
        const auto captureChain = [&](const AshBindingProvider& producer, const Json& reference) {
            auto resolved = producer.resolveDiagnosticChain(BaseContracts::DiagnosticReference(reference));
            if (auto error = std::get_if<OperationFailure>(&resolved)) throw std::runtime_error(error->message);
            const auto& chain = std::get<BaseContracts::DiagnosticChain>(resolved).value(); retainDiagnosticChain(chain);
            if (chainIDs.insert(chain.at("chainID").get<std::string>()).second) chains.array_items().push_back(chain);
        };
        captureChain(*this, std::get<BaseContracts::RealmIdentity>(sourceRealm).value().at("diagnosticReference"));
        captureChain(*this, std::get<BaseContracts::RealmIdentity>(destinationRealm).value().at("diagnosticReference"));
        for (const auto& transition : transitionResults.array_items()) captureChain(helper, transition.at("diagnosticReference"));
        for (const auto& reference : topology.at("diagnosticReferences").array_items()) captureChain(helper, reference);
        for (const auto& axiom : axioms.array_items()) captureChain(axiomHelper, envelopeReference(axiom.at("diagnosticEnvelope")));
        captureChain(*this, bundle.reference); semantic["diagnosticChains"] = std::move(chains);
        Json content{{"planElements", elements}, {"semanticInputs", semantic}, {"diagnosticReferences", Json::array({bundle.reference})}}; const auto hash = sha256(content.dump()); content["planID"] = "generation-" + hash; content["planHash"] = hash;
        return BaseContracts::GenerationPlan(content);
    } catch (const std::exception& error) { return failed("GENERATION", error); }
}
OperationAttempt<BaseContracts::ArtifactEmissionPlan> AshBindingProvider::planArtifactEmission(const BaseContracts::GenerationPlan& plan) const {
    try {
        const auto& input = plan.value(); Json planContent = input; planContent.object_items().erase("planID"); planContent.object_items().erase("planHash"); const auto hash = sha256(planContent.dump());
        if (input.at("planHash") != Json(hash) || input.at("planID") != Json("generation-" + hash)) throw std::invalid_argument("Generation plan content/identity hash mismatch");
        const auto& semantic = input.at("semanticInputs");
        if (!semantic.contains("diagnosticChains")) return blocked("EMISSION", "Complete plan lacks its actual generation diagnostic chains", input);
        std::map<std::string, Json> chains;
        for (const auto& chain : semantic.at("diagnosticChains").array_items()) {
            validateDiagnosticChain(chain, true); const auto id = chain.at("chainID").get<std::string>();
            if (!chains.emplace(id, chain).second) throw std::invalid_argument("Duplicate plan diagnostic chain identity");
        }
        const auto resolvePlanReference = [&](const Json& reference) -> const Json& {
            const BaseContracts::DiagnosticReference checked(reference); const auto found = chains.find(checked.value().at("chainRootReference").get<std::string>());
            if (found == chains.end() || !referenceResolves(found->second, reference)) throw std::invalid_argument("Plan diagnostic reference lacks its genuine contained chain");
            for (const auto& envelope : found->second.at("diagnostics").array_items()) if (envelope.at("diagnosticID") == reference.at("diagnosticID")) return envelope;
            throw std::invalid_argument("Plan diagnostic envelope unavailable");
        };
        Json generationEvidence = semantic; generationEvidence.object_items().erase("diagnosticChains");
        if (input.at("diagnosticReferences").size() != 1) throw std::invalid_argument("Plan requires its one actual generation diagnostic reference");
        Json measuredGenerationEvidence = resolvePlanReference(input.at("diagnosticReferences").at(size_t{0})).at("evidence"); measuredGenerationEvidence.object_items().erase("ashDiagnostic");
        if (measuredGenerationEvidence != generationEvidence) throw std::invalid_argument("Plan semantic evidence differs from its actual generation diagnostic");
        const auto verifyBound = [](const Json& object) { Json content = object; content.object_items().erase("contentHash");
            if (object.at("sourceReference").get<std::string>().empty() || object.at("revision").get<std::string>().empty() || object.at("contentHash") != Json(sha256(content.dump()))) throw std::invalid_argument("Plan-contained helper source/revision/hash mismatch"); };
        const auto& profile = semantic.at("registeredProfile"); const auto& naming = semantic.at("registeredNamingPolicy"); const auto& target = semantic.at("targetConstraints");
        verifyBound(profile); verifyBound(naming); verifyBound(target); const auto& request = semantic.at("request");
        if (request.at("profile_id") != profile.at("profileID") || request.at("naming_policy_id") != naming.at("namingPolicyID") || request.at("emission_target_kind") != target.at("emissionTargetKind") || semantic.at("roleAssignments") != profile.at("roleAssignments") || input.at("planElements") != naming.at("artifactDescriptions")) throw std::invalid_argument("Plan elements/helper identities disagree with contained semantic inputs");
        (void)validatedState(BaseContracts::AshState(semantic.at("normalizedState"))); (void)validatedState(BaseContracts::AshState(semantic.at("transformedState")));
        (void)resolvePlanReference(semantic.at("sourceRealm").at("diagnosticReference")); (void)resolvePlanReference(semantic.at("destinationRealm").at("diagnosticReference"));
        for (const auto& transition : semantic.at("transitionResults").array_items()) (void)resolvePlanReference(transition.at("diagnosticReference"));
        for (const auto& reference : semantic.at("topology").at("diagnosticReferences").array_items()) (void)resolvePlanReference(reference);
        for (const auto& axiom : semantic.at("axiomDiagnostics").array_items()) if (resolvePlanReference(envelopeReference(axiom.at("diagnosticEnvelope"))) != axiom.at("diagnosticEnvelope")) throw std::invalid_argument("Plan axiom envelope differs from contained chain");
        Json artifacts = Json::array(), traceability = Json::array(); std::set<std::string> elementIDs, artifactIDs;
        for (const auto& element : input.at("planElements").array_items()) {
            if (!element.contains("elementID") || !element.contains("artifact") || !element.at("artifact").is_object()) return blocked("EMISSION", "Plan element lacks explicit complete artifact semantics", element);
            const auto id = element.at("elementID").get<std::string>(); const auto artifactID = element.at("artifact").at("artifactID").get<std::string>();
            if (id.empty() || artifactID.empty() || !elementIDs.insert(id).second || !artifactIDs.insert(artifactID).second) throw std::invalid_argument("Empty/duplicate plan element or artifact identity");
            for (const char* field : {"kind", "name", "contentDescription"}) if (element.at("artifact").at(field).get<std::string>().empty()) throw std::invalid_argument("Incomplete plan artifact description");
            if (element.at("namingPolicyID") != naming.at("namingPolicyID") || element.at("projectName") != request.at("project_name") || element.at("emissionTargetKind") != target.at("emissionTargetKind") || element.at("targetConstraintsHash") != target.at("contentHash")) throw std::invalid_argument("Plan element target/naming binding mismatch");
            artifacts.array_items().push_back(element.at("artifact")); traceability.array_items().push_back(Json{{"planElementID", id}, {"artifactHash", sha256(element.at("artifact").dump())}});
        }
        if (artifacts.empty()) return blocked("EMISSION", "No plan-defined artifacts", input);
        for (const auto& [id, chain] : chains) { (void)id; retainDiagnosticChain(chain); }
        const auto bundle = diagnostic(Json{{"generationPlanID", input.at("planID")}, {"planHash", input.at("planHash")}, {"planElements", input.at("planElements")}, {"artifacts", artifacts}, {"traceability", traceability}, {"normalizedState", semantic.at("normalizedState")}, {"transformedState", semantic.at("transformedState")}},
            "specs/interfaces/contracts/artifact-emitter-contract.md", "ASH-STATE-STRUCTURE-001", "ASH-STATE", "Verified complete plan-contained state and diagnostic evidence; copied each abstract artifact exactly once with its actual plan-element traceability, without rerunning semantic generation");
        for (auto& item : traceability.array_items()) item["diagnosticReference"] = bundle.reference;
        return BaseContracts::ArtifactEmissionPlan(Json{{"emissionPlanID", "emission-" + sha256(input.dump())}, {"generationPlanID", input.at("planID")}, {"artifacts", artifacts}, {"traceability", traceability}});
    } catch (const std::exception& error) { return failed("EMISSION", error); }
}
} // namespace Aeostara
