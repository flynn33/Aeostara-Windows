// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/Json.h"
#include <array>
#include <exception>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace Aeostara {
using AshState = std::array<unsigned char, 9>;
enum class MappingStatus { Mapped, Ambiguous, Blocked, Failed };
enum class SystemClass { Stable, Unstable, Correctable, Degraded, Contained, Failed, SafeHalt };
enum class RecoveryCategory { NoAction, NormalizeState, ApplyCorrection, FallbackRequired, ContainmentRequired, EscalationRequired, TerminalNoRecovery };
enum class OperationPhase { Parse, Normalize, Project, Diagnose, Classify, Plan, Policy, Backup,
    Execute, Verify, Rollback, Fallback, Contain, Halt, Audit, Export };
enum class OperationCode { Ambiguous, Blocked, Failed, InvalidInput, UnsupportedEvidence,
    StaleState, PolicyRejected, StorageFailure, VerificationFailed, RestorationFailed,
    DiagnosticUnavailable, InternalFailure };
enum class MutationState { NotStarted, MayHaveMutated, Mutated, VerifiedRestored, RestoreFailed, Unknown };
enum class DiagnosticPersistence { NotRequested, Persisted, Failed, Unavailable, Unknown };
struct UnavailableEvidence final { std::string field; std::string reason; };
struct OperationFault final { OperationPhase phase; OperationCode code; std::string message; Json evidence = Json::object(); };
struct OperationFailure final {
    OperationPhase phase;
    OperationCode code;
    std::string message;
    Json evidence = Json::object();
    std::optional<MappingStatus> mappingStatus;
    std::vector<UnavailableEvidence> unavailableEvidence;
    MutationState mutationState = MutationState::NotStarted;
    OperationFault primaryFailure;
    std::vector<OperationFault> secondaryFailures;
    Json diagnosticChain; // null means genuinely unavailable, never an invented envelope
    DiagnosticPersistence diagnosticPersistence = DiagnosticPersistence::Unavailable;
    OperationFailure(OperationPhase actualPhase, OperationCode actualCode, std::string actualMessage,
                     Json availableEvidence = Json::object());
    bool mutationMayHaveOccurred() const noexcept;
    void addSecondary(OperationPhase actualPhase, OperationCode actualCode, std::string actualMessage,
                      Json availableEvidence = Json::object());
};
OperationPhase operationPhase(std::string_view sourcePhase);
OperationCode operationCode(std::string_view sourceCode);
std::string to_string(OperationPhase value);
std::string to_string(OperationCode value);
std::string to_string(MutationState value);
std::string to_string(DiagnosticPersistence value);
Json operationFailureJson(const OperationFailure& failure); // checked adopted outer-channel serialization
OperationFailure operationFailureFromException(OperationPhase phase, OperationCode code,
    const std::exception& error, Json availableEvidence = Json::object());
// Streaming canonical identities for explicit captured document/raw regions.
// Per-document limits remain independent of the complete-input aggregate.
std::string hashCompleteSemanticInput(const Json& completeInput);
std::string hashCapturedSnapshot(const Json& capturedSnapshot);
void validateCompleteSemanticInput(const Json& completeInput);
template<class T> using OperationAttempt = std::variant<T, OperationFailure>;
struct DimensionEvidence final {
    std::string coordinate;
    MappingStatus status = MappingStatus::Blocked;
    std::optional<unsigned char> bit;
    Json sources = Json::array();
    std::vector<std::string> faults;
};
struct Projection final {
    AshState state{};
    std::array<DimensionEvidence, 9> dimensions;
    std::string completeSemanticInputHash;
    Json diagnosticChain;
};
struct StateDiagnostic final {
    AshState state{};
    std::string admissibility;
    std::string transformationCompatibility;
    std::string normalizationStatus;
    std::string recoverabilityRelevance;
    bool valid = false;
    std::vector<AshState> orbit;
    std::vector<std::string> ruleIDs;
    Json diagnosticChain;
};
struct SemanticAnalysis final {
    Projection projection;
    StateDiagnostic diagnostic;
    SystemClass stateClass = SystemClass::Failed;
    RecoveryCategory recovery = RecoveryCategory::EscalationRequired;
    std::optional<AshState> selectedTarget;
    std::optional<std::string> selectedFallbackID;
    std::string witnessID;
    Json diagnosticChain;
};
struct SemanticPlan final {
    SemanticAnalysis analysis;
    Json operations = Json::array();
    Json expectedDocument;
    std::string rawPreimageHash;
    std::string canonicalPreimageHash;
    std::string semanticDependencyHash;
    std::string planHash;
    std::string policyBundleHash;
    std::string inverseRuleID;
    std::string adapterTargetIdentity;
    Json policyDecisionEvidence = Json::object();
};
struct InvariantEvaluation final {
    bool passed = false;
    bool available = true;
    Json results = Json::array();
};
// Copies immutable authority documents; pure evaluation never owns a writer.
class SemanticEnvironment final {
public:
    SemanticEnvironment(std::string profileBytes, std::string catalogBytes,
                        std::string registryBytes, std::string sourceAuthorityBytes);
    static SemanticEnvironment adoptedDirect();
    static SemanticEnvironment adoptedClaims();
    const Json& profile() const noexcept { return profile_; }
    const Json& catalog() const noexcept { return catalog_; }
    const Json& registry() const noexcept { return registry_; }
    const Json& sourceAuthority() const noexcept { return sourceAuthority_; }
    OperationAttempt<Projection> project(const Json& completeInput) const;
    StateDiagnostic diagnose(const AshState& state) const;
    OperationAttempt<SemanticAnalysis> analyze(const Json& completeInput) const;
    OperationAttempt<SemanticPlan> plan(const Json& completeInput) const;
    InvariantEvaluation evaluateInvariants(const Json& completeInput, const Json& actualDocument,
                                          std::string_view role, const Json& operations) const;
private:
    std::size_t authorityInputBytes_ = 0;
    Json profile_, catalog_, registry_, sourceAuthority_;
    std::string profileRawHash_, catalogRawHash_, registryRawHash_, sourceAuthorityRawHash_;
    Json profileRef_, catalogRef_, codewordRef_, sourceAuthorityRef_;
};
std::string sha256(std::string_view bytes);
std::string to_string(MappingStatus value);
std::string to_string(SystemClass value);
std::string to_string(RecoveryCategory value);
RecoveryCategory recoveryForClass(SystemClass value);
RecoveryCategory blockedRecoveryTransition(RecoveryCategory current, std::string_view actualCondition);
SystemClass classifySystemState(const StateDiagnostic& diagnostic, bool safeHalt, bool contained,
                               bool correctionPathKnown, bool fallbackAvailable);
Json stateJson(const AshState& state);
AshState parseAshState(const Json& value);
const std::array<AshState, 16>& canonicalCodewords();
} // namespace Aeostara
