// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include "AeostaraCore/Diagnostics.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <windows.h>
using namespace Aeostara;
namespace {
std::filesystem::path nativePath(const std::string& utf8) { return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(utf8.data()), utf8.size())); }
int assertions = 0;
void check(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
class FixedClock final : public IDiagnosticClock {
public:
    std::uint64_t now = 100;
    std::string utcNow() const override { return "2026-10-11T20:00:00Z"; }
    std::uint64_t monotonicMilliseconds() const override { return now; }
};
class FixedIdentity final : public IDiagnosticIdentity {
public:
    std::string sessionIdentity() const override { return "SYNTHETIC_SESSION_ID"; }
};
class LyingWriter final : public IDiagnosticExportWriter {
public:
    void write(const std::string&, const std::string&, const std::string&) override {}
};
class PartialWriter final : public IDiagnosticExportWriter {
public:
    void write(const std::string& directory, const std::string& filename, const std::string& bytes) override {
        std::ofstream out(nativePath(directory) / filename, std::ios::binary);
        out << bytes.substr(0, bytes.size() / 2); out.close();
        throw std::runtime_error("Injected partial write");
    }
};
}
int main(int argc, char** argv) {
    try {
        const bool development = argc > 1 && std::string(argv[1]) == "development";
        check(createDefaultDiagnostics()->profile() == (development ? DiagnosticProfileType::Development : DiagnosticProfileType::Lean), "Compiled default diagnostic profile differs");
        auto clock = std::make_shared<FixedClock>();
        auto limits = DiagnosticLimits::development(); limits.eventCount = 2; limits.retentionMilliseconds = 50;
        auto diagnostics = createDiagnostics(development ? DiagnosticProfileType::Development : DiagnosticProfileType::Lean, limits,
            Json{{"sourceCommit", "9bf3b7d8dfc0715889121697f0b628e8a1dd8856"}, {"credential", "CONTEXT_SECRET_CANARY"}}, clock, std::make_shared<FixedIdentity>());
        DiagnosticEvent event; event.operationID = "OPERATION_SECRET_CANARY"; event.phase = DiagnosticPhaseType::Policy;
        event.outcome = DiagnosticOutcomeType::Blocked; event.objectHash = std::string(64, 'a');
        event.ruleReferences = {"RULE_SECRET_CANARY"};
        event.primaryError = DiagnosticError{DiagnosticErrorType::Policy, 5, {{DiagnosticErrorType::Storage, 32, {}}}};
        event.secondaryErrors = {{DiagnosticErrorType::Recovery, 19, {}}};
        event.evidence = Json{{"allowed", false}, {"credential", "EVIDENCE_SECRET_CANARY"}, {"stateClass", "FAILED"}};
        diagnostics->capture(event);
        auto snapshot = diagnostics->snapshot();
        const auto retained = snapshot.document.canonical();
        check(retained.find("SECRET_CANARY") == std::string::npos, "Pre-retention redaction failed");
        check(snapshot.document.at("events").size() == 1, "Event missing");
        check(snapshot.document.at("sessionID") == diagnostics->snapshot().document.at("sessionID"), "Session identity changed");
        check(snapshot.document.at("buildContext").at("osBuild").get<unsigned long>() > 0, "Native OS build was not obtained on this acceptance host");
        check(snapshot.document.at("incidents").size() == 1, "Incident missing");
        check(snapshot.document.at("events")[0].at("primaryError").at("causes").size() == 1, "Nested cause missing");
        check(snapshot.document.at("events")[0].at("evidence").contains("allowed") == development, "Backend profile composition differs");
        snapshot.document["credential"] = "EXPORT_ROOT_SECRET_CANARY";
        snapshot.document["events"][0]["component"] = "EXPORT_METADATA_SECRET_CANARY";
        const auto exported = makeDiagnosticExport(snapshot);
        check(exported.json.find("SECRET_CANARY") == std::string::npos, "Export JSON privacy recheck failed");
        check(exported.markdown.find("SECRET_CANARY") == std::string::npos, "Export Markdown privacy recheck failed");
        check(exported.markdown.find(exported.json) != std::string::npos, "Markdown lost detailed JSON fields");
        diagnostics->capture(event); diagnostics->capture(event);
        check(diagnostics->snapshot().document.at("events").size() == 2, "Event quota failed");
        check(diagnostics->health().dropped == 1, "Dropped event count missing");
        clock->now = 151;
        check(diagnostics->snapshot().document.at("events").empty(), "TTL expiry failed");
        check(diagnostics->health().dropped == 3, "TTL loss count missing");
        clock->now = 152; diagnostics->capture(event);
        auto latest = diagnostics->snapshot();
        auto tiny = latest; tiny.exportByteLimit = 1;
        bool limited = false; try { (void)makeDiagnosticExport(tiny); } catch (const std::length_error&) { limited = true; }
        check(limited, "Export byte limit failed");
        const auto base = std::filesystem::temp_directory_path() / (development ? L"Aeostara diagnostics development" : L"Aeostara diagnostics lean");
        std::filesystem::create_directories(base);
        const auto identity = std::to_string(clock->now) + "-" + std::to_string(GetCurrentProcessId());
        auto utf8 = [](const std::filesystem::path& path) { const auto value = path.u8string(); return std::string(reinterpret_cast<const char*>(value.data()), value.size()); };
        const auto good = exportDiagnosticBundle(latest, utf8(base / (L"Unicode \u03a9 " + std::wstring(identity.begin(), identity.end()))));
        check(good.complete && std::filesystem::exists(nativePath(good.jsonPath)) && std::filesystem::exists(nativePath(good.markdownPath)), "Complete export missing");
        const auto collision = exportDiagnosticBundle(latest, utf8(base / (L"Unicode \u03a9 " + std::wstring(identity.begin(), identity.end()))));
        check(!collision.complete, "Existing export overwritten");
        LyingWriter liar;
        const auto dishonest = exportDiagnosticBundle(latest, utf8(base / ("liar-" + identity)), &liar);
        check(!dishonest.complete && !dishonest.stagingPath.empty(), "Unchecked writer claimed complete");
        PartialWriter partial;
        const auto interrupted = exportDiagnosticBundle(latest, utf8(base / ("partial-" + identity)), &partial);
        check(!interrupted.complete && !interrupted.stagingPath.empty(), "Partial export claimed complete");
        bool rejected = false;
        try { (void)makeDiagnosticBundle("MAPPING", "ERROR", "Actual mapping failure", Json::object(), Json::array(), {}, "existing-chain", "parent"); }
        catch (const std::invalid_argument&) { rejected = true; }
        check(rejected, "Fabricated diagnostic chain accepted");
        const Json rules = Json::array({Json{{"ruleID", "ASH-STA-VAL-001"}, {"sourceReference", "source/test"}, {"ruleFamily", "state"}}});
        const auto bundle = makeDiagnosticBundle("MAPPING", "ERROR", "Actual mapping failure", Json::object(), rules, {"source/test"});
        check(bundle.chain.at("rootDiagnosticID") == bundle.envelope.at("diagnosticID"), "Root diagnostic not resolved");
        check(bundle.reference.at("chainRootReference") == bundle.chain.at("chainID"), "Chain reference not resolved");
        auto canonicalRules = [](const std::string& id) { return Json::array({Json{{"ruleID", id}, {"sourceReference", "synthetic/source-bound-rule"}, {"ruleFamily", id.substr(0, id.find('-', 4))}}}); };
        AshDiagnosticContext detected{"STATE_VALIDITY", "DETECTION", "BLOCKED", "synthetic-subject", {"Actual supplied state is diagnosed"}};
        const auto detectedBundle = makeAshDiagnosticBundle(detected, "ERROR", "State requires recovery", Json::object(),
            canonicalRules("ASH-STATE-VALIDITY-001"), {"synthetic/source-bound-rule"});
        const auto same = makeAshDiagnosticBundle(detected, "ERROR", "State requires recovery", Json::object(),
            canonicalRules("ASH-STATE-VALIDITY-001"), {"synthetic/source-bound-rule"});
        check(same.chain == detectedBundle.chain, "Canonical diagnostics are not deterministic");
        const auto& canonical = detectedBundle.envelope.at("evidence").at("ashDiagnostic");
        check(canonical.at("chain_root_reference") == detectedBundle.envelope.at("diagnosticID"), "Canonical root must reference the actual originating diagnostic");
        AshDiagnosticContext recovered{"RECOVERY", "RECOVERY", "BLOCKED", "synthetic-subject", {"Recovery was actually blocked"}};
        const auto recovery = makeAshDiagnosticBundle(recovered, "ERROR", "Recovery blocked", Json::object(),
            canonicalRules("ASH-RECOVERY-ACTION-001"), {"synthetic/source-bound-rule"}, &detectedBundle.chain);
        check(recovery.envelope.at("parentDiagnosticID") == detectedBundle.envelope.at("diagnosticID"), "Recovery parent missing");
        check(recovery.chain.at("diagnostics").size() == 2, "Prior diagnostic omitted");
        AshDiagnosticContext contained{"CONTAINMENT", "ESCALATION", "ESCALATED", "synthetic-subject", {"Recovery failure requires containment"}};
        const auto containment = makeAshDiagnosticBundle(contained, "CRITICAL", "Containment entered", Json::object(),
            canonicalRules("ASH-CONTAINMENT-TRIGGER-001"), {"synthetic/source-bound-rule"}, &recovery.chain);
        AshDiagnosticContext halted{"SAFE_HALT", "TERMINAL", "TERMINAL", "synthetic-subject", {"Containment could not be maintained"}};
        const auto halt = makeAshDiagnosticBundle(halted, "CRITICAL", "Safe halt entered", Json::object(),
            canonicalRules("ASH-HALT-TRIGGER-001"), {"synthetic/source-bound-rule"}, &containment.chain);
        check(halt.chain.at("diagnostics").size() == 4, "Full terminal chain missing");
        bool resumeRejected = false;
        try { (void)makeAshDiagnosticBundle(recovered, "ERROR", "Attempted resume", Json::object(), canonicalRules("ASH-RECOVERY-ACTION-001"), {"synthetic/source-bound-rule"}, &halt.chain); }
        catch (const std::invalid_argument&) { resumeRejected = true; }
        check(resumeRejected, "Terminal chain resumed");
        auto broken = recovery.chain; broken["diagnostics"][1]["parentDiagnosticID"] = "missing-parent";
        bool orphanRejected = false;
        try { (void)appendDiagnosticBundle(broken, "RECOVERY", "ERROR", "Attempted orphan", Json::object(), rules, {"source/test"}); }
        catch (const std::invalid_argument&) { orphanRejected = true; }
        check(orphanRejected, "Unresolved parent accepted");
        validateDiagnosticChain(halt.chain, true);
        auto wrongFamily = halt.chain;
        wrongFamily["diagnostics"][0]["ruleReferences"][0]["ruleFamily"] = "ASH-HALT";
        bool familyRejected = false;
        try { validateDiagnosticChain(wrongFamily, true); } catch (const std::invalid_argument&) { familyRejected = true; }
        check(familyRejected, "Canonical rule family mismatch accepted");
        auto unresolvedSource = halt.chain;
        unresolvedSource["diagnostics"][0]["ruleReferences"][0]["sourceReference"] = "absent/source";
        bool sourceRejected = false;
        try { validateDiagnosticChain(unresolvedSource, true); } catch (const std::invalid_argument&) { sourceRejected = true; }
        check(sourceRejected, "Canonical rule source mismatch accepted");
        std::cout << assertions << " diagnostic assertions passed (" << (development ? "development" : "lean") << ")\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
