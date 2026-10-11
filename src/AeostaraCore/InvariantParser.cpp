// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual

#include "AeostaraCore/InvariantParser.h"
#include "AeostaraCore/IFileSystem.h"

#include <stdexcept>

namespace Aeostara {

std::vector<Invariant> InvariantParser::parseFile(const std::string& path, IFileSystem& fs) {
    std::string content = fs.readFile(path);
    Json j = Json::parse(content);
    return parse(j);
}

std::vector<Invariant> InvariantParser::parse(const Json& j) {
    if (!j.is_array()) {
        throw std::invalid_argument("Invariants file must contain a JSON array");
    }

    std::vector<Invariant> invariants;
    invariants.reserve(j.size());

    for (const auto& item : j.array_items()) {
        invariants.push_back(item.get<Invariant>());
    }

    return invariants;
}

} // namespace Aeostara
