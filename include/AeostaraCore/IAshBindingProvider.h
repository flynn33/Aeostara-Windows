// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/BaseContracts.h"
#include "AeostaraCore/SemanticMapping.h"
#include "AeostaraCore/Diagnostics.h"
#include <map>
#include <mutex>
#include <vector>
namespace Aeostara {
// All DTO value branches validate their exact frozen schema; missing semantic
// context uses OperationFailure and never a fabricated schema-valid value.
class IAshBindingProvider {
public:
    virtual ~IAshBindingProvider() = default;
    virtual OperationAttempt<BaseContracts::AdmissibilityResult> evaluateAdmissibility(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<BaseContracts::StateValidityDiagnostic> diagnoseState(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<BaseContracts::SystemStateClass> classifyState(const BaseContracts::StateValidityDiagnostic& diagnostic) const = 0;
    virtual OperationAttempt<BaseContracts::TransitionResult> applyCodewordTransformation(const BaseContracts::CodewordTransformation& transformation) const = 0;
    virtual OperationAttempt<BaseContracts::RealmIdentity> resolveRealmIdentity(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<BaseContracts::TransitionResult> resolveTransition(const BaseContracts::AshState& state, const BaseContracts::TransitionDefinition& definition) const = 0;
    virtual OperationAttempt<BaseContracts::TopologyPlan> generateTopology(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<std::vector<BaseContracts::AxiomDiagnostic>> evaluateAxioms(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<BaseContracts::GenerationPlan> generateMaterializationPlan(const BaseContracts::AshState& state) const = 0;
    virtual OperationAttempt<BaseContracts::ArtifactEmissionPlan> planArtifactEmission(const BaseContracts::GenerationPlan& plan) const = 0;
    // SDK diagnostic transport extension: successful frozen DTO references
    // resolve to actual retained chains. Retention is bounded, without eviction;
    // operations requiring a new chain fail when 64 chains/16 MiB is exhausted.
    virtual OperationAttempt<BaseContracts::DiagnosticChain> resolveDiagnosticChain(const BaseContracts::DiagnosticReference& reference) const = 0;
};
// Immutable bindings contain actual topology depth/seed, axiom subject and
// source/state identity, transition applicability/registry, explicit generation
// request/profile/roles/artifacts/naming/target facts and resolvable diagnostics.
class AshBindingProvider final : public IAshBindingProvider {
public:
    AshBindingProvider(SemanticEnvironment environment, Json immutableBindings);
    OperationAttempt<BaseContracts::AdmissibilityResult> evaluateAdmissibility(const BaseContracts::AshState& state) const override;
    OperationAttempt<BaseContracts::StateValidityDiagnostic> diagnoseState(const BaseContracts::AshState& state) const override;
    OperationAttempt<BaseContracts::SystemStateClass> classifyState(const BaseContracts::StateValidityDiagnostic& diagnostic) const override;
    OperationAttempt<BaseContracts::TransitionResult> applyCodewordTransformation(const BaseContracts::CodewordTransformation& transformation) const override;
    OperationAttempt<BaseContracts::RealmIdentity> resolveRealmIdentity(const BaseContracts::AshState& state) const override;
    OperationAttempt<BaseContracts::TransitionResult> resolveTransition(const BaseContracts::AshState& state, const BaseContracts::TransitionDefinition& definition) const override;
    OperationAttempt<BaseContracts::TopologyPlan> generateTopology(const BaseContracts::AshState& state) const override;
    OperationAttempt<std::vector<BaseContracts::AxiomDiagnostic>> evaluateAxioms(const BaseContracts::AshState& state) const override;
    OperationAttempt<BaseContracts::GenerationPlan> generateMaterializationPlan(const BaseContracts::AshState& state) const override;
    OperationAttempt<BaseContracts::ArtifactEmissionPlan> planArtifactEmission(const BaseContracts::GenerationPlan& plan) const override;
    OperationAttempt<BaseContracts::DiagnosticChain> resolveDiagnosticChain(const BaseContracts::DiagnosticReference& reference) const override;
private:
    DiagnosticBundle diagnostic(const Json& evidence, const std::string& source,
        const char* id = "ASH-STATE-STRUCTURE-001", const char* family = "ASH-STATE",
        const std::string& explanation = "Full nine-coordinate state structure evaluated") const;
    void retainDiagnosticChain(const Json& chain) const;
    SemanticEnvironment environment_;
    Json bindings_;
    mutable std::mutex diagnosticMutex_;
    mutable std::map<std::string, Json> diagnosticChains_;
    mutable std::size_t diagnosticBytes_ = 0;
};
} // namespace Aeostara
