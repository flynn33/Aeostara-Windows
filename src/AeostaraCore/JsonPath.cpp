// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#include "AeostaraCore/JsonPath.h"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace Aeostara {

namespace {

std::vector<std::string> splitPath(const std::string& dotPath) {
    std::vector<std::string> parts;
    std::istringstream stream(dotPath);
    std::string part;
    while (std::getline(stream, part, '.')) {
        if (!part.empty()) {
            parts.push_back(part);
        }
    }
    return parts;
}

bool isArrayIndex(const std::string& key) {
    if (key.empty()) return false;
    for (char c : key) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

} // anonymous namespace

Json JsonPath::get(const Json& root, const std::string& dotPath) {
    auto parts = splitPath(dotPath);
    const Json* current = &root;

    for (const auto& part : parts) {
        if (current->is_object()) {
            const auto& members = current->object_items();
            auto it = members.find(part);
            if (it == members.end()) {
                return nullptr;
            }
            current = &it->second;
        } else if (current->is_array() && isArrayIndex(part)) {
            size_t index = std::stoull(part);
            if (index >= current->size()) {
                return nullptr;
            }
            current = &(*current)[index];
        } else {
            return nullptr;
        }
    }

    return *current;
}

void JsonPath::set(Json& root, const std::string& dotPath, const Json& value) {
    auto parts = splitPath(dotPath);
    if (parts.empty()) return;

    Json* current = &root;

    for (size_t i = 0; i < parts.size() - 1; ++i) {
        const auto& part = parts[i];
        const auto& nextPart = parts[i + 1];

        if (current->is_null() || (!current->is_object() && !current->is_array())) {
            if (isArrayIndex(nextPart)) {
                (*current)[part] = Json::array();
            } else {
                (*current)[part] = Json::object();
            }
        } else if (current->is_object()) {
            if (!current->contains(part)) {
                if (isArrayIndex(nextPart)) {
                    (*current)[part] = Json::array();
                } else {
                    (*current)[part] = Json::object();
                }
            }
        }

        if (current->is_object()) {
            current = &(*current)[part];
        } else if (current->is_array() && isArrayIndex(part)) {
            size_t index = std::stoull(part);
            while (current->size() <= index) {
                current->push_back(Json());
            }
            current = &(*current)[index];
        }
    }

    const auto& lastPart = parts.back();
    if (current->is_array() && isArrayIndex(lastPart)) {
        size_t index = std::stoull(lastPart);
        while (current->size() <= index) {
            current->push_back(Json());
        }
        (*current)[index] = value;
    } else {
        (*current)[lastPart] = value;
    }
}

bool JsonPath::exists(const Json& root, const std::string& dotPath) {
    auto parts = splitPath(dotPath);
    const Json* current = &root;

    for (const auto& part : parts) {
        if (current->is_object()) {
            const auto& members = current->object_items();
            auto it = members.find(part);
            if (it == members.end()) return false;
            current = &it->second;
        } else if (current->is_array() && isArrayIndex(part)) {
            size_t index = std::stoull(part);
            if (index >= current->size()) return false;
            current = &(*current)[index];
        } else {
            return false;
        }
    }

    return true;
}

void JsonPath::flattenRecursive(const Json& value,
                                 const std::string& prefix,
                                 std::map<std::string, Json>& result) {
    if (value.is_object()) {
        for (const auto& [member, child] : value.object_items()) {
            std::string key = prefix.empty() ? member : prefix + "." + member;
            flattenRecursive(child, key, result);
        }
    } else if (value.is_array()) {
        for (size_t i = 0; i < value.size(); ++i) {
            std::string key = prefix + "." + std::to_string(i);
            flattenRecursive(value[i], key, result);
        }
    } else {
        result[prefix] = value;
    }
}

std::map<std::string, Json> JsonPath::flatten(const Json& root) {
    std::map<std::string, Json> result;
    if (root.is_object() || root.is_array()) {
        flattenRecursive(root, "", result);
    }
    return result;
}

Json JsonPath::unflatten(const std::map<std::string, Json>& flat) {
    Json root = Json::object();
    for (const auto& [path, value] : flat) {
        set(root, path, value);
    }
    return root;
}

} // namespace Aeostara
