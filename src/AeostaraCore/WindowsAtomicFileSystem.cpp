// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/WindowsAtomicFileSystem.h"
#include "AeostaraCore/Diagnostics.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <bcrypt.h>
#include <array>
#include <filesystem>
#include <limits>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
namespace Aeostara::BaseBindings {
namespace {
class NativeFailure final : public std::runtime_error {
public:
    NativeFailure(const char* operation, DWORD code) : std::runtime_error(std::string(operation) + "; Windows error=" + std::to_string(code)), code_(code) {}
    DWORD code() const noexcept { return code_; }
private: DWORD code_;
};
[[noreturn]] void nativeError(const char* operation) { throw NativeFailure(operation, GetLastError()); }
struct Handle final {
    HANDLE value = INVALID_HANDLE_VALUE;
    explicit Handle(HANDLE handle = INVALID_HANDLE_VALUE) : value(handle) {}
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    Handle(Handle&& other) noexcept : value(other.value) { other.value = INVALID_HANDLE_VALUE; }
    Handle& operator=(Handle&& other) noexcept { if (this != &other) { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); value = other.value; other.value = INVALID_HANDLE_VALUE; } return *this; }
};
std::mutex& publicationMutex() { static std::mutex value; return value; }
std::wstring pathText(const Path& value) {
    if (value.empty() || value.size() > 32760 || value.find('\0') != std::string::npos) throw std::invalid_argument("Invalid bounded UTF8 path");
    const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (!count) throw std::invalid_argument("Path contains malformed UTF8");
    std::wstring result(static_cast<std::size_t>(count), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), count)) nativeError("UTF16 path conversion");
    for (auto& c : result) if (c == L'/') c = L'\\';
    if (result.size() < 3 || result[1] != L':' || result[2] != L'\\' ||
        !((result[0] >= L'A' && result[0] <= L'Z') || (result[0] >= L'a' && result[0] <= L'z'))) throw std::invalid_argument("Only absolute local drive paths are supported");
    const std::filesystem::path parsed(result);
    for (const auto& item : parsed.relative_path()) { const auto text = item.wstring();
        if (text.empty() || text == L"." || text == L".." || text.back() == L'.' || text.back() == L' ' || text.find_first_of(L":*?\"<>|") != std::wstring::npos) throw std::invalid_argument("Ambiguous or unsupported path component"); }
    return result;
}
bool samePath(const std::wstring& a, const std::wstring& b) { return CompareStringOrdinal(a.c_str(), static_cast<int>(a.size()), b.c_str(), static_cast<int>(b.size()), TRUE) == CSTR_EQUAL; }
std::string identity(HANDLE handle) { BY_HANDLE_FILE_INFORMATION info{}; if (!GetFileInformationByHandle(handle, &info)) nativeError("Resolve file identity");
    return "file:" + std::to_string(info.dwVolumeSerialNumber) + ":" + std::to_string(info.nFileIndexHigh) + ":" + std::to_string(info.nFileIndexLow); }
void requireRegular(HANDLE handle) { BY_HANDLE_FILE_INFORMATION info{}; if (!GetFileInformationByHandle(handle, &info)) nativeError("Read attributes");
    if ((info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY)) != 0 || GetFileType(handle) != FILE_TYPE_DISK) throw std::invalid_argument("Path is not a regular non-reparse disk file"); }
