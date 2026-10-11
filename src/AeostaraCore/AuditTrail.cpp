// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: LicenseRef-Raven-Forge-Dual

#include "AeostaraCore/AuditTrail.h"

#include <chrono>
#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <mutex>
#include <stdexcept>

namespace Aeostara {

namespace {

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_s(&tm, &time_t);

    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S");
    return ss.str();
}

std::string generateEventID() {
    static std::atomic<std::uint64_t> counter{0};
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(epoch).count();

    std::ostringstream ss;
    ss << "evt-" << std::hex << ms << "-" << std::hex << (counter.fetch_add(1, std::memory_order_relaxed) + 1);
    return ss.str();
}

std::mutex& auditMutex() {
    static std::mutex mutex;
    return mutex;
}

std::vector<AuditEvent> parseEvents(const std::string& content) {
    std::vector<AuditEvent> events;
    std::istringstream stream(content);
    std::string line;
    while (std::getline(stream, line)) {
        // Every persisted line must be an actual complete record. An invalid
        // record is corruption, not permission to hide history or start over.
        events.push_back(Json::parse(line).get<AuditEvent>());
    }
    return events;
}

} // anonymous namespace

JsonLinesAuditTrail::JsonLinesAuditTrail(std::string auditFilePath,
                                           std::shared_ptr<IFileSystem> fs)
    : auditFilePath_(std::move(auditFilePath))
    , fs_(std::move(fs)) {
    if (auditFilePath_.empty() || !fs_) throw std::invalid_argument("Audit storage path and filesystem capability are required");
}

void JsonLinesAuditTrail::record(const AuditEvent& event) {
    std::lock_guard lock(auditMutex());
    Json j = event;
    (void)j.get<AuditEvent>();
    std::string line = j.dump() + "\n";

    std::string existing;
    if (fs_->fileExists(auditFilePath_)) {
        existing = fs_->readFile(auditFilePath_);
        (void)parseEvents(existing);
        if (!existing.empty() && existing.back() != '\n') existing.push_back('\n');
    }
    const auto expected = existing + line;
    fs_->writeFile(auditFilePath_, expected);
    if (fs_->readFile(auditFilePath_) != expected)
        throw std::runtime_error("Audit persistence readback did not match the written records");
}

std::vector<AuditEvent> JsonLinesAuditTrail::getEvents() const {
    std::lock_guard lock(auditMutex());
    if (!fs_->fileExists(auditFilePath_)) return {};
    return parseEvents(fs_->readFile(auditFilePath_));
}

AuditEvent JsonLinesAuditTrail::createEvent(AuditEventType type,
                                              const std::string& configFile,
                                              const Json& details) {
    return AuditEvent{
        .eventID = generateEventID(),
        .type = type,
        .timestamp = currentTimestamp(),
        .configFile = configFile,
        .details = details
    };
}

} // namespace Aeostara
