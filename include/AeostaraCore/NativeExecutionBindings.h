// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/BaseAdapterBindings.h"
#include "AeostaraCore/BasePolicyGate.h"
#include "AeostaraCore/BaseAuditBindings.h"
#include "AeostaraCore/AtomicPersistenceContext.h"
#include "AeostaraCore/IConfigAdapter.h"
#include "AeostaraCore/IBackupProvider.h"
#include "AeostaraCore/IFileSystem.h"
#include <memory>
#include <mutex>
#include <optional>
#include <thread>

namespace Aeostara {
struct NativePlanningResult final {
    SemanticPlan semanticPlan;
    BaseContracts::RecoveryPlan recoveryPlan;
    BaseContracts::PolicyBundle policyBundle;
    BaseContracts::VerificationPlan verificationPlan;
    BaseContracts::DiagnosticChain diagnosticChain;
};
struct NativeExecutionOutcome final {
    bool success = false;
    std::optional<SemanticPlan> semanticPlan;
    std::optional<OperationFailure> failure;
    std::optional<std::string> rawHashBefore, rawHashAfter, backupPath;
    MutationState mutationState = MutationState::NotStarted;
    std::size_t completedOperationCount = 0;
    std::vector<OperationFault> diagnosticErrors;
    std::uint64_t diagnosticErrorsDropped = 0;
    DiagnosticHealth diagnosticHealth;
    Json diagnosticChain;
    Json auditChain;
};

// One lease and one authority serve full lifecycle and source phase interfaces.
// Captured planning has no host calls. Execution must be explicitly prepared;
// this native bootstrap resolves frozen ExecutionContext's required BackupResult
// from a real verified backup before a complete frozen context can exist.
class NativeExecutionSession final {
public:
    NativeExecutionSession(SemanticEnvironment environment, Json completeInput,
        std::string targetPath, std::string actualCreatedAt,
        std::shared_ptr<Aeostara::IFileSystem> filesystem,
        std::shared_ptr<Aeostara::IConfigAdapter> adapter,
        std::shared_ptr<Aeostara::IBackupProvider> backup,
        std::shared_ptr<BaseBindings::BaseAuditSink> audit,
        std::shared_ptr<IDiagnostics> diagnostics = {},
        std::shared_ptr<BaseBindings::IPolicyGate> hostPolicy = {},
        std::shared_ptr<IDiagnosticClock> clock = {},
        bool retainPersistenceCopySource = false);
    ~NativeExecutionSession();
    NativeExecutionSession(const NativeExecutionSession&) = delete;
    NativeExecutionSession& operator=(const NativeExecutionSession&) = delete;
    OperationAttempt<NativePlanningResult> planCaptured() const;
    OperationAttempt<BaseContracts::ExecutionContext> prepareExecution();
    OperationAttempt<BaseContracts::BackupResult> prepareBackup(const BaseContracts::ExecutionContext&, const BaseContracts::RecoveryPlan&);
    OperationAttempt<BaseContracts::ExecutionStepResult> executeStep(const BaseContracts::ExecutionContext&, const BaseContracts::RecoveryStep&);
    OperationAttempt<BaseContracts::ExecutionStepResult> applyMutation(const BaseContracts::ExecutionContext&, const BaseContracts::JsonMutationOperation&);
    OperationAttempt<BaseContracts::VerificationResult> verify(const BaseContracts::ExecutionContext&, const BaseContracts::VerificationPlan&) const;
    OperationAttempt<BaseContracts::RollbackResult> executeRollback(const BaseContracts::ExecutionContext&, const BaseContracts::RollbackPlan&);
    OperationAttempt<BaseContracts::RollbackPlan> rollbackPlan() const;
    OperationAttempt<BaseContracts::HealResult> complete(const BaseContracts::ExecutionContext&);
    OperationAttempt<std::shared_ptr<const BaseBindings::AtomicPersistenceContext>> authorizeAtomicPersistence(
        const BaseContracts::ExecutionContext&, BaseBindings::NativePersistencePurpose, BaseBindings::AtomicOperation) const;
    NativeExecutionOutcome run();
    NativeExecutionOutcome outcome() const;
    const Json& capturedInput() const noexcept { return input_; }
    const Json& capturedProfile() const noexcept { return environment_.profile(); }
private:
    struct State;
    std::unique_ptr<State> state_;
    SemanticEnvironment environment_;
    Json input_;
    std::string path_, createdAt_;
    std::shared_ptr<Aeostara::IFileSystem> fs_;
    std::shared_ptr<Aeostara::IConfigAdapter> adapter_;
    std::shared_ptr<Aeostara::IBackupProvider> backup_;
    std::shared_ptr<BaseBindings::BaseAuditSink> audit_;
    std::shared_ptr<IDiagnostics> diagnostics_;
    std::shared_ptr<BaseBindings::IPolicyGate> hostPolicy_;
    std::shared_ptr<IDiagnosticClock> clock_;
    bool retainPersistenceCopySource_;
    mutable std::recursive_mutex mutex_;
    DiagnosticBundle diagnostic(OperationPhase, const std::string&, const Json&, bool failed = false, OperationCode code = OperationCode::Failed) const;
    void auditPhase(const std::string&, const DiagnosticBundle&, const Json&) const;
    void checkContext(const BaseContracts::ExecutionContext&) const;
    void checkTarget(bool originalRequired) const;
    OperationFailure reject(OperationPhase, OperationCode, const std::string&, const Json& = Json::object()) const;
    OperationFailure failAndRestore(OperationPhase, OperationCode, const std::string&) const;
    OperationFailure failAndRestore(OperationFailure) const;
    void restoreAuthorized(OperationFailure&) const;
    void captureTelemetry(OperationPhase, const DiagnosticEvent&) const noexcept;
    void attachTelemetryErrors(OperationFailure&) const;
    OperationAttempt<BaseContracts::ExecutionStepResult> executeOperations(const BaseContracts::ExecutionContext&, const Json& frozenOperations, const std::string& stepID);
};
} // namespace Aeostara

