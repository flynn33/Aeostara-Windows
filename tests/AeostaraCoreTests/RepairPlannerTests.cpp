// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/RepairPlanner.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCoreTests {

TEST_CLASS(RepairPlannerTests)
{
public:

    TEST_METHOD(EmptyDrifts_ProducesEmptyPlan)
    {
        RepairPlanner planner;
        auto plan = planner.createPlan({});

        Assert::AreEqual(static_cast<size_t>(0), plan.actions.size());
        Assert::IsFalse(plan.planID.empty());
    }

    TEST_METHOD(ValueChanged_ProducesSetAction)
    {
        DriftEvent drift;
        drift.keyPath = "server.port";
        drift.type = DriftType::ValueChanged;
        drift.observedValue = 9090;
        drift.desiredValue = 8080;
        drift.description = "port drifted";

        RepairPlanner planner;
        auto plan = planner.createPlan({drift});

        Assert::AreEqual(static_cast<size_t>(1), plan.actions.size());
        Assert::IsTrue(RepairActionType::Set == plan.actions[0].actionType);
        Assert::AreEqual(std::string("server.port"), plan.actions[0].keyPath);
        Assert::AreEqual(9090, plan.actions[0].fromValue.get<int>());
        Assert::AreEqual(8080, plan.actions[0].toValue.get<int>());
    }

    TEST_METHOD(KeyAdded_ProducesAddAction)
    {
        DriftEvent drift;
        drift.keyPath = "server.host";
        drift.type = DriftType::KeyAdded;
        drift.observedValue = nullptr;
        drift.desiredValue = "localhost";
        drift.description = "key added";

        RepairPlanner planner;
        auto plan = planner.createPlan({drift});

        Assert::AreEqual(static_cast<size_t>(1), plan.actions.size());
        Assert::IsTrue(RepairActionType::Add == plan.actions[0].actionType);
    }

    TEST_METHOD(KeyRemoved_IsIgnored)
    {
        DriftEvent drift;
        drift.keyPath = "server.debug";
        drift.type = DriftType::KeyRemoved;
        drift.observedValue = true;
        drift.desiredValue = nullptr;
        drift.description = "key removed";

        RepairPlanner planner;
        auto plan = planner.createPlan({drift});

        Assert::AreEqual(static_cast<size_t>(0), plan.actions.size());
    }

    TEST_METHOD(Actions_SortedByKeyPath)
    {
        std::vector<DriftEvent> drifts;
        drifts.push_back(DriftEvent{"z.key", DriftType::ValueChanged, 1, 2, ""});
        drifts.push_back(DriftEvent{"a.key", DriftType::ValueChanged, 3, 4, ""});
        drifts.push_back(DriftEvent{"m.key", DriftType::ValueChanged, 5, 6, ""});

        RepairPlanner planner;
        auto plan = planner.createPlan(drifts);

        Assert::AreEqual(static_cast<size_t>(3), plan.actions.size());
        Assert::AreEqual(std::string("a.key"), plan.actions[0].keyPath);
        Assert::AreEqual(std::string("m.key"), plan.actions[1].keyPath);
        Assert::AreEqual(std::string("z.key"), plan.actions[2].keyPath);
    }

    TEST_METHOD(PlanID_IsDeterministic)
    {
        DriftEvent drift;
        drift.keyPath = "server.port";
        drift.type = DriftType::ValueChanged;
        drift.observedValue = 9090;
        drift.desiredValue = 8080;
        drift.description = "";

        RepairPlanner planner;
        auto plan1 = planner.createPlan({drift});
        auto plan2 = planner.createPlan({drift});

        Assert::AreEqual(plan1.planID, plan2.planID);
    }

    TEST_METHOD(PlanID_DiffersForDifferentActions)
    {
        DriftEvent drift1;
        drift1.keyPath = "server.port";
        drift1.type = DriftType::ValueChanged;
        drift1.observedValue = 9090;
        drift1.desiredValue = 8080;
        drift1.description = "";

        DriftEvent drift2;
        drift2.keyPath = "database.port";
        drift2.type = DriftType::ValueChanged;
        drift2.observedValue = 3306;
        drift2.desiredValue = 5432;
        drift2.description = "";

        RepairPlanner planner;
        auto plan1 = planner.createPlan({drift1});
        auto plan2 = planner.createPlan({drift2});

        Assert::AreNotEqual(plan1.planID, plan2.planID);
    }

    TEST_METHOD(Plan_RequiresBackup)
    {
        DriftEvent drift;
        drift.keyPath = "server.port";
        drift.type = DriftType::ValueChanged;
        drift.observedValue = 9090;
        drift.desiredValue = 8080;
        drift.description = "";

        RepairPlanner planner;
        auto plan = planner.createPlan({drift});
        Assert::IsTrue(plan.requiresBackup);
    }

    TEST_METHOD(Plan_HasTimestamp)
    {
        RepairPlanner planner;
        auto plan = planner.createPlan({});
        Assert::IsFalse(plan.timestamp.empty());
    }
};

} // namespace AeostaraCoreTests
