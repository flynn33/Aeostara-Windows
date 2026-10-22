// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/IFileSystem.h"
#include "AeostaraCore/Json.h"
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <stdexcept>
#include <system_error>
#include <vector>
namespace Aeostara {
namespace {
std::filesystem::path nativePath(const std::string& path) {
    if (path.empty() || path.find('\0') != std::string::npos || path.size() > JsonLimits::decodedStringBytes) throw std::invalid_argument("Filesystem path is empty, contains NUL, or exceeds the native input bound");
    const auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), nullptr, 0);
    if (!length) throw std::invalid_argument("Filesystem path is not valid UTF8");
    std::wstring text(static_cast<std::size_t>(length), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path.data(), static_cast<int>(path.size()), text.data(), length)) throw std::runtime_error("Cannot convert path");
    return std::filesystem::path(text);
}
std::wstring pathKey(const std::string& path) {
    auto key = std::filesystem::absolute(nativePath(path)).lexically_normal().wstring();
    if (!key.empty()) CharLowerBuffW(key.data(), static_cast<DWORD>(key.size()));
    return key;
}
[[noreturn]] void nativeError(const char* operation) { const DWORD actual = GetLastError(); throw std::system_error(std::error_code(static_cast<int>(actual), std::system_category()), operation); }
std::string readHandle(HANDLE handle) {
    LARGE_INTEGER length{}, zero{}; if (!GetFileSizeEx(handle, &length)) nativeError("GetFileSizeEx");
    if (length.QuadPart < 0 || length.QuadPart > static_cast<LONGLONG>(JsonLimits::rawDocumentBytes)) throw std::runtime_error("File exceeds adopted raw document limit");
    if (!SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) nativeError("Seek before read");
    std::string result(static_cast<std::size_t>(length.QuadPart), '\0'); DWORD read = 0;
    if (!result.empty()) {
        if (!ReadFile(handle, result.data(), static_cast<DWORD>(result.size()), &read, nullptr)) nativeError("Complete binary read");
        if (read != result.size()) throw std::runtime_error("Successful native read returned fewer bytes than the measured file size");
    }
    return result;
}
void writeHandle(HANDLE handle, const std::string& bytes) {
    if (bytes.size() > JsonLimits::rawDocumentBytes) throw std::runtime_error("Write exceeds adopted raw document limit");
    LARGE_INTEGER zero{}; if (!SetFilePointerEx(handle, zero, nullptr, FILE_BEGIN)) nativeError("Seek before write"); DWORD written = 0;
    if (!bytes.empty()) {
        if (!WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)) nativeError("Complete binary write");
        if (written != bytes.size()) throw std::runtime_error("Successful native write transferred fewer bytes than requested");
    }
    if (!SetEndOfFile(handle)) nativeError("Truncate after write");
    if (!FlushFileBuffers(handle)) nativeError("Flush file buffers");
}
struct Handle final {
    HANDLE value = INVALID_HANDLE_VALUE;
    ~Handle() { if (value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
class Lease final : public ITargetWriteLease {
public:
    explicit Lease(std::function<void()> release) : release_(std::move(release)) {}
    ~Lease() override { release_(); }
private:
    std::function<void()> release_;
};
}
struct DefaultFileSystem::State final {
    struct Held final { HANDLE handle; DWORD ownerThread; };
    std::mutex mutex;
    std::map<std::wstring, Held> leases;
    ~State() { for (auto [key, held] : leases) { (void)key; CloseHandle(held.handle); } }
};
DefaultFileSystem::DefaultFileSystem() : state_(std::make_shared<State>()) {}
DefaultFileSystem::~DefaultFileSystem() = default;
std::string DefaultFileSystem::readFile(const std::string& path) {
    std::lock_guard guard(state_->mutex); const auto key = pathKey(path); const auto found = state_->leases.find(key);
    if (found != state_->leases.end()) return readHandle(found->second.handle);
    Handle handle{CreateFileW(nativePath(path).c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle.value == INVALID_HANDLE_VALUE) nativeError("Open binary read"); return readHandle(handle.value);
}
void DefaultFileSystem::writeFile(const std::string& path, const std::string& content) {
    std::lock_guard guard(state_->mutex); const auto key = pathKey(path); const auto found = state_->leases.find(key);
    if (found != state_->leases.end()) { if (found->second.ownerThread != GetCurrentThreadId()) throw std::runtime_error("Write lease belongs to another execution thread"); writeHandle(found->second.handle, content); return; }
    Handle handle{CreateFileW(nativePath(path).c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle.value == INVALID_HANDLE_VALUE) nativeError("Open binary write"); writeHandle(handle.value, content);
}
bool DefaultFileSystem::fileExists(const std::string& path) { return std::filesystem::exists(nativePath(path)); }
bool DefaultFileSystem::copyFile(const std::string& from, const std::string& to) {
    try { const auto bytes = readFile(from); writeFile(to, bytes); return readFile(to) == bytes; } catch (...) { return false; }
}
bool DefaultFileSystem::writeFileExclusive(const std::string& path, const std::string& content) {
    Handle handle{CreateFileW(nativePath(path).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle.value == INVALID_HANDLE_VALUE) { if (GetLastError() == ERROR_FILE_EXISTS || GetLastError() == ERROR_ALREADY_EXISTS) return false; nativeError("Create unique storage"); }
    writeHandle(handle.value, content); return readHandle(handle.value) == content;
}
std::string DefaultFileSystem::pathIdentity(const std::string& path) {
    // File ID detects hard links and junction/symlink aliases, not just spelling.
    std::lock_guard guard(state_->mutex); const auto key = pathKey(path); HANDLE handle = INVALID_HANDLE_VALUE; Handle opened;
    const auto leased = state_->leases.find(key); if (leased != state_->leases.end()) handle = leased->second.handle;
    else { opened.value = CreateFileW(nativePath(path).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr); handle = opened.value; }
    if (handle != INVALID_HANDLE_VALUE) {
        BY_HANDLE_FILE_INFORMATION info{}; if (!GetFileInformationByHandle(handle, &info)) nativeError("Resolve file identity");
        return "file:" + std::to_string(info.dwVolumeSerialNumber) + ":" + std::to_string(info.nFileIndexHigh) + ":" + std::to_string(info.nFileIndexLow);
    }
    if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) nativeError("Resolve target identity");
    // For future destinations, resolve existing parent aliases and case first.
    const auto absolute = std::filesystem::absolute(nativePath(path)); const auto resolved = std::filesystem::weakly_canonical(absolute).wstring();
    auto normalized = resolved; if (!normalized.empty()) CharLowerBuffW(normalized.data(), static_cast<DWORD>(normalized.size()));
    const int bytes = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, normalized.data(), static_cast<int>(normalized.size()), nullptr, 0, nullptr, nullptr);
    if (!bytes) throw std::runtime_error("Cannot encode target identity"); std::string result(static_cast<std::size_t>(bytes), '\0');
    WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, normalized.data(), static_cast<int>(normalized.size()), result.data(), bytes, nullptr, nullptr); return "path:" + result;
}
std::unique_ptr<ITargetWriteLease> DefaultFileSystem::acquireWriteLease(const std::string& path) {
    std::lock_guard guard(state_->mutex); const auto key = pathKey(path); if (state_->leases.contains(key)) return {};
    Handle handle{CreateFileW(nativePath(path).c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr)};
    if (handle.value == INVALID_HANDLE_VALUE) nativeError("Acquire exclusive target lease");
    state_->leases.emplace(key, State::Held{handle.value, GetCurrentThreadId()}); auto state = state_;
    try {
        auto lease = std::make_unique<Lease>([state, key] { std::lock_guard lock(state->mutex); const auto found = state->leases.find(key); if (found != state->leases.end()) { CloseHandle(found->second.handle); state->leases.erase(found); } });
        handle.value = INVALID_HANDLE_VALUE; return lease;
    } catch (...) { state_->leases.erase(key); throw; }
}
std::string DefaultFileSystem::versionToken(const std::string& path) {
    std::lock_guard guard(state_->mutex); const auto key = pathKey(path); HANDLE handle = INVALID_HANDLE_VALUE; Handle opened;
    const auto leased = state_->leases.find(key); if (leased != state_->leases.end()) handle = leased->second.handle;
    else { opened.value = CreateFileW(nativePath(path).c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr); handle = opened.value; }
    if (handle == INVALID_HANDLE_VALUE) nativeError("Resolve file version"); BY_HANDLE_FILE_INFORMATION info{}; if (!GetFileInformationByHandle(handle, &info)) nativeError("Resolve version information");
    return "version:" + std::to_string(info.dwVolumeSerialNumber) + ":" + std::to_string(info.nFileIndexHigh) + ":" + std::to_string(info.nFileIndexLow) + ":" + std::to_string(info.ftLastWriteTime.dwHighDateTime) + ":" + std::to_string(info.ftLastWriteTime.dwLowDateTime) + ":" + std::to_string(info.nFileSizeHigh) + ":" + std::to_string(info.nFileSizeLow);
}
} // namespace Aeostara
