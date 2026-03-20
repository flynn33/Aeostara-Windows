// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/DriftAnalyzer.h"
#include "AeostaraCore/JsonPath.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCoreTests {

TEST_CLASS(DriftAnalyzerTests)
{
public:

    TEST_METHOD(NoDrift_WhenStatesMatch)
    {
        auto config = nlohmann::json::parse(Testing::SAMPLE_CONFIG);
        auto desired = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        EncodedState state;
        state.observed = JsonPath::flatten(config);
        state.desired = JsonPath::flatten(desired);

        DriftAnalyzer analyzer(state);
        Assert::IsFalse(analyzer.hasDrift());

        auto drifts = analyzer.analyze();
        Assert::AreEqual(static_cast<size_t>(0), drifts.size());
    }

    TEST_METHOD(DetectsDrift_WhenValuesChange)
    {
        auto config = nlohmann::json::parse(Testing::DRIFTED_CONFIG);
        auto desired = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        EncodedState state;
        state.observed = JsonPath::flatten(config);
        state.desired = JsonPath::flatten(desired);

        DriftAnalyzer analyzer(state);
        Assert::IsTrue(analyzer.hasDrift());

        auto drifts = analyzer.analyze();
        // Drifted: server.port (9090→8080), database.port (3306→5432),
        //          database.name (dev→prod), logging.level (DEBUG→INFO)
        Assert::IsTrue(drifts.size() >= 3);
    }

    TEST_METHOD(DriftEvent_HasCorrectType)
    {
        EncodedState state;
        state.observed["server.port"] = 9090;
        state.desired["server.port"] = 8080;

        DriftAnalyzer analyzer(state);
        auto drifts = analyzer.analyze();

        Assert::AreEqual(static_cast<size_t>(1), drifts.size());
        Assert::IsTrue(DriftType::ValueChanged == drifts[0].type);
        Assert::AreEqual(std::string("server.port"), drifts[0].keyPath);
    }

    TEST_METHOD(DetectsKeyAdded_WhenDesiredHasNewKey)
    {
        EncodedState state;
        state.observed["server.port"] = 8080;
        state.desired["server.port"] = 8080;
        state.desired["server.host"] = "localhost";

        DriftAnalyzer analyzer(state);
        auto drifts = analyzer.analyze();

        bool foundAdded = false;
        for (const auto& d : drifts) {
            if (d.keyPath == "server.host" && d.type == DriftType::KeyAdded) {
                foundAdded = true;
            }
        }
        Assert::IsTrue(foundAdded);
    }

    TEST_METHOD(DetectsKeyRemoved_WhenObservedHasExtraKey)
    {
        EncodedState state;
        state.observed["server.port"] = 8080;
        state.observed["server.debug"] = true;
        state.desired["server.port"] = 8080;

        DriftAnalyzer analyzer(state);
        auto drifts = analyzer.analyze();

        bool foundRemoved = false;
        for (const auto& d : drifts) {
            if (d.keyPath == "server.debug" && d.type == DriftType::KeyRemoved) {
                foundRemoved = true;
            }
        }
        Assert::IsTrue(foundRemoved);
    }

    TEST_METHOD(EmptyStates_NoDrift)
    {
        EncodedState state;
        DriftAnalyzer analyzer(state);

        Assert::IsFalse(analyzer.hasDrift());
        Assert::AreEqual(static_cast<size_t>(0), analyzer.analyze().size());
    }

    TEST_METHOD(DriftEvent_ContainsObservedAndDesiredValues)
    {
        EncodedState state;
        state.observed["database.port"] = 3306;
        state.desired["database.port"] = 5432;

        DriftAnalyzer analyzer(state);
        auto drifts = analyzer.analyze();

        Assert::AreEqual(static_cast<size_t>(1), drifts.size());
        Assert::AreEqual(3306, drifts[0].observedValue.get<int>());
        Assert::AreEqual(5432, drifts[0].desiredValue.get<int>());
    }
};

} // namespace AeostaraCoreTests
