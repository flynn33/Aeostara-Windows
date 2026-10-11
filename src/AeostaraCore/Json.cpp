// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/Json.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <set>

namespace Aeostara {
namespace {

[[noreturn]] void fail(JsonErrorCode code, const char* message, std::size_t offset = 0) {
    throw JsonError(code, message, offset);
}
bool digit(char c) noexcept { return c >= '0' && c <= '9'; }

std::size_t utf8_width(std::string_view text, std::size_t position) {
    const auto lead = static_cast<unsigned char>(text[position]);
    if (lead < 0x80) return 1;
    std::size_t width = 0;
    std::uint32_t scalar = 0;
    if (lead >= 0xc2 && lead <= 0xdf) { width = 2; scalar = lead & 0x1fU; }
    else if (lead >= 0xe0 && lead <= 0xef) { width = 3; scalar = lead & 0x0fU; }
    else if (lead >= 0xf0 && lead <= 0xf4) { width = 4; scalar = lead & 0x07U; }
    else fail(JsonErrorCode::InvalidUtf8, "Invalid UTF-8 leading byte", position);
    if (width > text.size() - position) fail(JsonErrorCode::InvalidUtf8, "Truncated UTF-8 scalar", position);
    for (std::size_t i = 1; i < width; ++i) {
        const auto next = static_cast<unsigned char>(text[position + i]);
        if ((next & 0xc0U) != 0x80U) fail(JsonErrorCode::InvalidUtf8, "Invalid UTF-8 continuation", position + i);
        scalar = (scalar << 6U) | (next & 0x3fU);
    }
    if ((width == 2 && scalar < 0x80U) || (width == 3 && scalar < 0x800U) ||
        (width == 4 && scalar < 0x10000U) || scalar > 0x10ffffU ||
        (scalar >= 0xd800U && scalar <= 0xdfffU))
        fail(JsonErrorCode::InvalidUtf8, "Invalid Unicode scalar encoding", position);
    return width;
}

void check_string(std::string_view text) {
    if (text.size() > JsonLimits::decodedStringBytes) fail(JsonErrorCode::Limit, "Decoded string limit exceeded");
    for (std::size_t i = 0; i < text.size();) i += utf8_width(text, i);
}

void append_scalar(std::string& result, std::uint32_t scalar) {
    if (scalar < 0x80U) result.push_back(static_cast<char>(scalar));
    else if (scalar < 0x800U) {
        result.push_back(static_cast<char>(0xc0U | (scalar >> 6U)));
        result.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
    } else if (scalar < 0x10000U) {
        result.push_back(static_cast<char>(0xe0U | (scalar >> 12U)));
        result.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3fU)));
        result.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
    } else {
        result.push_back(static_cast<char>(0xf0U | (scalar >> 18U)));
        result.push_back(static_cast<char>(0x80U | ((scalar >> 12U) & 0x3fU)));
        result.push_back(static_cast<char>(0x80U | ((scalar >> 6U) & 0x3fU)));
        result.push_back(static_cast<char>(0x80U | (scalar & 0x3fU)));
    }
}

