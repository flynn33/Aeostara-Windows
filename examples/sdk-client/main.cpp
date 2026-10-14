// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include <AeostaraCore/HealingEngine.h>
#include <AeostaraCore/JsonConfigAdapter.h>
#include <AeostaraCore/BackupManager.h>
#include <AeostaraCore/Version.h>
#include <AeostaraCore/NativeRequestBinding.h>
#include <AeostaraCore/BaseAuditBindings.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
using namespace Aeostara;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
std::string utf8(const std::filesystem::path& path) {
    const auto bytes = path.u8string(); return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}
std::string read(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary); require(bool(stream), "Cannot open synthetic input");
    std::ostringstream bytes; bytes << stream.rdbuf(); require(!stream.bad(), "Cannot read synthetic input"); return bytes.str();
}
class FaultAdapter final : public IConfigAdapter {
public:
    explicit FaultAdapter(std::shared_ptr<IFileSystem> fs) : fs_(std::move(fs)), actual_(fs_) {}
    ObservedState observe(const std::string& path) override { return actual_.observe(path); }
    EncodedState encode(const ObservedState& observed, const DesiredState& desired) override { return actual_.encode(observed, desired); }
    bool applyRepair(const std::string&, const RepairPlan&) override { throw std::runtime_error("Semantic operations required"); }
    bool applyOperations(const std::string& path, const Json& operations, const std::string& hash) override {
        const bool result = actual_.applyOperations(path, operations, hash);
        auto corrupt = Json::parse(fs_->readFile(path)); corrupt.replace_pointer("/state_vector/0", Json(1));
        fs_->writeFile(path, corrupt.dump()); return result;
    }
private:
    std::shared_ptr<IFileSystem> fs_; JsonConfigAdapter actual_;
};
class MeasuredFileSystem final : public IFileSystem {
public:
    explicit MeasuredFileSystem(std::string target) : target_(std::move(target)) {}
    std::size_t configurationWrites = 0, targetLeaseRequests = 0;
    std::string readFile(const std::string& path) override { return actual_.readFile(path); }
    void writeFile(const std::string& path, const std::string& bytes) override {
        if (path == target_) ++configurationWrites; actual_.writeFile(path, bytes);
    }
    bool fileExists(const std::string& path) override { return actual_.fileExists(path); }
    bool copyFile(const std::string& from, const std::string& to) override {
        if (to == target_) ++configurationWrites; return actual_.copyFile(from, to);
    }
    bool writeFileExclusive(const std::string& path, const std::string& bytes) override {
        if (path == target_) ++configurationWrites; return actual_.writeFileExclusive(path, bytes);
    }
    std::string pathIdentity(const std::string& path) override { return actual_.pathIdentity(path); }
    std::string versionToken(const std::string& path) override { return actual_.versionToken(path); }
    std::unique_ptr<ITargetWriteLease> acquireWriteLease(const std::string& path) override {
        if (path == target_) ++targetLeaseRequests; return actual_.acquireWriteLease(path);
    }
private:
    std::string target_; DefaultFileSystem actual_;
};
class MeasuredBackup final : public IBackupProvider {
public:
    explicit MeasuredBackup(std::shared_ptr<IFileSystem> fs) : actual_(std::move(fs)) {}
    std::size_t creations = 0, restorations = 0;
    std::string createBackup(const std::string& path) override { ++creations; return actual_.createBackup(path); }
    bool restoreBackup(const std::string& backup, const std::string& original) override {
        ++restorations; return actual_.restoreBackup(backup, original);
    }
    bool supportsBackup() const noexcept override { return actual_.supportsBackup(); }
private:
    BackupManager actual_;
};
Json readOnlyInput(const Json& seed) {
    auto input = seed;
    input["observedSnapshot"]["document"] = seed.at("desiredSnapshot").at("document");
    input["observedSnapshot"]["rawUtf8"] = seed.at("desiredSnapshot").at("rawUtf8");
    input["observedSnapshot"]["rawByteSha256"] = seed.at("desiredSnapshot").at("rawByteSha256");
    input["observedSnapshot"]["proposedCanonicalSha256"] = seed.at("desiredSnapshot").at("proposedCanonicalSha256");
    input["lifecycleContext"]["phase"] = "PROJECT";
    input["lifecycleContext"]["correctionPath"] = Json{{"status", "not_evaluated"}, {"codewords", Json::array()}};
    input["policyContext"] = Json{{"revision", "demo-policy-r1"}, {"authorizedTargetPolicyIDs", Json::array()}, {"mutationAuthorization", "not_requested"}};
    input["selectedTargetReference"] = Json{{"availability", "not_selected"}, {"reason", "Read-only request selects no recovery target"}};
    input["realizationRuleReference"] = Json{{"availability", "not_selected"}, {"reason", "Read-only request grants no mutation authority"}};
    input["executionContext"] = Json{{"availability", "not_started"}, {"backupEvidence", "not_requested"}, {"verificationPlan", "not_generated"}};
    input.object_items().erase("proposedPlan"); return input;
}
void scenario(const Json& seed, const std::filesystem::path& directory, int kind) {
    std::filesystem::create_directories(directory);
    const auto path = utf8(directory / "configuration.json"); auto fs = std::make_shared<MeasuredFileSystem>(path);
    const auto request = kind == 3 ? readOnlyInput(seed) : seed;
    fs->writeFile(path, request.at("observedSnapshot").at("rawUtf8").get<std::string>());
    fs->configurationWrites = 0;
    const auto original = fs->readFile(path); JsonConfigAdapter captureAdapter(fs);
    auto captured = bindCapturedNativeTarget(request, path, *fs, captureAdapter);
    if (auto error = std::get_if<OperationFailure>(&captured)) throw std::runtime_error(error->message);
    auto input = std::get<Json>(std::move(captured));
    if (kind == 1) input["policyContext"]["bundleSha256"] = std::string(64, '0');
    auto environment = std::make_shared<SemanticEnvironment>(SemanticEnvironment::adoptedDirect());
    auto diagnostics = createDefaultDiagnostics();
    std::shared_ptr<IConfigAdapter> adapter = kind == 2 ? std::shared_ptr<IConfigAdapter>(std::make_shared<FaultAdapter>(fs)) : std::shared_ptr<IConfigAdapter>(std::make_shared<JsonConfigAdapter>(fs));
    auto backup = std::make_shared<MeasuredBackup>(fs);
    HealingEngine engine(adapter, backup, fs, environment, diagnostics);
    const auto validation = engine.validateSemantic(path, input); const auto difference = engine.diffSemantic(path, input);
    require(fs->readFile(path) == original, "Projection/diff mutated configuration");
    if (kind == 0 || kind == 2) require(difference.semanticPlan.has_value(), "Complete authorized input did not produce plan");
    const auto auditPath = utf8(directory / "audit.jsonl");
    const auto result = engine.healSemantic(path, input, auditPath);
    if (kind == 0) {
        require(result.success && result.verification.success, "Authorized repair was not independently verified");
        require(Json::parse(fs->readFile(path)) == seed.at("desiredSnapshot").at("document"), "Measured postimage differs");
        require(result.backupPath && fs->readFile(*result.backupPath) == original, "Backup differs from captured bytes");
    } else if (kind == 3) {
        require(validation.valid && !validation.failure, "Read-only validation did not succeed");
        require(validation.semanticAnalysis && validation.semanticAnalysis->stateClass == SystemClass::Stable &&
            validation.semanticAnalysis->recovery == RecoveryCategory::NoAction, "Captured healthy state did not independently classify STABLE/NO_ACTION");
        require(difference.semanticPlan && difference.semanticPlan->operations.empty(), "Read-only planning produced mutation operations");
        require(result.success && result.verification.success && !result.failure, "Read-only lifecycle did not succeed");
        require(fs->configurationWrites == 0 && fs->targetLeaseRequests == 0 && backup->creations == 0 && backup->restorations == 0 &&
            !result.backupPath && !result.mutationMayHaveOccurred && fs->readFile(path) == original, "Read-only lifecycle reached a mutation or backup capability");
        require(!result.auditChain.is_null() && !result.diagnosticChain.is_null() && fs->fileExists(auditPath), "Read-only required business audit/diagnostic evidence unavailable");
        BaseBindings::BaseAuditSink persistedAudit(auditPath, fs);
        const auto stored = persistedAudit.readChain(result.auditChain.at("auditChainID").get<std::string>());
        require(std::holds_alternative<BaseContracts::AuditChain>(stored) && std::get<BaseContracts::AuditChain>(stored).value() == result.auditChain,
            "Read-only business audit was not independently reconstructed from actual storage");
    } else {
        require(!result.success && result.failure.has_value(), "Rejected/failed healing claimed success");
        require(fs->readFile(path) == original, "Rejected repair or verified restore changed original bytes");
        if (kind == 2) require(result.restoration == RestorationStatus::Verified, "Forced verification failure was not truthfully restored");
        else require(!result.mutationMayHaveOccurred, "Rejected policy reached configuration mutation");
    }
    require(diagnostics->profile() == DiagnosticProfileType::Lean, "Installed SDK contains development collector");
    const auto exported = exportDiagnosticBundle(diagnostics->snapshot(), utf8(directory / "incident-bundle"));
    require(exported.complete && fs->fileExists(exported.jsonPath) && fs->fileExists(exported.markdownPath), "Paired diagnostic export incomplete");
    std::cout << "scenario=" << kind << " validate=" << validation.valid << " healing=" << result.success
              << " originalRestored=" << (fs->readFile(path) == original) << " configurationWrites=" << fs->configurationWrites
              << " backupCreations=" << backup->creations << " pairedExport=true\n";
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) { std::cerr << "Usage: AeostaraSdkClient synthetic-input.json NEW-output-directory\n"; return 2; }
    try {
        const auto destination = std::filesystem::absolute(argv[2]); require(!std::filesystem::exists(destination), "Output directory already exists");
        const auto input = Json::parse(read(argv[1]));
        for (int kind = 0; kind < 4; ++kind) scenario(input, destination / ("scenario-" + std::to_string(kind)), kind);
        require(AeostaraVersion::toString() == AEOSTARA_SDK_VERSION, "Installed binary API and public header candidate versions differ");
        std::cout << "SDK=" << AeostaraVersion::toString() << " publicConsumerAssertions=passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
