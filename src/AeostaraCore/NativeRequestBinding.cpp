// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/NativeRequestBinding.h"
#include <stdexcept>
namespace Aeostara {
OperationAttempt<Json> bindCapturedNativeTarget(const Json& request, const std::string& path,
    IFileSystem& fs, IConfigAdapter& adapter) {
    try {
        if (path.empty()) throw std::invalid_argument("Explicit authorized native target path is required");
        const auto identity = fs.pathIdentity(path), version = fs.versionToken(path), bytes = fs.readFile(path);
        if (identity.empty() || version.empty()) throw std::invalid_argument("Actual native target identity/version unavailable");
        const auto document = Json::parse(bytes);
        if (adapter.observe(path).data != document || fs.pathIdentity(path) != identity ||
            fs.versionToken(path) != version || fs.readFile(path) != bytes)
            return OperationFailure(OperationPhase::Project, OperationCode::StaleState, "Target changed during read-only capture");
        Json input = request;
        auto& snapshot = input.at("observedSnapshot");
        snapshot["document"] = document; snapshot["rawUtf8"] = Json::captured_raw_utf8(bytes); snapshot["revision"] = version;
        snapshot["rawByteSha256"] = sha256(bytes); snapshot["proposedCanonicalSha256"] = sha256(document.dump());
        if (input.contains("proposedPlan")) {
            auto& plan = input.at("proposedPlan");
            // Existing operations and their expected values remain explicit.
            // An outdated witness or different coordinates fail normal planning.
            plan["expectedObservedVersion"] = version; plan["expectedObservedInputSha256"] = hashCapturedSnapshot(snapshot);
            plan["adapterTargetIdentity"] = identity;
            auto& evaluation = input.at("policyContext").at("evaluationRequest");
            evaluation["adapterTargetIdentity"] = identity; evaluation["rawBeforeSha256"] = snapshot.at("rawByteSha256");
            evaluation["canonicalBeforeSha256"] = snapshot.at("proposedCanonicalSha256");
            evaluation["planContentSha256"] = sha256(plan.dump());
        }
        return input;
    } catch (const std::exception& error) {
        return operationFailureFromException(OperationPhase::Project, OperationCode::Failed, error);
    }
}
} // namespace Aeostara
