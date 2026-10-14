// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include <memory>
#include <string>
namespace Aeostara {
class ITargetWriteLease {
public:
    virtual ~ITargetWriteLease() = default;
};
class IFileSystem {
public:
    virtual std::string readFile(const std::string& path) = 0;
    virtual void writeFile(const std::string& path, const std::string& content) = 0;
    virtual bool fileExists(const std::string& path) = 0;
    virtual bool copyFile(const std::string& from, const std::string& to) = 0;
    // False/empty defaults refuse execution for adapters without capabilities.
    virtual bool writeFileExclusive(const std::string&, const std::string&) { return false; }
    virtual std::string pathIdentity(const std::string&) { return {}; }
    virtual std::string versionToken(const std::string&) { return {}; }
    virtual std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string&) { return {}; }
    virtual ~IFileSystem() = default;
};
class DefaultFileSystem final : public IFileSystem {
public:
    DefaultFileSystem();
    ~DefaultFileSystem() override;
    std::string readFile(const std::string& path) override;
    void writeFile(const std::string& path, const std::string& content) override;
    bool fileExists(const std::string& path) override;
    bool copyFile(const std::string& from, const std::string& to) override;
    bool writeFileExclusive(const std::string& path, const std::string& content) override;
    std::string pathIdentity(const std::string& path) override;
    std::string versionToken(const std::string& path) override;
    std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string& path) override;
private:
    struct State;
    std::shared_ptr<State> state_;
};
} // namespace Aeostara
