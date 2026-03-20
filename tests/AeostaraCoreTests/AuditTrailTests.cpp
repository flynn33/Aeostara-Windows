// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/AuditTrail.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(AuditTrailTests)
{
public:

    TEST_METHOD(Record_AppendsToFile)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonLinesAuditTrail trail("audit.jsonl", fs);

        auto event = JsonLinesAuditTrail::createEvent(
            AuditEventType::HealStarted, "config.json", {{"action", "heal"}});
        trail.record(event);

        Assert::IsTrue(fs->fileExists("audit.jsonl"));
        auto content = fs->readFile("audit.jsonl");
        Assert::IsFalse(content.empty());
    }

    TEST_METHOD(Record_MultipleEvents_AppendsLines)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonLinesAuditTrail trail("audit.jsonl", fs);

        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::HealStarted, "config.json"));
        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::BackupCreated, "config.json"));
        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::RepairApplied, "config.json"));

        auto content = fs->readFile("audit.jsonl");
        // Count newlines — each event is one JSON line
        size_t lineCount = 0;
        for (char c : content) {
            if (c == '\n') lineCount++;
        }
        Assert::IsTrue(lineCount >= 3);
    }

    TEST_METHOD(GetEvents_ReturnsRecordedEvents)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonLinesAuditTrail trail("audit.jsonl", fs);

        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::HealStarted, "config.json"));
        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::VerificationSucceeded, "config.json"));

        auto events = trail.getEvents();
        Assert::AreEqual(static_cast<size_t>(2), events.size());
        Assert::IsTrue(AuditEventType::HealStarted == events[0].type);
        Assert::IsTrue(AuditEventType::VerificationSucceeded == events[1].type);
    }

    TEST_METHOD(CreateEvent_HasEventID)
    {
        auto event = JsonLinesAuditTrail::createEvent(
            AuditEventType::NoDrift, "config.json");

        Assert::IsFalse(event.eventID.empty());
    }

    TEST_METHOD(CreateEvent_HasTimestamp)
    {
        auto event = JsonLinesAuditTrail::createEvent(
            AuditEventType::NoDrift, "config.json");

        Assert::IsFalse(event.timestamp.empty());
    }

    TEST_METHOD(CreateEvent_HasCorrectType)
    {
        auto event = JsonLinesAuditTrail::createEvent(
            AuditEventType::PolicyBlocked, "config.json");

        Assert::IsTrue(AuditEventType::PolicyBlocked == event.type);
        Assert::AreEqual(std::string("config.json"), event.configFile);
    }

    TEST_METHOD(GetEvents_EmptyFile_ReturnsEmpty)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonLinesAuditTrail trail("audit.jsonl", fs);

        auto events = trail.getEvents();
        Assert::AreEqual(static_cast<size_t>(0), events.size());
    }

    TEST_METHOD(Record_WritesValidJSON)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonLinesAuditTrail trail("audit.jsonl", fs);

        trail.record(JsonLinesAuditTrail::createEvent(
            AuditEventType::HealStarted, "config.json", {{"test", true}}));

        auto content = fs->readFile("audit.jsonl");
        // Each line should be valid JSON — parse the first line
        auto firstNewline = content.find('\n');
        std::string firstLine = content.substr(0, firstNewline);
        auto parsed = nlohmann::json::parse(firstLine);
        Assert::IsTrue(parsed.contains("event_id") || parsed.contains("eventID"));
    }
};

} // namespace AeostaraCoreTests
