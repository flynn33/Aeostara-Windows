// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/BaseAdapterBindings.h"
#include "AeostaraCore/Diagnostics.h"
#include <map>
#include <mutex>

namespace Aeostara::BaseBindings {
struct CapturedDocument final {
    BaseContracts::JsonDocumentReference reference;
    std::string rawUtf8;
};

// Owns captured values and complete context; no reader, writer, callback, or
// service locator can yield mutation capability through this pure boundary.
class BaseDocumentBindings final : public IConfigAdapter,
    public IPlatformObservationAdapter, public ISemanticProjector {
public:
    BaseDocumentBindings(std::vector<CapturedDocument> documents,
        SemanticEnvironment environment, Json completeSemanticInput,
        Json actualRuleReferences, std::vector<std::string> actualSourceReferences);
    OperationAttempt<BaseContracts::JsonDocumentReference> canonicalize(const BaseContracts::JsonDocumentReference& documentReference) const override;
    OperationAttempt<ResolvedPointer> resolvePointer(const BaseContracts::JsonDocumentReference& documentReference, const BaseContracts::JsonPointer& pointer) const override;
    OperationAttempt<std::vector<BaseContracts::SurfaceDifference>> generateSurfaceDifferences(const BaseContracts::JsonDocumentReference& observed, const BaseContracts::JsonDocumentReference& desired) const override;
    OperationAttempt<BaseContracts::DiagnosticReference> validateMutationPreconditions(const BaseContracts::JsonDocumentReference& documentReference, const BaseContracts::JsonMutationOperation& operation) const override;
    OperationAttempt<BaseContracts::ObservedSystemState> observe(const BaseContracts::JsonDocumentReference& documentReference) const override;
    OperationAttempt<BaseContracts::MappingResult> project(const BaseContracts::ObservedSystemState& observedState, const BaseContracts::DesiredSystemIntent& desiredIntent, const BaseContracts::SemanticProjectionSpec& projectionSpec) const override;
    OperationAttempt<BaseContracts::DesiredSystemIntent> desiredIntent(const BaseContracts::JsonDocumentReference& documentReference, bool partialIntent) const;
    OperationAttempt<BaseContracts::SemanticProjectionSpec> projectionSpecification() const;
    std::vector<BaseContracts::DiagnosticChain> diagnosticChains() const;
    OperationAttempt<BaseContracts::DiagnosticChain> diagnosticChain(const BaseContracts::DiagnosticReference& reference) const;
private:
    struct Snapshot final { BaseContracts::JsonDocumentReference reference; std::string rawUtf8; Json document; };
    const Snapshot& resolve(const BaseContracts::JsonDocumentReference& reference) const;
    DiagnosticBundle diagnostic(const std::string& kind, const std::string& severity,
        const std::string& message, const Json& evidence) const;
    OperationFailure failure(OperationPhase phase, OperationCode code,
        const std::string& message, const Json& evidence = Json::object()) const;
    void validateSuppliedDiagnostic(const Json& reference) const;
    std::vector<Snapshot> documents_;
    SemanticEnvironment environment_;
    Json completeSemanticInput_, rules_;
    std::vector<std::string> sources_;
    mutable std::mutex diagnosticsMutex_;
    mutable std::map<std::string, BaseContracts::DiagnosticChain, UnsignedUtf8Less> chains_;
    mutable std::size_t chainBytes_ = 0;
};
} // namespace Aeostara::BaseBindings
