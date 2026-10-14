// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "AeostaraCore/SemanticMapping.h"
#include "AeostaraCore/Diagnostics.h"
#include "AdoptedSemanticDefinitions.inc"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <bcrypt.h>
#include <algorithm>
#include <limits>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <system_error>

namespace Aeostara {
OperationFailure::OperationFailure(OperationPhase p, OperationCode c, std::string m, Json e)
    : phase(p), code(c), message(std::move(m)), evidence(std::move(e)),
      primaryFailure{p, c, message, evidence} {
    if (message.empty()) throw std::invalid_argument("Empty primary failure");
    if (p == OperationPhase::Project) {
        if (c == OperationCode::Ambiguous) mappingStatus = MappingStatus::Ambiguous;
        else if (c == OperationCode::Blocked) mappingStatus = MappingStatus::Blocked;
        else mappingStatus = MappingStatus::Failed;
    }
    unavailableEvidence.push_back({"diagnosticChain", "No diagnostic chain has yet been captured for this failure"});
}
bool OperationFailure::mutationMayHaveOccurred() const noexcept {
    return mutationState == MutationState::MayHaveMutated || mutationState == MutationState::Mutated ||
           mutationState == MutationState::RestoreFailed || mutationState == MutationState::Unknown;
}
void OperationFailure::addSecondary(OperationPhase p, OperationCode c, std::string m, Json e) {
    if (m.empty()) throw std::invalid_argument("Empty secondary failure");
    secondaryFailures.push_back({p, c, std::move(m), std::move(e)});
}
OperationPhase operationPhase(std::string_view value) {
    if (value == "PARSE") return OperationPhase::Parse;
    if (value == "NORMALIZE" || value == "TRANSFORM") return OperationPhase::Normalize;
    if (value == "PROJECT") return OperationPhase::Project;
    if (value == "DIAGNOSE" || value == "ADMISSIBILITY" || value == "AXIOM") return OperationPhase::Diagnose;
    if (value == "CLASSIFY") return OperationPhase::Classify;
    if (value == "PLAN" || value == "REALM" || value == "TOPOLOGY" || value == "GENERATION" || value == "EMISSION" || value == "TRANSITION") return OperationPhase::Plan;
    if (value == "POLICY" || value == "PRECONDITION") return OperationPhase::Policy;
    if (value == "BACKUP") return OperationPhase::Backup;
    if (value == "EXECUTE") return OperationPhase::Execute;
    if (value == "VERIFY" || value == "VALIDATE") return OperationPhase::Verify;
    if (value == "ROLLBACK") return OperationPhase::Rollback;
    if (value == "FALLBACK") return OperationPhase::Fallback;
    if (value == "CONTAIN") return OperationPhase::Contain;
    if (value == "HALT") return OperationPhase::Halt;
    if (value == "AUDIT") return OperationPhase::Audit;
    if (value == "EXPORT") return OperationPhase::Export;
    throw std::invalid_argument("Unknown operation phase");
}
OperationCode operationCode(std::string_view value) {
    if (value == "AMBIGUOUS") return OperationCode::Ambiguous;
    if (value == "BLOCKED") return OperationCode::Blocked;
    if (value == "FAILED") return OperationCode::Failed;
    throw std::invalid_argument("Unknown operation failure code");
}
std::string to_string(OperationPhase value) {
    static constexpr const char* values[]{"PARSE", "NORMALIZE", "PROJECT", "DIAGNOSE", "CLASSIFY", "PLAN", "POLICY", "BACKUP", "EXECUTE", "VERIFY", "ROLLBACK", "FALLBACK", "CONTAIN", "HALT", "AUDIT", "EXPORT"};
    const auto index = static_cast<std::size_t>(value); if (index >= std::size(values)) throw std::invalid_argument("Invalid operation phase enum"); return values[index];
}
std::string to_string(OperationCode value) {
    static constexpr const char* values[]{"AMBIGUOUS", "BLOCKED", "FAILED", "INVALID_INPUT", "UNSUPPORTED_EVIDENCE", "STALE_STATE", "POLICY_REJECTED", "STORAGE_FAILURE", "VERIFICATION_FAILED", "RESTORATION_FAILED", "DIAGNOSTIC_UNAVAILABLE", "INTERNAL_FAILURE"};
    const auto index = static_cast<std::size_t>(value); if (index >= std::size(values)) throw std::invalid_argument("Invalid operation code enum"); return values[index];
}
std::string to_string(MutationState value) {
    static constexpr const char* values[]{"NOT_STARTED", "MAY_HAVE_MUTATED", "MUTATED", "VERIFIED_RESTORED", "RESTORE_FAILED", "UNKNOWN"};
    const auto index = static_cast<std::size_t>(value); if (index >= std::size(values)) throw std::invalid_argument("Invalid mutation state enum"); return values[index];
}
std::string to_string(DiagnosticPersistence value) {
    static constexpr const char* values[]{"NOT_REQUESTED", "PERSISTED", "FAILED", "UNAVAILABLE", "UNKNOWN"};
    const auto index = static_cast<std::size_t>(value); if (index >= std::size(values)) throw std::invalid_argument("Invalid diagnostic persistence enum"); return values[index];
}
OperationFailure operationFailureFromException(OperationPhase phase, OperationCode code, const std::exception& error, Json evidence) {
    if (const auto* native = dynamic_cast<const std::system_error*>(&error)) {
        if (!evidence.is_object()) evidence = Json{{"availableEvidence", evidence}};
        evidence["nativeError"] = Json{{"category", native->code().category().name()}, {"value", native->code().value()}, {"message", native->code().message()}};
    }
    return OperationFailure(phase, code, error.what(), std::move(evidence));
}
Json operationFailureJson(const OperationFailure& value) {
    if (value.message.empty() || value.primaryFailure.message != value.message || value.primaryFailure.phase != value.phase || value.primaryFailure.code != value.code) throw std::invalid_argument("Failure primary binding mismatch");
    Json unavailable = Json::array(); std::set<std::string> fields;
    for (const auto& item : value.unavailableEvidence) { if (item.field.empty() || item.reason.empty() || !fields.insert(item.field).second) throw std::invalid_argument("Invalid unavailable evidence"); unavailable.array_items().push_back(Json{{"field", item.field}, {"reason", item.reason}}); }
    if (value.diagnosticChain.is_null() && !fields.contains("diagnosticChain")) throw std::invalid_argument("Unavailable diagnostic chain requires explicit reason");
    if (!value.diagnosticChain.is_null() && fields.contains("diagnosticChain")) throw std::invalid_argument("Available diagnostic chain marked unavailable");
    if (value.diagnosticPersistence == DiagnosticPersistence::Persisted && value.diagnosticChain.is_null()) throw std::invalid_argument("No available chain can be claimed persisted");
    if (!value.diagnosticChain.is_null()) validateDiagnosticChain(value.diagnosticChain);
    auto faultJson = [](const OperationFault& item) { if (item.message.empty()) throw std::invalid_argument("Empty failure message"); return Json{{"phase", to_string(item.phase)}, {"code", to_string(item.code)}, {"message", item.message}, {"availableEvidence", item.evidence}}; };
    Json secondary = Json::array(); for (const auto& item : value.secondaryFailures) secondary.array_items().push_back(faultJson(item));
    Json result{{"phase", to_string(value.phase)}, {"code", to_string(value.code)}, {"availableEvidence", value.evidence}, {"unavailableEvidence", unavailable}, {"mutationState", to_string(value.mutationState)}, {"primaryFailure", faultJson(value.primaryFailure)}, {"secondaryFailures", secondary}, {"diagnosticChain", value.diagnosticChain}, {"diagnosticPersistence", to_string(value.diagnosticPersistence)}};
    if (value.mappingStatus) { if (*value.mappingStatus == MappingStatus::Mapped) throw std::invalid_argument("Failure cannot claim MAPPED"); result["mappingStatus"] = to_string(*value.mappingStatus); }
    return result;
}
namespace {
constexpr const char* contractHash = "98539d83ad6199cecc26eb7b9fcb92993073a2e511ebf70191330cd46ba35dc7";
constexpr const char* ashPin = "e123f5d7fdbb381179971f721a3292c31eb1cbc2";
Json selectFields(const Json& document, std::initializer_list<const char*> fields) {
    Json result = Json::object();
    for (auto field : fields) result[field] = document.at(field);
    return result;
}
DiagnosticBundle actualDiagnostic(const std::string& summary, const Json& evidence, const std::string& stage, const std::string& disposition, const char* ruleID, const char* family, const char* sourcePath, const Json* prior = nullptr, const std::string& kind = "STATE_VALIDITY") {
    const auto source = std::string("https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/") + sourcePath;
    AshDiagnosticContext context{kind, stage, disposition, "semantic-evidence-" + sha256(evidence.dump()), {summary, "Result derives from actual source-bound inputs; unavailable evidence is retained as unavailable"}};
    return makeAshDiagnosticBundle(context, disposition == "BLOCKED" ? "ERROR" : "INFO", summary, evidence, Json::array({Json{{"ruleID", ruleID}, {"ruleFamily", family}, {"sourceReference", source}}}), {source}, prior);
}
OperationFailure fault(std::string phase, std::string code, std::string message, Json evidence = Json::object()) {
    OperationFailure result(operationPhase(phase), operationCode(code), std::move(message), std::move(evidence));
    try { Json actualEvidence = result.evidence.is_object() ? result.evidence : Json{{"availableEvidence", result.evidence}};
        const auto bundle = actualDiagnostic(result.message, actualEvidence, phase == "PROJECT" ? "DETECTION" : "CLASSIFICATION", "BLOCKED", "ASH-STATE-STRUCTURE-001", "ASH-STATE", "specs/core/ash-state-space.pseudo.md");
        result.diagnosticChain = bundle.chain; result.diagnosticPersistence = DiagnosticPersistence::NotRequested; std::erase_if(result.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
    } catch (const std::exception& error) { result.addSecondary(OperationPhase::Export, OperationCode::DiagnosticUnavailable, error.what()); }
    return result;
}
bool hasState(const Json& catalog, const AshState& state) {
    for (const auto& entry : catalog.at("states").array_items()) if (parseAshState(entry) == state) return true;
    return false;
}
AshState xorState(const AshState& left, const AshState& right) {
    AshState result{};
    for (std::size_t i = 0; i < 9; ++i) result[i] = static_cast<unsigned char>(left[i] ^ right[i]);
    return result;
}
bool isCodeword(const AshState& state) {
    const auto& words = canonicalCodewords();
    return std::find(words.begin(), words.end(), state) != words.end();
}
AshState stringState(const std::string& text) {
    if (text.size() != 9) throw std::invalid_argument("State must contain nine bits");
    AshState result{};
    for (std::size_t i = 0; i < 9; ++i) {
        if (text[i] != '0' && text[i] != '1') throw std::invalid_argument("Nonbinary state");
        result[i] = static_cast<unsigned char>(text[i] - '0');
    }
    return result;
}
Json canonicalDefinition() {
    Json result = Json::object(); result["coordinateOrder"] = Json::array(); result["members"] = Json::array();
    for (unsigned i = 0; i < 9; ++i) result["coordinateOrder"].array_items().push_back("b" + std::to_string(i));
    for (const auto& state : canonicalCodewords()) {
        std::string word; for (auto bit : state) word.push_back(static_cast<char>('0' + bit));
        result["members"].array_items().push_back(word);
    }
    return result;
}
void checkSnapshot(const Json& snapshot) {
    const auto raw = snapshot.at("rawUtf8").get<std::string>();
    if (snapshot.at("sourceName").get<std::string>().empty() || snapshot.at("revision").get<std::string>().empty()) throw std::invalid_argument("Snapshot identity absent");
    const auto parsed = Json::parse(raw);
    if (parsed != snapshot.at("document")) throw std::invalid_argument("Snapshot raw bytes differ from document");
    if (sha256(raw) != snapshot.at("rawByteSha256").get<std::string>()) throw std::invalid_argument("Snapshot raw hash mismatch");
    if (sha256(parsed.dump()) != snapshot.at("proposedCanonicalSha256").get<std::string>()) throw std::invalid_argument("Snapshot canonical hash mismatch");
}
Json snapshotEvidence(const Json& snapshot, const std::string& pointer, const Json* value) {
    Json evidence = Json::object();
    evidence["sourceName"] = snapshot.at("sourceName"); evidence["snapshotID"] = snapshot.at("revision");
    evidence["rawHash"] = snapshot.at("rawByteSha256"); evidence["canonicalHash"] = snapshot.at("proposedCanonicalSha256");
    evidence["pointer"] = pointer; evidence["present"] = value != nullptr;
    if (value) evidence["value"] = *value;
    evidence["ruleReferences"] = Json::array({"JASH-003", "JASH-004", "JASH-005", "JASH-006"});
    return evidence;
}
unsigned char binary(const Json& value) {
    if (!value.is_number() || (value != Json(0) && value != Json(1))) throw std::invalid_argument("Expected exact numeric binary coordinate");
    return static_cast<unsigned char>(value == Json(1) ? 1 : 0);
}
int statusRank(MappingStatus status) {
    switch (status) { case MappingStatus::Mapped: return 0; case MappingStatus::Ambiguous: return 1; case MappingStatus::Blocked: return 2; case MappingStatus::Failed: return 3; }
    throw std::invalid_argument("Unknown mapping status");
}
void mark(DimensionEvidence& evidence, MappingStatus status, std::string reason) {
    if (statusRank(status) > statusRank(evidence.status)) evidence.status = status;
    evidence.faults.push_back(std::move(reason)); evidence.bit.reset();
}
Json referenceIdentity(const Json& reference) {
    if (reference.contains("identity")) return selectFields(reference, {"identity", "revision", "sha256"});
    return reference;
}
Json semanticInput(const Json& input) {
    Json result = input;
    for (const char* key : {"mappingProfile", "proposedSourceAuthorityReference"}) {
        if (result.contains(key)) result[key] = referenceIdentity(result.at(key));
    }
    if (result.contains("registryReferences")) {
        for (auto& [name, value] : result["registryReferences"].object_items()) { (void)name; value = referenceIdentity(value); }
    }
    if (result.contains("canonicalizationReference")) {
        Json rule = Json::object(); rule["ruleID"] = result.at("canonicalizationReference").at("ruleID");
        rule["contractSha256"] = result.at("canonicalizationReference").at("contractReference").at("sha256"); result["canonicalizationReference"] = rule;
    }
    // These are supplied export sequencing, not projection/classification inputs.
    result.object_items().erase("diagnosticContext"); result.object_items().erase("auditContext");
    return result;
}
void checkReference(const Json& ref, const Json& definition, const char* idField, const std::string& rawHash) {
    if (ref.at("identity") != definition.at(idField) || ref.at("revision") != definition.at("revision") || ref.at("sha256") != Json(rawHash)) throw std::invalid_argument("Immutable definition reference mismatch");
}
}

namespace {
std::string hashStream(const std::function<void(const std::function<void(std::string_view)>&)>& emit) {
    struct Handles final { BCRYPT_ALG_HANDLE algorithm = nullptr; BCRYPT_HASH_HANDLE hash = nullptr; ~Handles() { if (hash) BCryptDestroyHash(hash); if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0); } } handles;
    auto& algorithm = handles.algorithm; auto& hash = handles.hash;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0) throw std::runtime_error("Cannot open SHA256 provider");
    DWORD objectSize = 0, read = 0;
    status = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &read, 0);
    std::vector<unsigned char> storage(objectSize); std::array<unsigned char, 32> digest{};
    if (status >= 0) status = BCryptCreateHash(algorithm, &hash, storage.data(), objectSize, nullptr, 0, 0);
    if (status < 0) throw std::runtime_error("SHA256 initialization failed");
    emit([&](std::string_view bytes) {
        if (bytes.size() > std::numeric_limits<ULONG>::max()) throw std::invalid_argument("Hash chunk exceeds native API range");
        if (BCryptHashData(hash, reinterpret_cast<PUCHAR>(const_cast<char*>(bytes.data())), static_cast<ULONG>(bytes.size()), 0) < 0) throw std::runtime_error("SHA256 chunk calculation failed");
    });
    if (status >= 0) status = BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0);
    if (status < 0) throw std::runtime_error("SHA256 calculation failed");
    constexpr char digits[] = "0123456789abcdef"; std::string result; result.reserve(64);
    for (auto value : digest) { result.push_back(digits[value >> 4]); result.push_back(digits[value & 15]); }
    return result;
}
std::vector<JsonSourceRegion> sourceRegions(const Json& input) {
    std::vector<JsonSourceRegion> regions{{"/observedSnapshot/document", "/observedSnapshot/rawUtf8"}, {"/desiredSnapshot/document", "/desiredSnapshot/rawUtf8"}};
    const auto& evidence = input.at("evidenceSnapshots"); if (!evidence.is_array()) throw std::invalid_argument("Evidence snapshots must be an array");
    for (std::size_t index = 0; index < evidence.size(); ++index) { const auto root = "/evidenceSnapshots/" + std::to_string(index); regions.push_back({root + "/document", root + "/rawUtf8"}); }
    return regions;
}
}
std::string sha256(std::string_view bytes) { return hashStream([&](const auto& consume) { consume(bytes); }); }
std::string hashCompleteSemanticInput(const Json& input) { const auto regions = sourceRegions(input); return hashStream([&](const auto& consume) { input.canonical_sources(regions, consume); }); }
std::string hashCapturedSnapshot(const Json& snapshot) { return hashStream([&](const auto& consume) { snapshot.canonical_sources({{"/document", "/rawUtf8"}}, consume); }); }
void validateCompleteSemanticInput(const Json& input) { input.canonical_sources(sourceRegions(input), [](std::string_view) {}); }
const std::array<AshState, 16>& canonicalCodewords() {
    static const std::array<AshState, 16> words{stringState("000000000"), stringState("000011110"), stringState("001100110"), stringState("001111000"), stringState("010101010"), stringState("010110100"), stringState("011001100"), stringState("011010010"), stringState("100101100"), stringState("100110010"), stringState("101001010"), stringState("101010100"), stringState("110000110"), stringState("110011000"), stringState("111100000"), stringState("111111110")};
    return words;
}
Json stateJson(const AshState& state) { Json result = Json::array(); for (auto bit : state) { if (bit > 1) throw std::invalid_argument("Malformed state coordinate"); result.array_items().emplace_back(static_cast<int>(bit)); } return result; }
AshState parseAshState(const Json& value) {
    if (value.is_string()) return stringState(value.get<std::string>());
    if (!value.is_array() || value.size() != 9) throw std::invalid_argument("Expected nine-coordinate state");
    AshState result{}; for (std::size_t i = 0; i < 9; ++i) result[i] = binary(value.at(i)); return result;
}
std::string to_string(MappingStatus value) { switch (value) { case MappingStatus::Mapped: return "MAPPED"; case MappingStatus::Ambiguous: return "AMBIGUOUS"; case MappingStatus::Blocked: return "BLOCKED"; case MappingStatus::Failed: return "FAILED"; } throw std::invalid_argument("Invalid status"); }
std::string to_string(SystemClass value) { switch (value) { case SystemClass::Stable: return "STABLE"; case SystemClass::Unstable: return "UNSTABLE"; case SystemClass::Correctable: return "CORRECTABLE"; case SystemClass::Degraded: return "DEGRADED"; case SystemClass::Contained: return "CONTAINED"; case SystemClass::Failed: return "FAILED"; case SystemClass::SafeHalt: return "SAFE_HALT"; } throw std::invalid_argument("Invalid class"); }
std::string to_string(RecoveryCategory value) { switch (value) { case RecoveryCategory::NoAction: return "NO_ACTION"; case RecoveryCategory::NormalizeState: return "NORMALIZE_STATE"; case RecoveryCategory::ApplyCorrection: return "APPLY_CORRECTION"; case RecoveryCategory::FallbackRequired: return "FALLBACK_REQUIRED"; case RecoveryCategory::ContainmentRequired: return "CONTAINMENT_REQUIRED"; case RecoveryCategory::EscalationRequired: return "ESCALATION_REQUIRED"; case RecoveryCategory::TerminalNoRecovery: return "TERMINAL_NO_RECOVERY"; } throw std::invalid_argument("Invalid recovery category"); }
RecoveryCategory recoveryForClass(SystemClass value) { switch (value) { case SystemClass::Stable: return RecoveryCategory::NoAction; case SystemClass::Unstable: return RecoveryCategory::NormalizeState; case SystemClass::Correctable: return RecoveryCategory::ApplyCorrection; case SystemClass::Degraded: return RecoveryCategory::FallbackRequired; case SystemClass::Contained: return RecoveryCategory::ContainmentRequired; case SystemClass::Failed: return RecoveryCategory::EscalationRequired; case SystemClass::SafeHalt: return RecoveryCategory::TerminalNoRecovery; } throw std::invalid_argument("Invalid class"); }
RecoveryCategory blockedRecoveryTransition(RecoveryCategory current, std::string_view condition) {
    if (current == RecoveryCategory::TerminalNoRecovery) return current;
    if (current == RecoveryCategory::NormalizeState && condition == "normalization_path_not_computable") return RecoveryCategory::ContainmentRequired;
    if (current == RecoveryCategory::ApplyCorrection && condition == "correction_path_not_computable") return RecoveryCategory::FallbackRequired;
    if (current == RecoveryCategory::FallbackRequired && (condition == "fallback_registry_missing" || condition == "no_valid_fallback_candidates")) return RecoveryCategory::ContainmentRequired;
    if (current == RecoveryCategory::ContainmentRequired && condition == "containment_boundary_breached") return RecoveryCategory::TerminalNoRecovery;
    if (current == RecoveryCategory::EscalationRequired && condition == "external_authority_unreachable") return RecoveryCategory::TerminalNoRecovery;
    throw std::invalid_argument("Condition does not authorize this recovery transition");
}
SystemClass classifySystemState(const StateDiagnostic& diagnostic, bool safeHalt, bool contained, bool correctionPathKnown, bool fallbackAvailable) {
    for (auto bit : diagnostic.state) if (bit > 1) throw std::invalid_argument("Malformed classifier state");
    if (safeHalt) return SystemClass::SafeHalt; if (contained) return SystemClass::Contained;
    if (diagnostic.admissibility == "VALID" && diagnostic.normalizationStatus == "ALREADY_VALID") return SystemClass::Stable;
    if (diagnostic.admissibility == "TRANSFORMATION_COMPATIBLE" && diagnostic.normalizationStatus == "NORMALIZABLE") return correctionPathKnown ? SystemClass::Correctable : SystemClass::Unstable;
    if (diagnostic.admissibility == "TRANSFORMATION_INCOMPATIBLE") return fallbackAvailable ? SystemClass::Degraded : SystemClass::Failed;
    return SystemClass::Degraded;
}
SemanticEnvironment::SemanticEnvironment(std::string profileBytes, std::string catalogBytes, std::string registryBytes, std::string sourceAuthorityBytes)
    {
    for (const auto* bytes : {&profileBytes, &catalogBytes, &registryBytes, &sourceAuthorityBytes}) { if (bytes->size() > JsonLimits::rawDocumentBytes || bytes->size() > JsonLimits::aggregateInputBytes - authorityInputBytes_) throw std::invalid_argument("Authority document/aggregate input limit exceeded"); authorityInputBytes_ += bytes->size(); }
    profile_ = Json::parse(profileBytes); catalog_ = Json::parse(catalogBytes); registry_ = Json::parse(registryBytes); sourceAuthority_ = Json::parse(sourceAuthorityBytes);
    profileRawHash_ = sha256(profileBytes); catalogRawHash_ = sha256(catalogBytes); registryRawHash_ = sha256(registryBytes); sourceAuthorityRawHash_ = sha256(sourceAuthorityBytes);
    if (catalog_.at("catalogID") != Json("AEOSTARA-DEMO-K-V1") || catalog_.at("revision") != Json(1) || catalog_.at("states") != Json::array({"000000000", "000000001"})) throw std::invalid_argument("Unadopted recognized-valid catalog");
    profileRef_ = selectFields(profile_, {"profileID", "revision"});
    profileRef_["semanticDefinitionHash"] = sha256(selectFields(profile_, {"profileID", "revision", "canonicalizationRuleID", "evaluatorID", "recognizedValidCatalogID", "acceptedCarrierShape", "dimensions", "mutationScope", "inverseRuleID"}).dump());
    catalogRef_ = selectFields(catalog_, {"catalogID", "revision"}); catalogRef_["semanticDefinitionHash"] = sha256(selectFields(catalog_, {"catalogID", "revision", "states"}).dump());
    codewordRef_ = Json::object(); codewordRef_["repository"] = "flynn33/ASH-Pattern-System"; codewordRef_["commit"] = ashPin; codewordRef_["path"] = "specs/core/codeword-set.pseudo.md"; codewordRef_["semanticDefinitionHash"] = sha256(canonicalDefinition().dump());
    sourceAuthorityRef_ = selectFields(sourceAuthority_, {"authorityID", "revision"}); sourceAuthorityRef_["semanticDefinitionHash"] = sha256(selectFields(sourceAuthority_, {"authorityID", "revision", "allowedSourceNames", "allowedEvidenceKinds", "provenanceRuleID"}).dump());
}
SemanticEnvironment SemanticEnvironment::adoptedDirect() { return {adoptedDirectBytes, adoptedCatalogBytes, adoptedRegistryBytes, adoptedAuthorityBytes}; }
SemanticEnvironment SemanticEnvironment::adoptedClaims() { return {adoptedClaimsBytes, adoptedCatalogBytes, adoptedRegistryBytes, adoptedAuthorityBytes}; }

