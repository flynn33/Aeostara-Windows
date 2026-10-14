// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "DiagnosticsInternal.h"
#include "AeostaraCore/BaseContracts.h"
#include "AeostaraCore/SemanticMapping.h"
#include "AeostaraCore/Version.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <deque>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <regex>
#include <stdexcept>
#include <windows.h>
#include <objbase.h>

namespace Aeostara {
namespace {
const char* phaseName(DiagnosticPhaseType p) {
    static constexpr const char* names[] = {"OBSERVATION", "NORMALIZATION", "PROJECTION", "DIAGNOSIS",
        "RECOVERABILITY", "POLICY", "PLANNING", "BACKUP", "EXECUTION", "VERIFICATION", "RECOVERY",
        "CONTAINMENT", "COMPLETION", "EXPORT"};
    const auto i = static_cast<std::size_t>(p);
    if (i >= std::size(names)) throw std::invalid_argument("Invalid diagnostic phase");
    return names[i];
}
const char* outcomeName(DiagnosticOutcomeType p) {
    static constexpr const char* names[] = {"STARTED", "PASSED", "BLOCKED", "FAILED", "RESTORED", "RECOVERY_FAILED"};
    const auto i = static_cast<std::size_t>(p);
    if (i >= std::size(names)) throw std::invalid_argument("Invalid diagnostic outcome");
    return names[i];
}
const char* errorName(DiagnosticErrorType p) {
    static constexpr const char* names[] = {"NONE", "INPUT", "MAPPING", "POLICY", "STORAGE", "STALE",
        "EXECUTION", "VERIFICATION", "RECOVERY", "AUDIT", "INTERNAL"};
    const auto i = static_cast<std::size_t>(p);
    if (i >= std::size(names)) throw std::invalid_argument("Invalid diagnostic error kind");
    return names[i];
}
bool isHash(const std::string& text) {
    return text.size() == 64 && std::all_of(text.begin(), text.end(), [](char c) {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
    });
}
std::string opaque(const std::string& input) { return input.empty() ? "unavailable" : sha256(input); }
Json safeError(const DiagnosticError& error, std::size_t depth, DiagnosticHealth& health) {
    Json causes = Json::array();
    if (depth < 4) {
        for (std::size_t i = 0; i < std::min<std::size_t>(error.causes.size(), 8); ++i)
            causes.push_back(safeError(error.causes[i], depth + 1, health));
    }
    if ((depth >= 4 && !error.causes.empty()) || error.causes.size() > 8) ++health.truncated;
    return Json{{"kind", errorName(error.kind)}, {"nativeCode", error.nativeCode ? Json(*error.nativeCode) : Json(nullptr)},
        {"nativeCodeUnavailableReason", error.nativeCode ? Json(nullptr) : Json("No native code captured")},
        {"messageUnavailableReason", "Unstructured messages are excluded from retained diagnostics"}, {"causes", causes}};
}
bool safeKey(std::string_view key) {
    static constexpr std::string_view keys[] = {"bytes", "count", "durationMilliseconds", "sourceHash", "targetHash",
        "beforeHash", "afterHash", "backupHash", "state", "targetState", "state_vector", "stateClass", "recoveryCategory",
        "admissibility", "normalization", "compatible", "available", "allowed", "verified", "restored", "mutationMayHaveOccurred",
        "step", "sourceCount", "documentCount", "policyHash", "planHash", "witnessHash", "profileHash", "registryHash"};
    return std::find(std::begin(keys), std::end(keys), key) != std::end(keys);
}
bool vocabulary(std::string_view text) {
    static constexpr std::string_view values[] = {"STABLE", "UNSTABLE", "CORRECTABLE", "DEGRADED", "CONTAINED", "FAILED", "SAFE_HALT",
        "NO_ACTION", "NORMALIZE_STATE", "APPLY_CORRECTION", "FALLBACK_REQUIRED", "CONTAINMENT_REQUIRED", "ESCALATION_REQUIRED", "TERMINAL_NO_RECOVERY",
        "VALID", "INVALID", "COMPATIBLE", "INCOMPATIBLE", "NORMALIZABLE", "NOT_NORMALIZABLE", "MAPPED", "BLOCKED", "AMBIGUOUS"};
    return std::find(std::begin(values), std::end(values), text) != std::end(values);
}
Json safeEvidence(const Json& value, DiagnosticHealth& health, std::size_t depth = 0) {
    if (depth > 4) { ++health.truncated; return nullptr; }
    if (value.is_boolean() || value.is_null()) return value;
    if (value.is_number()) {
        try { (void)value.get<std::int64_t>(); return value; }
        catch (...) { ++health.redacted; return nullptr; }
    }
    if (value.is_string()) {
        const auto& text = value.string_value();
        if (isHash(text) || vocabulary(text)) return value;
        ++health.redacted;
        return nullptr;
    }
    if (value.is_array()) {
        Json result = Json::array();
        for (std::size_t i = 0; i < std::min<std::size_t>(value.size(), 16); ++i)
            result.push_back(safeEvidence(value[i], health, depth + 1));
        if (value.size() > 16) ++health.truncated;
        return result;
    }
    Json result = Json::object();
    std::size_t examined = 0;
    for (const auto& [key, item] : value.object_items()) {
        if (++examined > 64) { ++health.truncated; break; }
        if (safeKey(key)) result[key] = safeEvidence(item, health, depth + 1);
        else ++health.redacted;
    }
    return result;
}
Json healthJson(const DiagnosticHealth& h) {
    return Json{{"dropped", h.dropped}, {"truncated", h.truncated}, {"redacted", h.redacted},
        {"captureFailures", h.captureFailures}, {"exportFailures", h.exportFailures ? Json(*h.exportFailures) : Json(nullptr)},
        {"exportFailuresUnavailableReason", "Standalone exports are observed by their calling host"}};
}
bool exportLiteral(const std::string& text) {
    static constexpr std::string_view values[] = {"aeostara.diagnostics/1", AEOSTARA_SDK_VERSION, "LEAN", "DEVELOPMENT", "AeostaraCore", "unavailable",
        "No native code captured", "Unstructured messages are excluded from retained diagnostics", "No checked hash captured", "Event byte limit",
        "Protected operations require verified authorization and evidence", "UNKNOWN", "No crash-time, power-loss or external process guarantee",
        "Structured allowlist; credentials and raw payloads are excluded", "Native compiler and architecture metadata; host OS build unavailable",
        "Standalone exports are observed by their calling host", "Native OS version acquisition unavailable", "SDK source commit unavailable in this build"};
    if (std::find(std::begin(values), std::end(values), text) != std::end(values) || vocabulary(text)) return true;
    for (int i = 0; i < 14; ++i) if (text == phaseName(static_cast<DiagnosticPhaseType>(i))) return true;
    for (int i = 0; i < 6; ++i) if (text == outcomeName(static_cast<DiagnosticOutcomeType>(i))) return true;
    for (int i = 0; i < 11; ++i) if (text == errorName(static_cast<DiagnosticErrorType>(i))) return true;
    return false;
}
Json checkedExportNode(const Json& value, DiagnosticHealth& health, std::size_t depth = 0) {
    if (depth > 16) { ++health.truncated; return nullptr; }
    if (value.is_boolean() || value.is_null()) return value;
    if (value.is_number()) {
        try { (void)value.get<std::uint64_t>(); return value; } catch (...) {}
        try { (void)value.get<std::int64_t>(); return value; } catch (...) { ++health.redacted; return nullptr; }
    }
    if (value.is_string()) {
        const auto& text = value.string_value();
        static const std::regex utc("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$");
        const bool gitHash = text.size() == 40 && std::all_of(text.begin(), text.end(), [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
        });
        if (isHash(text) || gitHash || exportLiteral(text) || std::regex_match(text, utc)) return value;
        ++health.redacted; return nullptr;
    }
    if (value.is_array()) {
        Json out = Json::array();
        for (std::size_t i = 0; i < std::min<std::size_t>(value.size(), 4096); ++i) out.push_back(checkedExportNode(value[i], health, depth + 1));
        if (value.size() > 4096) ++health.truncated;
        return out;
    }
    static constexpr std::string_view keys[] = {"schema", "sdkVersion", "profile", "sessionID", "buildContext", "compilerVersion", "pointerBits", "sourceCommit",
        "processID", "osMajor", "osMinor", "osBuild", "osArchitecture", "osVersionUnavailableReason", "sourceDirty", "hostSourceCommit", "sourceCommitUnavailableReason",
        "environmentUnavailableReason", "captureStartUtc", "captureEndUtc", "events", "incidents", "loss", "rawMessagesUnavailableReason", "bundleID",
        "eventID", "operationID", "sequence", "utc", "monotonicMilliseconds", "component", "phase", "outcome", "objectHash", "objectHashUnavailableReason",
        "ruleReferenceHashes", "primaryError", "secondaryErrors", "evidence", "evidenceUnavailableReason", "incidentID", "triggerPhase", "affectedObjectHash",
        "expectedContract", "observedOutcome", "timelineEventIDs", "causeStatus", "coverageGap", "kind", "nativeCode", "nativeCodeUnavailableReason",
        "messageUnavailableReason", "causes", "dropped", "truncated", "redacted", "captureFailures", "exportFailures", "exportFailuresUnavailableReason", "exportRedactions"};
    Json out = Json::object();
    for (const auto& [key, item] : value.object_items()) {
        if (key == "evidence") out[key] = safeEvidence(item, health);
        else if (std::find(std::begin(keys), std::end(keys), key) != std::end(keys)) out[key] = checkedExportNode(item, health, depth + 1);
        else ++health.redacted;
    }
    return out;
}
class NativeClock final : public IDiagnosticClock {
public:
    std::string utcNow() const override {
        const auto now = std::chrono::system_clock::now();
        const auto t = std::chrono::system_clock::to_time_t(now);
        std::tm parts{};
        if (gmtime_s(&parts, &t) != 0) throw std::runtime_error("UTC clock conversion failed");
        std::ostringstream out; out << std::put_time(&parts, "%Y-%m-%dT%H:%M:%SZ"); return out.str();
    }
    std::uint64_t monotonicMilliseconds() const override {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
};
struct StoredEvent final { Json value; std::uint64_t time; std::size_t bytes; };
class NativeIdentity final : public IDiagnosticIdentity {
public:
    std::string sessionIdentity() const override {
        GUID identity{};
        if (CoCreateGuid(&identity) != S_OK) throw std::runtime_error("Diagnostic session identity unavailable");
        return std::string(reinterpret_cast<const char*>(&identity), sizeof(identity));
    }
};
class Collector final : public IDiagnostics {
public:
    Collector(DiagnosticProfileType profile, DiagnosticLimits limits, Json context, std::shared_ptr<IDiagnosticClock> clock,
        std::shared_ptr<const DiagnosticDetail::IEvidenceRetention> retention, std::shared_ptr<IDiagnosticIdentity> identity)
        : profile_(profile), limits_(limits), clock_(clock ? std::move(clock) : std::make_shared<NativeClock>()), retention_(std::move(retention)) {
        if (!limits_.eventCount || limits_.eventCount > 4096 || limits_.eventBytes < 1024 || limits_.eventBytes > 65536 ||
            limits_.memoryBytes < limits_.eventBytes || limits_.memoryBytes > 16777216 ||
            !limits_.retentionMilliseconds || limits_.retentionMilliseconds > 86400000 ||
            limits_.exportBytes < 2048 || limits_.exportBytes > 16777216)
            throw std::invalid_argument("Invalid bounded diagnostic limits");
        if (!retention_) throw std::invalid_argument("Diagnostic retention backend missing");
        const auto session = (identity ? identity : std::make_shared<NativeIdentity>())->sessionIdentity();
        if (session.empty() || session.size() > 1024) throw std::invalid_argument("Invalid diagnostic session identity");
        sessionID_ = sha256(session);
        context_ = Json{{"compilerVersion", _MSC_FULL_VER}, {"pointerBits", sizeof(void*) * 8}, {"processID", GetCurrentProcessId()},
            {"osVersionUnavailableReason", "Native OS version acquisition unavailable"}};
#ifdef AEOSTARA_SOURCE_COMMIT
        context_["sourceCommit"] = AEOSTARA_SOURCE_COMMIT;
        context_["sourceCommitUnavailableReason"] = nullptr;
#else
        context_["sourceCommit"] = nullptr;
        context_["sourceCommitUnavailableReason"] = "SDK source commit unavailable in this build";
#endif
#ifdef AEOSTARA_SOURCE_DIRTY
        context_["sourceDirty"] = AEOSTARA_SOURCE_DIRTY != 0;
#else
        context_["sourceDirty"] = nullptr;
#endif
        SYSTEM_INFO system{}; GetNativeSystemInfo(&system);
        context_["osArchitecture"] = system.wProcessorArchitecture;
        using GetNativeVersion = LONG(WINAPI*)(OSVERSIONINFOW*);
        const auto module = GetModuleHandleW(L"ntdll.dll");
        const auto raw = module ? GetProcAddress(module, "RtlGetVersion") : nullptr;
        GetNativeVersion getVersion = nullptr;
        static_assert(sizeof(getVersion) == sizeof(raw));
        std::memcpy(&getVersion, &raw, sizeof(raw));
        OSVERSIONINFOW version{}; version.dwOSVersionInfoSize = sizeof(version);
        if (getVersion && getVersion(&version) == 0) {
            context_["osMajor"] = version.dwMajorVersion; context_["osMinor"] = version.dwMinorVersion;
            context_["osBuild"] = version.dwBuildNumber; context_["osVersionUnavailableReason"] = nullptr;
        }
        if (context.is_object() && context.contains("sourceCommit") && context.at("sourceCommit").is_string()) {
            DiagnosticHealth check;
            context_["hostSourceCommit"] = checkedExportNode(context.at("sourceCommit"), check);
        }
        start_ = clock_->utcNow();
    }
    void capture(const DiagnosticEvent& event) noexcept override {
        try {
            std::lock_guard lock(mutex_);
            const auto time = clock_->monotonicMilliseconds();
            expire(time);
            Json rules = Json::array();
            for (std::size_t i = 0; i < std::min<std::size_t>(event.ruleReferences.size(), 16); ++i)
                rules.push_back(opaque(event.ruleReferences[i]));
            if (event.ruleReferences.size() > 16) ++health_.truncated;
            Json secondary = Json::array();
            for (std::size_t i = 0; i < std::min<std::size_t>(event.secondaryErrors.size(), 8); ++i)
                secondary.push_back(safeError(event.secondaryErrors[i], 0, health_));
            if (event.secondaryErrors.size() > 8) ++health_.truncated;
            const auto sequence = ++sequence_;
            Json value{{"eventID", sha256(sessionID_ + "/" + std::to_string(sequence) + "/" + opaque(event.operationID))},
                {"operationID", opaque(event.operationID)}, {"sequence", sequence}, {"utc", clock_->utcNow()},
                {"monotonicMilliseconds", time}, {"component", "AeostaraCore"}, {"phase", phaseName(event.phase)},
                {"outcome", outcomeName(event.outcome)}, {"objectHash", isHash(event.objectHash) ? Json(event.objectHash) : Json(nullptr)},
                {"objectHashUnavailableReason", isHash(event.objectHash) ? Json(nullptr) : Json("No checked hash captured")},
                {"ruleReferenceHashes", rules}, {"primaryError", event.primaryError ? safeError(*event.primaryError, 0, health_) : Json(nullptr)},
                {"secondaryErrors", secondary}, {"evidence", retention_->retain(event.evidence, health_)}};
            auto bytes = value.canonical().size();
            if (bytes > limits_.eventBytes) {
                value["evidence"] = Json::object(); value["evidenceUnavailableReason"] = "Event byte limit"; ++health_.truncated;
                bytes = value.canonical().size();
            }
            if (bytes > limits_.eventBytes) { ++health_.dropped; return; }
            while (!events_.empty() && (events_.size() >= limits_.eventCount || retainedBytes_ + bytes > limits_.memoryBytes)) dropFront();
            events_.push_back(StoredEvent{std::move(value), time, bytes}); retainedBytes_ += bytes;
        } catch (...) { failures_.fetch_add(1, std::memory_order_relaxed); }
    }
    DiagnosticSnapshot snapshot() const override {
        std::lock_guard lock(mutex_);
        const auto time = clock_->monotonicMilliseconds();
        expire(time);
        Json events = Json::array(), incidents = Json::array();
        for (const auto& stored : events_) {
            events.push_back(stored.value);
            const auto outcome = stored.value.at("outcome").get<std::string>();
            if (outcome == "FAILED" || outcome == "BLOCKED" || outcome == "RECOVERY_FAILED") {
                Json refs = Json::array();
                for (const auto& related : events_)
                    if (related.value.at("operationID") == stored.value.at("operationID")) refs.push_back(related.value.at("eventID"));
                incidents.push_back(Json{{"incidentID", stored.value.at("eventID")}, {"triggerPhase", stored.value.at("phase")},
                    {"affectedObjectHash", stored.value.at("objectHash")}, {"expectedContract", "Protected operations require verified authorization and evidence"},
                    {"observedOutcome", stored.value.at("outcome")}, {"timelineEventIDs", refs},
                    {"primaryError", stored.value.at("primaryError")}, {"secondaryErrors", stored.value.at("secondaryErrors")},
                    {"causeStatus", "UNKNOWN"}, {"coverageGap", "No crash-time, power-loss or external process guarantee"}});
            }
        }
        auto health = health_; health.captureFailures += failures_.load(std::memory_order_relaxed);
        Json document{{"schema", "aeostara.diagnostics/1"}, {"sdkVersion", AEOSTARA_SDK_VERSION},
            {"profile", profile_ == DiagnosticProfileType::Lean ? "LEAN" : "DEVELOPMENT"}, {"sessionID", sessionID_}, {"buildContext", context_},
            {"captureStartUtc", start_}, {"captureEndUtc", clock_->utcNow()}, {"events", events}, {"incidents", incidents},
            {"loss", healthJson(health)}, {"rawMessagesUnavailableReason", "Structured allowlist; credentials and raw payloads are excluded"}};
        document["bundleID"] = sha256(document.canonical());
        return DiagnosticSnapshot{std::move(document), limits_.exportBytes};
    }
    DiagnosticProfileType profile() const noexcept override { return profile_; }
    DiagnosticHealth health() const noexcept override {
        try { std::lock_guard lock(mutex_); auto h = health_; h.captureFailures += failures_.load(std::memory_order_relaxed); return h; }
        catch (...) { DiagnosticHealth h; h.captureFailures = failures_.load(std::memory_order_relaxed) + 1; return h; }
    }
private:
    void dropFront() const { retainedBytes_ -= events_.front().bytes; events_.pop_front(); ++health_.dropped; }
    void expire(std::uint64_t now) const {
        while (!events_.empty() && now >= events_.front().time && now - events_.front().time > limits_.retentionMilliseconds) dropFront();
    }
    DiagnosticProfileType profile_;
    DiagnosticLimits limits_;
    std::shared_ptr<IDiagnosticClock> clock_;
    std::shared_ptr<const DiagnosticDetail::IEvidenceRetention> retention_;
    Json context_;
    std::string start_;
    std::string sessionID_;
    mutable std::mutex mutex_;
    mutable std::deque<StoredEvent> events_;
    mutable std::size_t retainedBytes_ = 0;
    mutable DiagnosticHealth health_;
    std::uint64_t sequence_ = 0;
    std::atomic<std::uint64_t> failures_{0};
};
std::string markdownEscape(std::string_view input) {
    std::string out;
    for (const char c : input) {
        if (c == '<') out += "&lt;";
        else if (c == '>') out += "&gt;";
        else if (c == '&') out += "&amp;";
        else { if (c == '\\' || c == '`' || c == '*' || c == '_' || c == '[' || c == ']' || c == '|') out += '\\'; out += c; }
    }
    return out;
}
class NativeExportWriter final : public IDiagnosticExportWriter {
public:
    void write(const std::string& staging, const std::string& filename, const std::string& bytes) override {
        const auto path = std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(staging.data()), staging.size())) / filename;
        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file.is_open()) throw std::runtime_error("Export file open failed");
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size())); file.flush();
        if (!file) throw std::runtime_error("Export file write or flush failed");
        file.close(); if (file.fail()) throw std::runtime_error("Export file close failed");
        std::ifstream check(path, std::ios::binary);
        const std::string actual((std::istreambuf_iterator<char>(check)), std::istreambuf_iterator<char>());
        if (!check.eof() && check.fail()) throw std::runtime_error("Export file read-back failed");
        if (actual != bytes) throw std::runtime_error("Export file read-back mismatch");
    }
};
std::string pathUtf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}
void checkExportFile(const std::filesystem::path& path, const std::string& expected) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) throw std::runtime_error("Export read-back open failed");
    const std::string actual((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (file.bad() || actual != expected) throw std::runtime_error("Export read-back mismatch");
}
} // namespace

