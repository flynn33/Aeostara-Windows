// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "CppUnitTest.h"
#include "AeostaraCore/WindowsAtomicFileSystem.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
using namespace Aeostara;
using namespace Aeostara::BaseBindings;
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
#include "WindowsAtomicDefaultChecks.inc"
namespace AeostaraCoreTests {
TEST_CLASS(WindowsAtomicFileSystemTests) {
public:
    TEST_METHOD(UnscopedMutationsFailClosedAndReadsAreMeasured) { Assert::AreEqual(23u, runWindowsAtomicDefaultChecks()); }
};
}
