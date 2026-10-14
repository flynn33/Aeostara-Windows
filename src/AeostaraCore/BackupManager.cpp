// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "AeostaraCore/BackupManager.h"
#include "AeostaraCore/SemanticMapping.h"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <stdexcept>
namespace Aeostara {
BackupManager::BackupManager(std::shared_ptr<IFileSystem> fs) : fs_(std::move(fs)) { if (!fs_) throw std::invalid_argument("Backup filesystem unavailable"); }
std::string BackupManager::generateBackupPath(const std::string& filePath) {
    std::array<unsigned char, 16> random{};
    if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) throw std::runtime_error("Cannot generate unique backup identity");
    constexpr char digits[] = "0123456789abcdef"; std::string identity;
    for (auto byte : random) { identity.push_back(digits[byte >> 4]); identity.push_back(digits[byte & 15]); }
    return filePath + ".backup." + identity;
}
std::string BackupManager::createBackup(const std::string& filePath) {
    if (!fs_->fileExists(filePath)) return {};
    const auto original = fs_->readFile(filePath); const auto originalHash = sha256(original);
    for (unsigned attempt = 0; attempt < 8; ++attempt) {
        const auto path = generateBackupPath(filePath);
        if (!fs_->writeFileExclusive(path, original)) continue;
        const auto actual = fs_->readFile(path);
        if (actual != original || sha256(actual) != originalHash || fs_->readFile(filePath) != original) throw std::runtime_error("Backup storage/preimage verification failed");
        return path;
    }
    throw std::runtime_error("No confirmed unique backup could be created");
}
bool BackupManager::restoreBackup(const std::string& backupPath, const std::string& originalPath) {
    if (!fs_->fileExists(backupPath)) return false;
    const auto bytes = fs_->readFile(backupPath); fs_->writeFile(originalPath, bytes); return fs_->readFile(originalPath) == bytes;
}
} // namespace Aeostara
