// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/BaseAdapterBindings.h"
#include <memory>
#include <optional>
namespace Aeostara { class NativeExecutionSession; }
namespace Aeostara::BaseBindings {
enum class NativePersistencePurpose { BackupSnapshot, AuditSnapshot };
enum class AtomicOperation { Write, Append, Copy };

// Only a completed measured native lifecycle can issue this exact persistence
// capability. It does not authorize replacing the protected configuration.
class AtomicPersistenceContext final {
public:
    NativePersistencePurpose purpose() const noexcept { return purpose_; }
    AtomicOperation operation() const noexcept { return operation_; }
    const Path& sourcePath() const noexcept { return sourcePath_; }
    const Path& targetPath() const noexcept { return targetPath_; }
    const std::string& sourceIdentity() const noexcept { return sourceIdentity_; }
    const std::string& targetIdentity() const noexcept { return targetIdentity_; }
    const std::string& expectedPreimage() const noexcept { return preimage_; }
    const std::string& operationInput() const noexcept { return operationInput_; }
    const std::string& exactResult() const noexcept { return result_; }
    const Path& carrierPath() const noexcept { return carrierPath_; }
    const std::string& carrierIdentity() const noexcept { return carrierIdentity_; }
    const std::string& carrierRawHash() const noexcept { return carrierRawHash_; }
    const Path& backupPath() const noexcept { return backupPath_; }
    const std::string& backupIdentity() const noexcept { return backupIdentity_; }
    const std::string& backupRawHash() const noexcept { return backupRawHash_; }
    const Path& auditPath() const noexcept { return auditPath_; }
    const std::string& auditIdentity() const noexcept { return auditIdentity_; }
    const BaseContracts::RecoveryPlan& plan() const noexcept { return plan_; }
    const BaseContracts::PolicyDecision& policy() const noexcept { return policy_; }
    const BaseContracts::BackupResult& backup() const noexcept { return backup_; }
    const BaseContracts::ExecutionContext& executionContext() const noexcept { return context_; }
    const BaseContracts::DiagnosticChain& diagnosticChain() const noexcept { return chain_; }
    const BaseContracts::AuditChain& auditChain() const noexcept { return auditChain_; }
    const std::optional<BaseContracts::BackupRecord>& copySourceRecord() const noexcept { return copySourceRecord_; }
private:
    friend class Aeostara::NativeExecutionSession;
    AtomicPersistenceContext(NativePersistencePurpose purpose, AtomicOperation operation,
        Path sourcePath, Path targetPath, std::string sourceIdentity, std::string targetIdentity,
        std::string preimage, std::string operationInput, std::string exactResult,
        Path carrierPath, std::string carrierIdentity, std::string carrierRawHash,
        Path backupPath, std::string backupIdentity, std::string backupRawHash,
        Path auditPath, std::string auditIdentity,
        BaseContracts::RecoveryPlan plan, BaseContracts::PolicyDecision policy,
        BaseContracts::BackupResult backup, BaseContracts::ExecutionContext context,
        BaseContracts::DiagnosticChain chain, BaseContracts::AuditChain auditChain,
        std::optional<BaseContracts::BackupRecord> copySourceRecord = {})
        : purpose_(purpose), operation_(operation), sourcePath_(std::move(sourcePath)),
          targetPath_(std::move(targetPath)), sourceIdentity_(std::move(sourceIdentity)),
          targetIdentity_(std::move(targetIdentity)), preimage_(std::move(preimage)),
          operationInput_(std::move(operationInput)), result_(std::move(exactResult)),
          carrierPath_(std::move(carrierPath)), carrierIdentity_(std::move(carrierIdentity)),
          carrierRawHash_(std::move(carrierRawHash)), backupPath_(std::move(backupPath)),
          backupIdentity_(std::move(backupIdentity)), backupRawHash_(std::move(backupRawHash)),
          auditPath_(std::move(auditPath)), auditIdentity_(std::move(auditIdentity)),
          plan_(std::move(plan)), policy_(std::move(policy)), backup_(std::move(backup)),
          context_(std::move(context)), chain_(std::move(chain)), auditChain_(std::move(auditChain)),
          copySourceRecord_(std::move(copySourceRecord)) {}
    NativePersistencePurpose purpose_;
    AtomicOperation operation_;
    Path sourcePath_, targetPath_;
    std::string sourceIdentity_, targetIdentity_, preimage_, operationInput_, result_;
    Path carrierPath_;
    std::string carrierIdentity_, carrierRawHash_;
    Path backupPath_;
    std::string backupIdentity_, backupRawHash_;
    Path auditPath_;
    std::string auditIdentity_;
    BaseContracts::RecoveryPlan plan_;
    BaseContracts::PolicyDecision policy_;
    BaseContracts::BackupResult backup_;
    BaseContracts::ExecutionContext context_;
    BaseContracts::DiagnosticChain chain_;
    BaseContracts::AuditChain auditChain_;
    std::optional<BaseContracts::BackupRecord> copySourceRecord_;
};
} // namespace Aeostara::BaseBindings
