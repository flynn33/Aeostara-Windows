// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "CommandParser.h"

#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCLITests {

// Helper to simulate argc/argv from a vector of strings
struct ArgvHelper {
    std::vector<std::string> args;
    std::vector<char*> ptrs;

    ArgvHelper(std::initializer_list<std::string> list) : args(list) {
        for (auto& s : args) {
            ptrs.push_back(s.data());
        }
    }

    int argc() const { return static_cast<int>(ptrs.size()); }
    char** argv() { return ptrs.data(); }
};

TEST_CLASS(CommandParserTests)
{
public:

    TEST_METHOD(Parse_ValidateCommand)
    {
        ArgvHelper args{"aeostara", "validate", "config.json", "--desired", "desired.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("validate"), cmd.command);
        Assert::AreEqual(std::string("config.json"), cmd.configPath);
        Assert::AreEqual(std::string("desired.json"), cmd.desiredPath);
    }

    TEST_METHOD(Parse_DiffCommand)
    {
        ArgvHelper args{"aeostara", "diff", "config.json", "--desired", "desired.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("diff"), cmd.command);
    }

    TEST_METHOD(Parse_HealCommand)
    {
        ArgvHelper args{"aeostara", "heal", "config.json", "--desired", "desired.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("heal"), cmd.command);
    }

    TEST_METHOD(Parse_WithInvariants)
    {
        ArgvHelper args{"aeostara", "validate", "config.json",
                        "--desired", "desired.json",
                        "--invariants", "invariants.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("invariants.json"), cmd.invariantsPath);
    }

    TEST_METHOD(Parse_WithAuditPath)
    {
        ArgvHelper args{"aeostara", "heal", "config.json",
                        "--desired", "desired.json",
                        "--audit", "my-audit.jsonl"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("my-audit.jsonl"), cmd.auditPath);
    }

    TEST_METHOD(Parse_NoArgs_ShowsHelp)
    {
        ArgvHelper args{"aeostara"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.help);
    }

    TEST_METHOD(Parse_UnknownCommand_Invalid)
    {
        ArgvHelper args{"aeostara", "destroy", "config.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsFalse(cmd.valid);
    }

    TEST_METHOD(Parse_MissingDesired_Invalid)
    {
        ArgvHelper args{"aeostara", "validate", "config.json"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsFalse(cmd.valid);
    }

    TEST_METHOD(Parse_HelpFlag)
    {
        ArgvHelper args{"aeostara", "--help"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.help);
    }

    TEST_METHOD(Parse_AllOptions)
    {
        ArgvHelper args{"aeostara", "heal", "config.json",
                        "--desired", "desired.json",
                        "--invariants", "inv.json",
                        "--audit", "audit.jsonl"};
        auto cmd = CommandParser::parse(args.argc(), args.argv());

        Assert::IsTrue(cmd.valid);
        Assert::AreEqual(std::string("heal"), cmd.command);
        Assert::AreEqual(std::string("config.json"), cmd.configPath);
        Assert::AreEqual(std::string("desired.json"), cmd.desiredPath);
        Assert::AreEqual(std::string("inv.json"), cmd.invariantsPath);
        Assert::AreEqual(std::string("audit.jsonl"), cmd.auditPath);
    }
};

} // namespace AeostaraCLITests
