// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/RollbackManager.h"
#include "AeostaraCore/BackupManager.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(RollbackManagerTests)
{
public:

    TEST_METHOD(CreateRollbackPlan_HasCorrectFields)
    {
        auto plan = RollbackManager::createRollbackPlan(
            "PLAN-001", "/backup/config.json.bak", "/etc/config.json");

        Assert::AreEqual(std::string("PLAN-001"), plan.planID);
        Assert::AreEqual(std::string("/backup/config.json.bak"), plan.backupFilePath);
        Assert::AreEqual(std::string("/etc/config.json"), plan.originalFilePath);
    }

    TEST_METHOD(ExecuteRollback_RestoresFromBackup)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 9999})";
        fs->files["config.json.bak"] = R"({"port": 8080})";

        auto backup = std::make_shared<BackupManager>(fs);
        RollbackManager rollback(backup);

        RollbackPlan plan;
        plan.planID = "RB-001";
        plan.backupFilePath = "config.json.bak";
        plan.originalFilePath = "config.json";

        bool success = rollback.executeRollback(plan);

        Assert::IsTrue(success);
        Assert::AreEqual(std::string(R"({"port": 8080})"), fs->readFile("config.json"));
    }

    TEST_METHOD(ExecuteRollback_MissingBackup_ReturnsFalse)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 9999})";

        auto backup = std::make_shared<BackupManager>(fs);
        RollbackManager rollback(backup);

        RollbackPlan plan;
        plan.planID = "RB-002";
        plan.backupFilePath = "nonexistent.bak";
        plan.originalFilePath = "config.json";

        bool success = rollback.executeRollback(plan);

        Assert::IsFalse(success);
    }

    TEST_METHOD(RollbackPlan_Serialization)
    {
        RollbackPlan plan;
        plan.planID = "RB-TEST";
        plan.backupFilePath = "/tmp/backup.json";
        plan.originalFilePath = "/etc/config.json";

        nlohmann::json j = plan;
        auto restored = j.get<RollbackPlan>();

        Assert::AreEqual(plan.planID, restored.planID);
        Assert::AreEqual(plan.backupFilePath, restored.backupFilePath);
        Assert::AreEqual(plan.originalFilePath, restored.originalFilePath);
    }
};

} // namespace AeostaraCoreTests