OperationAttempt<Projection> SemanticEnvironment::project(const Json& input) const {
    Projection result; MappingStatus overall = MappingStatus::Mapped;
    try {
        for (const char* field : {"observedSnapshot", "desiredSnapshot", "mappingProfile", "canonicalizationReference", "ashAuthorityReference", "applicableInvariants", "evidenceSnapshots", "evidenceReferenceManifest", "lifecycleContext", "registryReferences", "policyContext", "selectedTargetReference", "realizationRuleReference", "executionContext", "diagnosticContext", "auditContext"}) if (!input.contains(field)) return fault("PROJECT", "BLOCKED", std::string("Missing complete semantic input: ") + field);
        if (input.at("canonicalizationReference").at("ruleID") != Json("JASH-CANON-EXACT-DECIMAL-V1") || input.at("canonicalizationReference").at("contractReference").at("sha256") != Json(contractHash)) throw std::invalid_argument("Canonicalization authority mismatch");
        if (input.at("ashAuthorityReference").at("commit") != Json(ashPin)) throw std::invalid_argument("ASH pin mismatch");
        checkReference(input.at("mappingProfile"), profile_, "profileID", profileRawHash_);
        checkReference(input.at("registryReferences").at("recognizedValidStates"), catalog_, "catalogID", catalogRawHash_);
        checkReference(input.at("registryReferences").at("fallback"), registry_, "registryID", registryRawHash_);
        const auto& evidenceSources = input.at("evidenceSnapshots");
        if (!evidenceSources.is_array() || evidenceSources.size() > JsonLimits::sourceDocuments - 6) throw std::invalid_argument("Complete semantic source-document count exceeds adopted limit");
        std::size_t aggregateBytes = authorityInputBytes_;
        const auto account = [&](const Json& snapshot) {
            const auto raw = snapshot.at("rawUtf8").get<std::string>();
            if (raw.size() > JsonLimits::rawDocumentBytes || raw.size() > JsonLimits::aggregateInputBytes - aggregateBytes) throw std::invalid_argument("Complete semantic raw document/aggregate input limit exceeded");
            aggregateBytes += raw.size();
        };
        account(input.at("observedSnapshot")); account(input.at("desiredSnapshot"));
        for (const auto& source : evidenceSources.array_items()) account(source);
        checkSnapshot(input.at("observedSnapshot")); checkSnapshot(input.at("desiredSnapshot"));
        std::map<std::string, const Json*> snapshots; snapshots.emplace("observed", &input.at("observedSnapshot")); snapshots.emplace("desired", &input.at("desiredSnapshot"));
        for (const auto& snapshot : input.at("evidenceSnapshots").array_items()) {
            checkSnapshot(snapshot); auto name = snapshot.at("sourceName").get<std::string>();
            if (!snapshots.emplace(name, &snapshot).second) throw std::invalid_argument("Duplicate immutable source name");
        }
        if (!profile_.at("dimensions").is_array() || profile_.at("dimensions").size() != 9) throw std::invalid_argument("Profile must define exactly nine dimensions");
        const auto& observed = input.at("observedSnapshot").at("document");
        bool badCarrier = !observed.is_object(); const Json* carrier = observed.resolve_pointer("/state_vector");
        if (carrier && (!carrier->is_array() || carrier->size() > 9)) badCarrier = true;
        const bool supportedEvaluator = profile_.at("evaluatorID") == Json("JASH-BINARY-EXACT-V1");
        std::map<std::string, const Json*> manifest;
        bool needsClaims = profile_.at("profileID") == Json("AEOSTARA-DEMO-DIRECT-BINARY-CLAIMS-V1");
        bool provenanceUnavailable = false;
        if (needsClaims) {
            for (const char* field : {"proposedSourceAuthorityDefinition", "registeredAuthorityRef"}) if (!input.contains(field)) provenanceUnavailable = true;
            if (!input.at("evidenceReferenceManifest").contains("references")) provenanceUnavailable = true;
            if (!provenanceUnavailable) {
                if (selectFields(input.at("proposedSourceAuthorityDefinition"), {"authorityID", "revision", "allowedSourceNames", "allowedEvidenceKinds", "provenanceRuleID"}) != selectFields(sourceAuthority_, {"authorityID", "revision", "allowedSourceNames", "allowedEvidenceKinds", "provenanceRuleID"}) || input.at("registeredAuthorityRef") != sourceAuthorityRef_ || input.at("evidenceReferenceManifest").at("authorityRef") != sourceAuthorityRef_) throw std::invalid_argument("Unregistered or mismatched source authority");
                if (input.at("evidenceReferenceManifest").at("manifestID").get<std::string>().empty() || input.at("evidenceReferenceManifest").at("revision") != Json(1)) throw std::invalid_argument("Malformed source manifest identity");
                for (const auto& record : input.at("evidenceReferenceManifest").at("references").array_items()) {
                    auto reference = record.at("reference").get<std::string>(); if (reference.empty() || !manifest.emplace(reference, &record).second) throw std::invalid_argument("Empty/duplicate source manifest reference");
                }
            }
        }
        for (std::size_t i = 0; i < 9; ++i) {
            auto& dimension = result.dimensions[i]; dimension.coordinate = "b" + std::to_string(i); dimension.status = MappingStatus::Mapped;
            const auto& definition = profile_.at("dimensions").at(i);
            if (definition.at("dimension") != Json(dimension.coordinate) || definition.at("index") != Json(static_cast<int>(i))) throw std::invalid_argument("Profile coordinate ordering mismatch");
            if (badCarrier) mark(dimension, MappingStatus::Failed, "Wrong carrier type or more than nine entries");
            if (!supportedEvaluator) mark(dimension, MappingStatus::Blocked, "Unsupported profile evaluator");
            std::optional<unsigned char> agreed;
            for (const auto& binding : definition.at("sourceBindings").array_items()) {
                const auto name = binding.at("sourceName").get<std::string>(); const auto pointer = binding.at("pointer").get<std::string>();
                if (binding.at("required") != Json(true)) throw std::invalid_argument("Unadopted optional binding");
                if ((name == "observed" && pointer != "/state_vector/" + std::to_string(i)) || (name == "claims" && pointer != "/claims/" + std::to_string(i) + "/value") || (name != "observed" && name != "claims")) throw std::invalid_argument("Unadopted source binding");
                if (binding.at("evidenceKind") != Json("JSON_BINARY_COORDINATE")) { mark(dimension, MappingStatus::Blocked, "Unsupported evidence kind"); continue; }
                auto source = snapshots.find(name); if (source == snapshots.end()) { mark(dimension, MappingStatus::Blocked, "Required source unavailable: " + name); continue; }
                const auto& snapshot = *source->second; const auto* value = snapshot.at("document").resolve_pointer(pointer);
                dimension.sources.array_items().push_back(snapshotEvidence(snapshot, pointer, value));
                if (!value) { mark(dimension, MappingStatus::Blocked, "Required Pointer absent: " + pointer); continue; }
                if (name == "claims") {
                    if (provenanceUnavailable) { mark(dimension, MappingStatus::Blocked, "Source authority/manifest unavailable"); continue; }
                    const auto* metadata = snapshot.at("document").resolve_pointer("/claims/" + std::to_string(i));
                    if (!metadata || !metadata->is_object() || !metadata->contains("coordinate") || !metadata->contains("kind") || !metadata->contains("sourceReference")) { mark(dimension, MappingStatus::Failed, "Malformed claims metadata"); continue; }
                    if (metadata->at("coordinate") != Json(dimension.coordinate) || !metadata->at("kind").is_string() || !metadata->at("sourceReference").is_string()) { mark(dimension, MappingStatus::Failed, "Claims metadata mismatch"); continue; }
                    if (metadata->at("kind") != Json("JSON_BINARY_COORDINATE")) { mark(dimension, MappingStatus::Blocked, "Unsupported claim evidence kind"); continue; }
                    auto logical = metadata->at("sourceReference").get<std::string>(); if (logical.empty()) { mark(dimension, MappingStatus::Failed, "Empty claim source reference"); continue; }
                    auto resolved = manifest.find(logical); if (resolved == manifest.end()) { mark(dimension, MappingStatus::Blocked, "Claim reference unresolved"); continue; }
                    const auto& record = *resolved->second;
                    if (record.at("authorityID") != sourceAuthority_.at("authorityID") || record.at("sourceName") != Json(name) || record.at("snapshotID") != snapshot.at("revision") || record.at("canonicalDocumentHash") != snapshot.at("proposedCanonicalSha256") || record.at("pointer") != Json(pointer) || record.at("coordinate") != Json(dimension.coordinate) || record.at("evidenceKind") != Json("JSON_BINARY_COORDINATE")) { mark(dimension, MappingStatus::Failed, "Tampered claim provenance binding"); continue; }
                }
                try { auto bit = binary(*value); if (agreed && *agreed != bit) mark(dimension, MappingStatus::Ambiguous, "Genuine bound sources disagree"); else agreed = bit; }
                catch (const std::exception& error) { mark(dimension, MappingStatus::Failed, error.what()); }
            }
            if (dimension.status == MappingStatus::Mapped && agreed) { dimension.bit = agreed; result.state[i] = *agreed; }
            else if (dimension.status == MappingStatus::Mapped) mark(dimension, MappingStatus::Blocked, "No genuine coordinate evidence");
            if (statusRank(dimension.status) > statusRank(overall)) overall = dimension.status;
        }
        result.completeSemanticInputHash = hashCompleteSemanticInput(semanticInput(input));
        if (overall == MappingStatus::Mapped) { Json evidence{{"projectedState", stateJson(result.state)}, {"semanticInputHash", result.completeSemanticInputHash}, {"profileRef", profileRef_}}; result.diagnosticChain = actualDiagnostic("Nine coordinates mapped from genuine registered source evidence", evidence, "DETECTION", "RESOLVED", "ASH-STATE-STRUCTURE-001", "ASH-STATE", "specs/core/ash-state-space.pseudo.md").chain; return result; }
        Json evidence = Json::object(); evidence["mappingStatus"] = to_string(overall); evidence["dimensions"] = Json::array();
        for (const auto& dimension : result.dimensions) { Json entry = Json::object(); entry["coordinate"] = dimension.coordinate; entry["status"] = to_string(dimension.status); entry["sources"] = dimension.sources; entry["faults"] = dimension.faults; if (dimension.bit) entry["bit"] = static_cast<int>(*dimension.bit); evidence["dimensions"].array_items().push_back(entry); }
        return fault("PROJECT", to_string(overall), "No unique genuine full projected state", std::move(evidence));
    } catch (const std::exception& error) { return fault("PROJECT", "FAILED", error.what()); }
}

