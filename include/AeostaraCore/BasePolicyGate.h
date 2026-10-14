// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/BaseAdapterBindings.h"
#include "AeostaraCore/Diagnostics.h"
namespace Aeostara {
struct BoundPolicyContracts final { BaseContracts::RecoveryPlan plan; BaseContracts::PolicyBundle bundle; };
BoundPolicyContracts makeBoundPolicyContracts(const SemanticPlan& actualPlan, const Json& completeInput,
    const DiagnosticBundle& actualDiagnostic, const std::string& actualCreatedAt);
struct BoundPolicyDecision final { BaseContracts::PolicyDecision decision; Json diagnosticChain; };
class BasePolicyGate final : public BaseBindings::IPolicyGate {
public:
    BasePolicyGate(SemanticEnvironment environment, Json completeInput, BoundPolicyContracts contracts,
                   Json actualPriorChain);
    OperationAttempt<BaseContracts::PolicyDecision> evaluate(const BaseContracts::RecoveryPlan& plan,
        const BaseContracts::PolicyBundle& bundle) const override;
    OperationAttempt<BoundPolicyDecision> evaluateWithChain(const BaseContracts::RecoveryPlan& plan,
        const BaseContracts::PolicyBundle& bundle) const;
private:
    SemanticEnvironment environment_; Json input_; BoundPolicyContracts contracts_; Json priorChain_;
};
}
