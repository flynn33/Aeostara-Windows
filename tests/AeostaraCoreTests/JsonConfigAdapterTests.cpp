// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/JsonConfigAdapter.h"
#include "AeostaraCore/JsonPath.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(JsonConfigAdapterTests)
{
public:

    TEST_METHOD(Observe_ReadsAndParsesJSON)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = SAMPLE_CONFIG;

        JsonConfigAdapter adapter(fs);
        auto observed = adapter.observe("config.json");

        Assert::AreEqual(std::string("config.json"), observed.sourceFile);
        Assert::IsTrue(observed.data.is_object());
        Assert::AreEqual(8080, observed.data["server"]["port"].get<int>());
    }

    TEST_METHOD(Observe_InvalidJSON_Throws)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["bad.json"] = "{ not valid }";

        JsonConfigAdapter adapter(fs);

        Assert::ExpectException<nlohmann::json::parse_error>([&]() {
            adapter.observe("bad.json");
        });
    }

    TEST_METHOD(Observe_NonexistentFile_Throws)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonConfigAdapter adapter(fs);

        Assert::ExpectException<std::runtime_error>([&]() {
            adapter.observe("missing.json");
        });
    }

    TEST_METHOD(Encode_FlattensBothStates)
    {
        auto fs = std::make_shared<MockFileSystem>();
        JsonConfigAdapter adapter(fs);

        ObservedState observed;
        observed.sourceFile = "config.json";
        observed.data = nlohmann::json::parse(SAMPLE_CONFIG);

        DesiredState desired;
        desired.data = nlohmann::json::parse(SAMPLE_DESIRED);

        auto encoded = adapter.encode(observed, desired);

        Assert::IsTrue(encoded.observed.size() > 0);
        Assert::IsTrue(encoded.desired.size() > 0);
        Assert::IsTrue(encoded.observed.count("server.port") > 0);
        Assert::IsTrue(encoded.desired.count("server.port") > 0);
    }

    TEST_METHOD(ApplyRepair_ModifiesFile)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"server":{"port":9090}})";

        JsonConfigAdapter adapter(fs);

        RepairPlan plan;
        plan.planID = "TEST-001";
        plan.actions.push_back(RepairAction{
            "server.port", RepairActionType::Set, 9090, 8080, "fix port"});

        bool success = adapter.applyRepair("config.json", plan);

        Assert::IsTrue(success);
        auto updated = nlohmann::json::parse(fs->readFile("config.json"));
        Assert::AreEqual(8080, updated["server"]["port"].get<int>());
    }

    TEST_METHOD(ApplyRepair_AddsNewKey)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"server":{}})";

        JsonConfigAdapter adapter(fs);

        RepairPlan plan;
        plan.planID = "TEST-002";
        plan.actions.push_back(RepairAction{
            "server.host", RepairActionType::Add, nullptr, "localhost", "add host"});

        bool success = adapter.applyRepair("config.json", plan);

        Assert::IsTrue(success);
        auto updated = nlohmann::json::parse(fs->readFile("config.json"));
        Assert::AreEqual(std::string("localhost"), updated["server"]["host"].get<std::string>());
    }

    TEST_METHOD(ApplyRepair_MultipleActions)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = DRIFTED_CONFIG;

        JsonConfigAdapter adapter(fs);

        RepairPlan plan;
        plan.planID = "TEST-003";
        plan.actions.push_back(RepairAction{
            "server.port", RepairActionType::Set, 9090, 8080, ""});
        plan.actions.push_back(RepairAction{
            "database.port", RepairActionType::Set, 3306, 5432, ""});
        plan.actions.push_back(RepairAction{
            "database.name", RepairActionType::Set, "aeostara_dev", "aeostara_prod", ""});

        bool success = adapter.applyRepair("config.json", plan);

        Assert::IsTrue(success);
        auto updated = nlohmann::json::parse(fs->readFile("config.json"));
        Assert::AreEqual(8080, updated["server"]["port"].get<int>());
        Assert::AreEqual(5432, updated["database"]["port"].get<int>());
        Assert::AreEqual(std::string("aeostara_prod"), updated["database"]["name"].get<std::string>());
    }

    TEST_METHOD(Observe_HasTimestamp)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = SAMPLE_CONFIG;

        JsonConfigAdapter adapter(fs);
        auto observed = adapter.observe("config.json");

        Assert::IsFalse(observed.timestamp.empty());
    }
};

} // namespace AeostaraCoreTests
