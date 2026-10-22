// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/Contracts.h"
#include <string>
namespace Aeostara {
class IConfigAdapter {
public:
    virtual ObservedState observe(const std::string& filePath) = 0;
    virtual EncodedState encode(const ObservedState& observed, const DesiredState& desired) = 0;
    virtual bool applyRepair(const std::string& filePath, const RepairPlan& plan) = 0;
    // Mechanical host writer. Policy/backup/exclusive/precondition gates belong
    // to the engine; no default implementation can mutate through this capability.
    virtual bool applyOperations(const std::string&, const Json&, const std::string&) { return false; }
    virtual ~IConfigAdapter() = default;
};
} // namespace Aeostara
