// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include <AeostaraCore/HealingEngine.h>
#include <AeostaraCore/JsonConfigAdapter.h>
#include <AeostaraCore/BackupManager.h>
#include <AeostaraCore/Version.h>
#include <AeostaraCore/NativeRequestBinding.h>
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
void scenario(const Json& seed, const std::filesystem::path& directory, int kind) {
    std::filesystem::create_directories(directory);
    auto fs = std::make_shared<DefaultFileSystem>(); const auto path = utf8(directory / "configuration.json");
    fs->writeFile(path, seed.at("observedSnapshot").at("rawUtf8").get<std::string>());
    const auto original = fs->readFile(path); JsonConfigAdapter captureAdapter(fs);
    auto captured = bindCapturedNativeTarget(seed, path, *fs, captureAdapter);
    if (auto error = std::get_if<OperationFailure>(&captured)) throw std::runtime_error(error->message);
    auto input = std::get<Json>(std::move(captured));
    if (kind == 1) input["policyContext"]["bundleSha256"] = std::string(64, '0');
    auto environment = std::make_shared<SemanticEnvironment>(SemanticEnvironment::adoptedDirect());
    auto diagnostics = createDefaultDiagnostics();
    std::shared_ptr<IConfigAdapter> adapter = kind == 2 ? std::shared_ptr<IConfigAdapter>(std::make_shared<FaultAdapter>(fs)) : std::shared_ptr<IConfigAdapter>(std::make_shared<JsonConfigAdapter>(fs));
    HealingEngine engine(adapter, std::make_shared<BackupManager>(fs), fs, environment, diagnostics);
    const auto validation = engine.validateSemantic(path, input); const auto difference = engine.diffSemantic(path, input);
    require(fs->readFile(path) == original, "Projection/diff mutated configuration");
    if (kind == 0 || kind == 2) require(difference.semanticPlan.has_value(), "Complete authorized input did not produce plan");
    const auto result = engine.healSemantic(path, input, utf8(directory / "audit.jsonl"));
    if (kind == 0) {
        require(result.success && result.verification.success, "Authorized repair was not independently verified");
        require(Json::parse(fs->readFile(path)) == seed.at("desiredSnapshot").at("document"), "Measured postimage differs");
        require(result.backupPath && fs->readFile(*result.backupPath) == original, "Backup differs from captured bytes");
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
              << " originalRestored=" << (fs->readFile(path) == original) << " pairedExport=true\n";
}
}
int wmain(int argc, wchar_t** argv) {
    if (argc != 3) { std::cerr << "Usage: AeostaraSdkClient synthetic-input.json NEW-output-directory\n"; return 2; }
    try {
        const auto destination = std::filesystem::absolute(argv[2]); require(!std::filesystem::exists(destination), "Output directory already exists");
        const auto input = Json::parse(read(argv[1]));
        for (int kind = 0; kind < 3; ++kind) scenario(input, destination / ("scenario-" + std::to_string(kind)), kind);
        std::cout << "SDK=" << AeostaraVersion::toString() << " publicConsumerAssertions=passed\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
