// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/BaseDocumentBindings.h"
#include <algorithm>
#include <functional>
#include <set>
#include <stdexcept>

namespace Aeostara::BaseBindings {
namespace {
Json pointerValue(const std::string& pointer, const Json* value) {
    return Json{{"pointer", pointer}, {"tokens", Json(Json::pointer_tokens(pointer))},
        {"pointsToMissing", value == nullptr}, {"pointsToNull", value && value->is_null()}};
}
void checkedPointer(const Json& pointer, const Json* value) {
    const auto text = pointer.at("pointer").get<std::string>();
    if (pointer != pointerValue(text, value)) throw std::invalid_argument("Pointer tokens or presence flags differ from captured document");
}
std::string escapedToken(std::string_view token) {
    std::string result;
    for (auto c : token) { if (c == '~') result += "~0"; else if (c == '/') result += "~1"; else result += c; }
    return result;
}
unsigned jsonType(const Json& value) {
    if (value.is_null()) return 0; if (value.is_boolean()) return 1;
    if (value.is_number()) return 2; if (value.is_string()) return 3;
    return value.is_array() ? 4u : 5u;
}
Json chainReference(const Json& envelope) {
    return Json{{"diagnosticID", envelope.at("diagnosticID")}, {"chainRootReference", envelope.at("chainRootReference")},
        {"diagnosticKind", envelope.at("diagnosticKind")}, {"ruleReferences", envelope.at("ruleReferences")}};
}
}

BaseDocumentBindings::BaseDocumentBindings(std::vector<CapturedDocument> captured,
    SemanticEnvironment environment, Json input, Json actualRules, std::vector<std::string> actualSources)
    : environment_(std::move(environment)), completeSemanticInput_(std::move(input)),
      rules_(std::move(actualRules)), sources_(std::move(actualSources)) {
    if (captured.empty() || captured.size() > JsonLimits::sourceDocuments) throw std::invalid_argument("Captured document inventory is absent or exceeds limit");
    if (!rules_.is_array() || rules_.empty() || sources_.empty()) throw std::invalid_argument("Actual diagnostic rule/source references required");
    for (const auto& rule : rules_.array_items()) {
        BaseContracts::validate_contract(BaseContracts::ContractKind::RuleReference, rule);
        if (rule.at("sourceReference").get<std::string>().empty() || rule.at("ruleFamily").get<std::string>().empty()) throw std::invalid_argument("Empty actual rule source/family");
    }
    for (const auto& source : sources_) if (source.empty()) throw std::invalid_argument("Empty actual diagnostic source reference");
    validateCompleteSemanticInput(completeSemanticInput_);
    std::set<std::string> ids; std::size_t rawBytes = 0;
    for (auto& document : captured) {
        const auto& ref = document.reference.value();
        if (ref.at("documentID").get<std::string>().empty() || ref.at("sourceReference").get<std::string>().empty() || !ids.insert(ref.at("documentID").get<std::string>()).second) throw std::invalid_argument("Captured reference identity/source absent or duplicated");
        if (document.rawUtf8.size() > JsonLimits::aggregateInputBytes - rawBytes) throw std::length_error("Captured aggregate bytes exceed limit");
        rawBytes += document.rawUtf8.size();
        auto parsed = Json::parse(document.rawUtf8);
        if (ref.at("canonicalHash") != Json(sha256(parsed.canonical()))) throw std::invalid_argument("Captured document canonical hash differs from actual bytes");
        documents_.push_back(Snapshot{std::move(document.reference), std::move(document.rawUtf8), std::move(parsed)});
    }
    // Bind the named semantic snapshots to real owned bytes. Other immutable
    // context is evaluated by the adopted SemanticEnvironment, not guessed here.
    for (const auto& pair : {std::pair{"observedSnapshot", "OBSERVED"}, std::pair{"desiredSnapshot", "DESIRED"}}) {
        const auto& snapshot = completeSemanticInput_.at(pair.first); const Snapshot* match = nullptr;
        for (const auto& owned : documents_) if (owned.reference.value().at("role") == Json(pair.second) && owned.rawUtf8 == snapshot.at("rawUtf8").get<std::string>() && owned.document == snapshot.at("document")) {
            if (match) throw std::invalid_argument("Ambiguous owned snapshot binding"); match = &owned;
        }
        if (!match || snapshot.at("rawByteSha256") != Json(sha256(match->rawUtf8)) || snapshot.at("proposedCanonicalSha256") != match->reference.value().at("canonicalHash")) throw std::invalid_argument("Complete context snapshot lacks exact owned document binding");
    }
}

const BaseDocumentBindings::Snapshot& BaseDocumentBindings::resolve(const BaseContracts::JsonDocumentReference& reference) const {
    for (const auto& snapshot : documents_) {
        if (snapshot.reference == reference) return snapshot;
        auto canonical = snapshot.reference.value(); canonical["role"] = "CANONICALIZED";
        if (canonical == reference.value()) return snapshot;
    }
    throw std::invalid_argument("Document reference is not the exact captured identity/source/hash/role");
}
DiagnosticBundle BaseDocumentBindings::diagnostic(const std::string& kind, const std::string& severity,
    const std::string& message, const Json& evidence) const {
    auto bundle = makeDiagnosticBundle(kind, severity, message, evidence, rules_, sources_);
    auto checked = BaseContracts::DiagnosticChain(bundle.chain); const auto id = bundle.chain.at("chainID").get<std::string>();
    const auto bytes = bundle.chain.canonical().size();
    std::lock_guard lock(diagnosticsMutex_);
    if (const auto found = chains_.find(id); found != chains_.end()) {
        if (found->second != checked) throw std::runtime_error("Diagnostic identity collision");
    } else {
        if (chains_.size() >= 64 || bytes > JsonLimits::canonicalDocumentBytes - chainBytes_) throw std::length_error("Mandatory diagnostic value retention capacity exceeded");
        chains_.emplace(id, std::move(checked)); chainBytes_ += bytes;
    }
    return bundle;
}
OperationFailure BaseDocumentBindings::failure(OperationPhase phase, OperationCode code,
    const std::string& message, const Json& evidence) const {
    OperationFailure result(phase, code, message, evidence);
    try {
        const auto bundle = diagnostic(phase == OperationPhase::Project ? "MAPPING" : "JSON_SEMANTICS", "ERROR", message, evidence);
        result.diagnosticChain = bundle.chain; result.diagnosticPersistence = DiagnosticPersistence::NotRequested;
        result.unavailableEvidence.clear();
    } catch (const std::exception& error) { result.addSecondary(phase, OperationCode::DiagnosticUnavailable, error.what()); }
    catch (...) { result.addSecondary(phase, OperationCode::DiagnosticUnavailable, "Nonstandard diagnostic attachment failure"); }
    return result;
}
std::vector<BaseContracts::DiagnosticChain> BaseDocumentBindings::diagnosticChains() const {
    std::lock_guard lock(diagnosticsMutex_); std::vector<BaseContracts::DiagnosticChain> result;
    for (const auto& [id, chain] : chains_) { (void)id; result.push_back(chain); } return result;
}
OperationAttempt<BaseContracts::DiagnosticChain> BaseDocumentBindings::diagnosticChain(const BaseContracts::DiagnosticReference& reference) const {
    std::lock_guard lock(diagnosticsMutex_);
    const auto found = chains_.find(reference.value().at("chainRootReference").get<std::string>());
    if (found != chains_.end()) for (const auto& envelope : found->second.value().at("diagnostics").array_items()) if (chainReference(envelope) == reference.value()) return found->second;
    return OperationFailure(OperationPhase::Diagnose, OperationCode::Blocked, "Diagnostic reference does not resolve to an actual owned chain", reference.value());
}
void BaseDocumentBindings::validateSuppliedDiagnostic(const Json& reference) const {
    const auto result = diagnosticChain(BaseContracts::DiagnosticReference(reference));
    if (std::holds_alternative<OperationFailure>(result)) throw std::invalid_argument("Supplied diagnostic is foreign or unresolvable in this captured context");
}
OperationAttempt<BaseContracts::JsonDocumentReference> BaseDocumentBindings::canonicalize(const BaseContracts::JsonDocumentReference& reference) const {
    try { const auto& snapshot = resolve(reference); auto value = snapshot.reference.value(); value["role"] = "CANONICALIZED";
        (void)diagnostic("JSON_SEMANTICS", "INFO", "Exact owned document canonicalized without mutation", Json{{"documentReference", value}, {"rawHash", sha256(snapshot.rawUtf8)}, {"canonicalizationRuleID", "JASH-CANON-EXACT-DECIMAL-V1"}});
        return BaseContracts::JsonDocumentReference(std::move(value));
    } catch (const std::exception& error) { return failure(OperationPhase::Normalize, OperationCode::InvalidInput, error.what(), reference.value()); }
}
OperationAttempt<ResolvedPointer> BaseDocumentBindings::resolvePointer(const BaseContracts::JsonDocumentReference& reference, const BaseContracts::JsonPointer& pointer) const {
    try { const auto& snapshot = resolve(reference); const auto text = pointer.value().at("pointer").get<std::string>(); const auto* found = snapshot.document.resolve_pointer(text);
        checkedPointer(pointer.value(), found);
        const auto resolved = pointerValue(text, found);
        (void)diagnostic("JSON_SEMANTICS", "INFO", "Pointer resolved with missing and null distinguished", Json{{"documentReference", reference.value()}, {"pointer", resolved}});
        return ResolvedPointer{BaseContracts::JsonPointer(resolved), found ? std::variant<Json, Absent>(*found) : std::variant<Json, Absent>(Absent{})};
    } catch (const std::exception& error) { return failure(OperationPhase::Normalize, OperationCode::InvalidInput, error.what(), Json{{"documentReference", reference.value()}, {"pointer", pointer.value()}}); }
}
OperationAttempt<std::vector<BaseContracts::SurfaceDifference>> BaseDocumentBindings::generateSurfaceDifferences(const BaseContracts::JsonDocumentReference& observed, const BaseContracts::JsonDocumentReference& desired) const {
    try {
        const auto& left = resolve(observed); const auto& right = resolve(desired);
        if (left.reference.value().at("role") != Json("OBSERVED") || right.reference.value().at("role") != Json("DESIRED")) throw std::invalid_argument("Surface comparison requires captured observed and desired roles");
        std::vector<BaseContracts::SurfaceDifference> result;
        const auto emit = [&](const std::string& path, const char* kind, const Json* before, const Json* after) {
            Json evidence{{"pointer", path}, {"differenceKind", kind}, {"observedDocument", observed.value()}, {"desiredDocument", desired.value()}, {"observedPresent", before != nullptr}, {"desiredPresent", after != nullptr}};
            if (before) evidence["observedValue"] = *before; if (after) evidence["desiredValue"] = *after;
            const auto bundle = diagnostic("JSON_SEMANTICS", "INFO", "Surface difference recorded as evidence only", evidence);
            result.emplace_back(Json{{"differenceID", "surface-" + sha256(evidence.canonical())}, {"path", pointerValue(path, before)}, {"differenceKind", kind}, {"observedDocument", observed.value()}, {"desiredDocument", desired.value()}, {"diagnosticReference", bundle.reference}});
        };
        std::function<void(const Json*, const Json*, const std::string&)> walk;
        walk = [&](const Json* before, const Json* after, const std::string& path) {
            if (!before) { emit(path, "ADDED", before, after); return; }
            if (!after) { emit(path, "REMOVED", before, after); return; }
            if (*before == *after) return;
            if (jsonType(*before) != jsonType(*after)) { emit(path, "TYPE_CHANGED", before, after); return; }
            if (before->is_object()) {
                std::set<std::string, UnsignedUtf8Less> keys;
                for (const auto& [key, child] : before->object_items()) { (void)child; keys.insert(key); }
                for (const auto& [key, child] : after->object_items()) { (void)child; keys.insert(key); }
                for (const auto& key : keys) walk(before->contains(key) ? &before->at(key) : nullptr, after->contains(key) ? &after->at(key) : nullptr, path + "/" + escapedToken(key));
            } else emit(path, before->is_array() ? "ARRAY_CHANGED" : "REPLACED", before, after);
        };
        walk(&left.document, &right.document, "");
        (void)diagnostic("JSON_SEMANTICS", "INFO", "Deterministic surface comparison completed without authorizing recovery", Json{{"observedDocument", observed.value()}, {"desiredDocument", desired.value()}, {"differenceCount", result.size()}});
        return result;
    } catch (const std::exception& error) { return failure(OperationPhase::Normalize, OperationCode::InvalidInput, error.what(), Json{{"observedDocument", observed.value()}, {"desiredDocument", desired.value()}}); }
}
OperationAttempt<BaseContracts::DiagnosticReference> BaseDocumentBindings::validateMutationPreconditions(const BaseContracts::JsonDocumentReference& reference, const BaseContracts::JsonMutationOperation& operation) const {
    try {
        const auto& snapshot = resolve(reference); const auto& value = operation.value(); validateSuppliedDiagnostic(value.at("diagnosticReference"));
        if (value.at("operationID").get<std::string>().empty()) throw std::invalid_argument("Operation identity absent");
        if (value.at("preconditionHash") != Json(sha256(snapshot.rawUtf8))) return failure(OperationPhase::Policy, OperationCode::StaleState, "Exact captured raw preimage differs from mutation precondition", value);
        auto planned = environment_.plan(completeSemanticInput_); if (auto error = std::get_if<OperationFailure>(&planned)) return failure(error->phase, error->code, error->message, error->evidence);
        const auto& plan = std::get<SemanticPlan>(planned); const auto path = value.at("path").at("pointer").get<std::string>(); const auto* present = snapshot.document.resolve_pointer(path); checkedPointer(value.at("path"), present);
        bool matched = false;
        for (const auto& item : plan.operations.array_items()) if (item.at("operation") == value.at("op") && item.at("pointer") == Json(path) && value.contains("value") && item.at("value") == value.at("value") && present && item.at("expectedPreimage") == *present) matched = true;
        if (!matched || snapshot.rawUtf8 != completeSemanticInput_.at("observedSnapshot").at("rawUtf8").get<std::string>()) return failure(OperationPhase::Policy, OperationCode::Blocked, "Operation is outside independently planned adopted inverse scope", value);
        const auto bundle = diagnostic("JSON_SEMANTICS", "INFO", "Captured raw hash, existing slot and adopted plan operation preconditions validated; execution gates remain required", Json{{"operation", value}, {"planHash", plan.planHash}, {"rawHash", plan.rawPreimageHash}});
        return BaseContracts::DiagnosticReference(bundle.reference);
    } catch (const std::exception& error) { return failure(OperationPhase::Policy, OperationCode::InvalidInput, error.what(), operation.value()); }
}
OperationAttempt<BaseContracts::ObservedSystemState> BaseDocumentBindings::observe(const BaseContracts::JsonDocumentReference& reference) const {
    try { const auto& snapshot = resolve(reference); if (snapshot.reference.value().at("role") != Json("OBSERVED")) throw std::invalid_argument("Observation requires actual observed-role snapshot");
        const auto bundle = diagnostic("JSON_SEMANTICS", "INFO", "Actual captured observation validated without fabricating state", Json{{"documentReference", reference.value()}, {"rawHash", sha256(snapshot.rawUtf8)}});
        return BaseContracts::ObservedSystemState(Json{{"observedSystemStateID", "observation-" + sha256(reference.value().canonical())}, {"documentReference", reference.value()}, {"surfaceDifferences", Json::array()}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failure(OperationPhase::Normalize, OperationCode::InvalidInput, error.what(), reference.value()); }
}
OperationAttempt<BaseContracts::DesiredSystemIntent> BaseDocumentBindings::desiredIntent(const BaseContracts::JsonDocumentReference& reference, bool partialIntent) const {
    try { const auto& snapshot = resolve(reference); if (snapshot.reference.value().at("role") != Json("DESIRED")) throw std::invalid_argument("Intent requires actual desired-role snapshot");
        const auto bundle = diagnostic("JSON_SEMANTICS", "INFO", "Explicit desired intent recorded without treating it as an authorized recovery target", Json{{"documentReference", reference.value()}, {"partialIntent", partialIntent}});
        return BaseContracts::DesiredSystemIntent(Json{{"desiredSystemIntentID", "intent-" + sha256(Json{{"documentReference", reference.value()}, {"partialIntent", partialIntent}}.canonical())}, {"documentReference", reference.value()}, {"partialIntent", partialIntent}, {"diagnosticReference", bundle.reference}});
    } catch (const std::exception& error) { return failure(OperationPhase::Normalize, OperationCode::InvalidInput, error.what(), reference.value()); }
}
OperationAttempt<BaseContracts::SemanticProjectionSpec> BaseDocumentBindings::projectionSpecification() const {
    try {
        const auto& profile = environment_.profile(); Json dimensions = Json::array();
        for (const auto& dimension : profile.at("dimensions").array_items()) {
            Json pointers = Json::array(), evidence = Json::array();
            for (const auto& binding : dimension.at("sourceBindings").array_items()) {
                const auto name = binding.at("sourceName").get<std::string>(); const Json* snapshot = nullptr;
                if (name == "observed") snapshot = &completeSemanticInput_.at("observedSnapshot");
                else for (const auto& item : completeSemanticInput_.at("evidenceSnapshots").array_items()) if (item.at("sourceName") == Json(name)) { if (snapshot) throw std::invalid_argument("Ambiguous declared evidence source"); snapshot = &item; }
                const auto text = binding.at("pointer").get<std::string>(); const Json* found = snapshot ? snapshot->at("document").resolve_pointer(text) : nullptr;
                pointers.push_back(pointerValue(text, found)); evidence.push_back(binding);
            }
            const auto bundle = diagnostic("MAPPING", "INFO", "Adopted profile source bindings captured without inventing coordinate values", Json{{"dimension", dimension}, {"mappingProfile", completeSemanticInput_.at("mappingProfile")}});
            dimensions.push_back(Json{{"dimensionID", dimension.at("dimension")}, {"sourcePointers", pointers}, {"mappingRule", profile.at("evaluatorID")}, {"evidence", evidence}, {"diagnosticReference", bundle.reference}});
        }
        return BaseContracts::SemanticProjectionSpec(Json{{"projectionSpecID", profile.at("profileID")}, {"dimensions", dimensions}, {"version", profile.at("revision").number_text()}, {"sourceReferences", Json(sources_)}});
    } catch (const std::exception& error) { return failure(OperationPhase::Project, OperationCode::InvalidInput, error.what()); }
}
OperationAttempt<BaseContracts::MappingResult> BaseDocumentBindings::project(const BaseContracts::ObservedSystemState& observed, const BaseContracts::DesiredSystemIntent& desired, const BaseContracts::SemanticProjectionSpec& spec) const {
    try {
        const auto expectedObserved = observe(BaseContracts::JsonDocumentReference(observed.value().at("documentReference"))); if (auto error = std::get_if<OperationFailure>(&expectedObserved)) return *error;
        const auto expectedDesired = desiredIntent(BaseContracts::JsonDocumentReference(desired.value().at("documentReference")), desired.value().at("partialIntent").get<bool>()); if (auto error = std::get_if<OperationFailure>(&expectedDesired)) return *error;
        const auto expectedSpec = projectionSpecification(); if (auto error = std::get_if<OperationFailure>(&expectedSpec)) return *error;
        if (std::get<BaseContracts::ObservedSystemState>(expectedObserved) != observed || std::get<BaseContracts::DesiredSystemIntent>(expectedDesired) != desired || std::get<BaseContracts::SemanticProjectionSpec>(expectedSpec) != spec) throw std::invalid_argument("Supplied projection DTO differs from exact captured document/profile/diagnostic context");
        auto projected = environment_.project(completeSemanticInput_);
        if (auto error = std::get_if<OperationFailure>(&projected)) {
            auto result = failure(error->phase, error->code, error->message, error->evidence); result.mappingStatus = error->mappingStatus;
            result.unavailableEvidence.push_back({"projectedState", "No unique complete genuine nine-coordinate projection exists"}); return result;
        }
        const auto& actual = std::get<Projection>(projected); const auto bundle = diagnostic("MAPPING", "INFO", "All nine adopted coordinate evaluations produced a unique genuine projection", Json{{"completeSemanticInputHash", actual.completeSemanticInputHash}, {"coordinates", stateJson(actual.state)}});
        Json bindings = Json::array();
        for (std::size_t i = 0; i < actual.dimensions.size(); ++i) {
            const auto& dimension = actual.dimensions[i]; if (!dimension.bit || dimension.status != MappingStatus::Mapped || dimension.sources.empty()) throw std::logic_error("Mapped projection omitted genuine coordinate evidence");
            Json pointers = Json::array();
            for (const auto& source : dimension.sources.array_items()) pointers.push_back(pointerValue(source.at("pointer").get<std::string>(), &source.at("value")));
            bindings.push_back(Json{{"dimensionID", dimension.coordinate}, {"sourcePointers", pointers}, {"mappingRule", environment_.profile().at("evaluatorID")}, {"evidence", dimension.sources}, {"diagnosticReference", bundle.reference}});
        }
        const auto coordinates = stateJson(actual.state); const auto stateHash = sha256(coordinates.canonical());
        Json state{{"stateID", "ASH-STATE-" + stateHash}, {"coordinates", coordinates}, {"sourceReference", "semantic-input-" + actual.completeSemanticInputHash}, {"canonicalHash", stateHash}};
        return BaseContracts::MappingResult(Json{{"mappingResultID", "mapping-" + actual.completeSemanticInputHash}, {"status", "MAPPED"}, {"projectedState", state}, {"dimensionBindings", bindings}, {"ambiguities", Json::array()}, {"diagnosticReferences", Json::array({bundle.reference})}});
    } catch (const std::exception& error) { return failure(OperationPhase::Project, OperationCode::InvalidInput, error.what(), Json{{"observed", observed.value()}, {"desired", desired.value()}, {"spec", spec.value()}}); }
}
} // namespace Aeostara::BaseBindings