DiagnosticLimits DiagnosticLimits::development() noexcept { return {512, 16384, 4194304, 3600000, 8388608}; }
namespace DiagnosticDetail {
Json redactEvidence(const Json& evidence, DiagnosticHealth& health) { return safeEvidence(evidence, health); }
std::shared_ptr<IDiagnostics> makeCollector(DiagnosticProfileType profile, DiagnosticLimits limits, Json context,
    std::shared_ptr<IDiagnosticClock> clock, std::shared_ptr<const IEvidenceRetention> retention, std::shared_ptr<IDiagnosticIdentity> identity) {
    return std::make_shared<Collector>(profile, limits, std::move(context), std::move(clock), std::move(retention), std::move(identity));
}
}
DiagnosticExport makeDiagnosticExport(const DiagnosticSnapshot& snapshot) {
    if (!snapshot.document.is_object() || snapshot.document.at("schema") != Json("aeostara.diagnostics/1"))
        throw std::invalid_argument("Unsupported diagnostic snapshot");
    DiagnosticHealth recheck;
    Json document = checkedExportNode(snapshot.document, recheck);
    document["exportRedactions"] = recheck.redacted;
    DiagnosticExport result;
    result.json = document.dump(2) + "\n";
    std::ostringstream md;
    md << "# Aeostara diagnostic bundle\n\nBundle: " << markdownEscape(document.at("bundleID").get<std::string>())
       << "\n\nProfile: " << markdownEscape(document.at("profile").get<std::string>())
       << "\n\nThe retained evidence below contains every event, incident, nested error, unavailable reason and loss field from the paired JSON.\n\n";
    md << "## Incident index\n\n";
    if (document.at("incidents").empty()) md << "No retained incidents. Consult loss accounting for expired or dropped events.\n\n";
    for (const auto& incident : document.at("incidents").array_items()) {
        md << "- " << markdownEscape(incident.at("incidentID").get<std::string>()) << ": "
           << markdownEscape(incident.at("triggerPhase").get<std::string>()) << ", "
           << markdownEscape(incident.at("observedOutcome").get<std::string>()) << ". "
           << incident.at("timelineEventIDs").size() << " correlated retained events; causal finding UNKNOWN.\n";
    }
    md << "\n## Event timeline\n\n| Sequence | UTC | Phase | Outcome | Event |\n| --- | --- | --- | --- | --- |\n";
    for (const auto& event : document.at("events").array_items()) {
        md << "| " << event.at("sequence").get<std::uint64_t>() << " | " << markdownEscape(event.at("utc").get<std::string>())
           << " | " << markdownEscape(event.at("phase").get<std::string>()) << " | " << markdownEscape(event.at("outcome").get<std::string>())
           << " | " << markdownEscape(event.at("eventID").get<std::string>()) << " |\n";
    }
    md << "\n## Full retained evidence\n\n";
    md << "```json\n" << result.json << "```\n";
    result.markdown = md.str();
    if (result.json.size() + result.markdown.size() > snapshot.exportByteLimit)
        throw std::length_error("Diagnostic export byte limit exceeded");
    return result;
}
DiagnosticExportResult exportDiagnosticBundle(const DiagnosticSnapshot& snapshot, const std::string& destination,
    IDiagnosticExportWriter* injectedWriter) noexcept {
    DiagnosticExportResult result;
    try {
        const auto payload = makeDiagnosticExport(snapshot);
        const auto finalPath = std::filesystem::absolute(std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(destination.data()), destination.size()))).lexically_normal();
        if (std::filesystem::exists(finalPath)) throw std::runtime_error("Export destination already exists");
        GUID id{}; if (CoCreateGuid(&id) != S_OK) throw std::runtime_error("Export staging identity unavailable");
        wchar_t idText[40]{}; if (!StringFromGUID2(id, idText, 40)) throw std::runtime_error("Export staging identity conversion failed");
        const auto staging = finalPath.parent_path() / (finalPath.filename().wstring() + L".incomplete-" + idText);
        if (!std::filesystem::create_directory(staging)) throw std::runtime_error("Export staging creation failed");
        result.stagingPath = pathUtf8(staging);
        const auto stagingUtf8 = staging.u8string();
        const std::string stagingString(reinterpret_cast<const char*>(stagingUtf8.data()), stagingUtf8.size());
        NativeExportWriter nativeWriter;
        auto& writer = injectedWriter ? *injectedWriter : static_cast<IDiagnosticExportWriter&>(nativeWriter);
        writer.write(stagingString, "diagnostics.json", payload.json);
        checkExportFile(staging / "diagnostics.json", payload.json);
        result.jsonPath = pathUtf8(staging / "diagnostics.json");
        writer.write(stagingString, "diagnostics.md", payload.markdown);
        checkExportFile(staging / "diagnostics.md", payload.markdown);
        result.markdownPath = pathUtf8(staging / "diagnostics.md");
        std::filesystem::rename(staging, finalPath);
        result.stagingPath.clear();
        result.jsonPath = pathUtf8(finalPath / "diagnostics.json"); result.markdownPath = pathUtf8(finalPath / "diagnostics.md");
        result.complete = true;
    } catch (const std::system_error& failure) {
        result.error = "Diagnostic export incomplete; inspect retained snapshot and reported staging paths";
        result.primaryError = DiagnosticError{DiagnosticErrorType::Storage, failure.code().value(), {}};
    } catch (const std::invalid_argument&) {
        result.error = "Diagnostic export input rejected";
        result.primaryError = DiagnosticError{DiagnosticErrorType::Input, {}, {}};
    } catch (const std::length_error&) {
        result.error = "Diagnostic export byte limit exceeded";
        result.primaryError = DiagnosticError{DiagnosticErrorType::Input, {}, {}};
    } catch (...) {
        result.error = "Diagnostic export incomplete; inspect retained snapshot and reported staging paths";
        result.primaryError = DiagnosticError{DiagnosticErrorType::Storage, {}, {}};
    }
    return result;
}
DiagnosticBundle makeDiagnosticBundle(const std::string& kind, const std::string& severity, const std::string& message,
    const Json& evidence, const Json& rules, const std::vector<std::string>& sources,
    const std::string& root, const std::optional<std::string>& parent) {
    if (parent || !root.empty()) throw std::invalid_argument("This factory creates root diagnostics only; child diagnostics require a complete resolved chain");
    Json seed{{"diagnosticKind", kind}, {"severity", severity}, {"message", message}, {"ruleReferences", rules},
        {"sourceReferences", Json(sources)}, {"evidence", evidence}, {"parentDiagnosticID", parent ? Json(*parent) : Json(nullptr)},
        {"suppliedChainRoot", root}};
    const auto diagnosticID = "diagnostic-" + sha256(seed.canonical());
    const auto chainID = root.empty() ? "chain-" + sha256(diagnosticID) : root;
    Json envelope{{"diagnosticID", diagnosticID}, {"chainRootReference", chainID},
        {"parentDiagnosticID", parent ? Json(*parent) : Json(nullptr)}, {"diagnosticKind", kind}, {"severity", severity},
        {"message", message}, {"ruleReferences", rules}, {"sourceReferences", Json(sources)}, {"evidence", evidence}};
    Json reference{{"diagnosticID", diagnosticID}, {"chainRootReference", chainID}, {"diagnosticKind", kind}, {"ruleReferences", rules}};
    Json chain{{"chainID", chainID}, {"rootDiagnosticID", diagnosticID}, {"diagnostics", Json::array({envelope})}};
    (void)BaseContracts::DiagnosticEnvelope::from_json_value(envelope);
    (void)BaseContracts::DiagnosticReference::from_json_value(reference);
    (void)BaseContracts::DiagnosticChain::from_json_value(chain);
    return {std::move(envelope), std::move(reference), std::move(chain)};
}
namespace {
void validateChain(const Json& chain) {
    (void)BaseContracts::DiagnosticChain::from_json_value(chain);
    const auto& diagnostics = chain.at("diagnostics");
    const auto root = chain.at("rootDiagnosticID").get<std::string>();
    if (diagnostics[0].at("diagnosticID") != Json(root) || !diagnostics[0].at("parentDiagnosticID").is_null())
        throw std::invalid_argument("Diagnostic chain has no genuine root");
    std::vector<std::string> ids;
    for (std::size_t i = 0; i < diagnostics.size(); ++i) {
        const auto id = diagnostics[i].at("diagnosticID").get<std::string>();
        if (std::find(ids.begin(), ids.end(), id) != ids.end()) throw std::invalid_argument("Duplicate diagnostic identity");
        if (diagnostics[i].at("chainRootReference") != chain.at("chainID")) throw std::invalid_argument("Unresolved diagnostic chain root");
        if (i && diagnostics[i].at("parentDiagnosticID") != diagnostics[i - 1].at("diagnosticID"))
            throw std::invalid_argument("Diagnostic chain has an unresolved parent");
        ids.push_back(id);
    }
}
int stageRank(const std::string& stage) {
    static constexpr std::string_view stages[] = {"DETECTION", "CLASSIFICATION", "RECOVERY", "ESCALATION", "TERMINAL"};
    const auto found = std::find(std::begin(stages), std::end(stages), stage);
    if (found == std::end(stages)) throw std::invalid_argument("Invalid canonical diagnostic stage");
    return static_cast<int>(found - std::begin(stages));
}
void validateAsh(const Json& ash, const Json& envelope, const std::string& rootID, const Json* parent) {
    static constexpr std::string_view kinds[] = {"STATE_VALIDITY", "RECOVERY", "FALLBACK", "CONTAINMENT", "SAFE_HALT"};
    static constexpr std::string_view dispositions[] = {"RESOLVED", "PENDING", "BLOCKED", "ESCALATED", "TERMINAL"};
    const auto kind = ash.at("diagnostic_kind").get<std::string>();
    const auto disposition = ash.at("disposition").get<std::string>();
    const auto stage = ash.at("stage").get<std::string>();
    if (std::find(std::begin(kinds), std::end(kinds), kind) == std::end(kinds) ||
        std::find(std::begin(dispositions), std::end(dispositions), disposition) == std::end(dispositions))
        throw std::invalid_argument("Invalid canonical diagnostic convention");
    const auto rank = stageRank(stage);
    if (ash.at("subject_reference").get<std::string>().empty() || ash.at("summary").get<std::string>().empty() ||
        ash.at("summary").get<std::string>().find_first_of("\r\n") != std::string::npos || ash.at("notes").empty())
        throw std::invalid_argument("Canonical diagnostic explanation or subject missing");
    for (const auto& note : ash.at("notes").array_items())
        if (note.get<std::string>().empty()) throw std::invalid_argument("Empty canonical diagnostic note");
    if (ash.at("diagnostic_kind") != envelope.at("diagnosticKind") || ash.at("severity") != envelope.at("severity") ||
        ash.at("summary") != envelope.at("message") || ash.at("parent_diagnostic_reference") != envelope.at("parentDiagnosticID") ||
        ash.at("chain_root_reference") != Json(rootID)) throw std::invalid_argument("Canonical/base diagnostic representations disagree");
    Json actualIDs = Json::array();
    static const std::regex rule("^ASH-(STATE|CODEWORD|ADMISSIBILITY|CLASSIFICATION|RECOVERY|FALLBACK|CONTAINMENT|HALT)-[A-Z]+-[0-9]{3}$");
    for (const auto& reference : envelope.at("ruleReferences").array_items()) {
        const auto id = reference.at("ruleID").get<std::string>();
        if (!std::regex_match(id, rule)) throw std::invalid_argument("Noncanonical diagnostic rule family");
        if (reference.at("ruleFamily") != Json(id.substr(0, id.find('-', 4))))
            throw std::invalid_argument("Canonical diagnostic rule family disagrees with identity");
        bool sourceResolved = false;
        for (const auto& source : envelope.at("sourceReferences").array_items())
            if (source == reference.at("sourceReference")) sourceResolved = true;
        if (!sourceResolved) throw std::invalid_argument("Canonical diagnostic rule source is unresolved");
        actualIDs.push_back(id);
    }
    if (actualIDs.empty() || actualIDs != ash.at("rule_ids")) throw std::invalid_argument("Unresolved canonical diagnostic rules");
    if (parent) {
        const auto parentStage = parent->at("stage").get<std::string>();
        if (parentStage == "TERMINAL" || rank < stageRank(parentStage)) throw std::invalid_argument("Diagnostic stage regressed or resumed terminal chain");
        if (stage == "ESCALATION" && parentStage != "RECOVERY" && parentStage != "CLASSIFICATION")
            throw std::invalid_argument("Escalation diagnostic has invalid predecessor");
        if (stage == "TERMINAL" && parentStage != "ESCALATION" && parentStage != "RECOVERY")
            throw std::invalid_argument("Terminal diagnostic has invalid predecessor");
    } else if (stage == "ESCALATION" || stage == "TERMINAL") throw std::invalid_argument("Escalation or terminal diagnostic requires a genuine predecessor");
    if (stage == "TERMINAL" && disposition != "TERMINAL") throw std::invalid_argument("Terminal stage lacks terminal disposition");
}
}
void validateDiagnosticChain(const Json& chain, bool requireCanonicalAsh) {
    validateChain(chain);
    bool hasCanonical = false;
    for (const auto& envelope : chain.at("diagnostics").array_items())
        hasCanonical = hasCanonical || envelope.at("evidence").contains("ashDiagnostic");
    if (!hasCanonical && !requireCanonicalAsh) return;
    const Json* parent = nullptr;
    const auto rootID = chain.at("rootDiagnosticID").get<std::string>();
    for (const auto& envelope : chain.at("diagnostics").array_items()) {
        const auto& ash = envelope.at("evidence").at("ashDiagnostic");
        validateAsh(ash, envelope, rootID, parent);
        parent = &ash;
    }
}
DiagnosticBundle appendDiagnosticBundle(const Json& original, const std::string& kind, const std::string& severity,
    const std::string& message, const Json& evidence, const Json& rules, const std::vector<std::string>& sources) {
    validateChain(original);
    const auto& parent = original.at("diagnostics")[original.at("diagnostics").size() - 1];
    Json seed{{"chain", original}, {"kind", kind}, {"severity", severity}, {"message", message},
        {"evidence", evidence}, {"rules", rules}, {"sources", Json(sources)}};
    const auto id = "diagnostic-" + sha256(seed.canonical());
    Json envelope{{"diagnosticID", id}, {"chainRootReference", original.at("chainID")}, {"parentDiagnosticID", parent.at("diagnosticID")},
        {"diagnosticKind", kind}, {"severity", severity}, {"message", message}, {"ruleReferences", rules}, {"sourceReferences", Json(sources)}, {"evidence", evidence}};
    Json reference{{"diagnosticID", id}, {"chainRootReference", original.at("chainID")}, {"diagnosticKind", kind}, {"ruleReferences", rules}};
    Json chain = original; chain["diagnostics"].push_back(envelope);
    (void)BaseContracts::DiagnosticEnvelope::from_json_value(envelope);
    (void)BaseContracts::DiagnosticReference::from_json_value(reference);
    validateChain(chain);
    return {std::move(envelope), std::move(reference), std::move(chain)};
}
DiagnosticBundle makeAshDiagnosticBundle(const AshDiagnosticContext& context, const std::string& severity,
    const std::string& summary, const Json& evidence, const Json& rules, const std::vector<std::string>& sources, const Json* original) {
    if (!evidence.is_object()) throw std::invalid_argument("Diagnostic evidence must be an object");
    Json ids = Json::array();
    for (const auto& reference : rules.array_items()) ids.push_back(reference.at("ruleID"));
    Json ash{{"diagnostic_kind", context.kind}, {"severity", severity}, {"stage", context.stage}, {"disposition", context.disposition},
        {"subject_reference", context.subjectReference}, {"parent_diagnostic_reference", nullptr}, {"chain_root_reference", nullptr},
        {"rule_ids", ids}, {"summary", summary}, {"notes", Json(context.notes)}};
    if (original) {
        validateChain(*original);
        const auto rootID = original->at("rootDiagnosticID").get<std::string>();
        const Json* previous = nullptr;
        for (const auto& entry : original->at("diagnostics").array_items()) {
            const auto& actual = entry.at("evidence").at("ashDiagnostic");
            validateAsh(actual, entry, rootID, previous); previous = &actual;
        }
        ash["parent_diagnostic_reference"] = original->at("diagnostics")[original->at("diagnostics").size() - 1].at("diagnosticID");
        ash["chain_root_reference"] = rootID;
    }
    Json actualEvidence = evidence; actualEvidence["ashDiagnostic"] = ash;
    auto bundle = original ? appendDiagnosticBundle(*original, context.kind, severity, summary, actualEvidence, rules, sources)
                           : makeDiagnosticBundle(context.kind, severity, summary, actualEvidence, rules, sources);
    if (!original) ash["chain_root_reference"] = bundle.envelope.at("diagnosticID");
    // The root identity derives from the reference-free canonical content above;
    // resolved self references do not recursively participate in their own hash.
    bundle.envelope["evidence"]["ashDiagnostic"] = ash;
    bundle.chain["diagnostics"][bundle.chain.at("diagnostics").size() - 1] = bundle.envelope;
    const Json* previous = original ? &original->at("diagnostics")[original->at("diagnostics").size() - 1].at("evidence").at("ashDiagnostic") : nullptr;
    validateAsh(ash, bundle.envelope, bundle.chain.at("rootDiagnosticID").get<std::string>(), previous);
    validateChain(bundle.chain);
    return bundle;
}
} // namespace Aeostara
