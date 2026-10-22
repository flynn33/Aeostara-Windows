// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "CppUnitTest.h"
#include "AeostaraCore/Json.h"
#include <functional>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace Aeostara;
namespace AeostaraCoreTests {
TEST_CLASS(JsonValueTests) {
    static void rejects(const std::function<void()>& action, JsonErrorCode code) {
        try { action(); } catch (const JsonError& error) { Assert::IsTrue(error.code() == code); return; }
        Assert::Fail(L"Expected owned typed JSON error");
    }
public:
    TEST_METHOD(FractionalNumbersCompareWithIntegersAndZero) {
        Assert::AreEqual(1, Json::parse("0.1").compare_number(Json::parse("0")));
        Assert::AreEqual(1, Json::parse("1.1").compare_number(Json::parse("1")));
        Assert::AreEqual(-1, Json::parse("0").compare_number(Json::parse("0.1")));
        Assert::AreEqual(-1, Json::parse("-0.1").compare_number(Json::parse("0")));
        Assert::AreEqual(-1, Json::parse("-1.1").compare_number(Json::parse("-1")));
        Assert::AreEqual(1, Json::parse("-1").compare_number(Json::parse("-1.1")));
        Assert::AreEqual(1, Json::parse("1e-1000").compare_number(Json::parse("0")));
        Assert::AreEqual(-1, Json::parse("-1e-1000").compare_number(Json::parse("0")));
    }
    TEST_METHOD(ExactDecimal_CanonicalForms) {
        for (const auto& [input, expected] : std::vector<std::pair<std::string, std::string>>{
            {"1.0", "1"}, {"1e3", "1000"}, {"1e-3", "0.001"}, {"-0.00", "0"},
            {"123.4500", "123.45"}, {"9007199254740993", "9007199254740993"}})
            Assert::AreEqual(expected, Json::parse(input).canonical());
    }
    TEST_METHOD(ExactDecimal_EqualityAndOrdering) {
        Assert::IsTrue(Json::parse("1e0") == Json::parse("1.000"));
        Assert::IsTrue(Json(1) != Json(true));
        Assert::AreEqual(1, Json::parse("9007199254740993").compare_number(Json::parse("9007199254740992")));
        Assert::AreEqual(-1, Json::parse("-0.01").compare_number(Json::parse("-0.001")));
    }
    TEST_METHOD(NativeConversion_RejectsCoercionAndLoss) {
        Assert::IsTrue(Json::parse("18446744073709551615").get<std::uint64_t>() == UINT64_MAX);
        rejects([] { (void)Json::parse("18446744073709551616").get<std::uint64_t>(); }, JsonErrorCode::Range);
        rejects([] { (void)Json::parse("1.1").get<int>(); }, JsonErrorCode::Range);
        rejects([] { (void)Json(true).get<int>(); }, JsonErrorCode::Type);
    }
    TEST_METHOD(StrictSyntax_RejectsExtensionsAndIncompleteValues) {
        for (const auto input : {"01", "+1", "1.", "1e", "NaN", "Infinity", "[1,]", "{\"a\":1,}", "/*c*/1", "true false"})
            rejects([&] { (void)Json::parse(input); }, JsonErrorCode::Syntax);
        rejects([] { (void)Json::parse(std::string("\xef\xbb\xbf", 3) + "null"); }, JsonErrorCode::Syntax);
    }
    TEST_METHOD(DuplicateDecodedKeys_AreRejected) {
        rejects([] { (void)Json::parse("{\"a\":1,\"\\u0061\":2}"); }, JsonErrorCode::DuplicateKey);
        rejects([] { (void)Json{{"x", 1}, {"x", 2}}; }, JsonErrorCode::DuplicateKey);
    }
    TEST_METHOD(Unicode_PreservesScalarsAndRejectsInvalidEncodings) {
        Assert::AreEqual(std::string("\"") + "\xf0\x9f\x98\x80" + "\"", Json::parse("\"\\ud83d\\ude00\"").canonical());
        rejects([] { (void)Json::parse("\"\\udc00\""); }, JsonErrorCode::InvalidUtf8);
        for (const auto& bytes : {std::string("\xc0\x80", 2), std::string("\xed\xa0\x80", 3), std::string("\xf4\x90\x80\x80", 4), std::string("\xe2\x82", 2)})
            rejects([&] { (void)Json::parse(std::string("\"") + bytes + "\""); }, JsonErrorCode::InvalidUtf8);
    }
    TEST_METHOD(CanonicalOrderingAndEscapes_AreDeterministic) {
        Assert::AreEqual(std::string("{\"a\":3,\"z\":2,\"\xc3\xa9\":1}"), Json::parse("{\"\xc3\xa9\":1,\"z\":2,\"a\":3}").canonical());
        Assert::AreEqual(std::string("[3,1,2]"), Json::parse("[3,1,2]").canonical());
        Assert::AreEqual(std::string("\"\\u0000\\b\\t\\n\\f\\r/\\\\\\\"\""), Json::parse("\"\\u0000\\b\\t\\n\\f\\r/\\\\\\\"\"").canonical());
    }
    TEST_METHOD(Pointer_SeparatesMissingNullAndLiteralKeys) {
        const Json document = Json::parse("{\"\":null,\"a/b\":{\"~x\":[0,1]},\"a.b\":2}");
        Assert::IsTrue(document.resolve_pointer("") == &document);
        Assert::IsTrue(document.resolve_pointer("/")->is_null());
        Assert::IsTrue(document.resolve_pointer("/absent") == nullptr);
        Assert::AreEqual(1, document.resolve_pointer("/a~1b/~0x/1")->get<int>());
        Assert::AreEqual(2, document.resolve_pointer("/a.b")->get<int>());
        Assert::IsTrue(document.resolve_pointer("/a~1b/~0x/18446744073709551616") == nullptr);
    }
    TEST_METHOD(Pointer_RejectsInvalidEscapesIndicesAndScalarTraversal) {
        const Json document = Json::parse("{\"a\":[0]}");
        for (const auto pointer : {"a", "/a~2", "/a/01", "/a/-", "/a/+1"})
            rejects([&] { (void)document.resolve_pointer(pointer); }, JsonErrorCode::InvalidPointer);
        rejects([&] { (void)document.resolve_pointer("/a/0/x"); }, JsonErrorCode::Type);
        rejects([] { (void)Json::pointer_tokens(std::string(65, '/')); }, JsonErrorCode::Limit);
    }
    TEST_METHOD(Replacement_RequiresExistingBoundedTargetAndPreservesFailure) {
        Json document = Json::parse("{\"x\":1}");
        document.replace_pointer("/x", 2);
        Assert::AreEqual(2, document.at("x").get<int>());
        rejects([&] { document.replace_pointer("/absent", 3); }, JsonErrorCode::Range);
        Json deepest = Json::parse(std::string(63, '[') + "0" + std::string(63, ']'));
        const Json original = deepest;
        std::string pointer;
        for (int i = 0; i < 63; ++i) pointer += "/0";
        rejects([&] { deepest.replace_pointer(pointer, Json::array({0})); }, JsonErrorCode::Limit);
        Assert::IsTrue(deepest == original);
    }
    TEST_METHOD(DepthAndNodeLimits_EnforceExactBoundaries) {
        Assert::IsTrue(Json::parse(std::string(63, '[') + "0" + std::string(63, ']')).is_array());
        rejects([] { (void)Json::parse(std::string(64, '[') + "0" + std::string(64, ']')); }, JsonErrorCode::Limit);
        std::string nodes = "[";
        for (std::size_t i = 0; i < JsonLimits::nodes; ++i) { if (i) nodes += ','; nodes += '0'; }
        nodes += ']';
        rejects([&] { (void)Json::parse(nodes); }, JsonErrorCode::Limit);
        nodes.erase(nodes.size() - 3, 2);
        Assert::AreEqual(std::size_t(99999), Json::parse(nodes).size());
    }
    TEST_METHOD(ByteStringAndCanonicalLimits_AreEnforced) {
        rejects([] { (void)Json::parse(std::string(JsonLimits::rawDocumentBytes + 1, ' ')); }, JsonErrorCode::Limit);
        Assert::IsTrue(Json(std::string(JsonLimits::decodedStringBytes, 'x')).is_string());
        rejects([] { (void)Json(std::string(JsonLimits::decodedStringBytes + 1, 'x')); }, JsonErrorCode::Limit);
        rejects([] { (void)Json::array({Json(std::string(JsonLimits::decodedStringBytes, '\x01')), Json(std::string(JsonLimits::decodedStringBytes, '\x01')), Json(std::string(JsonLimits::decodedStringBytes, '\x01'))}).canonical(); }, JsonErrorCode::Limit);
    }
    TEST_METHOD(NumberLimits_CountOriginalDigitsAndWrittenExponent) {
        Assert::IsTrue(Json::parse(std::string(1024, '1')).is_number());
        rejects([] { (void)Json::parse(std::string(1025, '1')); }, JsonErrorCode::Limit);
        Assert::AreEqual(std::size_t(4097), Json::parse("1e4096").number_text().size());
        rejects([] { (void)Json::parse("0e4097"); }, JsonErrorCode::Limit);
    }
    TEST_METHOD(OwnedCopyAndPresentation_RoundTrip) {
        Json original{{"server", {{"port", 8080}}}};
        Json copy = original;
        copy["server"]["port"] = 9090;
        Assert::AreEqual(8080, original["server"]["port"].get<int>());
        Assert::IsTrue(Json::parse(original.dump(2)) == original);
        const std::map<std::string, Json> map{{"number", 42}, {"text", "value"}};
        Assert::IsTrue(Json(map).get<std::map<std::string, Json>>() == map);
    }
};
} // namespace AeostaraCoreTests
