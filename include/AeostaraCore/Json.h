// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/SdkCompatibility.h"

#include <charconv>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <functional>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace Aeostara {

enum class JsonErrorCode { Syntax, InvalidUtf8, DuplicateKey, Type, Range, Limit, InvalidPointer };

class JsonError final : public std::runtime_error {
public:
    JsonError(JsonErrorCode code, std::string message, std::size_t offset = 0);
    JsonErrorCode code() const noexcept { return code_; }
    std::size_t offset() const noexcept { return offset_; }
private:
    JsonErrorCode code_;
    std::size_t offset_;
};

struct JsonLimits final {
    static constexpr std::size_t rawDocumentBytes = 8388608;
    static constexpr std::size_t aggregateInputBytes = 67108864;
    static constexpr std::size_t sourceDocuments = 32;
    static constexpr std::size_t canonicalDocumentBytes = 16777216;
    static constexpr std::size_t depth = 64;
    static constexpr std::size_t nodes = 100000;
    static constexpr std::size_t decodedStringBytes = 1048576;
    static constexpr std::size_t coefficientDigits = 1024;
    static constexpr int writtenExponent = 4096;
    static constexpr int effectiveExponent = 5120;
    static constexpr std::size_t pointerTokens = 64;
};

struct UnsignedUtf8Less final {
    using is_transparent = void;
    bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
};

// Native captured-input transport names source document and raw-byte regions
// explicitly. These regions are never treated as ordinary JSON string values.
struct JsonSourceRegion final {
    std::string documentPointer;
    std::string rawUtf8Pointer;
};

class Json final {
public:
    struct Number final {
        std::string text;
        bool operator==(const Number&) const = default;
    };
    using Array = std::vector<Json>;
    using Object = std::map<std::string, Json, UnsignedUtf8Less>;
    using parse_error = JsonError;

    Json() noexcept : data_(nullptr) {}
    Json(std::nullptr_t) noexcept : data_(nullptr) {}
    Json(bool value) noexcept : data_(value) {}
    template<std::integral T> requires (!std::same_as<T, bool>)
    Json(T value) : data_(Number{std::to_string(value)}) {}
    Json(double value);
    Json(float value) : Json(static_cast<double>(value)) {}
    Json(const char* value);
    Json(std::string value);
    Json(std::string_view value) : Json(std::string(value)) {}
    Json(Array value) : data_(std::move(value)) {}
    Json(Object value);
    Json(std::initializer_list<std::pair<const std::string, Json>> values);
    template<class T, class Allocator>
    Json(const std::vector<T, Allocator>& values) : data_(Array{}) {
        auto& result = std::get<Array>(data_);
        result.reserve(values.size());
        for (const auto& item : values) result.emplace_back(item);
    }
    template<class T, class Compare, class Allocator>
    Json(const std::map<std::string, T, Compare, Allocator>& values) : data_(Object{}) {
        for (const auto& [key, value] : values) {
            validate_string(key);
            std::get<Object>(data_).emplace(key, Json(value));
        }
    }
    template<class T> requires (!std::same_as<std::remove_cvref_t<T>, Json> &&
        requires(Json& target, const T& source) { to_json(target, source); })
    Json(const T& value) : data_(nullptr) { to_json(*this, value); }

    static Json parse(std::string_view bytes);
    static Json number(std::string_view exactLexeme);
    static Json object() { return Json(Object{}); }
    static Json array() { return Json(Array{}); }
    static Json array(std::initializer_list<Json> values) { return Json(Array(values)); }

    bool is_null() const noexcept { return std::holds_alternative<std::nullptr_t>(data_); }
    bool is_boolean() const noexcept { return std::holds_alternative<bool>(data_); }
    bool is_number() const noexcept { return std::holds_alternative<Number>(data_); }
    bool is_number_integer() const noexcept;
    bool is_string() const noexcept { return std::holds_alternative<std::string>(data_); }
    bool is_array() const noexcept { return std::holds_alternative<Array>(data_); }
    bool is_object() const noexcept { return std::holds_alternative<Object>(data_); }
    std::size_t size() const noexcept;
    bool empty() const noexcept { return size() == 0; }

