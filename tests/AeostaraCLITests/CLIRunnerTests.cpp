// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "CppUnitTest.h"
#include "CLIRunner.h"
#include <iostream>
#include <sstream>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AeostaraCLITests {
class FailedEngine final : public Aeostara::IHealingEngine {
public:
    ValidationResult validate(const std::string&, const std::string&, const std::string&) override {
        ValidationResult result;
        result.failure.emplace(Aeostara::OperationPhase::Parse, Aeostara::OperationCode::InvalidInput, "invalid JSON");
        return result;
    }
    DiffResult diff(const std::string&, const std::string&, const std::string&) override {
        DiffResult result;
        result.failure.emplace(Aeostara::OperationPhase::Plan, Aeostara::OperationCode::Blocked, "authority absent");
        return result;
    }
    HealResult heal(const std::string&, const std::string&, const std::string&, const std::string&) override {
        HealResult result;
        result.failure.emplace(Aeostara::OperationPhase::Rollback, Aeostara::OperationCode::RestorationFailed, "restore failed");
        result.failure->mutationState = Aeostara::MutationState::RestoreFailed;
        result.mutationMayHaveOccurred = true;
        result.restoration = Aeostara::RestorationStatus::Failed;
        return result;
    }
    ValidationResult validateSemantic(const std::string&, const Aeostara::Json&) override { return validate({}, {}, {}); }
    DiffResult diffSemantic(const std::string&, const Aeostara::Json&) override { return diff({}, {}, {}); }
    HealResult healSemantic(const std::string&, const Aeostara::Json&, const std::string&) override { return heal({}, {}, {}, {}); }
};
class CapturedOutput final {
public:
    std::ostringstream text;
private:
    std::streambuf* original_;
public:
    CapturedOutput() : original_(std::cout.rdbuf(text.rdbuf())) {}
    ~CapturedOutput() { std::cout.rdbuf(original_); }
};
TEST_CLASS(CLIRunnerTests) {
public:
    TEST_METHOD(EmptyDiffWithBlockedPlanCannotReportSuccess) {
        CapturedOutput captured;
        Aeostara::ParsedCommand command; command.command = "diff";
        Aeostara::CLIRunner runner(std::make_shared<FailedEngine>());
        Assert::AreEqual(1, runner.run(command));
        const auto output = Aeostara::Json::parse(captured.text.str());
        Assert::AreEqual(std::string("BLOCKED"), output.at("failure").at("code").get<std::string>());
    }
    TEST_METHOD(ParseFailureRetainsOuterFailureAndErrorExit) {
        CapturedOutput captured;
        Aeostara::ParsedCommand command; command.command = "validate";
        Aeostara::CLIRunner runner(std::make_shared<FailedEngine>());
        Assert::AreEqual(2, runner.run(command));
        const auto output = Aeostara::Json::parse(captured.text.str());
        Assert::AreEqual(std::string("PARSE"), output.at("failure").at("phase").get<std::string>());
    }
    TEST_METHOD(FailedRestorationRetainsMutationStateAndErrorExit) {
        CapturedOutput captured;
        Aeostara::ParsedCommand command; command.command = "heal";
        Aeostara::CLIRunner runner(std::make_shared<FailedEngine>());
        Assert::AreEqual(2, runner.run(command));
        const auto output = Aeostara::Json::parse(captured.text.str());
        Assert::IsTrue(output.at("mutationMayHaveOccurred").get<bool>());
        Assert::AreEqual(std::string("RESTORE_FAILED"), output.at("failure").at("mutationState").get<std::string>());
    }
};
}
