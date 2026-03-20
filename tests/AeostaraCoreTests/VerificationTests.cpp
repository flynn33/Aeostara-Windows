// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/Verification.h"
#include "AeostaraCore/InvariantParser.h"

#include "TestHelpers.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
using namespace Aeostara::Testing;

namespace AeostaraCoreTests {

TEST_CLASS(VerificationTests)
{
public:

    TEST_METHOD(Verify_MatchingConfig_Succeeds)
    {
        MockFileSystem fs;
        fs.files["config.json"] = Testing::SAMPLE_CONFIG;

        DesiredState desired;
        desired.data = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        Verifier verifier;
        auto result = verifier.verify("config.json", desired, {}, fs);

        Assert::IsTrue(result.success);
        Assert::AreEqual(static_cast<size_t>(0), result.failedChecks.size());
    }

    TEST_METHOD(Verify_DriftedConfig_Fails)
    {
        MockFileSystem fs;
        fs.files["config.json"] = Testing::DRIFTED_CONFIG;

        DesiredState desired;
        desired.data = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        Verifier verifier;
        auto result = verifier.verify("config.json", desired, {}, fs);

        Assert::IsFalse(result.success);
        Assert::IsTrue(result.failedChecks.size() > 0);
    }

    TEST_METHOD(Verify_WithInvariants_PassesWhenSatisfied)
    {
        MockFileSystem fs;
        fs.files["config.json"] = Testing::SAMPLE_CONFIG;

        DesiredState desired;
        desired.data = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        auto invariants = InvariantParser::parse(nlohmann::json::parse(Testing::SAMPLE_INVARIANTS));

        Verifier verifier;
        auto result = verifier.verify("config.json", desired, invariants, fs);

        Assert::IsTrue(result.success);
    }

    TEST_METHOD(Verify_WithInvariants_FailsWhenViolated)
    {
        // Config has port 3306, invariant requires 5432
        MockFileSystem fs;
        fs.files["config.json"] = Testing::DRIFTED_CONFIG;

        DesiredState desired;
        desired.data = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        auto invariants = InvariantParser::parse(nlohmann::json::parse(Testing::SAMPLE_INVARIANTS));

        Verifier verifier;
        auto result = verifier.verify("config.json", desired, invariants, fs);

        Assert::IsFalse(result.success);
    }

    TEST_METHOD(Verify_HasTimestamp)
    {
        MockFileSystem fs;
        fs.files["config.json"] = Testing::SAMPLE_CONFIG;

        DesiredState desired;
        desired.data = nlohmann::json::parse(Testing::SAMPLE_DESIRED);

        Verifier verifier;
        auto result = verifier.verify("config.json", desired, {}, fs);

        Assert::IsFalse(result.verifiedAt.empty());
    }

    TEST_METHOD(Verify_NonexistentFile_Fails)
    {
        MockFileSystem fs;

        DesiredState desired;
        desired.data = {{"key", "value"}};

        Verifier verifier;
        // Should handle gracefully (fail, not crash)
        try {
            auto result = verifier.verify("missing.json", desired, {}, fs);
            Assert::IsFalse(result.success);
        } catch (const std::runtime_error&) {
            // Also acceptable: throwing on missing file
        }
    }
};

} // namespace AeostaraCoreTests
