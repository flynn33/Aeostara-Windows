// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "CppUnitTest.h"
#include "AeostaraCore/Json.h"
#include <algorithm>
#include <stdexcept>
#include "JsonAggregateChecks.inc"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace AeostaraCoreTests {
TEST_CLASS(JsonAggregateTests) {
public:
    TEST_METHOD(CapturedSourcesHaveIndependentDocumentLimits) { Assert::AreEqual(20u, runJsonAggregateChecks()); }
};
}