StateDiagnostic SemanticEnvironment::diagnose(const AshState& state) const {
    for (auto bit : state) if (bit > 1) throw std::invalid_argument("Malformed ASH state");
    StateDiagnostic result; result.state = state; result.ruleIDs = {"ASH-STATE-STRUCTURE-001", "ASH-CODEWORD-STRUCTURE-001", "ASH-ADMISSIBILITY-CLASSIFICATION-001", "ASH-STATE-VALIDITY-001"};
    bool reachesValid = false;
    for (const auto& codeword : canonicalCodewords()) { auto candidate = xorState(state, codeword); result.orbit.push_back(candidate); reachesValid = reachesValid || hasState(catalog_, candidate); }
    result.valid = hasState(catalog_, state);
    result.admissibility = result.valid ? "VALID" : reachesValid ? "TRANSFORMATION_COMPATIBLE" : "TRANSFORMATION_INCOMPATIBLE";
    result.transformationCompatibility = result.valid || reachesValid ? "COMPATIBLE" : "INCOMPATIBLE";
    result.normalizationStatus = result.valid ? "ALREADY_VALID" : reachesValid ? "NORMALIZABLE" : "NOT_NORMALIZABLE";
    result.recoverabilityRelevance = result.valid ? "NO_RECOVERY_NEEDED" : reachesValid ? "RECOVERY_APPLICABLE" : "NOT_RECOVERABLE";
    Json evidence{{"actualState", stateJson(state)}, {"admissibility", result.admissibility}, {"normalizationStatus", result.normalizationStatus}, {"catalogRef", catalogRef_}, {"codewordSetRef", codewordRef_}};
    result.diagnosticChain = actualDiagnostic("Full-vector validity independently calculated against adopted K and canonical C", evidence, "DETECTION", "RESOLVED", "ASH-STATE-VALIDITY-001", "ASH-STATE", "specs/core/state-validity-diagnostics.pseudo.md").chain;
    return result;
}
OperationAttempt<SemanticAnalysis> SemanticEnvironment::analyze(const Json& input) const {
    auto projected = project(input); if (auto error = std::get_if<OperationFailure>(&projected)) return *error;
    SemanticAnalysis result; result.projection = std::get<Projection>(std::move(projected)); result.diagnostic = diagnose(result.projection.state);
    try {
        const auto& context = input.at("lifecycleContext"); const auto& path = context.at("correctionPath"); const auto selection = path.at("status").get<std::string>(); bool known = false;
        if (selection == "selected") {
            if (parseAshState(path.at("sourceState")) != result.projection.state || path.at("profileRef") != profileRef_ || path.at("catalogRef") != catalogRef_ || path.at("codewordSetRef") != codewordRef_) throw std::invalid_argument("Witness source/authority mismatch");
            auto target = parseAshState(path.at("targetState")); if (!hasState(catalog_, target)) throw std::invalid_argument("Witness target outside adopted K");
            const auto& words = path.at("codewords"); if (!words.is_array() || words.empty() || words.size() > 16) throw std::invalid_argument("Witness must contain 1..16 codewords");
            auto current = result.projection.state; for (const auto& word : words.array_items()) { auto c = parseAshState(word); if (!isCodeword(c)) throw std::invalid_argument("Witness contains noncanonical codeword"); current = xorState(current, c); }
            if (current != target) throw std::invalid_argument("Witness does not reach selected target");
            auto source = path.at("evidenceSourceReference").get<std::string>(); if (source.empty() || source.find('\\') != std::string::npos || source.starts_with("/") || source.starts_with("http:" ) || source.starts_with("https:")) throw std::invalid_argument("Witness source is not logical immutable authority");
            Json payload = selectFields(path, {"sourceState", "targetState", "codewords", "evidenceSourceReference", "profileRef", "catalogRef", "codewordSetRef"}); payload["witnessRuleID"] = "JASH-CORRECTION-WITNESS-ID-V1";
            result.witnessID = sha256(payload.dump()); if (path.at("witnessID") != Json(result.witnessID)) throw std::invalid_argument("Witness content identity mismatch"); known = true; result.selectedTarget = target;
        } else if (selection == "not_selected") {
            if (!path.at("codewords").is_array() || !path.at("codewords").empty() || !path.at("targetState").is_null()) throw std::invalid_argument("Invalid explicit unselected witness");
        } else if (result.diagnostic.admissibility == "TRANSFORMATION_COMPATIBLE" && !context.at("is_in_safe_halt").get<bool>() && !context.at("is_in_containment").get<bool>()) return fault("CLASSIFY", "BLOCKED", "Correction context unavailable");
        std::vector<std::pair<std::pair<int, std::string>, AshState>> candidates;
        if (result.diagnostic.admissibility == "TRANSFORMATION_INCOMPATIBLE") {
            std::set<std::string> ids;
            for (const auto& entry : registry_.at("entries").array_items()) {
                auto id = entry.at("policy_id").get<std::string>(); if (!id.starts_with("FALLBACK-") || !ids.insert(id).second) throw std::invalid_argument("Invalid/duplicate fallback policy ID");
                if (entry.at("applicability_conditions") != Json::array({"diagnostic.admissibility == TRANSFORMATION_INCOMPATIBLE", "candidate belongs to adopted fixed K", "entry is enabled by immutable registry revision"}) || entry.at("validation_requirements") != Json::array({"candidate diagnoses VALID", "candidate classifies STABLE in fresh nonterminal validation context"}) || (entry.at("escalation_on_failure") != Json("TRY_NEXT") && entry.at("escalation_on_failure") != Json("ESCALATE_TO_CONTAINMENT"))) throw std::invalid_argument("Unsupported fallback registry predicate/escalation binding");
                auto candidate = parseAshState(entry.at("candidate_state_reference")); auto diagnostic = diagnose(candidate);
                if (diagnostic.valid && classifySystemState(diagnostic, false, false, false, false) == SystemClass::Stable) candidates.push_back({{entry.at("ordering_rank").get<int>(), id}, candidate});
            }
            std::sort(candidates.begin(), candidates.end(), [](const auto& left, const auto& right) { return left.first < right.first; });
        }
        result.stateClass = classifySystemState(result.diagnostic, context.at("is_in_safe_halt").get<bool>(), context.at("is_in_containment").get<bool>(), known, !candidates.empty()); result.recovery = recoveryForClass(result.stateClass);
        if (result.stateClass == SystemClass::Degraded && !candidates.empty()) { result.selectedFallbackID = candidates.front().first.second; result.selectedTarget = candidates.front().second; }
        if (result.recovery == RecoveryCategory::ContainmentRequired && context.at("containmentBoundaryBreached").get<bool>()) result.recovery = blockedRecoveryTransition(result.recovery, "containment_boundary_breached");
        if (result.recovery == RecoveryCategory::EscalationRequired && !context.at("externalAuthorityReachable").get<bool>()) result.recovery = blockedRecoveryTransition(result.recovery, "external_authority_unreachable");
        Json evidence{{"actualState", stateJson(result.projection.state)}, {"stateClass", to_string(result.stateClass)}, {"recoveryCategory", to_string(result.recovery)}, {"lifecycleContext", context}, {"witnessID", result.witnessID}, {"catalogRef", catalogRef_}, {"codewordSetRef", codewordRef_}};
        result.diagnosticChain = actualDiagnostic("Canonical classification uses actual full-state diagnosis and complete lifecycle context", evidence, "CLASSIFICATION", "RESOLVED", "ASH-CLASSIFICATION-MAPPING-001", "ASH-CLASSIFICATION", "specs/core/system-state-classification.pseudo.md", &result.projection.diagnosticChain).chain;
        return result;
    } catch (const std::exception& error) { return fault("CLASSIFY", "FAILED", error.what()); }
}

