// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual
#pragma once
#include "AeostaraCore/Json.h"
#include <string_view>

namespace Aeostara::BaseContracts {
enum class ContractKind {
    AdmissibilityResult,
    ArtifactEmissionPlan,
    AshSemanticState,
    AshState,
    AuditChain,
    AuditEvent,
    AxiomDiagnostic,
    BackupRecord,
    BackupResult,
    CanonicalCodewordSet,
    CodewordTransformation,
    ContainmentDecision,
    DesiredState,
    DesiredSystemIntent,
    DiagnosticChain,
    DiagnosticEnvelope,
    DiagnosticReference,
    DriftEvent,
    DryRunResult,
    EncodedState,
    EscalationDecision,
    ExecutionContext,
    ExecutionStepResult,
    FallbackDecision,
    GenerationPlan,
    HealResult,
    Invariant,
    JsonCanonicalizationRule,
    JsonDocumentReference,
    JsonMutationOperation,
    JsonPointer,
    MappingAmbiguity,
    MappingResult,
    ModuleManifest,
    ObservedState,
    ObservedSystemState,
    PolicyBundle,
    PolicyDecision,
    RealmIdentity,
    RecoveryCategory,
    RecoveryOutcome,
    RecoveryPlan,
    RecoveryStep,
    RepairAction,
    RepairPlan,
    RollbackPlan,
    RollbackResult,
    RuleReference,
    SafeHaltDecision,
    SemanticDimensionBinding,
    SemanticProjectionSpec,
    StateValidityDiagnostic,
    SurfaceDifference,
    SystemStateClass,
    TopologyPlan,
    TransitionDefinition,
    TransitionResult,
    VerificationPlan,
    VerificationResult,
    Count
};
enum class ContractViolation { Type, Required, AdditionalProperty, Enum, Constant, Pattern, MinimumItems, MaximumItems };
class ContractError final : public std::runtime_error {
public:
    ContractError(ContractKind kind, ContractViolation violation, std::string pointer, std::string message);
    ContractKind kind() const noexcept { return kind_; }
    ContractViolation violation() const noexcept { return violation_; }
    const std::string& pointer() const noexcept { return pointer_; }
private:
    ContractKind kind_;
    ContractViolation violation_;
    std::string pointer_;
};

std::string_view contract_name(ContractKind kind);
void validate_contract(ContractKind kind, const Json& value);

// Structural validation preserves frozen fields. Semantic algorithms must additionally
// establish real evidence, identities, relationships, and the operation's preconditions.
template<ContractKind Kind> class ContractValue final {
public:
    explicit ContractValue(Json value) : value_(std::move(value)) { validate_contract(Kind, value_); }
    static ContractValue from_json_value(const Json& value) { return ContractValue(value); }
    const Json& value() const noexcept { return value_; }
    static constexpr ContractKind kind = Kind;
    bool operator==(const ContractValue&) const = default;
private:
    Json value_;
};
template<ContractKind Kind> void to_json(Json& value, const ContractValue<Kind>& contract) { value = contract.value(); }
template<ContractKind Kind> void from_json(const Json& value, ContractValue<Kind>& contract) { contract = ContractValue<Kind>(value); }
using AdmissibilityResult = ContractValue<ContractKind::AdmissibilityResult>;
using ArtifactEmissionPlan = ContractValue<ContractKind::ArtifactEmissionPlan>;
using AshSemanticState = ContractValue<ContractKind::AshSemanticState>;
using AshState = ContractValue<ContractKind::AshState>;
using AuditChain = ContractValue<ContractKind::AuditChain>;
using AuditEvent = ContractValue<ContractKind::AuditEvent>;
using AxiomDiagnostic = ContractValue<ContractKind::AxiomDiagnostic>;
using BackupRecord = ContractValue<ContractKind::BackupRecord>;
using BackupResult = ContractValue<ContractKind::BackupResult>;
using CanonicalCodewordSet = ContractValue<ContractKind::CanonicalCodewordSet>;
using CodewordTransformation = ContractValue<ContractKind::CodewordTransformation>;
using ContainmentDecision = ContractValue<ContractKind::ContainmentDecision>;
using DesiredState = ContractValue<ContractKind::DesiredState>;
using DesiredSystemIntent = ContractValue<ContractKind::DesiredSystemIntent>;
using DiagnosticChain = ContractValue<ContractKind::DiagnosticChain>;
using DiagnosticEnvelope = ContractValue<ContractKind::DiagnosticEnvelope>;
using DiagnosticReference = ContractValue<ContractKind::DiagnosticReference>;
using DriftEvent = ContractValue<ContractKind::DriftEvent>;
using DryRunResult = ContractValue<ContractKind::DryRunResult>;
using EncodedState = ContractValue<ContractKind::EncodedState>;
using EscalationDecision = ContractValue<ContractKind::EscalationDecision>;
using ExecutionContext = ContractValue<ContractKind::ExecutionContext>;
using ExecutionStepResult = ContractValue<ContractKind::ExecutionStepResult>;
using FallbackDecision = ContractValue<ContractKind::FallbackDecision>;
using GenerationPlan = ContractValue<ContractKind::GenerationPlan>;
using HealResult = ContractValue<ContractKind::HealResult>;
using Invariant = ContractValue<ContractKind::Invariant>;
using JsonCanonicalizationRule = ContractValue<ContractKind::JsonCanonicalizationRule>;
using JsonDocumentReference = ContractValue<ContractKind::JsonDocumentReference>;
using JsonMutationOperation = ContractValue<ContractKind::JsonMutationOperation>;
using JsonPointer = ContractValue<ContractKind::JsonPointer>;
using MappingAmbiguity = ContractValue<ContractKind::MappingAmbiguity>;
using MappingResult = ContractValue<ContractKind::MappingResult>;
using ModuleManifest = ContractValue<ContractKind::ModuleManifest>;
using ObservedState = ContractValue<ContractKind::ObservedState>;
using ObservedSystemState = ContractValue<ContractKind::ObservedSystemState>;
using PolicyBundle = ContractValue<ContractKind::PolicyBundle>;
using PolicyDecision = ContractValue<ContractKind::PolicyDecision>;
using RealmIdentity = ContractValue<ContractKind::RealmIdentity>;
using RecoveryCategory = ContractValue<ContractKind::RecoveryCategory>;
using RecoveryOutcome = ContractValue<ContractKind::RecoveryOutcome>;
using RecoveryPlan = ContractValue<ContractKind::RecoveryPlan>;
using RecoveryStep = ContractValue<ContractKind::RecoveryStep>;
using RepairAction = ContractValue<ContractKind::RepairAction>;
using RepairPlan = ContractValue<ContractKind::RepairPlan>;
using RollbackPlan = ContractValue<ContractKind::RollbackPlan>;
using RollbackResult = ContractValue<ContractKind::RollbackResult>;
using RuleReference = ContractValue<ContractKind::RuleReference>;
using SafeHaltDecision = ContractValue<ContractKind::SafeHaltDecision>;
using SemanticDimensionBinding = ContractValue<ContractKind::SemanticDimensionBinding>;
using SemanticProjectionSpec = ContractValue<ContractKind::SemanticProjectionSpec>;
using StateValidityDiagnostic = ContractValue<ContractKind::StateValidityDiagnostic>;
using SurfaceDifference = ContractValue<ContractKind::SurfaceDifference>;
using SystemStateClass = ContractValue<ContractKind::SystemStateClass>;
using TopologyPlan = ContractValue<ContractKind::TopologyPlan>;
using TransitionDefinition = ContractValue<ContractKind::TransitionDefinition>;
using TransitionResult = ContractValue<ContractKind::TransitionResult>;
using VerificationPlan = ContractValue<ContractKind::VerificationPlan>;
using VerificationResult = ContractValue<ContractKind::VerificationResult>;
} // namespace Aeostara::BaseContracts
