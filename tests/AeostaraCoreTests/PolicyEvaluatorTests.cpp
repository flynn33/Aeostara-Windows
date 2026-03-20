// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/PolicyEvaluator.h"
#include "AeostaraCore/InvariantParser.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCoreTests {

TEST_CLASS(PolicyEvaluatorTests)
{
public:

    TEST_METHOD(AllowsRepair_WhenInvariantsPass)
    {
        auto invariants = InvariantParser::parse(nlohmann::json::parse(Testing::SAMPLE_INVARIANTS));

        // Desired state satisfies invariants: port=5432, ssl=true
        std::map<std::string, nlohmann::json> desiredFlat;
        desiredFlat["database.port"] = 5432;
        desiredFlat["server.ssl_enabled"] = true;

        RepairPlan plan;
        plan.planID = "TEST-001";

        PolicyEvaluator evaluator;
        auto decision = evaluator.evaluate(plan, invariants, desiredFlat);

        Assert::IsTrue(decision.allowed);
        Assert::AreEqual(static_cast<size_t>(0), decision.violations.size());
    }

    TEST_METHOD(BlocksRepair_WhenCriticalInvariantViolated)
    {
        auto invariants = InvariantParser::parse(nlohmann::json::parse(Testing::BLOCKING_INVARIANTS));

        // Desired state violates: port must be 3306, but desired is 5432
        std::map<std::string, nlohmann::json> desiredFlat;
        desiredFlat["database.port"] = 5432;

        RepairPlan plan;
        plan.planID = "TEST-002";

        PolicyEvaluator evaluator;
        auto decision = evaluator.evaluate(plan, invariants, desiredFlat);

        Assert::IsFalse(decision.allowed);
        Assert::IsTrue(decision.violations.size() > 0);
    }

    TEST_METHOD(EvaluateExpression_EqualityTrue)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["database.port"] = 5432;

        Assert::IsTrue(PolicyEvaluator::evaluateExpression("database.port == 5432", flat));
    }

    TEST_METHOD(EvaluateExpression_EqualityFalse)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["database.port"] = 3306;

        Assert::IsFalse(PolicyEvaluator::evaluateExpression("database.port == 5432", flat));
    }

    TEST_METHOD(EvaluateExpression_NotEqual)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["database.port"] = 3306;

        Assert::IsTrue(PolicyEvaluator::evaluateExpression("database.port != 5432", flat));
    }

    TEST_METHOD(EvaluateExpression_BooleanTrue)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["server.ssl_enabled"] = true;

        Assert::IsTrue(PolicyEvaluator::evaluateExpression("server.ssl_enabled == true", flat));
    }

    TEST_METHOD(EvaluateExpression_BooleanFalse)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["server.ssl_enabled"] = false;

        Assert::IsFalse(PolicyEvaluator::evaluateExpression("server.ssl_enabled == true", flat));
    }

    TEST_METHOD(EvaluateExpression_GreaterThan)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["server.port"] = 9090;

        Assert::IsTrue(PolicyEvaluator::evaluateExpression("server.port > 8080", flat));
    }

    TEST_METHOD(EvaluateExpression_LessThan)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["server.port"] = 80;

        Assert::IsTrue(PolicyEvaluator::evaluateExpression("server.port < 8080", flat));
    }

    TEST_METHOD(EvaluateExpression_MissingKey_ReturnsFalse)
    {
        std::map<std::string, nlohmann::json> flat;

        Assert::IsFalse(PolicyEvaluator::evaluateExpression("nonexistent.key == 5432", flat));
    }

    TEST_METHOD(ParseExpression_Equality)
    {
        auto parsed = PolicyEvaluator::parseExpression("database.port == 5432");
        Assert::AreEqual(std::string("database.port"), parsed.keyPath);
        Assert::AreEqual(std::string("=="), parsed.op);
        Assert::AreEqual(5432, parsed.expectedValue.get<int>());
    }

    TEST_METHOD(ParseExpression_StringValue)
    {
        auto parsed = PolicyEvaluator::parseExpression("logging.level == INFO");
        Assert::AreEqual(std::string("logging.level"), parsed.keyPath);
        Assert::AreEqual(std::string("=="), parsed.op);
    }

    TEST_METHOD(NoInvariants_AllowsAll)
    {
        std::vector<Invariant> empty;
        std::map<std::string, nlohmann::json> desiredFlat;
        desiredFlat["x"] = 1;

        RepairPlan plan;
        plan.planID = "TEST-003";

        PolicyEvaluator evaluator;
        auto decision = evaluator.evaluate(plan, empty, desiredFlat);

        Assert::IsTrue(decision.allowed);
    }
};

} // namespace AeostaraCoreTests
