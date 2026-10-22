// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "AeostaraCore/SdkVersion.h"

namespace Aeostara {

struct AeostaraVersion final {
    static constexpr int major = AEOSTARA_VERSION_MAJOR;
    static constexpr int minor = AEOSTARA_VERSION_MINOR;
    static constexpr int patch = AEOSTARA_VERSION_PATCH;

    static std::string toString();
};

} // namespace Aeostara
