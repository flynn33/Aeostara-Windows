// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT

#pragma once

#include "AeostaraCore/IBackupProvider.h"
#include "AeostaraCore/IFileSystem.h"

#include <memory>

namespace Aeostara {

/// Timestamped file backup implementation.
class BackupManager final : public IBackupProvider {
public:
    explicit BackupManager(std::shared_ptr<IFileSystem> fs);

    std::string createBackup(const std::string& filePath) override;
    bool supportsBackup() const noexcept override { return true; }
    bool restoreBackup(const std::string& backupPath, const std::string& originalPath) override;

private:
    std::shared_ptr<IFileSystem> fs_;

    static std::string generateBackupPath(const std::string& filePath);
};

} // namespace Aeostara
