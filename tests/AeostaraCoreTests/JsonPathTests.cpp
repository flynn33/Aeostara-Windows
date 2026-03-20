// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// Proprietary and Confidential.

#include "CppUnitTest.h"

#include "AeostaraCore/JsonPath.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;

namespace AeostaraCoreTests {

TEST_CLASS(JsonPathTests)
{
public:

    TEST_METHOD(Get_TopLevelKey)
    {
        auto j = nlohmann::json::parse(R"({"name":"Alice"})");
        auto result = JsonPath::get(j, "name");
        Assert::AreEqual(std::string("Alice"), result.get<std::string>());
    }

    TEST_METHOD(Get_NestedKey)
    {
        auto j = nlohmann::json::parse(R"({"server":{"port":8080}})");
        auto result = JsonPath::get(j, "server.port");
        Assert::AreEqual(8080, result.get<int>());
    }

    TEST_METHOD(Get_DeeplyNestedKey)
    {
        auto j = nlohmann::json::parse(R"({"a":{"b":{"c":{"d":42}}}})");
        auto result = JsonPath::get(j, "a.b.c.d");
        Assert::AreEqual(42, result.get<int>());
    }

    TEST_METHOD(Get_NonexistentPath_ReturnsNull)
    {
        auto j = nlohmann::json::parse(R"({"server":{"port":8080}})");
        auto result = JsonPath::get(j, "server.host");
        Assert::IsTrue(result.is_null());
    }

    TEST_METHOD(Set_TopLevelKey)
    {
        nlohmann::json j = nlohmann::json::object();
        JsonPath::set(j, "name", "Bob");
        Assert::AreEqual(std::string("Bob"), j["name"].get<std::string>());
    }

    TEST_METHOD(Set_NestedKey_CreatesIntermediates)
    {
        nlohmann::json j = nlohmann::json::object();
        JsonPath::set(j, "server.port", 9090);
        Assert::AreEqual(9090, j["server"]["port"].get<int>());
    }

    TEST_METHOD(Set_OverwritesExistingValue)
    {
        auto j = nlohmann::json::parse(R"({"server":{"port":8080}})");
        JsonPath::set(j, "server.port", 9090);
        Assert::AreEqual(9090, j["server"]["port"].get<int>());
    }

    TEST_METHOD(Exists_ReturnsTrueForPresent)
    {
        auto j = nlohmann::json::parse(R"({"server":{"port":8080}})");
        Assert::IsTrue(JsonPath::exists(j, "server.port"));
        Assert::IsTrue(JsonPath::exists(j, "server"));
    }

    TEST_METHOD(Exists_ReturnsFalseForAbsent)
    {
        auto j = nlohmann::json::parse(R"({"server":{"port":8080}})");
        Assert::IsFalse(JsonPath::exists(j, "server.host"));
        Assert::IsFalse(JsonPath::exists(j, "database"));
    }

    TEST_METHOD(Flatten_SimpleObject)
    {
        auto j = nlohmann::json::parse(R"({"a":1,"b":"two"})");
        auto flat = JsonPath::flatten(j);

        Assert::AreEqual(static_cast<size_t>(2), flat.size());
        Assert::AreEqual(1, flat["a"].get<int>());
        Assert::AreEqual(std::string("two"), flat["b"].get<std::string>());
    }

    TEST_METHOD(Flatten_NestedObject)
    {
        auto j = nlohmann::json::parse(R"({"server":{"host":"localhost","port":8080}})");
        auto flat = JsonPath::flatten(j);

        Assert::AreEqual(static_cast<size_t>(2), flat.size());
        Assert::AreEqual(std::string("localhost"), flat["server.host"].get<std::string>());
        Assert::AreEqual(8080, flat["server.port"].get<int>());
    }

    TEST_METHOD(Flatten_ArrayElements)
    {
        auto j = nlohmann::json::parse(R"({"tags":["a","b","c"]})");
        auto flat = JsonPath::flatten(j);

        Assert::AreEqual(static_cast<size_t>(3), flat.size());
        Assert::AreEqual(std::string("a"), flat["tags.0"].get<std::string>());
        Assert::AreEqual(std::string("b"), flat["tags.1"].get<std::string>());
        Assert::AreEqual(std::string("c"), flat["tags.2"].get<std::string>());
    }

    TEST_METHOD(Unflatten_ReconstructsNestedObject)
    {
        std::map<std::string, nlohmann::json> flat;
        flat["server.host"] = "localhost";
        flat["server.port"] = 8080;
        flat["database.name"] = "prod";

        auto result = JsonPath::unflatten(flat);

        Assert::AreEqual(std::string("localhost"), result["server"]["host"].get<std::string>());
        Assert::AreEqual(8080, result["server"]["port"].get<int>());
        Assert::AreEqual(std::string("prod"), result["database"]["name"].get<std::string>());
    }

    TEST_METHOD(Flatten_Unflatten_RoundTrip)
    {
        auto j = nlohmann::json::parse(R"({"server":{"host":"localhost","port":8080,"ssl_enabled":true}})");
        auto flat = JsonPath::flatten(j);
        auto restored = JsonPath::unflatten(flat);
        Assert::IsTrue(j == restored);
    }

    TEST_METHOD(Flatten_EmptyObject)
    {
        auto j = nlohmann::json::object();
        auto flat = JsonPath::flatten(j);
        Assert::AreEqual(static_cast<size_t>(0), flat.size());
    }

    TEST_METHOD(Get_BooleanValue)
    {
        auto j = nlohmann::json::parse(R"({"server":{"ssl_enabled":true}})");
        auto result = JsonPath::get(j, "server.ssl_enabled");
        Assert::IsTrue(result.get<bool>());
    }
};

} // namespace AeostaraCoreTests
