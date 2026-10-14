// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/IHealingEngine.h"
#include "AeostaraCore/IConfigAdapter.h"
#include "AeostaraCore/IBackupProvider.h"
#include "AeostaraCore/IAuditSink.h"
#include "AeostaraCore/IFileSystem.h"
#include "AeostaraCore/Diagnostics.h"
#include "AeostaraCore/BasePolicyGate.h"
#include <memory>
namespace Aeostara {
class HealingEngine final : public IHealingEngine {
public:
    HealingEngine(std::shared_ptr<IConfigAdapter> adapter, std::shared_ptr<IBackupProvider> backup,
                  std::shared_ptr<IFileSystem> fs, std::shared_ptr<const SemanticEnvironment> environment = {},
                  std::shared_ptr<IDiagnostics> diagnostics = {}, std::shared_ptr<BaseBindings::IPolicyGate> policyGate = {});
    ValidationResult validate(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath) override;
    DiffResult diff(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath) override;
    HealResult heal(const std::string& configPath, const std::string& desiredPath, const std::string& invariantsPath, const std::string& auditPath) override;
    ValidationResult validateSemantic(const std::string& configPath, const Json& completeInput) override;
    DiffResult diffSemantic(const std::string& configPath, const Json& completeInput) override;
    HealResult healSemantic(const std::string& configPath, const Json& completeInput, const std::string& auditPath) override;
private:
    std::shared_ptr<IConfigAdapter> adapter_;
    std::shared_ptr<IBackupProvider> backup_;
    std::shared_ptr<IFileSystem> fs_;
    std::shared_ptr<const SemanticEnvironment> environment_;
    std::shared_ptr<IDiagnostics> diagnostics_;
    std::shared_ptr<BaseBindings::IPolicyGate> policyGate_;
    DesiredState loadDesired(const std::string&) const;
    std::vector<Invariant> loadInvariants(const std::string&) const;
};
} // namespace Aeostara
