// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once

#include "AeostaraCore/BaseContracts.h"
#include "AeostaraCore/IAshBindingProvider.h"
#include "AeostaraCore/SemanticMapping.h"
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace Aeostara::BaseBindings {

// PATH and BYTE_SEQUENCE are native ownership bindings, not frozen JSON DTOs.
// Paths are UTF-8; native Windows adapters perform checked UTF-16 conversion.
using Path = std::string;
using ByteSequence = std::vector<std::byte>;
struct Absent final { bool operator==(const Absent&) const = default; };
struct ResolvedPointer final {
    BaseContracts::JsonPointer resolvedPointer;
    std::variant<Json, Absent> value;
};
// A completed file operation has genuine diagnostics. Failures and uncertain
// completion use OperationFailure rather than a manufactured completed receipt.
struct FileOperationReceipt final {
    std::string operationID;
    std::size_t bytesTransferred;
    BaseContracts::DiagnosticReference diagnosticReference;
};

// The ASH boundary is defined once with all ten frozen operations.
using IAshBindingProvider = Aeostara::IAshBindingProvider;

class IAuditSink {
public:
    virtual ~IAuditSink() = default;
    virtual OperationAttempt<BaseContracts::AuditChain> appendEvent(const BaseContracts::AuditChain& chain, const BaseContracts::AuditEvent& event) = 0;
    virtual OperationAttempt<BaseContracts::DiagnosticReference> persistChain(const BaseContracts::AuditChain& chain) = 0;
    virtual OperationAttempt<BaseContracts::AuditChain> readChain(const std::string& auditChainID) const = 0;
};

class IBackupProvider {
public:
    virtual ~IBackupProvider() = default;
    virtual OperationAttempt<BaseContracts::BackupResult> prepareBackup(const BaseContracts::ExecutionContext& context, const BaseContracts::RecoveryPlan& plan) = 0;
    virtual OperationAttempt<BaseContracts::RollbackResult> executeRollback(const BaseContracts::ExecutionContext& context, const BaseContracts::RollbackPlan& plan) = 0;
};

class IConfigAdapter {
public:
    virtual ~IConfigAdapter() = default;
    virtual OperationAttempt<BaseContracts::JsonDocumentReference> canonicalize(const BaseContracts::JsonDocumentReference& documentReference) const = 0;
    virtual OperationAttempt<ResolvedPointer> resolvePointer(const BaseContracts::JsonDocumentReference& documentReference, const BaseContracts::JsonPointer& pointer) const = 0;
    virtual OperationAttempt<std::vector<BaseContracts::SurfaceDifference>> generateSurfaceDifferences(const BaseContracts::JsonDocumentReference& observed, const BaseContracts::JsonDocumentReference& desired) const = 0;
    virtual OperationAttempt<BaseContracts::DiagnosticReference> validateMutationPreconditions(const BaseContracts::JsonDocumentReference& documentReference, const BaseContracts::JsonMutationOperation& operation) const = 0;
};

class IExecutionAdapter {
public:
    virtual ~IExecutionAdapter() = default;
    virtual OperationAttempt<BaseContracts::ExecutionStepResult> executeStep(const BaseContracts::ExecutionContext& context, const BaseContracts::RecoveryStep& step) = 0;
};

class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual OperationAttempt<ByteSequence> read(const Path& path) const = 0;
    virtual OperationAttempt<FileOperationReceipt> writeAtomically(const Path& path, const ByteSequence& bytes) = 0;
    virtual OperationAttempt<FileOperationReceipt> appendAtomically(const Path& path, const ByteSequence& bytes) = 0;
    virtual OperationAttempt<FileOperationReceipt> copyAtomically(const Path& source, const Path& destination) = 0;
    virtual OperationAttempt<bool> exists(const Path& path) const = 0;
    virtual OperationAttempt<std::string> computeSHA256(const Path& path) const = 0;
};

class IHealingEngine {
public:
    virtual ~IHealingEngine() = default;
    virtual OperationAttempt<BaseContracts::DryRunResult> dryRun(const BaseContracts::ObservedSystemState& observedState, const BaseContracts::DesiredSystemIntent& desiredIntent, const BaseContracts::SemanticProjectionSpec& projectionSpec, const BaseContracts::PolicyBundle& policyBundle) const = 0;
    virtual OperationAttempt<BaseContracts::HealResult> heal(const BaseContracts::ObservedSystemState& observedState, const BaseContracts::DesiredSystemIntent& desiredIntent, const BaseContracts::SemanticProjectionSpec& projectionSpec, const BaseContracts::PolicyBundle& policyBundle, const BaseContracts::ExecutionContext& executionContext) = 0;
};

class IPlatformMutationAdapter {
public:
    virtual ~IPlatformMutationAdapter() = default;
    virtual OperationAttempt<BaseContracts::ExecutionStepResult> applyMutation(const BaseContracts::ExecutionContext& context, const BaseContracts::JsonMutationOperation& operation) = 0;
};

class IPlatformObservationAdapter {
public:
    virtual ~IPlatformObservationAdapter() = default;
    virtual OperationAttempt<BaseContracts::ObservedSystemState> observe(const BaseContracts::JsonDocumentReference& documentReference) const = 0;
};

class IPolicyGate {
public:
    virtual ~IPolicyGate() = default;
    virtual OperationAttempt<BaseContracts::PolicyDecision> evaluate(const BaseContracts::RecoveryPlan& plan, const BaseContracts::PolicyBundle& bundle) const = 0;
};

class ISemanticProjector {
public:
    virtual ~ISemanticProjector() = default;
    virtual OperationAttempt<BaseContracts::MappingResult> project(const BaseContracts::ObservedSystemState& observedState, const BaseContracts::DesiredSystemIntent& desiredIntent, const BaseContracts::SemanticProjectionSpec& projectionSpec) const = 0;
};

class IVerificationAdapter {
public:
    virtual ~IVerificationAdapter() = default;
    virtual OperationAttempt<BaseContracts::VerificationResult> verify(const BaseContracts::ExecutionContext& context, const BaseContracts::VerificationPlan& plan) const = 0;
};

} // namespace Aeostara::BaseBindings
