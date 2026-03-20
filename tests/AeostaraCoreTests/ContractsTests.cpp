// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/Contracts.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCoreTests {

TEST_CLASS(ContractsTests)
{
public:

    // --- ObservedState ---

    TEST_METHOD(ObservedState_RoundTrip)
    {
        ObservedState state;
        state.sourceFile = "config.json";
        state.data = {{"key", "value"}};
        state.timestamp = "2026-01-01T00:00:00Z";

        nlohmann::json j = state;
        auto restored = j.get<ObservedState>();

        Assert::AreEqual(state.sourceFile, restored.sourceFile);
        Assert::AreEqual(state.timestamp, restored.timestamp);
        Assert::IsTrue(state.data == restored.data);
    }

    // --- DesiredState ---

    TEST_METHOD(DesiredState_RoundTrip)
    {
        DesiredState state;
        state.data = {{"server", {{"port", 8080}}}};
        state.source = "desired.json";

        nlohmann::json j = state;
        auto restored = j.get<DesiredState>();

        Assert::AreEqual(state.source, restored.source);
        Assert::IsTrue(state.data == restored.data);
    }

    // --- EncodedState ---

    TEST_METHOD(EncodedState_RoundTrip)
    {
        EncodedState state;
        state.observed["server.port"] = 8080;
        state.desired["server.port"] = 9090;

        nlohmann::json j = state;
        auto restored = j.get<EncodedState>();

        Assert::IsTrue(state.observed == restored.observed);
        Assert::IsTrue(state.desired == restored.desired);
    }

    // --- DriftType ---

    TEST_METHOD(DriftType_ToString)
    {
        Assert::AreEqual(std::string("ValueChanged"), to_string(DriftType::ValueChanged));
        Assert::AreEqual(std::string("KeyAdded"), to_string(DriftType::KeyAdded));
        Assert::AreEqual(std::string("KeyRemoved"), to_string(DriftType::KeyRemoved));
    }

    TEST_METHOD(DriftType_FromString)
    {
        Assert::IsTrue(DriftType::ValueChanged == driftTypeFromString("ValueChanged"));
        Assert::IsTrue(DriftType::KeyAdded == driftTypeFromString("KeyAdded"));
        Assert::IsTrue(DriftType::KeyRemoved == driftTypeFromString("KeyRemoved"));
    }

    // --- DriftEvent ---

    TEST_METHOD(DriftEvent_RoundTrip)
    {
        DriftEvent event;
        event.keyPath = "server.port";
        event.type = DriftType::ValueChanged;
        event.observedValue = 9090;
        event.desiredValue = 8080;
        event.description = "port changed";

        nlohmann::json j = event;
        auto restored = j.get<DriftEvent>();

        Assert::AreEqual(event.keyPath, restored.keyPath);
        Assert::IsTrue(event.type == restored.type);
        Assert::IsTrue(event.observedValue == restored.observedValue);
        Assert::IsTrue(event.desiredValue == restored.desiredValue);
    }

    // --- RepairAction ---

    TEST_METHOD(RepairActionType_ToString)
    {
        Assert::AreEqual(std::string("Set"), to_string(RepairActionType::Set));
        Assert::AreEqual(std::string("Add"), to_string(RepairActionType::Add));
        Assert::AreEqual(std::string("Remove"), to_string(RepairActionType::Remove));
    }

    TEST_METHOD(RepairAction_RoundTrip)
    {
        RepairAction action;
        action.keyPath = "database.port";
        action.actionType = RepairActionType::Set;
        action.fromValue = 3306;
        action.toValue = 5432;
        action.rationale = "restore standard port";

        nlohmann::json j = action;
        auto restored = j.get<RepairAction>();

        Assert::AreEqual(action.keyPath, restored.keyPath);
        Assert::IsTrue(action.actionType == restored.actionType);
        Assert::IsTrue(action.fromValue == restored.fromValue);
        Assert::IsTrue(action.toValue == restored.toValue);
    }

    // --- RepairPlan ---

    TEST_METHOD(RepairPlan_RoundTrip)
    {
        RepairPlan plan;
        plan.planID = "PLAN-001";
        plan.timestamp = "2026-01-01T00:00:00Z";
        plan.requiresBackup = true;
        plan.actions.push_back(RepairAction{
            "server.port", RepairActionType::Set, 9090, 8080, "fix port"});

        nlohmann::json j = plan;
        auto restored = j.get<RepairPlan>();

        Assert::AreEqual(plan.planID, restored.planID);
        Assert::AreEqual(plan.timestamp, restored.timestamp);
        Assert::IsTrue(plan.requiresBackup == restored.requiresBackup);
        Assert::AreEqual(plan.actions.size(), restored.actions.size());
    }

    // --- VerificationResult ---

    TEST_METHOD(VerificationResult_RoundTrip)
    {
        VerificationResult result;
        result.success = true;
        result.verifiedAt = "2026-01-01T00:00:00Z";
        result.failedChecks = {};

        nlohmann::json j = result;
        auto restored = j.get<VerificationResult>();

        Assert::IsTrue(result.success == restored.success);
        Assert::AreEqual(result.verifiedAt, restored.verifiedAt);
    }

    // --- RollbackPlan ---

    TEST_METHOD(RollbackPlan_RoundTrip)
    {
        RollbackPlan plan;
        plan.planID = "RB-001";
        plan.backupFilePath = "/backup/config.json.bak";
        plan.originalFilePath = "/etc/config.json";

        nlohmann::json j = plan;
        auto restored = j.get<RollbackPlan>();

        Assert::AreEqual(plan.planID, restored.planID);
        Assert::AreEqual(plan.backupFilePath, restored.backupFilePath);
        Assert::AreEqual(plan.originalFilePath, restored.originalFilePath);
    }

    // --- AuditEvent ---

    TEST_METHOD(AuditEventType_ToString)
    {
        Assert::AreEqual(std::string("ValidationPerformed"), to_string(AuditEventType::ValidationPerformed));
        Assert::AreEqual(std::string("HealStarted"), to_string(AuditEventType::HealStarted));
        Assert::AreEqual(std::string("PolicyBlocked"), to_string(AuditEventType::PolicyBlocked));
        Assert::AreEqual(std::string("NoDrift"), to_string(AuditEventType::NoDrift));
    }

    TEST_METHOD(AuditEvent_RoundTrip)
    {
        AuditEvent event;
        event.eventID = "EVT-001";
        event.type = AuditEventType::HealStarted;
        event.timestamp = "2026-01-01T00:00:00Z";
        event.configFile = "config.json";
        event.details = {{"action", "heal"}};

        nlohmann::json j = event;
        auto restored = j.get<AuditEvent>();

        Assert::AreEqual(event.eventID, restored.eventID);
        Assert::IsTrue(event.type == restored.type);
        Assert::AreEqual(event.configFile, restored.configFile);
    }

    // --- ModuleManifest ---

    TEST_METHOD(ModuleManifest_RoundTrip)
    {
        ModuleManifest manifest;
        manifest.moduleID = "aeostara-json";
        manifest.displayName = "Aeostara JSON Healer";
        manifest.version = "0.1.0";
        manifest.description = "JSON config healing";
        manifest.supportedConfigTypes = {"json"};

        nlohmann::json j = manifest;
        auto restored = j.get<ModuleManifest>();

        Assert::AreEqual(manifest.moduleID, restored.moduleID);
        Assert::AreEqual(manifest.displayName, restored.displayName);
        Assert::AreEqual(manifest.version, restored.version);
        Assert::AreEqual(manifest.supportedConfigTypes.size(), restored.supportedConfigTypes.size());
    }

    TEST_METHOD(ObservedState_Equality)
    {
        ObservedState a;
        a.sourceFile = "test.json";
        a.data = {{"x", 1}};
        a.timestamp = "2026-01-01T00:00:00Z";

        ObservedState b = a;
        Assert::IsTrue(a == b);

        b.sourceFile = "other.json";
        Assert::IsFalse(a == b);
    }
};

} // namespace AeostaraCoreTests