std::string canonical_number(std::string_view text) {
    if (text.empty()) fail(JsonErrorCode::Syntax, "Empty JSON number");
    if (text.size() > JsonLimits::rawDocumentBytes) fail(JsonErrorCode::Limit, "Raw JSON number limit exceeded");
    std::size_t position = 0;
    const bool negative = text.front() == '-';
    if (negative) ++position;
    const auto start = position;
    if (position == text.size()) fail(JsonErrorCode::Syntax, "Incomplete JSON number");
    if (text[position] == '0') ++position;
    else if (text[position] >= '1' && text[position] <= '9') {
        while (position < text.size() && digit(text[position])) {
            if (position - start == JsonLimits::coefficientDigits)
                fail(JsonErrorCode::Limit, "JSON coefficient digit limit exceeded");
            ++position;
        }
    } else fail(JsonErrorCode::Syntax, "Invalid JSON number integer");
    std::string coefficient(text.substr(start, position - start));
    std::size_t fractionLength = 0;
    if (position < text.size() && text[position] == '.') {
        const auto fractionStart = ++position;
        while (position < text.size() && digit(text[position])) {
            if (coefficient.size() + position - fractionStart == JsonLimits::coefficientDigits)
                fail(JsonErrorCode::Limit, "JSON coefficient digit limit exceeded");
            ++position;
        }
        fractionLength = position - fractionStart;
        if (fractionLength == 0) fail(JsonErrorCode::Syntax, "Missing JSON fractional digits");
        if (coefficient.size() + fractionLength > JsonLimits::coefficientDigits)
            fail(JsonErrorCode::Limit, "JSON coefficient digit limit exceeded");
        coefficient.append(text.substr(fractionStart, fractionLength));
    }
    if (coefficient.size() > JsonLimits::coefficientDigits) fail(JsonErrorCode::Limit, "JSON coefficient digit limit exceeded");
    int writtenExponent = 0;
    if (position < text.size() && (text[position] == 'e' || text[position] == 'E')) {
        ++position;
        bool exponentNegative = false;
        if (position < text.size() && (text[position] == '+' || text[position] == '-'))
            exponentNegative = text[position++] == '-';
        const auto exponentStart = position;
        while (position < text.size() && digit(text[position])) {
            const int next = text[position++] - '0';
            if (writtenExponent > (JsonLimits::writtenExponent - next) / 10)
                fail(JsonErrorCode::Limit, "JSON written exponent limit exceeded");
            writtenExponent = writtenExponent * 10 + next;
        }
        if (position == exponentStart) fail(JsonErrorCode::Syntax, "Missing JSON exponent digits");
        if (exponentNegative) writtenExponent = -writtenExponent;
    }
    if (position != text.size()) fail(JsonErrorCode::Syntax, "Invalid JSON number suffix", position);
    int exponent = writtenExponent - static_cast<int>(fractionLength);
    while (coefficient.size() > 1 && coefficient.back() == '0') { coefficient.pop_back(); ++exponent; }
    if (exponent < -JsonLimits::effectiveExponent || exponent > JsonLimits::effectiveExponent)
        fail(JsonErrorCode::Limit, "JSON effective exponent limit exceeded");
    const auto nonzero = coefficient.find_first_not_of('0');
    if (nonzero == std::string::npos) return "0";
    coefficient.erase(0, nonzero);
    std::string result = negative ? "-" : "";
    const int point = static_cast<int>(coefficient.size()) + exponent;
    if (point <= 0) {
        result += "0.";
        result.append(static_cast<std::size_t>(-point), '0');
        result += coefficient;
    } else if (static_cast<std::size_t>(point) >= coefficient.size()) {
        result += coefficient;
        result.append(static_cast<std::size_t>(point) - coefficient.size(), '0');
    } else {
        result.append(coefficient, 0, static_cast<std::size_t>(point));
        result.push_back('.');
        result.append(coefficient, static_cast<std::size_t>(point), std::string::npos);
    }
    return result;
}

