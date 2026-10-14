// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT

#pragma once

#include "AeostaraCore/IFileSystem.h"

#include <map>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>

namespace Aeostara::Testing {

/// In-memory mock file system for testing.
class MockFileSystem final : public IFileSystem {
public:
    std::map<std::string, std::string> files;
    std::function<void(const std::string&, const std::string&)> afterWrite;
    std::size_t configurationWrites = 0;
    std::string version = "demo-observed-r1";
    bool hasLease = false;
    class Lease final : public ITargetWriteLease { public: explicit Lease(bool& state) : state_(state) {} ~Lease() override { state_ = false; } private: bool& state_; };
    bool writeFileExclusive(const std::string& path, const std::string& content) override { if (files.contains(path)) return false; writeFile(path, content); return readFile(path) == content; }
    std::string pathIdentity(const std::string& path) override { return path == "config.json" ? "fixture:configuration:demo-state-carrier" : "mock:" + path; }
    std::string versionToken(const std::string&) override { return version; }
    std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string&) override { if (hasLease) return {}; hasLease = true; return std::make_unique<Lease>(hasLease); }

    std::string readFile(const std::string& path) override {
        auto it = files.find(path);
        if (it == files.end()) {
            throw std::runtime_error("File not found: " + path);
        }
        return it->second;
    }

    void writeFile(const std::string& path, const std::string& content) override {
        files[path] = content;
        if (path == "config.json") ++configurationWrites;
        if (afterWrite) afterWrite(path, content);
    }

    bool fileExists(const std::string& path) override {
        return files.find(path) != files.end();
    }

    bool copyFile(const std::string& from, const std::string& to) override {
        auto it = files.find(from);
        if (it == files.end()) return false;
        files[to] = it->second;
        return true;
    }
};

// Sample JSON config for testing
inline const char* SAMPLE_CONFIG = R"({
  "server": {
    "host": "localhost",
    "port": 8080,
    "ssl_enabled": true
  },
  "database": {
    "host": "db.example.com",
    "port": 5432,
    "name": "aeostara_prod"
  },
  "logging": {
    "level": "INFO",
    "file": "/var/log/aeostara.log"
  }
})";

inline const char* SAMPLE_DESIRED = R"({
  "server": {
    "host": "localhost",
    "port": 8080,
    "ssl_enabled": true
  },
  "database": {
    "host": "db.example.com",
    "port": 5432,
    "name": "aeostara_prod"
  },
  "logging": {
    "level": "INFO",
    "file": "/var/log/aeostara.log"
  }
})";

inline const char* DRIFTED_CONFIG = R"({
  "server": {
    "host": "localhost",
    "port": 9090,
    "ssl_enabled": true
  },
  "database": {
    "host": "db.example.com",
    "port": 3306,
    "name": "aeostara_dev"
  },
  "logging": {
    "level": "DEBUG",
    "file": "/var/log/aeostara.log"
  }
})";

inline const char* SAMPLE_INVARIANTS = R"([
  {
    "invariant_id": "INV-001",
    "name": "Database Port Standard",
    "description": "Database port must be 5432",
    "severity": "high",
    "expression": "database.port == 5432",
    "applies_to": ["database"],
    "auto_remediate": true
  },
  {
    "invariant_id": "INV-002",
    "name": "SSL Required",
    "description": "SSL must be enabled",
    "severity": "critical",
    "expression": "server.ssl_enabled == true",
    "applies_to": ["server"],
    "auto_remediate": true
  }
])";

inline const char* BLOCKING_INVARIANTS = R"([
  {
    "invariant_id": "INV-BLOCK",
    "name": "Port Must Be 3306",
    "description": "Database port must remain 3306",
    "severity": "critical",
    "expression": "database.port == 3306",
    "applies_to": ["database"],
    "auto_remediate": false
  }
])";

} // namespace Aeostara::Testing
