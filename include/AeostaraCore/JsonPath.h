// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "AeostaraCore/Json.h"

#include <map>
#include <string>

namespace Aeostara {

class JsonPath final {
public:
    /// Resolve a dot-separated path (e.g., "a.b.c") to a value in the JSON tree.
    /// Returns nullptr json if the path does not exist.
    static Json get(const Json& root, const std::string& dotPath);

    /// Set a value at a dot-separated path, creating intermediate objects as needed.
    static void set(Json& root, const std::string& dotPath, const Json& value);

    /// Check if a dot-separated path exists in the JSON tree.
    static bool exists(const Json& root, const std::string& dotPath);

    /// Flatten a nested JSON object into a sorted map of dot-separated paths to values.
    /// Arrays are indexed as "key.0", "key.1", etc.
    static std::map<std::string, Json> flatten(const Json& root);

    /// Reconstruct a nested JSON object from a flattened dot-path map.
    static Json unflatten(const std::map<std::string, Json>& flat);

private:
    static void flattenRecursive(const Json& value,
                                  const std::string& prefix,
                                  std::map<std::string, Json>& result);
};

} // namespace Aeostara