class Parser final {
public:
    explicit Parser(std::string_view bytes) : bytes_(bytes) {}
    Json parse() {
        if (bytes_.size() > JsonLimits::rawDocumentBytes) fail(JsonErrorCode::Limit, "Raw JSON document limit exceeded");
        skip();
        Json result = value(1);
        skip();
        if (position_ != bytes_.size()) fail(JsonErrorCode::Syntax, "Trailing JSON content", position_);
        result.validate();
        return result;
    }
private:
    std::string_view bytes_;
    std::size_t position_ = 0;
    std::size_t nodes_ = 0;
    void skip() {
        while (position_ < bytes_.size() && (bytes_[position_] == ' ' || bytes_[position_] == '\t' ||
            bytes_[position_] == '\r' || bytes_[position_] == '\n')) ++position_;
    }
    bool consume(char c) {
        if (position_ < bytes_.size() && bytes_[position_] == c) { ++position_; return true; }
        return false;
    }
    void require(char c) { if (!consume(c)) fail(JsonErrorCode::Syntax, "Expected JSON delimiter", position_); }
    std::uint32_t hex4() {
        if (bytes_.size() - position_ < 4) fail(JsonErrorCode::Syntax, "Incomplete Unicode escape", position_);
        std::uint32_t result = 0;
        for (int i = 0; i < 4; ++i) {
            const char c = bytes_[position_++];
            int nibble = 0;
            if (c >= '0' && c <= '9') nibble = c - '0';
            else if (c >= 'a' && c <= 'f') nibble = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') nibble = c - 'A' + 10;
            else fail(JsonErrorCode::Syntax, "Invalid Unicode escape", position_ - 1);
            result = (result << 4U) | static_cast<std::uint32_t>(nibble);
        }
        return result;
    }
    std::string string() {
        require('"');
        std::string result;
        while (position_ < bytes_.size()) {
            const auto current = static_cast<unsigned char>(bytes_[position_]);
            if (current == '"') { ++position_; return result; }
            if (current < 0x20U) fail(JsonErrorCode::Syntax, "Unescaped JSON string control", position_);
            if (current == '\\') {
                if (++position_ == bytes_.size()) fail(JsonErrorCode::Syntax, "Incomplete JSON escape", position_);
                switch (bytes_[position_++]) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 't': result.push_back('\t'); break;
                case 'n': result.push_back('\n'); break;
                case 'f': result.push_back('\f'); break;
                case 'r': result.push_back('\r'); break;
                case 'u': {
                    auto scalar = hex4();
                    if (scalar >= 0xd800U && scalar <= 0xdbffU) {
                        require('\\'); require('u');
                        const auto lower = hex4();
                        if (lower < 0xdc00U || lower > 0xdfffU)
                            fail(JsonErrorCode::InvalidUtf8, "Unpaired high surrogate", position_);
                        scalar = 0x10000U + ((scalar - 0xd800U) << 10U) + lower - 0xdc00U;
                    } else if (scalar >= 0xdc00U && scalar <= 0xdfffU)
                        fail(JsonErrorCode::InvalidUtf8, "Unpaired low surrogate", position_);
                    append_scalar(result, scalar);
                    break;
                }
                default: fail(JsonErrorCode::Syntax, "Invalid JSON escape", position_ - 1);
                }
            } else {
                const auto width = utf8_width(bytes_, position_);
                result.append(bytes_.substr(position_, width));
                position_ += width;
            }
            if (result.size() > JsonLimits::decodedStringBytes) fail(JsonErrorCode::Limit, "Decoded string limit exceeded", position_);
        }
        fail(JsonErrorCode::Syntax, "Unterminated JSON string", position_);
    }
    Json value(std::size_t depth) {
        if (depth > JsonLimits::depth) fail(JsonErrorCode::Limit, "JSON nesting depth limit exceeded", position_);
        if (++nodes_ > JsonLimits::nodes) fail(JsonErrorCode::Limit, "JSON node limit exceeded", position_);
        skip();
        if (position_ == bytes_.size()) fail(JsonErrorCode::Syntax, "Missing JSON value", position_);
        if (bytes_[position_] == '"') return Json(string());
        if (consume('{')) {
            Json::Object result;
            skip();
            if (consume('}')) return Json(std::move(result));
            do {
                skip();
                auto key = string();
                if (result.contains(key)) fail(JsonErrorCode::DuplicateKey, "Duplicate decoded JSON key", position_);
                skip(); require(':');
                auto child = value(depth + 1);
                result.emplace(std::move(key), std::move(child));
                skip();
                if (consume('}')) return Json(std::move(result));
                require(',');
            } while (true);
        }
        if (consume('[')) {
            Json::Array result;
            skip();
            if (consume(']')) return Json(std::move(result));
            do {
                result.push_back(value(depth + 1));
                skip();
                if (consume(']')) return Json(std::move(result));
                require(',');
            } while (true);
        }
        for (const auto& [literal, result] : std::initializer_list<std::pair<std::string_view, Json>>{
            {"true", Json(true)}, {"false", Json(false)}, {"null", Json(nullptr)}}) {
            if (bytes_.substr(position_, literal.size()) == literal) { position_ += literal.size(); return result; }
        }
        const auto start = position_;
        while (position_ < bytes_.size() && (digit(bytes_[position_]) || bytes_[position_] == '-' ||
            bytes_[position_] == '+' || bytes_[position_] == '.' || bytes_[position_] == 'e' || bytes_[position_] == 'E')) ++position_;
        if (position_ == start) fail(JsonErrorCode::Syntax, "Invalid JSON value", position_);
        try { return Json::number(bytes_.substr(start, position_ - start)); }
        catch (const JsonError& error) { throw JsonError(error.code(), error.what(), start + error.offset()); }
    }
};

