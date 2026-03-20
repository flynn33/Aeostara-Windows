// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/BackupManager.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(BackupManagerTests)
{
public:

    TEST_METHOD(CreateBackup_CopiesFile)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 8080})";

        BackupManager manager(fs);
        auto backupPath = manager.createBackup("config.json");

        Assert::IsFalse(backupPath.empty());
        Assert::IsTrue(fs->fileExists(backupPath));
        Assert::AreEqual(std::string(R"({"port": 8080})"), fs->readFile(backupPath));
    }

    TEST_METHOD(CreateBackup_OriginalUnchanged)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 8080})";

        BackupManager manager(fs);
        manager.createBackup("config.json");

        Assert::AreEqual(std::string(R"({"port": 8080})"), fs->readFile("config.json"));
    }

    TEST_METHOD(RestoreBackup_RestoresContent)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 9090})";
        fs->files["config.json.bak"] = R"({"port": 8080})";

        BackupManager manager(fs);
        bool success = manager.restoreBackup("config.json.bak", "config.json");

        Assert::IsTrue(success);
        Assert::AreEqual(std::string(R"({"port": 8080})"), fs->readFile("config.json"));
    }

    TEST_METHOD(RestoreBackup_MissingBackup_ReturnsFalse)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["config.json"] = R"({"port": 9090})";

        BackupManager manager(fs);
        bool success = manager.restoreBackup("nonexistent.bak", "config.json");

        Assert::IsFalse(success);
    }

    TEST_METHOD(CreateBackup_NonexistentFile_BackupIsEmpty)
    {
        auto fs = std::make_shared<MockFileSystem>();
        BackupManager manager(fs);

        // copyFile returns false for nonexistent source, backup path still generated
        auto backupPath = manager.createBackup("missing.json");
        Assert::IsFalse(backupPath.empty());
        // The backup file should not exist since source was missing
        Assert::IsFalse(fs->fileExists(backupPath));
    }

    TEST_METHOD(BackupPath_ContainsOriginalName)
    {
        auto fs = std::make_shared<MockFileSystem>();
        fs->files["myconfig.json"] = "{}";

        BackupManager manager(fs);
        auto backupPath = manager.createBackup("myconfig.json");

        // Backup path should reference the original filename
        Assert::IsTrue(backupPath.find("myconfig") != std::string::npos);
    }
};

} // namespace AeostaraCoreTests
