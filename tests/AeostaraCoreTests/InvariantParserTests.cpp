// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/InvariantParser.h"
#include "AeostaraCore/IFileSystem.h"
#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(InvariantParserTests)
{
public:

    TEST_METHOD(Parse_SampleInvariants)
    {
        auto j = nlohmann::json::parse(SAMPLE_INVARIANTS);
        auto invariants = InvariantParser::parse(j);

        Assert::AreEqual(static_cast<size_t>(2), invariants.size());
    }

    TEST_METHOD(Parse_FirstInvariant_HasCorrectFields)
    {
        auto j = nlohmann::json::parse(SAMPLE_INVARIANTS);
        auto invariants = InvariantParser::parse(j);

        Assert::AreEqual(std::string("INV-001"), invariants[0].invariantID);
        Assert::AreEqual(std::string("Database Port Standard"), invariants[0].name);
        Assert::AreEqual(std::string("database.port == 5432"), invariants[0].expression);
        Assert::IsTrue(InvariantSeverity::High == invariants[0].severity);
        Assert::IsTrue(invariants[0].autoRemediate);
    }

    TEST_METHOD(Parse_SecondInvariant_IsCritical)
    {
        auto j = nlohmann::json::parse(SAMPLE_INVARIANTS);
        auto invariants = InvariantParser::parse(j);

        Assert::AreEqual(std::string("INV-002"), invariants[1].invariantID);
        Assert::IsTrue(InvariantSeverity::Critical == invariants[1].severity);
        Assert::IsTrue(invariants[1].autoRemediate);
    }

    TEST_METHOD(ParseFile_FromMockFileSystem)
    {
        MockFileSystem fs;
        fs.files["invariants.json"] = SAMPLE_INVARIANTS;

        auto invariants = InvariantParser::parseFile("invariants.json", fs);

        Assert::AreEqual(static_cast<size_t>(2), invariants.size());
        Assert::AreEqual(std::string("INV-001"), invariants[0].invariantID);
    }

    TEST_METHOD(ParseFile_NonexistentFile_Throws)
    {
        MockFileSystem fs;

        Assert::ExpectException<std::runtime_error>([&]() {
            InvariantParser::parseFile("missing.json", fs);
        });
    }

    TEST_METHOD(Parse_EmptyArray)
    {
        auto j = nlohmann::json::parse("[]");
        auto invariants = InvariantParser::parse(j);
        Assert::AreEqual(static_cast<size_t>(0), invariants.size());
    }

    TEST_METHOD(Parse_BlockingInvariant_NotAutoRemediate)
    {
        auto j = nlohmann::json::parse(BLOCKING_INVARIANTS);
        auto invariants = InvariantParser::parse(j);

        Assert::AreEqual(static_cast<size_t>(1), invariants.size());
        Assert::IsFalse(invariants[0].autoRemediate);
        Assert::IsTrue(InvariantSeverity::Critical == invariants[0].severity);
    }

    TEST_METHOD(Invariant_RoundTrip)
    {
        Invariant inv;
        inv.invariantID = "INV-TEST";
        inv.name = "Test Rule";
        inv.description = "A test invariant";
        inv.severity = InvariantSeverity::High;
        inv.expression = "server.port == 8080";
        inv.appliesTo = {"server"};
        inv.autoRemediate = true;

        nlohmann::json j = inv;
        auto restored = j.get<Invariant>();

        Assert::AreEqual(inv.invariantID, restored.invariantID);
        Assert::AreEqual(inv.name, restored.name);
        Assert::AreEqual(inv.expression, restored.expression);
        Assert::IsTrue(inv.severity == restored.severity);
        Assert::IsTrue(inv.autoRemediate == restored.autoRemediate);
    }

    TEST_METHOD(InvariantViolation_RoundTrip)
    {
        InvariantViolation viol;
        viol.invariantID = "INV-001";
        viol.keyPath = "database.port";
        viol.actualValue = 3306;
        viol.expectedValue = 5432;
        viol.severity = InvariantSeverity::High;

        nlohmann::json j = viol;
        auto restored = j.get<InvariantViolation>();

        Assert::AreEqual(viol.invariantID, restored.invariantID);
        Assert::AreEqual(viol.keyPath, restored.keyPath);
        Assert::IsTrue(viol.actualValue == restored.actualValue);
    }
};

} // namespace AeostaraCoreTests
