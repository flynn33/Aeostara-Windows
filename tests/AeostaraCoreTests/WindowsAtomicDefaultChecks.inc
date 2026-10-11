// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual
static unsigned runWindowsAtomicDefaultChecks() {
    unsigned checks = 0;
    const auto require = [&](bool condition) { ++checks; if (!condition) throw std::runtime_error("Atomic boundary assertion " + std::to_string(checks)); };
    const auto directory = std::filesystem::temp_directory_path() / ("aeostara-atomic-" + sha256(std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())).substr(0, 16));
    std::filesystem::create_directory(directory);
    const auto target = directory / "observed.json", other = directory / "other.json", missing = directory / "missing.json";
    { std::ofstream output(target, std::ios::binary); output << "{\"actual\":1}"; }
    {
        WindowsAtomicFileSystem fs(directory.string());
        auto present = fs.exists(target.string()); require(std::holds_alternative<bool>(present) && std::get<bool>(present));
        auto absent = fs.exists(missing.string()); require(std::holds_alternative<bool>(absent) && !std::get<bool>(absent));
        auto read = fs.read(target.string()); require(std::holds_alternative<ByteSequence>(read)); require(std::get<ByteSequence>(read).size() == 12);
        auto hash = fs.computeSHA256(target.string()); require(std::holds_alternative<std::string>(hash) && std::get<std::string>(hash) == sha256("{\"actual\":1}"));
        ByteSequence bytes{std::byte{'x'}};
        for (const auto& result : {fs.writeAtomically(target.string(), bytes), fs.appendAtomically(target.string(), bytes), fs.copyAtomically(target.string(), other.string())}) {
            require(std::holds_alternative<OperationFailure>(result)); const auto& error = std::get<OperationFailure>(result);
            require(error.code == OperationCode::Blocked && error.mutationState == MutationState::NotStarted); require(!error.diagnosticChain.is_null());
        }
        require(std::get<std::string>(fs.computeSHA256(target.string())) == sha256("{\"actual\":1}")); require(!std::filesystem::exists(other));
        require(std::holds_alternative<OperationFailure>(fs.read(missing.string())));
        require(std::holds_alternative<OperationFailure>(fs.read(directory.string())));
        require(std::holds_alternative<OperationFailure>(fs.read((directory / ".." / "foreign.json").string())));
        require(std::holds_alternative<OperationFailure>(fs.read(target.string() + ":stream")));
        require(std::holds_alternative<OperationFailure>(fs.read("relative.json")));
        require(std::holds_alternative<OperationFailure>(fs.read(target.string() + std::string("\0suffix", 7))));
        require(!fs.diagnosticChains().empty());
    }
    std::filesystem::remove(target); std::filesystem::remove(directory);
    return checks;
}