class Writer final {
public:
    explicit Writer(int indent) : indent_(indent) {
        if (indent < -1 || indent > 16) fail(JsonErrorCode::Range, "Indent must be -1 through 16");
    }
    std::string write(const Json& value) { emit(value, 1); return std::move(result_); }
private:
    int indent_;
    std::size_t nodes_ = 0;
    std::string result_;
    void append(std::string_view value) {
        if (value.size() > JsonLimits::canonicalDocumentBytes - result_.size())
            fail(JsonErrorCode::Limit, "JSON output byte limit exceeded");
        result_.append(value);
    }
    void character(char value) { append(std::string_view(&value, 1)); }
    void line(std::size_t depth) {
        if (indent_ >= 0) {
            character('\n');
            append(std::string(depth * static_cast<std::size_t>(indent_), ' '));
        }
    }
    void string(std::string_view value) {
        check_string(value);
        character('"');
        constexpr char digits[] = "0123456789abcdef";
        for (const char c : value) {
            switch (c) {
            case '"': append("\\\""); break;
            case '\\': append("\\\\"); break;
            case '\b': append("\\b"); break;
            case '\t': append("\\t"); break;
            case '\n': append("\\n"); break;
            case '\f': append("\\f"); break;
            case '\r': append("\\r"); break;
            default:
                if (static_cast<unsigned char>(c) < 0x20U) {
                    append("\\u00");
                    character(digits[(static_cast<unsigned char>(c) >> 4U) & 0xfU]);
                    character(digits[static_cast<unsigned char>(c) & 0xfU]);
                } else character(c);
            }
        }
        character('"');
    }
    void emit(const Json& value, std::size_t depth) {
        if (depth > JsonLimits::depth) fail(JsonErrorCode::Limit, "JSON nesting depth limit exceeded");
        if (++nodes_ > JsonLimits::nodes) fail(JsonErrorCode::Limit, "JSON node limit exceeded");
        if (value.is_null()) append("null");
        else if (value.is_boolean()) append(value.get<bool>() ? "true" : "false");
        else if (value.is_number()) append(value.number_text());
        else if (value.is_string()) string(value.string_value());
        else if (value.is_array()) {
            character('[');
            bool first = true;
            for (const auto& child : value.array_items()) {
                if (!first) character(',');
                line(depth); emit(child, depth + 1); first = false;
            }
            if (!first) line(depth - 1);
            character(']');
        } else {
            character('{');
            bool first = true;
            for (const auto& [key, child] : value.object_items()) {
                if (!first) character(',');
                line(depth); string(key); character(':');
                if (indent_ >= 0) character(' ');
                emit(child, depth + 1); first = false;
            }
            if (!first) line(depth - 1);
            character('}');
        }
    }
};

