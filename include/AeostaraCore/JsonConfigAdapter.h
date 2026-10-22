// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "AeostaraCore/IConfigAdapter.h"
#include "AeostaraCore/IFileSystem.h"
#include <memory>
namespace Aeostara {
class JsonConfigAdapter final : public IConfigAdapter {
public:
    explicit JsonConfigAdapter(std::shared_ptr<IFileSystem> fs);
    ObservedState observe(const std::string& filePath) override;
    EncodedState encode(const ObservedState& observed, const DesiredState& desired) override;
    bool applyRepair(const std::string& filePath, const RepairPlan& plan) override;
    bool applyOperations(const std::string& filePath, const Json& operations, const std::string& rawPreimageHash) override;
private:
    std::shared_ptr<IFileSystem> fs_;
};
} // namespace Aeostara
