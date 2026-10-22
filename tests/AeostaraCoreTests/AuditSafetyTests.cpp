// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "CppUnitTest.h"
#include "AeostaraCore/AuditTrail.h"
#include <atomic>
#include <mutex>
#include <set>
#include <thread>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
namespace AeostaraCoreTests {
class AuditFileSystem final : public IFileSystem {
public:
    std::map<std::string, std::string> files;
    bool failRead = false;
    bool failWrite = false;
    bool dropWrites = false;
    bool corruptReadback = false;
    int writes = 0;
    std::string readFile(const std::string& path) override {
        if (failRead) throw std::runtime_error("Injected audit read failure");
        const auto found = files.find(path);
        if (found == files.end()) throw std::runtime_error("Absent audit file");
        if (corruptReadback && writes > 0) return found->second + "changed";
        return found->second;
    }
    void writeFile(const std::string& path, const std::string& bytes) override {
        ++writes;
        if (failWrite) throw std::runtime_error("Injected audit write failure");
        if (!dropWrites) files[path] = bytes;
    }
    bool fileExists(const std::string& path) override { return files.contains(path); }
    bool copyFile(const std::string& from, const std::string& to) override {
        if (!files.contains(from)) return false;
        files[to] = files[from]; return true;
    }
};
TEST_CLASS(AuditSafetyTests) {
public:
    TEST_METHOD(MissingStorageCapability_IsRejectedAtConstruction) {
        Assert::ExpectException<std::invalid_argument>([] { JsonLinesAuditTrail audit("audit", {}); });
        Assert::ExpectException<std::invalid_argument>([] { JsonLinesAuditTrail audit("", std::make_shared<AuditFileSystem>()); });
    }
    TEST_METHOD(ExistingHistoryReadFailure_DoesNotOverwrite) {
        auto files = std::make_shared<AuditFileSystem>();
        files->files["audit"] = "existing-history";
        files->failRead = true;
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<std::runtime_error>([&] { audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); });
        Assert::AreEqual(0, files->writes);
        Assert::AreEqual(std::string("existing-history"), files->files.at("audit"));
    }
    TEST_METHOD(MalformedHistory_BlocksAppendAndRead) {
        auto files = std::make_shared<AuditFileSystem>();
        files->files["audit"] = "{bad}\n";
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<JsonError>([&] { audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); });
        Assert::ExpectException<JsonError>([&] { (void)audit.getEvents(); });
        Assert::AreEqual(0, files->writes);
    }
    TEST_METHOD(BlankRecord_IsCorruption) {
        auto files = std::make_shared<AuditFileSystem>();
        files->files["audit"] = "\n";
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<JsonError>([&] { (void)audit.getEvents(); });
    }
    TEST_METHOD(ExistingHistoryReadFailure_IsNotEmptySuccess) {
        auto files = std::make_shared<AuditFileSystem>();
        files->files["audit"] = "history";
        files->failRead = true;
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<std::runtime_error>([&] { (void)audit.getEvents(); });
    }
    TEST_METHOD(WriteFailure_IsReported) {
        auto files = std::make_shared<AuditFileSystem>();
        files->failWrite = true;
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<std::runtime_error>([&] { audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); });
    }
    TEST_METHOD(SilentWriteAndChangedReadback_AreRejected) {
        auto files = std::make_shared<AuditFileSystem>();
        files->dropWrites = true;
        JsonLinesAuditTrail audit("audit", files);
        Assert::ExpectException<std::runtime_error>([&] { audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); });
        files->dropWrites = false;
        files->corruptReadback = true;
        Assert::ExpectException<std::runtime_error>([&] { audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); });
    }
    TEST_METHOD(CompleteLastRecordWithoutNewline_IsPreserved) {
        auto files = std::make_shared<AuditFileSystem>();
        const auto first = JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target");
        files->files["audit"] = Json(first).canonical();
        JsonLinesAuditTrail audit("audit", files);
        audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::BackupCreated, "target"));
        const auto events = audit.getEvents();
        Assert::AreEqual(std::size_t(2), events.size());
        Assert::AreEqual(first.eventID, events.front().eventID);
    }
    TEST_METHOD(ConcurrentInstances_PreserveEveryUniqueEvent) {
        auto files = std::make_shared<AuditFileSystem>();
        JsonLinesAuditTrail first("audit", files), second("audit", files);
        std::atomic<bool> failed{false};
        auto append = [&](JsonLinesAuditTrail& audit) {
            try { for (int i = 0; i < 20; ++i) audit.record(JsonLinesAuditTrail::createEvent(AuditEventType::HealStarted, "target")); }
            catch (...) { failed.store(true); }
        };
        std::thread a(append, std::ref(first)), b(append, std::ref(second));
        a.join(); b.join();
        Assert::IsFalse(failed.load());
        const auto events = first.getEvents();
        std::set<std::string> ids;
        for (const auto& event : events) ids.insert(event.eventID);
        Assert::AreEqual(std::size_t(40), events.size());
        Assert::AreEqual(std::size_t(40), ids.size());
    }
};
} // namespace AeostaraCoreTests
