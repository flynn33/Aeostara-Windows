// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual

#pragma once

#include "AeostaraCore/Json.h"

#include <string>
#include <vector>

namespace Aeostara {

// ---------------------------------------------------------------------------
// InvariantSeverity
// ---------------------------------------------------------------------------
enum class InvariantSeverity {
    Low,
    Medium,
    High,
    Critical
};

std::string to_string(InvariantSeverity severity);
InvariantSeverity invariantSeverityFromString(const std::string& str);

void to_json(Json& j, InvariantSeverity severity);
void from_json(const Json& j, InvariantSeverity& severity);

// ---------------------------------------------------------------------------
// Invariant — a constraint rule from the invariant rule format
// ---------------------------------------------------------------------------
struct Invariant final {
    std::string invariantID;
    std::string name;
    std::string description;
    InvariantSeverity severity = InvariantSeverity::Medium;
    std::string expression;
    std::vector<std::string> appliesTo;
    bool autoRemediate = false;

    bool operator==(const Invariant&) const = default;
};

void to_json(Json& j, const Invariant& inv);
void from_json(const Json& j, Invariant& inv);

// ---------------------------------------------------------------------------
// InvariantViolation — result of evaluating an invariant against a state
// ---------------------------------------------------------------------------
struct InvariantViolation final {
    std::string invariantID;
    std::string keyPath;
    Json actualValue;
    Json expectedValue;
    InvariantSeverity severity;

    bool operator==(const InvariantViolation&) const = default;
};

void to_json(Json& j, const InvariantViolation& v);
void from_json(const Json& j, InvariantViolation& v);

} // namespace Aeostara