class CapturedInputWriter final {
public:
    CapturedInputWriter(const Json& value, const std::vector<JsonSourceRegion>& regions,
        const std::function<void(std::string_view)>& sink) : value_(value), sink_(sink) {
        if (!sink_) fail(JsonErrorCode::Type, "Captured input requires an actual chunk consumer");
        if (regions.size() > JsonLimits::sourceDocuments) fail(JsonErrorCode::Limit, "Captured source count exceeded");
        std::vector<std::vector<std::string>> paths;
        std::size_t aggregateRaw = 0;
        for (const auto& region : regions) {
            const auto documentTokens = Json::pointer_tokens(region.documentPointer);
            const auto rawTokens = Json::pointer_tokens(region.rawUtf8Pointer);
            if (documentTokens.empty() || rawTokens.empty()) fail(JsonErrorCode::InvalidPointer, "Captured source regions must be proper child values");
            for (const auto& tokens : {documentTokens, rawTokens}) {
                for (const auto& previous : paths) {
                    const auto common = std::min(tokens.size(), previous.size());
                    if (std::equal(tokens.begin(), tokens.begin() + static_cast<std::ptrdiff_t>(common), previous.begin())) fail(JsonErrorCode::InvalidPointer, "Captured source regions overlap");
                }
                paths.push_back(tokens);
            }
            const auto* document = value.resolve_pointer(region.documentPointer);
            const auto* raw = value.resolve_pointer(region.rawUtf8Pointer);
            if (!document || !raw || !raw->is_string()) fail(JsonErrorCode::Type, "Captured document/raw region unavailable");
            const auto& text = raw->string_value();
            if (text.size() > JsonLimits::rawDocumentBytes || text.size() > JsonLimits::aggregateInputBytes - aggregateRaw) fail(JsonErrorCode::Limit, "Captured raw source byte limit exceeded");
            aggregateRaw += text.size();
            if (Json::parse(text) != *document) fail(JsonErrorCode::Type, "Captured raw bytes do not equal the declared parsed source document");
            document->validate();
            documents_.insert(document); rawStrings_.insert(raw);
        }
    }
    void write() { emit(value_, 1); flush(); }
private:
    const Json& value_;
    const std::function<void(std::string_view)>& sink_;
    std::set<const Json*> documents_, rawStrings_;
    std::string buffer_;
    std::size_t controlBytes_ = 0, controlNodes_ = 0;
    void flush() { if (!buffer_.empty()) { sink_(buffer_); buffer_.clear(); } }
    void append(std::string_view bytes, bool control = true) {
        if (control) { if (bytes.size() > JsonLimits::canonicalDocumentBytes - controlBytes_) fail(JsonErrorCode::Limit, "Captured control metadata output limit exceeded"); controlBytes_ += bytes.size(); }
        while (!bytes.empty()) { const auto count = std::min<std::size_t>(8192 - buffer_.size(), bytes.size()); buffer_.append(bytes.substr(0, count)); bytes.remove_prefix(count); if (buffer_.size() == 8192) flush(); }
    }
    void string(std::string_view text, bool control) {
        if (control) check_string(text);
        else for (std::size_t i = 0; i < text.size();) i += utf8_width(text, i);
        append("\"", control); constexpr char digits[] = "0123456789abcdef";
        for (const auto c : text) {
            switch (c) {
            case '"': append("\\\"", control); break;
            case '\\': append("\\\\", control); break;
            case '\b': append("\\b", control); break;
            case '\t': append("\\t", control); break;
            case '\n': append("\\n", control); break;
            case '\f': append("\\f", control); break;
            case '\r': append("\\r", control); break;
            default:
                if (static_cast<unsigned char>(c) < 0x20U) { const char encoded[]{'\\', 'u', '0', '0', digits[(static_cast<unsigned char>(c) >> 4U) & 0xfU], digits[static_cast<unsigned char>(c) & 0xfU]}; append(std::string_view(encoded, 6), control); }
                else append(std::string_view(&c, 1), control);
            }
        }
        append("\"", control);
    }
    void emit(const Json& value, std::size_t depth) {
        if (documents_.contains(&value)) { const auto canonical = value.canonical(); append(canonical, false); return; }
        if (rawStrings_.contains(&value)) { string(value.string_value(), false); return; }
        if (depth > JsonLimits::depth || ++controlNodes_ > JsonLimits::nodes) fail(JsonErrorCode::Limit, "Captured control metadata depth or node limit exceeded");
        if (value.is_null()) append("null");
        else if (value.is_boolean()) append(value.get<bool>() ? "true" : "false");
        else if (value.is_number()) append(value.number_text());
        else if (value.is_string()) string(value.string_value(), true);
        else if (value.is_array()) { append("["); bool first = true; for (const auto& child : value.array_items()) { if (!first) append(","); emit(child, depth + 1); first = false; } append("]"); }
        else { append("{"); bool first = true; for (const auto& [key, child] : value.object_items()) { if (!first) append(","); string(key, true); append(":"); emit(child, depth + 1); first = false; } append("}"); }
    }
};

std::optional<std::size_t> pointer_index(std::string_view token) {
    if (token.empty() || (token.size() > 1 && token.front() == '0') ||
        !std::all_of(token.begin(), token.end(), digit))
        fail(JsonErrorCode::InvalidPointer, "Invalid JSON Pointer array index");
    std::size_t result = 0;
    const auto converted = std::from_chars(token.data(), token.data() + token.size(), result);
    // A syntactically valid index beyond native range is necessarily absent.
    if (converted.ec == std::errc::result_out_of_range) return std::nullopt;
    if (converted.ec != std::errc{}) fail(JsonErrorCode::InvalidPointer, "Invalid JSON Pointer array index");
    return result;
}

} // namespace

JsonError::JsonError(JsonErrorCode code, std::string message, std::size_t offset)
    : std::runtime_error(std::move(message)), code_(code), offset_(offset) {}

