// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/JsonConfigAdapter.h"
#include "AeostaraCore/JsonPath.h"
#include "AeostaraCore/SemanticMapping.h"
#include <chrono>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
namespace Aeostara {
namespace {
std::string timestamp() { const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); std::tm value{}; if (gmtime_s(&value, &now) != 0) throw std::runtime_error("Actual observation time unavailable"); std::ostringstream result; result << std::put_time(&value, "%Y-%m-%dT%H:%M:%SZ"); return result.str(); }
}
JsonConfigAdapter::JsonConfigAdapter(std::shared_ptr<IFileSystem> fs) : fs_(std::move(fs)) { if (!fs_) throw std::invalid_argument("Config filesystem unavailable"); }
ObservedState JsonConfigAdapter::observe(const std::string& path) { return {.sourceFile = path, .data = Json::parse(fs_->readFile(path)), .timestamp = timestamp()}; }
EncodedState JsonConfigAdapter::encode(const ObservedState& observed, const DesiredState& desired) { return {.observed = JsonPath::flatten(observed.data), .desired = JsonPath::flatten(desired.data)}; }
bool JsonConfigAdapter::applyRepair(const std::string& path, const RepairPlan& plan) {
    // Legacy surface-plan adapter remains mechanical and validates each preimage;
    // it is never used as a semantic authorization producer by HealingEngine.
    if (!plan.requiresBackup) throw std::invalid_argument("Repair requires backup");
    auto document = Json::parse(fs_->readFile(path));
    for (const auto& action : plan.actions) {
        if (action.actionType == RepairActionType::Remove) throw std::invalid_argument("Unsupported remove action");
        const auto current = JsonPath::get(document, action.keyPath);
        const bool exists = JsonPath::exists(document, action.keyPath);
        if (action.actionType == RepairActionType::Set && (!exists || current != action.fromValue)) throw std::invalid_argument("Repair preimage mismatch");
        if (action.actionType == RepairActionType::Add && exists) throw std::invalid_argument("Add requires an absent key");
        JsonPath::set(document, action.keyPath, action.toValue);
    }
    fs_->writeFile(path, document.dump(2)); return true;
}
bool JsonConfigAdapter::applyOperations(const std::string& path, const Json& operations, const std::string& rawPreimageHash) {
    const auto bytes = fs_->readFile(path); if (sha256(bytes) != rawPreimageHash) throw std::runtime_error("Stale raw preimage before first adapter write");
    auto document = Json::parse(bytes); std::set<std::string> pointers;
    for (const auto& operation : operations.array_items()) {
        if (operation.at("operation") != Json("replace")) throw std::invalid_argument("Unsupported semantic operation");
        const auto pointer = operation.at("pointer").get<std::string>(); const auto tokens = Json::pointer_tokens(pointer);
        if (tokens.size() != 2 || tokens[0] != "state_vector" || tokens[1].size() != 1 || tokens[1][0] < '0' || tokens[1][0] > '8' || !pointers.insert(pointer).second) throw std::invalid_argument("Operation exceeds adopted unique carrier slot scope");
        const auto* actual = document.resolve_pointer(pointer); if (!actual || *actual != operation.at("expectedPreimage")) throw std::invalid_argument("Slot preimage mismatch");
        const auto& value = operation.at("value"); if (!value.is_number() || (value != Json(0) && value != Json(1))) throw std::invalid_argument("Inverse target is not a numeric bit");
        document.replace_pointer(pointer, value);
    }
    fs_->writeFile(path, document.dump()); return true;
}
} // namespace Aeostara