namespace Aeostara::BaseBindings {
// Source DTOs are bound to complete immutable captured values, not accepted as
// execution authority. All five boundaries share the same session and receipts.
class NativeExecutionBindings final : public IBackupProvider, public IExecutionAdapter,
    public IPlatformMutationAdapter, public IVerificationAdapter, public IHealingEngine {
public:
    NativeExecutionBindings(std::shared_ptr<NativeExecutionSession> session,
        BaseContracts::ObservedSystemState observed, BaseContracts::DesiredSystemIntent desired,
        BaseContracts::SemanticProjectionSpec projection,
        std::vector<BaseContracts::DiagnosticChain> capturedDiagnosticChains);
    OperationAttempt<BaseContracts::ExecutionContext> prepareExecution();
    OperationAttempt<BaseContracts::BackupResult> prepareBackup(const BaseContracts::ExecutionContext&, const BaseContracts::RecoveryPlan&) override;
    OperationAttempt<BaseContracts::RollbackResult> executeRollback(const BaseContracts::ExecutionContext&, const BaseContracts::RollbackPlan&) override;
    OperationAttempt<BaseContracts::ExecutionStepResult> executeStep(const BaseContracts::ExecutionContext&, const BaseContracts::RecoveryStep&) override;
    OperationAttempt<BaseContracts::ExecutionStepResult> applyMutation(const BaseContracts::ExecutionContext&, const BaseContracts::JsonMutationOperation&) override;
    OperationAttempt<BaseContracts::VerificationResult> verify(const BaseContracts::ExecutionContext&, const BaseContracts::VerificationPlan&) const override;
    OperationAttempt<BaseContracts::DryRunResult> dryRun(const BaseContracts::ObservedSystemState&, const BaseContracts::DesiredSystemIntent&, const BaseContracts::SemanticProjectionSpec&, const BaseContracts::PolicyBundle&) const override;
    OperationAttempt<BaseContracts::HealResult> heal(const BaseContracts::ObservedSystemState&, const BaseContracts::DesiredSystemIntent&, const BaseContracts::SemanticProjectionSpec&, const BaseContracts::PolicyBundle&, const BaseContracts::ExecutionContext&) override;
private:
    void checkInputs(const BaseContracts::ObservedSystemState&, const BaseContracts::DesiredSystemIntent&, const BaseContracts::SemanticProjectionSpec&, const BaseContracts::PolicyBundle&, const NativePlanningResult&) const;
    std::shared_ptr<NativeExecutionSession> session_;
    BaseContracts::ObservedSystemState observed_;
    BaseContracts::DesiredSystemIntent desired_;
    BaseContracts::SemanticProjectionSpec projection_;
};
} // namespace Aeostara::BaseBindings
