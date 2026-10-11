// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual
#pragma once
#include "AeostaraCore/BaseAdapterBindings.h"
#include "AeostaraCore/IFileSystem.h"
#include <map>
#include <memory>
#include <mutex>
namespace Aeostara::BaseBindings {
class BaseAuditSink final : public IAuditSink {
public:
    BaseAuditSink(std::string auditPath, std::shared_ptr<Aeostara::IFileSystem> filesystem);
    const std::string& storagePath() const noexcept { return auditPath_; }
    OperationAttempt<BaseContracts::DiagnosticReference> registerDiagnosticChain(const BaseContracts::DiagnosticChain& chain);
    OperationAttempt<BaseContracts::AuditChain> appendEvent(const BaseContracts::AuditChain& chain, const BaseContracts::AuditEvent& event) override;
    OperationAttempt<BaseContracts::DiagnosticReference> persistChain(const BaseContracts::AuditChain& chain) override;
    OperationAttempt<BaseContracts::AuditChain> readChain(const std::string& auditChainID) const override;
private:
    void validateChain(const BaseContracts::AuditChain& chain) const;
    void validateReference(const Json& reference) const;
    BaseContracts::DiagnosticReference retainDiagnosticChain(const BaseContracts::DiagnosticChain& chain) const;
    BaseContracts::AuditChain decodeSnapshot(const Json& snapshot) const;
    OperationFailure failure(OperationCode code, const std::string& message, const Json& evidence = Json::object()) const;
    std::string auditPath_;
    std::shared_ptr<Aeostara::IFileSystem> filesystem_;
    mutable std::mutex mutex_;
    mutable std::map<std::string, BaseContracts::DiagnosticChain, UnsignedUtf8Less> diagnosticChains_;
    mutable std::size_t diagnosticBytes_ = 0;
};
} // namespace Aeostara::BaseBindings