InvariantEvaluation SemanticEnvironment::evaluateInvariants(const Json& input, const Json& document, std::string_view role, const Json& operations) const {
    InvariantEvaluation result; result.passed = true;
    try {
        const auto& bundle = input.at("applicableInvariants");
        if (bundle.at("bundleID").get<std::string>().empty() || bundle.at("sourceReference").get<std::string>().empty() || !bundle.at("revision").is_number()) throw std::invalid_argument("Invariant authority incomplete");
        Json definition = selectFields(bundle, {"bundleID", "revision", "sourceReference", "predicates"});
        if (bundle.contains("proposedDefinitionHash") && bundle.at("proposedDefinitionHash") != Json(sha256(definition.dump()))) throw std::invalid_argument("Invariant definition hash mismatch");
        std::set<std::string> ids;
        for (const auto& invariant : bundle.at("predicates").array_items()) {
            const auto id = invariant.at("invariantID").get<std::string>(); const auto declaredRole = invariant.at("role").get<std::string>();
            if (id.empty() || !ids.insert(id).second || invariant.at("sourceReference").get<std::string>().empty() || !invariant.at("revision").is_number()) throw std::invalid_argument("Invalid invariant identity");
            if (declaredRole != "MUTATION_PRECONDITION" && declaredRole != "LIFECYCLE_SUCCESS_POSTCONDITION" && declaredRole != "RESTORATION_SAFETY" && declaredRole != "DIAGNOSTIC_ONLY") throw std::invalid_argument("Undefined invariant role");
            const auto& predicate = invariant.at("predicate"); const auto op = predicate.at("operator").get<std::string>();
            bool passed = false, available = true;
            if (op == "ALL_PLANNED_POINTERS_IN_SET") {
                passed = true;
                for (const auto& operation : operations.array_items()) {
                    bool allowed = false; for (const auto& pointer : predicate.at("allowedPointers").array_items()) if (pointer == operation.at("pointer")) allowed = true;
                    passed = passed && allowed;
                }
            } else if (op == "TYPE_IS" || op == "EQUALS" || op == "STATE_IN_K") {
                if (predicate.at("sourceName") != Json("observed")) available = false;
                const auto* actual = document.resolve_pointer(predicate.at("pointer").get<std::string>());
                if (!actual) available = false;
                if (available && op == "TYPE_IS") {
                    const auto type = predicate.at("type").get<std::string>();
                    if (type == "array") passed = actual->is_array(); else if (type == "object") passed = actual->is_object(); else if (type == "number") passed = actual->is_number(); else if (type == "string") passed = actual->is_string(); else if (type == "boolean") passed = actual->is_boolean(); else if (type == "null") passed = actual->is_null(); else available = false;
                } else if (available && op == "EQUALS") passed = *actual == predicate.at("value");
                else if (available && op == "STATE_IN_K") passed = hasState(catalog_, parseAshState(*actual));
            } else available = false;
            Json record = Json::object(); record["invariantID"] = id; record["role"] = declaredRole; record["result"] = !available ? "UNAVAILABLE" : passed ? "PASS" : "VIOLATION"; record["predicate"] = predicate; result.results.array_items().push_back(record);
            if (!available) { result.available = false; result.passed = false; }
            if (declaredRole == role && invariant.at("mandatory").get<bool>() && !passed) result.passed = false;
        }
    } catch (const std::exception& error) { result.available = false; result.passed = false; Json record = Json::object(); record["result"] = "UNAVAILABLE"; record["error"] = error.what(); result.results.array_items().push_back(record); }
    return result;
}