std::string readHandle(HANDLE handle) { LARGE_INTEGER size{}, zero{}; if (!GetFileSizeEx(handle, &size)) nativeError("Read file size");
    if (size.QuadPart < 0 || size.QuadPart > static_cast<LONGLONG>(JsonLimits::rawDocumentBytes)) throw std::length_error("File exceeds adopted raw byte limit");
    if (!SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) nativeError("Seek for bounded read");
    std::string bytes(static_cast<std::size_t>(size.QuadPart), '\0'); DWORD read = 0;
    if (!bytes.empty() && (!ReadFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) || read != bytes.size())) nativeError("Complete bounded read");
    return bytes;
}
ByteSequence byteSequence(const std::string& bytes) { ByteSequence result; result.reserve(bytes.size()); for (const auto c : bytes) result.push_back(static_cast<std::byte>(static_cast<unsigned char>(c))); return result; }
std::string byteString(const ByteSequence& bytes) { if (bytes.size() > JsonLimits::rawDocumentBytes) throw std::length_error("Operation exceeds adopted raw byte limit"); std::string result; result.reserve(bytes.size()); for (const auto c : bytes) result.push_back(static_cast<char>(std::to_integer<unsigned char>(c))); return result; }
std::wstring randomSuffix() { std::array<unsigned char, 16> random{}; if (BCryptGenRandom(nullptr, random.data(), static_cast<ULONG>(random.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) throw std::runtime_error("Native staging identity unavailable");
    constexpr wchar_t digits[] = L"0123456789abcdef"; std::wstring result; for (const auto c : random) { result.push_back(digits[c >> 4]); result.push_back(digits[c & 15]); } return result; }
Json referenceFor(const Json& envelope) { return Json{{"diagnosticID", envelope.at("diagnosticID")}, {"chainRootReference", envelope.at("chainRootReference")}, {"diagnosticKind", envelope.at("diagnosticKind")}, {"ruleReferences", envelope.at("ruleReferences")}}; }
}
struct WindowsAtomicFileSystem::State final {
    std::wstring directory;
    std::vector<Handle> ancestors;
    std::shared_ptr<const AtomicPersistenceContext> persistence;
    mutable std::recursive_mutex mutex;
    mutable std::vector<BaseContracts::DiagnosticChain> chains;
    mutable std::size_t diagnosticBytes = 0;
    explicit State(const Path& root, std::shared_ptr<const AtomicPersistenceContext> context) : directory(pathText(root)), persistence(std::move(context)) {
        while (directory.size() > 3 && directory.back() == L'\\') directory.pop_back();
        const auto drive = directory.substr(0, 3); if (GetDriveTypeW(drive.c_str()) != DRIVE_FIXED) throw std::invalid_argument("Atomic publication requires a local fixed drive");
        std::array<wchar_t, 32> filesystem{};
        if (!GetVolumeInformationW(drive.c_str(), nullptr, 0, nullptr, nullptr, nullptr, filesystem.data(), static_cast<DWORD>(filesystem.size()))) nativeError("Resolve local filesystem");
        if (!samePath(filesystem.data(), L"NTFS")) throw std::invalid_argument("Atomic publication binding currently supports local NTFS only");
        auto current = std::filesystem::path(drive);
        const auto retain = [&](const std::filesystem::path& path) { Handle handle(CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
            if (handle.value == INVALID_HANDLE_VALUE) nativeError("Retain directory namespace");
            BY_HANDLE_FILE_INFORMATION info{}; if (!GetFileInformationByHandle(handle.value, &info)) nativeError("Inspect retained directory");
            if ((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 || (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0) throw std::invalid_argument("Directory ancestry is missing or contains a reparse point"); ancestors.push_back(std::move(handle)); };
        retain(current); for (const auto& part : std::filesystem::path(directory).relative_path()) { current /= part; retain(current); }
    }
    std::wstring checked(const Path& path) const { const auto text = pathText(path); if (!samePath(std::filesystem::path(text).parent_path().wstring(), directory)) throw std::invalid_argument("Path is outside the owned direct-child directory scope"); return text; }
    Handle open(const Path& path, bool allowMissing = false) const { const auto text = checked(path); Handle handle(CreateFileW(text.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        if (handle.value == INVALID_HANDLE_VALUE) { const auto error = GetLastError(); if (allowMissing && error == ERROR_FILE_NOT_FOUND) return handle; throw NativeFailure("Open scoped regular file", error); } requireRegular(handle.value); return handle; }
    DiagnosticBundle diagnostic(const std::string& operation, const Json& evidence, bool failed) const {
        const std::string source = "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/recoverability-semantics.pseudo.md";
        const std::string interfaceSource = "https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/interfaces/IFileSystem.pseudo.md";
        const Json* parent = persistence ? &persistence->diagnosticChain().value() : nullptr;
        const auto bundle = makeAshDiagnosticBundle(AshDiagnosticContext{"RECOVERY", "RECOVERY", failed ? "BLOCKED" : "RESOLVED", "filesystem-" + sha256(Json{{"operation", operation}, {"evidence", evidence}}.dump()), {operation}}, failed ? "ERROR" : "INFO", operation, evidence,
            Json::array({Json{{"ruleID", "ASH-RECOVERY-ACTION-001"}, {"ruleFamily", "ASH-RECOVERY"}, {"sourceReference", source}}}), {source, interfaceSource}, parent);
        const auto bytes = bundle.chain.canonical().size(); for (const auto& prior : chains) if (prior.value() == bundle.chain) return bundle;
        if (chains.size() >= 64 || bytes > JsonLimits::canonicalDocumentBytes - diagnosticBytes) throw std::length_error("Mandatory filesystem diagnostic capacity exceeded");
        chains.emplace_back(bundle.chain); diagnosticBytes += bytes; return bundle;
    }
    OperationFailure failure(OperationCode code, const std::string& message, Json evidence, MutationState mutation = MutationState::NotStarted) const {
        OperationFailure result(OperationPhase::Execute, code, message, evidence); result.mutationState = mutation;
        try { const auto bundle = diagnostic(message, evidence, true); result.diagnosticChain = bundle.chain; result.diagnosticPersistence = DiagnosticPersistence::NotRequested; result.unavailableEvidence.clear(); }
        catch (const std::exception& cause) { result.addSecondary(OperationPhase::Export, OperationCode::DiagnosticUnavailable, cause.what()); } return result;
    }
    void validateCapability(AtomicOperation operation, const Path& source, const Path& target, const std::string& input) const {
        if (!persistence) throw std::invalid_argument("Completed measured persistence capability is absent"); const auto& cap = *persistence;
        if (cap.operation() != operation || cap.sourcePath() != source || cap.targetPath() != target || cap.operationInput() != input) throw std::invalid_argument("Operation differs from exact immutable persistence capability");
        (void)checked(cap.carrierPath()); (void)checked(cap.backupPath()); (void)checked(cap.auditPath());
        if (cap.plan().value().at("recoveryPlanID") != cap.executionContext().value().at("recoveryPlanID") || cap.policy().value() != cap.executionContext().value().at("policyDecision") || cap.backup().value() != cap.executionContext().value().at("backupResult") || cap.policy().value().at("decision") != Json("ALLOW") || cap.backup().value().at("status") != Json("SUCCEEDED")) throw std::invalid_argument("Plan, policy and backup receipts do not bind actual successful context");
        const auto& chain = cap.diagnosticChain().value(); std::set<std::string> seen;
        for (const auto& envelope : chain.at("diagnostics").array_items()) { const auto id = envelope.at("diagnosticID").get<std::string>(); if (id.empty() || envelope.at("chainRootReference") != chain.at("chainID") || seen.contains(id)) throw std::invalid_argument("Persistence diagnostic identities are invalid");
            if (seen.empty()) { if (!envelope.at("parentDiagnosticID").is_null() || envelope.at("diagnosticID") != chain.at("rootDiagnosticID")) throw std::invalid_argument("Persistence diagnostic root is invalid"); }
            else if (envelope.at("parentDiagnosticID").is_null() || !seen.contains(envelope.at("parentDiagnosticID").get<std::string>())) throw std::invalid_argument("Persistence diagnostic parent is unavailable"); seen.insert(id); }
        const auto resolves = [&](const Json& ref) { for (const auto& envelope : chain.at("diagnostics").array_items()) if (referenceFor(envelope) == ref) return true; return false; };
        for (const auto* dto : {&cap.plan().value(), &cap.policy().value(), &cap.backup().value(), &cap.executionContext().value()}) if (!resolves(dto->at("diagnosticReference"))) throw std::invalid_argument("Persistence lifecycle diagnostic does not resolve");
        auto carrier = open(cap.carrierPath()); auto backup = open(cap.backupPath()); auto audit = open(cap.auditPath()); auto destination = open(target);
        if (identity(carrier.value) != cap.carrierIdentity() || sha256(readHandle(carrier.value)) != cap.carrierRawHash() || identity(backup.value) != cap.backupIdentity() || sha256(readHandle(backup.value)) != cap.backupRawHash() || identity(audit.value) != cap.auditIdentity() || identity(destination.value) != cap.targetIdentity() || readHandle(destination.value) != cap.expectedPreimage()) throw std::invalid_argument("Measured carrier, backup, audit or target precondition changed");
        if (cap.targetIdentity() == cap.carrierIdentity() || cap.backupIdentity() == cap.carrierIdentity() || cap.auditIdentity() == cap.carrierIdentity() || cap.backupIdentity() == cap.auditIdentity()) throw std::invalid_argument("Persistence storage aliases protected carrier or another owned storage object");
        if (cap.backup().value().at("backupRecord").at("storageReference") != Json(cap.backupPath()) || cap.backup().value().at("backupRecord").at("sourceHash") != Json(cap.backupRawHash())) throw std::invalid_argument("Backup receipt differs from measured storage");
        if (cap.purpose() == NativePersistencePurpose::BackupSnapshot) {
            if (target != cap.backupPath() || sha256(cap.exactResult()) != cap.backupRawHash() || operation == AtomicOperation::Append) throw std::invalid_argument("Operation exceeds measured backup snapshot scope");
            if (operation == AtomicOperation::Copy) { if (!cap.copySourceRecord() || source == target) throw std::invalid_argument("Separate measured backup copy source is unavailable"); auto origin = open(source);
                const auto& receipt = cap.copySourceRecord()->value(); if (identity(origin.value) != cap.sourceIdentity() || cap.sourceIdentity() == cap.targetIdentity() || readHandle(origin.value) != input || receipt.at("storageReference") != Json(source) || receipt.at("sourceHash") != Json(sha256(input)) || !resolves(receipt.at("diagnosticReference"))) throw std::invalid_argument("Separate backup copy receipt or source measurement changed"); }
        } else {
            if (target != cap.auditPath() || operation == AtomicOperation::Copy) throw std::invalid_argument("Operation exceeds measured audit snapshot scope");
            if (operation == AtomicOperation::Write && input != cap.expectedPreimage()) throw std::invalid_argument("Audit snapshot rewrite exceeds idempotent persistence scope");
            const auto snapshot = Json{{"storageSchema", "aeostara.audit-snapshot/1"}, {"auditChain", cap.auditChain().value()}, {"diagnosticChain", chain}};
            if (operation == AtomicOperation::Append && input != snapshot.canonical() + "\n") throw std::invalid_argument("Audit append differs from exact measured genuine snapshot");
        }
        if ((operation == AtomicOperation::Append && cap.expectedPreimage() + input != cap.exactResult()) || (operation != AtomicOperation::Append && input != cap.exactResult())) throw std::invalid_argument("Exact authorized publication bytes are inconsistent");
    }
};
WindowsAtomicFileSystem::WindowsAtomicFileSystem(Path directory, std::shared_ptr<const AtomicPersistenceContext> context) : state_(std::make_unique<State>(directory, std::move(context))) {}
WindowsAtomicFileSystem::~WindowsAtomicFileSystem() = default;
OperationAttempt<ByteSequence> WindowsAtomicFileSystem::read(const Path& path) const { std::lock_guard lock(state_->mutex); try { auto file = state_->open(path); auto bytes = readHandle(file.value); (void)state_->diagnostic("Scoped file read completed", Json{{"path", path}, {"rawHash", sha256(bytes)}, {"bytes", bytes.size()}}, false); return byteSequence(bytes); } catch (const std::invalid_argument& error) { return state_->failure(OperationCode::InvalidInput, error.what(), Json{{"path", path}}); } catch (const NativeFailure& error) { return state_->failure(OperationCode::StorageFailure, error.what(), Json{{"path", path}, {"nativeCode", error.code()}}); } catch (const std::exception& error) { return state_->failure(OperationCode::StorageFailure, error.what(), Json{{"path", path}}); } }
OperationAttempt<bool> WindowsAtomicFileSystem::exists(const Path& path) const { std::lock_guard lock(state_->mutex); try { auto file = state_->open(path, true); const bool found = file.value != INVALID_HANDLE_VALUE; (void)state_->diagnostic("Scoped file existence resolved", Json{{"path", path}, {"exists", found}}, false); return found; } catch (const std::invalid_argument& error) { return state_->failure(OperationCode::InvalidInput, error.what(), Json{{"path", path}}); } catch (const std::exception& error) { return state_->failure(OperationCode::StorageFailure, error.what(), Json{{"path", path}}); } }
OperationAttempt<std::string> WindowsAtomicFileSystem::computeSHA256(const Path& path) const { std::lock_guard lock(state_->mutex); try { auto file = state_->open(path); const auto hash = sha256(readHandle(file.value)); (void)state_->diagnostic("Scoped file SHA256 measured", Json{{"path", path}, {"rawHash", hash}}, false); return hash; } catch (const std::invalid_argument& error) { return state_->failure(OperationCode::InvalidInput, error.what(), Json{{"path", path}}); } catch (const std::exception& error) { return state_->failure(OperationCode::StorageFailure, error.what(), Json{{"path", path}}); } }
OperationAttempt<FileOperationReceipt> WindowsAtomicFileSystem::writeAtomically(const Path& path, const ByteSequence& bytes) { return publish(AtomicOperation::Write, {}, path, bytes); }
OperationAttempt<FileOperationReceipt> WindowsAtomicFileSystem::appendAtomically(const Path& path, const ByteSequence& bytes) { return publish(AtomicOperation::Append, {}, path, bytes); }
OperationAttempt<FileOperationReceipt> WindowsAtomicFileSystem::copyAtomically(const Path& source, const Path& target) { return publish(AtomicOperation::Copy, source, target, {}); }
OperationAttempt<FileOperationReceipt> WindowsAtomicFileSystem::publish(AtomicOperation operation, const Path& source, const Path& target, const ByteSequence& supplied) {
    std::lock_guard lock(state_->mutex); std::lock_guard publication(publicationMutex()); bool publicationAttempted = false, stagingOwned = false; std::wstring staging;
    try {
        if (!state_->persistence) return state_->failure(OperationCode::Blocked, "Completed measured persistence capability is absent", Json{{"target", target}});
        const auto nativeTarget = state_->checked(target); std::string input;
        if (operation == AtomicOperation::Copy) { auto origin = state_->open(source); input = readHandle(origin.value); } else input = byteString(supplied);
        state_->validateCapability(operation, source, target, input); const auto& result = state_->persistence->exactResult();
        if (result.size() > JsonLimits::rawDocumentBytes) throw std::length_error("Publication exceeds adopted raw byte limit");
        // Reserve diagnostics before any physical storage mutation.
        (void)state_->diagnostic("Exact measured persistence publication prepared", Json{{"target", target}, {"beforeHash", sha256(state_->persistence->expectedPreimage())}, {"afterHash", sha256(result)}, {"bytes", result.size()}, {"compareAndSwap", false}, {"physicalCrashGuarantee", false}}, false);
        Handle stage;
        for (unsigned attempt = 0; attempt < 8; ++attempt) { staging = nativeTarget + L".stage." + randomSuffix(); stage = Handle(CreateFileW(staging.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr));
            if (stage.value != INVALID_HANDLE_VALUE) { stagingOwned = true; break; } const auto code = GetLastError(); if (code != ERROR_FILE_EXISTS && code != ERROR_ALREADY_EXISTS) throw NativeFailure("Create unique same-directory staging", code); }
        if (stage.value == INVALID_HANDLE_VALUE) throw std::runtime_error("Unique staging storage unavailable");
        DWORD written = 0; if (!result.empty() && (!WriteFile(stage.value, result.data(), static_cast<DWORD>(result.size()), &written, nullptr) || written != result.size())) nativeError("Complete staged write");
        if (!FlushFileBuffers(stage.value)) nativeError("Flush staged bytes"); if (readHandle(stage.value) != result) throw std::runtime_error("Staging readback mismatch"); stage = Handle();
        state_->validateCapability(operation, source, target, input);
        publicationAttempted = true; if (!MoveFileExW(staging.c_str(), nativeTarget.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) nativeError("Publish staged file by same-directory rename"); staging.clear(); stagingOwned = false;
        auto published = state_->open(target); const auto actual = readHandle(published.value); if (actual != result || sha256(actual) != sha256(result)) throw std::runtime_error("Published storage readback mismatch");
        const auto bundle = state_->diagnostic("Exact measured persistence publication read back", Json{{"target", target}, {"afterHash", sha256(actual)}, {"actualIdentity", identity(published.value)}, {"bytes", actual.size()}, {"compareAndSwap", false}, {"physicalCrashGuarantee", false}}, false);
        return FileOperationReceipt{"persistence-" + sha256(Json{{"target", target}, {"beforeHash", sha256(state_->persistence->expectedPreimage())}, {"afterHash", sha256(result)}, {"operation", static_cast<unsigned>(operation)}}.dump()), result.size(), BaseContracts::DiagnosticReference(bundle.reference)};
    } catch (const std::exception& error) {
        Json evidence{{"target", target}, {"publicationAttempted", publicationAttempted}, {"compareAndSwap", false}}; if (const auto* native = dynamic_cast<const NativeFailure*>(&error)) evidence["nativeCode"] = native->code();
        auto failure = state_->failure(dynamic_cast<const std::invalid_argument*>(&error) ? OperationCode::StaleState : OperationCode::StorageFailure, error.what(), evidence, publicationAttempted ? MutationState::MayHaveMutated : MutationState::NotStarted);
        if (stagingOwned && !staging.empty() && !DeleteFileW(staging.c_str())) failure.addSecondary(OperationPhase::Execute, OperationCode::StorageFailure, "Staging cleanup failed; Windows error=" + std::to_string(GetLastError()), Json{{"orphanStaging", true}});
        return failure;
    }
}
std::vector<BaseContracts::DiagnosticChain> WindowsAtomicFileSystem::diagnosticChains() const { std::lock_guard lock(state_->mutex); return state_->chains; }
} // namespace Aeostara::BaseBindings