bool UnsignedUtf8Less::operator()(std::string_view lhs, std::string_view rhs) const noexcept {
    return std::lexicographical_compare(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
        [](char a, char b) { return static_cast<unsigned char>(a) < static_cast<unsigned char>(b); });
}
void Json::validate_string(std::string_view text) { check_string(text); }
Json::Json(const char* value) : Json(value ? std::string(value) : throw JsonError(JsonErrorCode::Type, "Null C string")) {}
Json::Json(std::string value) : data_(std::move(value)) { validate_string(std::get<std::string>(data_)); }
Json::Json(Object value) : data_(std::move(value)) {
    for (const auto& [key, child] : std::get<Object>(data_)) { (void)child; validate_string(key); }
}
Json::Json(std::initializer_list<std::pair<const std::string, Json>> values) : data_(Object{}) {
    for (const auto& [key, value] : values) {
        validate_string(key);
        if (!std::get<Object>(data_).emplace(key, value).second) fail(JsonErrorCode::DuplicateKey, "Duplicate JSON initializer key");
    }
}
Json::Json(double value) : data_(nullptr) {
    if (!std::isfinite(value)) fail(JsonErrorCode::Range, "Non-finite JSON number");
    char buffer[64]{};
    const auto converted = std::to_chars(buffer, buffer + sizeof(buffer), value);
    if (converted.ec != std::errc{}) fail(JsonErrorCode::Range, "Cannot encode floating interop number");
    data_ = Number{canonical_number(std::string_view(buffer, static_cast<std::size_t>(converted.ptr - buffer)))};
}
Json Json::number(std::string_view exactLexeme) { Json result; result.data_ = Number{canonical_number(exactLexeme)}; return result; }
Json Json::parse(std::string_view bytes) { return Parser(bytes).parse(); }
bool Json::is_number_integer() const noexcept { return is_number() && std::get<Number>(data_).text.find('.') == std::string::npos; }
std::size_t Json::size() const noexcept {
    if (is_null()) return 0;
    if (is_array()) return std::get<Array>(data_).size();
    if (is_object()) return std::get<Object>(data_).size();
    return 1;
}
const Json::Array& Json::array_items() const { if (!is_array()) fail(JsonErrorCode::Type, "Expected JSON array"); return std::get<Array>(data_); }
Json::Array& Json::array_items() { if (!is_array()) fail(JsonErrorCode::Type, "Expected JSON array"); return std::get<Array>(data_); }
const Json::Object& Json::object_items() const { if (!is_object()) fail(JsonErrorCode::Type, "Expected JSON object"); return std::get<Object>(data_); }
Json::Object& Json::object_items() { if (!is_object()) fail(JsonErrorCode::Type, "Expected JSON object"); return std::get<Object>(data_); }
const std::string& Json::string_value() const { if (!is_string()) fail(JsonErrorCode::Type, "Expected JSON string"); return std::get<std::string>(data_); }
const std::string& Json::number_text() const { if (!is_number()) fail(JsonErrorCode::Type, "Expected JSON number"); return std::get<Number>(data_).text; }
int Json::compare_number(const Json& other) const {
    auto lhs = std::string_view(number_text());
    auto rhs = std::string_view(other.number_text());
    const bool lhsNegative = lhs.front() == '-';
    const bool rhsNegative = rhs.front() == '-';
    if (lhsNegative != rhsNegative) return lhsNegative ? -1 : 1;
    if (lhsNegative) { lhs.remove_prefix(1); rhs.remove_prefix(1); }
    auto lhsPoint = lhs.find('.'); if (lhsPoint == std::string_view::npos) lhsPoint = lhs.size();
    auto rhsPoint = rhs.find('.'); if (rhsPoint == std::string_view::npos) rhsPoint = rhs.size();
    int result = 0;
    if (lhsPoint != rhsPoint) result = lhsPoint < rhsPoint ? -1 : 1;
    else {
        for (std::size_t i = 0; i < lhsPoint && result == 0; ++i)
            if (lhs[i] != rhs[i]) result = lhs[i] < rhs[i] ? -1 : 1;
        const auto lhsFraction = lhsPoint < lhs.size() ? lhs.substr(lhsPoint + 1) : std::string_view{};
        const auto rhsFraction = rhsPoint < rhs.size() ? rhs.substr(rhsPoint + 1) : std::string_view{};
        const auto length = (std::max)(lhsFraction.size(), rhsFraction.size());
        for (std::size_t i = 0; i < length && result == 0; ++i) {
            const char a = i < lhsFraction.size() ? lhsFraction[i] : '0';
            const char b = i < rhsFraction.size() ? rhsFraction[i] : '0';
            if (a != b) result = a < b ? -1 : 1;
        }
    }
    return lhsNegative ? -result : result;
}
bool Json::contains(std::string_view key) const { return is_object() && std::get<Object>(data_).contains(key); }
const Json& Json::at(std::string_view key) const {
    const auto& object = object_items(); const auto found = object.find(key);
    if (found == object.end()) fail(JsonErrorCode::Range, "JSON object key is absent");
    return found->second;
}
Json& Json::at(std::string_view key) { return const_cast<Json&>(std::as_const(*this).at(key)); }
const Json& Json::at(std::size_t index) const {
    const auto& array = array_items();
    if (index >= array.size()) fail(JsonErrorCode::Range, "JSON array index is absent");
    return array[index];
}
Json& Json::at(std::size_t index) { return const_cast<Json&>(std::as_const(*this).at(index)); }
Json& Json::operator[](std::string_view key) {
    validate_string(key);
    if (is_null()) data_ = Object{};
    return object_items()[std::string(key)];
}
void Json::push_back(Json value) { if (is_null()) data_ = Array{}; array_items().push_back(std::move(value)); }
std::size_t Json::erase(std::string_view key) { return object_items().erase(std::string(key)); }
std::string Json::dump(int indent) const { return Writer(indent).write(*this); }
void Json::validate() const { (void)dump(); }
Json Json::captured_raw_utf8(std::string rawUtf8) {
    (void)parse(rawUtf8);
    Json value; value.data_ = std::move(rawUtf8); return value;
}
void Json::canonical_sources(const std::vector<JsonSourceRegion>& regions,
    const std::function<void(std::string_view)>& consumeChunk) const {
    CapturedInputWriter(*this, regions, consumeChunk).write();
}
std::vector<std::string> Json::pointer_tokens(std::string_view pointer) {
    check_string(pointer);
    if (pointer.empty()) return {};
    if (pointer.front() != '/') fail(JsonErrorCode::InvalidPointer, "JSON Pointer must start with slash");
    std::vector<std::string> result;
    std::size_t position = 1;
    do {
        if (result.size() == JsonLimits::pointerTokens) fail(JsonErrorCode::Limit, "JSON Pointer token limit exceeded");
        const auto end = pointer.find('/', position);
        const auto token = pointer.substr(position, end == std::string_view::npos ? pointer.size() - position : end - position);
        std::string decoded;
        for (std::size_t i = 0; i < token.size(); ++i) {
            if (token[i] != '~') decoded.push_back(token[i]);
            else {
                if (++i == token.size() || (token[i] != '0' && token[i] != '1'))
                    fail(JsonErrorCode::InvalidPointer, "Invalid JSON Pointer escape");
                decoded.push_back(token[i] == '0' ? '~' : '/');
            }
        }
        result.push_back(std::move(decoded));
        if (end == std::string_view::npos) break;
        position = end + 1;
    } while (true);
    return result;
}
const Json* Json::resolve_pointer(std::string_view pointer) const {
    const Json* current = this;
    for (const auto& token : pointer_tokens(pointer)) {
        if (current->is_object()) {
            const auto found = current->object_items().find(token);
            if (found == current->object_items().end()) return nullptr;
            current = &found->second;
        } else if (current->is_array()) {
            const auto index = pointer_index(token);
            if (!index || *index >= current->size()) return nullptr;
            current = &current->at(*index);
        } else fail(JsonErrorCode::Type, "JSON Pointer traverses scalar value");
    }
    return current;
}
Json* Json::resolve_pointer(std::string_view pointer) { return const_cast<Json*>(std::as_const(*this).resolve_pointer(pointer)); }
void Json::replace_pointer(std::string_view pointer, Json value) {
    Json candidate = *this;
    auto* target = candidate.resolve_pointer(pointer);
    if (!target) fail(JsonErrorCode::Range, "JSON Pointer replacement target is absent");
    *target = std::move(value);
    candidate.validate();
    *this = std::move(candidate);
}

} // namespace Aeostara