OperationAttempt<SemanticPlan> SemanticEnvironment::plan(const Json& input) const {
    auto analysis = analyze(input); if (auto error = std::get_if<OperationFailure>(&analysis)) return *error;
    SemanticPlan result; result.analysis = std::get<SemanticAnalysis>(std::move(analysis));
    std::string activePhase = "PLAN";
    try {
        const auto& snapshot = input.at("observedSnapshot"); const auto& original = snapshot.at("document"); result.expectedDocument = original;
        result.rawPreimageHash = snapshot.at("rawByteSha256").get<std::string>(); result.canonicalPreimageHash = snapshot.at("proposedCanonicalSha256").get<std::string>(); result.semanticDependencyHash = result.analysis.projection.completeSemanticInputHash;
        if (result.analysis.recovery == RecoveryCategory::NoAction) {
            const auto invariants = evaluateInvariants(input, original, "LIFECYCLE_SUCCESS_POSTCONDITION", result.operations);
            if (!invariants.passed) {
                Json evidence = Json::object(); evidence["stateClass"] = to_string(result.analysis.stateClass); evidence["recoveryCategory"] = to_string(result.analysis.recovery); evidence["invariants"] = invariants.results;
                return fault("PLAN", "BLOCKED", "NO_ACTION does not satisfy lifecycle success invariants", std::move(evidence));
            }
        } else {
            if (result.analysis.recovery != RecoveryCategory::NormalizeState && result.analysis.recovery != RecoveryCategory::ApplyCorrection && result.analysis.recovery != RecoveryCategory::FallbackRequired) return fault("PLAN", "BLOCKED", "Recovery requires explicitly bound containment/escalation/terminal host capability");
            if (!result.analysis.selectedTarget) return fault("PLAN", "BLOCKED", "No explicitly selected known target; mathematical path is not execution authority");
            if (!input.at("selectedTargetReference").contains("state") || parseAshState(input.at("selectedTargetReference").at("state")) != *result.analysis.selectedTarget || input.at("selectedTargetReference").at("availability") != Json("selected")) return fault("PLAN", "BLOCKED", "Target materialization authorization unavailable or mismatched");
            if (!input.at("realizationRuleReference").contains("ruleID") || input.at("realizationRuleReference").at("ruleID") != Json("JASH-DEMO-CARRIER-INVERSE-V1")) return fault("PLAN", "BLOCKED", "Adopted inverse rule unavailable");
            result.inverseRuleID = "JASH-DEMO-CARRIER-INVERSE-V1";
            for (std::size_t i = 0; i < 9; ++i) {
                if (result.analysis.projection.state[i] == result.analysis.selectedTarget->at(i)) continue;
                const auto pointer = "/state_vector/" + std::to_string(i); const auto* old = original.resolve_pointer(pointer); if (!old) throw std::invalid_argument("Inverse requires existing slot");
                Json operation = Json::object(); operation["operation"] = "replace"; operation["pointer"] = pointer; operation["expectedPreimage"] = *old; operation["value"] = static_cast<int>(result.analysis.selectedTarget->at(i)); result.operations.array_items().push_back(operation);
                result.expectedDocument.replace_pointer(pointer, operation.at("value"));
            }
            const auto preconditions = evaluateInvariants(input, original, "MUTATION_PRECONDITION", result.operations); if (!preconditions.passed) return fault("PLAN", "BLOCKED", "Mutation precondition invariant failed/unavailable", preconditions.results);
            const auto postconditions = evaluateInvariants(input, result.expectedDocument, "LIFECYCLE_SUCCESS_POSTCONDITION", result.operations); if (!postconditions.passed) return fault("PLAN", "BLOCKED", "Planned target violates lifecycle success invariants", postconditions.results);
        }
        Json planIdentity = Json::object(); planIdentity["operations"] = result.operations; planIdentity["expectedDocument"] = result.expectedDocument; planIdentity["rawPreimageHash"] = result.rawPreimageHash; planIdentity["canonicalPreimageHash"] = result.canonicalPreimageHash; planIdentity["semanticDependencyHash"] = result.semanticDependencyHash; planIdentity["inverseRuleID"] = result.inverseRuleID; planIdentity["witnessID"] = result.analysis.witnessID; planIdentity["recoveryCategory"] = to_string(result.analysis.recovery); result.planHash = sha256(planIdentity.dump());
        if (result.operations.empty()) return result;
        activePhase = "POLICY";
        const auto& policy = input.at("policyContext"); if (!policy.contains("bundle")) return fault("POLICY", "BLOCKED", "Actual policy bundle unavailable");
        const auto& bundle = policy.at("bundle"); result.policyBundleHash = sha256(bundle.dump()); if (!policy.contains("bundleSha256") || policy.at("bundleSha256") != Json(result.policyBundleHash)) throw std::invalid_argument("Policy bundle content identity mismatch");
        if (bundle.at("policyBundleID").get<std::string>().empty() || bundle.at("authorityReference").get<std::string>().empty() || bundle.at("revision") != Json(1)) throw std::invalid_argument("Policy authority incomplete");
        if (!policy.contains("evaluationRequest") || !input.contains("proposedPlan")) return fault("POLICY", "BLOCKED", "Content-bound authorization request/plan unavailable");
        if (result.policyBundleHash != "c68bc8b85cdfe697e8fd69ad89a553e18c2bcc692270978df698e54597ab1f82") return fault("POLICY", "BLOCKED", "Policy bundle is outside the exact adopted immutable SDK policy binding");
        const auto& submittedPlan = input.at("proposedPlan");
        if (submittedPlan.at("expectedObservedInputSha256") != Json(hashCapturedSnapshot(input.at("observedSnapshot")))) throw std::invalid_argument("Submitted plan observed-snapshot content identity mismatch");
        if (submittedPlan.at("planReference") != policy.at("authorizationPlanReference") || submittedPlan.at("planReference") != input.at("selectedTargetReference").at("authorizationPlanReference")) throw std::invalid_argument("Plan/target/policy authorization identity mismatch");
        if (submittedPlan.at("operations") != result.operations || parseAshState(submittedPlan.at("targetState")) != *result.analysis.selectedTarget || submittedPlan.at("selectedInverseRule") != Json(result.inverseRuleID)) throw std::invalid_argument("Submitted plan differs from independently calculated inverse operations");
        const auto& dependencies = submittedPlan.at("semanticDependencyReferences");
        if (dependencies.at("profileRef") != profileRef_ || dependencies.at("catalogRef") != catalogRef_ || dependencies.at("codewordSetRef") != codewordRef_ || dependencies.at("canonicalizationRuleID") != Json("JASH-CANON-EXACT-DECIMAL-V1")) throw std::invalid_argument("Submitted plan semantic dependency mismatch");
        if (dependencies.at("registryRef").at("registryID") != registry_.at("registryID") || dependencies.at("registryRef").at("revision") != registry_.at("revision") || dependencies.at("registryRef").at("proposedContentIdentity") != Json(sha256(selectFields(registry_, {"registryID", "revision", "entries"}).dump()))) throw std::invalid_argument("Submitted plan registry identity mismatch");
        if (dependencies.at("invariantBundleRef").at("bundleID") != input.at("applicableInvariants").at("bundleID") || dependencies.at("invariantBundleRef").at("revision") != input.at("applicableInvariants").at("revision") || dependencies.at("invariantBundleRef").at("proposedDefinitionHash") != input.at("applicableInvariants").at("proposedDefinitionHash")) throw std::invalid_argument("Submitted plan invariant identity mismatch");
        const auto& request = policy.at("evaluationRequest"); Json targetIdentity = Json::object(); targetIdentity["state"] = stateJson(*result.analysis.selectedTarget); targetIdentity["catalogRef"] = catalogRef_;
        if (request.at("recoveryCategory") != Json(to_string(result.analysis.recovery)) || request.at("planContentSha256") != Json(sha256(submittedPlan.dump())) || request.at("targetIdentity") != targetIdentity || request.at("targetIdentitySha256") != Json(sha256(targetIdentity.dump())) || request.at("rawBeforeSha256") != Json(result.rawPreimageHash) || request.at("canonicalBeforeSha256") != Json(result.canonicalPreimageHash) || request.at("inverseRuleID") != Json(result.inverseRuleID) || request.at("witnessID") != Json(result.analysis.witnessID)) throw std::invalid_argument("Authorization request does not bind actual plan/target/preimage/witness");
        Json pointers = Json::array(); for (const auto& operation : result.operations.array_items()) pointers.array_items().push_back(operation.at("pointer"));
        if (request.at("allowedPointersRequested") != pointers) throw std::invalid_argument("Authorization requested scope differs from actual operations");
        result.adapterTargetIdentity = request.at("adapterTargetIdentity").get<std::string>(); if (result.adapterTargetIdentity.empty() || submittedPlan.at("adapterTargetIdentity") != Json(result.adapterTargetIdentity)) throw std::invalid_argument("Missing/mismatched adapter target identity");
        bool allowed = false;
        for (const auto& rule : bundle.at("rules").array_items()) {
            if (rule.at("recoveryCategory") != Json(to_string(result.analysis.recovery)) || rule.at("inverseRuleID") != Json(result.inverseRuleID)) continue;
            if (parseAshState(rule.at("allowedTargetIdentity").at("state")) != *result.analysis.selectedTarget || rule.at("allowedTargetIdentity").at("catalogRef") != catalogRef_ || rule.at("allowedOperation") != Json("replace")) continue;
            if (rule.at("requireExactRawPreimage") != Json(true) || rule.at("requirePlanAndTargetIdentityMatch") != Json(true) || rule.at("requireSelectedValidatedWitness") != Json(true) || rule.at("requireVerifiedBackupBeforeWrite") != Json(true) || rule.at("requireGeneratedVerificationPlan") != Json(true) || rule.at("defaultForNoMatchingRule") != Json("BLOCK")) continue;
            bool scope = true; for (const auto& operation : result.operations.array_items()) { bool permitted = false; for (const auto& pointer : rule.at("allowedPointers").array_items()) if (pointer == operation.at("pointer")) permitted = true; scope = scope && permitted; }
            if (scope && !result.analysis.witnessID.empty()) allowed = true;
        }
        if (!allowed) return fault("POLICY", "BLOCKED", "No actual policy rule authorizes this target, witness and operation scope");
        result.policyDecisionEvidence["decision"] = "ALLOW"; result.policyDecisionEvidence["bundleHash"] = result.policyBundleHash; result.policyDecisionEvidence["requestHash"] = sha256(request.dump()); result.policyDecisionEvidence["runtimePlanHash"] = result.planHash; result.policyDecisionEvidence["rawPreimageHash"] = result.rawPreimageHash; result.policyDecisionEvidence["operations"] = result.operations;
        return result;
    } catch (const std::exception& error) { return fault(activePhase, "FAILED", error.what()); }
}
} // namespace Aeostara
