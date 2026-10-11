// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#pragma once
#include "AeostaraCore/Contracts.h"
#include "AeostaraCore/Invariant.h"
#include "AeostaraCore/SemanticMapping.h"
#include "AeostaraCore/Diagnostics.h"
#include <optional>
#include <string>
#include <vector>
namespace Aeostara {
enum class RestorationStatus { NotNeeded, Verified, Failed };
class IHealingEngine {
public:
    struct ValidationResult final {
        bool valid = false;
        std::vector<std::string> errors;
        std::vector<DriftEvent> drifts;
        std::vector<InvariantViolation> violations;
        std::optional<SemanticAnalysis> semanticAnalysis;
        std::optional<OperationFailure> failure;
    };
    struct DiffResult final {
        std::vector<DriftEvent> drifts;
        RepairPlan proposedPlan;
        std::optional<SemanticPlan> semanticPlan;
        std::optional<OperationFailure> failure;
    };
    struct HealResult final {
        bool success = false;
        RepairPlan executedPlan;
        VerificationResult verification;
        std::optional<RollbackPlan> rollback;
        std::vector<AuditEvent> auditEvents;
        std::string message;
        std::optional<SemanticAnalysis> semanticAnalysis;
        std::optional<OperationFailure> failure;
        RestorationStatus restoration = RestorationStatus::NotNeeded;
        bool mutationMayHaveOccurred = false;
        std::optional<std::string> rawHashBefore;
        std::optional<std::string> rawHashAfter;
        std::optional<std::string> backupPath;
        Json diagnosticChain;
        Json auditChain;
        std::vector<OperationFault> diagnosticErrors;
        std::uint64_t diagnosticErrorsDropped = 0;
        std::optional<DiagnosticHealth> diagnosticHealth;
    };
    virtual ValidationResult validate(const std::string&, const std::string&, const std::string&) = 0;
    virtual DiffResult diff(const std::string&, const std::string&, const std::string&) = 0;
    virtual HealResult heal(const std::string&, const std::string&, const std::string&, const std::string&) = 0;
    virtual ValidationResult validateSemantic(const std::string&, const Json&) = 0;
    virtual DiffResult diffSemantic(const std::string&, const Json&) = 0;
    virtual HealResult healSemantic(const std::string&, const Json&, const std::string&) = 0;
    virtual ~IHealingEngine() = default;
};
} // namespace Aeostara