    const Array& array_items() const;
    Array& array_items();
    const Object& object_items() const;
    Object& object_items();
    const std::string& string_value() const;
    const std::string& number_text() const;
    int compare_number(const Json& other) const;
    bool contains(std::string_view key) const;
    const Json& at(std::string_view key) const;
    Json& at(std::string_view key);
    const Json& at(std::size_t index) const;
    Json& at(std::size_t index);
    const Json& operator[](std::string_view key) const { return at(key); }
    Json& operator[](std::string_view key);
    const Json& operator[](std::size_t index) const { return at(index); }
    Json& operator[](std::size_t index) { return at(index); }
    void push_back(Json value);
    std::size_t erase(std::string_view key);

    template<class T> T get() const {
        if constexpr (requires(const Json& value) { T::from_json_value(value); }) {
            return T::from_json_value(*this);
        } else {
            T result{};
            get_to(result);
            return result;
        }
    }
    template<class T> void get_to(T& result) const {
        if constexpr (std::same_as<T, Json>) result = *this;
        else if constexpr (std::same_as<T, std::string>) result = string_value();
        else if constexpr (std::same_as<T, bool>) {
            if (!is_boolean()) throw JsonError(JsonErrorCode::Type, "Expected JSON boolean");
            result = std::get<bool>(data_);
        } else if constexpr (std::integral<T>) {
            const auto& text = number_text();
            if (!is_number_integer()) throw JsonError(JsonErrorCode::Range, "Number is not an exact integer");
            T parsed{};
            const auto conversion = std::from_chars(text.data(), text.data() + text.size(), parsed);
            if (conversion.ec != std::errc{} || conversion.ptr != text.data() + text.size())
                throw JsonError(JsonErrorCode::Range, "Number is outside requested integer range");
            result = parsed;
        } else if constexpr (std::floating_point<T>) {
            const auto& text = number_text();
            T parsed{};
            const auto conversion = std::from_chars(text.data(), text.data() + text.size(), parsed);
            if (conversion.ec != std::errc{} || conversion.ptr != text.data() + text.size() || !std::isfinite(parsed))
                throw JsonError(JsonErrorCode::Range, "Number is outside requested floating range");
            result = parsed;
        } else if constexpr (requires { typename T::value_type; result.push_back(typename T::value_type{}); }) {
            T parsed;
            for (const auto& item : array_items()) parsed.push_back(item.template get<typename T::value_type>());
            result = std::move(parsed);
        } else if constexpr (requires { typename T::mapped_type; result.emplace(std::string{}, typename T::mapped_type{}); }) {
            T parsed;
            for (const auto& [key, value] : object_items()) parsed.emplace(key, value.template get<typename T::mapped_type>());
            result = std::move(parsed);
        } else from_json(*this, result);
    }
    template<class T> T value(std::string_view key, const T& fallback) const {
        if (!is_object()) throw JsonError(JsonErrorCode::Type, "Expected JSON object");
        return contains(key) ? at(key).template get<T>() : fallback;
    }
    std::string value(std::string_view key, const char* fallback) const {
        return value(key, std::string(fallback));
    }

    std::string dump(int indent = -1) const;
    std::string canonical() const { return dump(); }
    void validate() const;
    static Json captured_raw_utf8(std::string rawUtf8);
    void canonical_sources(const std::vector<JsonSourceRegion>& sourceRegions,
        const std::function<void(std::string_view)>& consumeChunk) const;
    static std::vector<std::string> pointer_tokens(std::string_view pointer);
    const Json* resolve_pointer(std::string_view pointer) const;
    Json* resolve_pointer(std::string_view pointer);
    void replace_pointer(std::string_view pointer, Json value);
    bool operator==(const Json&) const = default;

private:
    std::variant<std::nullptr_t, bool, Number, std::string, Array, Object> data_;
    static void validate_string(std::string_view text);
};

} // namespace Aeostara
