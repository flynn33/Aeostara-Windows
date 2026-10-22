// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/BasePolicyGate.h"
#include <stdexcept>
namespace Aeostara {
BoundPolicyContracts makeBoundPolicyContracts(const SemanticPlan& actual, const Json& input,
    const DiagnosticBundle& diagnostic, const std::string& createdAt) {
    if (createdAt.empty()) throw std::invalid_argument("Actual plan creation time unavailable");
    BaseContracts::DiagnosticReference checked(diagnostic.reference); (void)checked;
    Json operations = Json::array();
    for (const auto& operation : actual.operations.array_items()) operations.array_items().push_back(Json{{"operationID", "operation-" + sha256(operation.dump())}, {"op", operation.at("operation")}, {"path", Json{{"pointer", operation.at("pointer")}, {"tokens", Json(Json::pointer_tokens(operation.at("pointer").get<std::string>()))}, {"pointsToMissing", false}, {"pointsToNull", operation.at("expectedPreimage").is_null()}}}, {"value", operation.at("value")}, {"preconditionHash", actual.rawPreimageHash}, {"diagnosticReference", diagnostic.reference}});
    Json steps = Json::array();
    if (!operations.empty()) steps.array_items().push_back(Json{{"stepID", "step-" + actual.planHash}, {"stepType", actual.analysis.recovery == RecoveryCategory::FallbackRequired ? "SELECT_FALLBACK" : to_string(actual.analysis.recovery)}, {"description", "Apply exact authorized source-bound carrier replacements"}, {"jsonMutationOperations", operations}, {"diagnosticReference", diagnostic.reference}});
    Json plan{{"recoveryPlanID", "recovery-" + actual.planHash}, {"planDisposition", operations.empty() ? "NO_ACTION" : "MUTATION_REQUIRED"}, {"diagnosticReference", diagnostic.reference}, {"stateClass", to_string(actual.analysis.stateClass)}, {"recoveryCategory", to_string(actual.analysis.recovery)}, {"requiresPolicyGate", !operations.empty()}, {"requiresBackup", !operations.empty()}, {"recoverySteps", steps}, {"planHash", actual.planHash}, {"createdAt", createdAt}};
    const auto& source = input.at("policyContext").at("bundle");
    Json bundle{{"policyBundleID", source.at("policyBundleID")}, {"sourceReferences", Json::array({source.at("authorityReference")})}, {"rules", source.at("rules")}, {"bundleHash", actual.policyBundleHash}};
    return {BaseContracts::RecoveryPlan(plan), BaseContracts::PolicyBundle(bundle)};
}
BasePolicyGate::BasePolicyGate(SemanticEnvironment environment, Json input, BoundPolicyContracts contracts, Json chain)
    : environment_(std::move(environment)), input_(std::move(input)), contracts_(std::move(contracts)), priorChain_(std::move(chain)) { validateCompleteSemanticInput(input_); validateDiagnosticChain(priorChain_, true); }
OperationAttempt<BoundPolicyDecision> BasePolicyGate::evaluateWithChain(const BaseContracts::RecoveryPlan& requested, const BaseContracts::PolicyBundle& bundle) const {
    const auto linkedFailure = [&](OperationFailure failure) {
        try {
            const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/recoverability-semantics.pseudo.md";
            const auto& predecessor = priorChain_.at("diagnostics").array_items().back().at("evidence").at("ashDiagnostic");
            const auto diagnostic = makeAshDiagnosticBundle({"RECOVERY", predecessor.at("stage").get<std::string>(), "BLOCKED", requested.value().at("recoveryPlanID").get<std::string>(),
                {"Actual policy request rejected before mutation", failure.message}}, "ERROR", failure.message,
                Json{{"failureCode", to_string(failure.code)}, {"availableEvidence", failure.evidence}},
                Json::array({Json{{"ruleID", "ASH-RECOVERY-ACTION-001"}, {"ruleFamily", "ASH-RECOVERY"}, {"sourceReference", source}}}), {source}, &priorChain_);
            failure.diagnosticChain = diagnostic.chain; failure.diagnosticPersistence = DiagnosticPersistence::NotRequested;
            std::erase_if(failure.unavailableEvidence, [](const UnavailableEvidence& item) { return item.field == "diagnosticChain"; });
        } catch (const std::exception& error) { failure.addSecondary(OperationPhase::Export, OperationCode::DiagnosticUnavailable, error.what()); }
        return failure;
    };
    try {
        if (requested != contracts_.plan || bundle != contracts_.bundle) return linkedFailure(OperationFailure(OperationPhase::Policy, OperationCode::PolicyRejected, "Foreign policy request differs from complete immutable plan/bundle binding"));
        auto computed = environment_.plan(input_); if (auto error = std::get_if<OperationFailure>(&computed)) return *error;
        const auto& actual = std::get<SemanticPlan>(computed);
        if (requested.value().at("planHash") != Json(actual.planHash) || bundle.value().at("bundleHash") != Json(actual.policyBundleHash)) throw std::invalid_argument("Policy plan/bundle content differs from actual independent calculation");
        Json evidence = actual.policyDecisionEvidence; evidence["frozenPlan"] = requested.value(); evidence["frozenBundle"] = bundle.value();
        const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/recoverability-semantics.pseudo.md";
        AshDiagnosticContext context{"RECOVERY", "CLASSIFICATION", "RESOLVED", requested.value().at("recoveryPlanID").get<std::string>(), {"Actual policy rule evaluated against exact immutable plan, target, preimage, inverse, witness and permitted scope"}};
        const auto diagnostic = makeAshDiagnosticBundle(context, "INFO", "Actual complete policy binding authorizes exact mutation plan", evidence, Json::array({Json{{"ruleID", "ASH-RECOVERY-ACTION-001"}, {"ruleFamily", "ASH-RECOVERY"}, {"sourceReference", source}}}), {source, input_.at("policyContext").at("bundle").at("authorityReference").get<std::string>()}, &priorChain_);
        return BoundPolicyDecision{BaseContracts::PolicyDecision(Json{{"policyDecisionID", "policy-" + sha256(evidence.dump())}, {"decision", "ALLOW"}, {"policyBundleID", bundle.value().at("policyBundleID")}, {"reasons", Json::array({"Exact independently evaluated target, witness, preimage and operation scope authorized"})}, {"diagnosticReference", diagnostic.reference}}), diagnostic.chain};
    } catch (const std::exception& error) { return linkedFailure(operationFailureFromException(OperationPhase::Policy, OperationCode::Failed, error)); }
}
OperationAttempt<BaseContracts::PolicyDecision> BasePolicyGate::evaluate(const BaseContracts::RecoveryPlan& plan, const BaseContracts::PolicyBundle& bundle) const {
    auto calculated = evaluateWithChain(plan, bundle); if (auto error = std::get_if<OperationFailure>(&calculated)) return *error;
    return std::get<BoundPolicyDecision>(std::move(calculated)).decision;
}
}
