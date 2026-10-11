// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "CLIRunner.h"

#include <iostream>

namespace Aeostara {
namespace {
int failureExitCode(const OperationFailure& failure) {
    switch (failure.code) {
    case OperationCode::Ambiguous:
    case OperationCode::Blocked:
    case OperationCode::UnsupportedEvidence:
    case OperationCode::StaleState:
    case OperationCode::PolicyRejected:
    case OperationCode::VerificationFailed:
        return 1;
    default:
        return 2;
    }
}
}

CLIRunner::CLIRunner(std::shared_ptr<IHealingEngine> engine)
    : engine_(std::move(engine)) {}

int CLIRunner::run(const ParsedCommand& cmd) const {
    if (cmd.command == "validate") return runValidate(cmd);
    if (cmd.command == "diff")     return runDiff(cmd);
    if (cmd.command == "heal")     return runHeal(cmd);

    std::cerr << "Unknown command: " << cmd.command << "\n";
    return 2;
}

int CLIRunner::runValidate(const ParsedCommand& cmd) const {
    try {
        auto result = engine_->validate(
            cmd.configPath, cmd.desiredPath, cmd.invariantsPath);

        Json output;
        output["command"] = "validate";
        output["configFile"] = cmd.configPath;
        output["valid"] = result.valid;
        if (result.failure) output["failure"] = operationFailureJson(*result.failure);

        if (!result.errors.empty()) {
            output["errors"] = result.errors;
        }

        if (!result.drifts.empty()) {
            output["driftCount"] = result.drifts.size();
            Json driftsJson = Json::array();
            for (const auto& d : result.drifts) {
                driftsJson.push_back(Json(d));
            }
            output["drifts"] = driftsJson;
        }

        if (!result.violations.empty()) {
            output["violationCount"] = result.violations.size();
            Json violationsJson = Json::array();
            for (const auto& v : result.violations) {
                violationsJson.push_back(Json(v));
            }
            output["violations"] = violationsJson;
        }

        std::cout << output.dump(2) << "\n";

        if (result.failure) return failureExitCode(*result.failure);

        if (!result.valid) {
            std::cerr << "Drift detected: " << result.drifts.size() << " difference(s)\n";
            return 1;
        }

        std::cerr << "Valid: no drift detected\n";
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}

int CLIRunner::runDiff(const ParsedCommand& cmd) const {
    try {
        auto result = engine_->diff(
            cmd.configPath, cmd.desiredPath, cmd.invariantsPath);

        Json output;
        output["command"] = "diff";
        output["configFile"] = cmd.configPath;
        output["driftCount"] = result.drifts.size();

        Json driftsJson = Json::array();
        for (const auto& d : result.drifts) {
            driftsJson.push_back(Json(d));
        }
        output["drifts"] = driftsJson;

        output["proposedPlan"] = Json(result.proposedPlan);
        if (result.failure) output["failure"] = operationFailureJson(*result.failure);

        std::cout << output.dump(2) << "\n";

        if (result.failure) return failureExitCode(*result.failure);

        if (result.drifts.empty()) {
            std::cerr << "No drift detected\n";
            return 0;
        }

        std::cerr << "Drift detected: " << result.drifts.size()
                  << " difference(s), "
                  << result.proposedPlan.actions.size() << " repair action(s)\n";
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}

int CLIRunner::runHeal(const ParsedCommand& cmd) const {
    try {
        auto result = engine_->heal(
            cmd.configPath, cmd.desiredPath, cmd.invariantsPath, cmd.auditPath);

        Json output;
        output["command"] = "heal";
        output["configFile"] = cmd.configPath;
        output["success"] = result.success;
        output["message"] = result.message;
        output["mutationMayHaveOccurred"] = result.mutationMayHaveOccurred;
        if (result.failure) output["failure"] = operationFailureJson(*result.failure);

        if (!result.executedPlan.actions.empty()) {
            output["plan"] = Json(result.executedPlan);
        }

        output["verification"] = Json(result.verification);

        if (result.rollback.has_value()) {
            output["rollback"] = Json(result.rollback.value());
        }

        Json eventsJson = Json::array();
        for (const auto& e : result.auditEvents) {
            eventsJson.push_back(Json(e));
        }
        output["auditEvents"] = eventsJson;

        std::cout << output.dump(2) << "\n";

        if (result.failure) return failureExitCode(*result.failure);

        if (result.success) {
            std::cerr << "Heal successful: " << result.message << "\n";
            return 0;
        }

        std::cerr << "Heal failed: " << result.message << "\n";
        return 1;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 2;
    }
}

} // namespace Aeostara
