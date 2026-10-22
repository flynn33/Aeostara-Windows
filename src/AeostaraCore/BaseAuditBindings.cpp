// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0
#include "AeostaraCore/BaseAuditBindings.h"
#include <set>
#include <sstream>
#include <stdexcept>
namespace Aeostara::BaseBindings {
namespace {
std::mutex& persistenceMutex() { static std::mutex value; return value; }
Json referenceFor(const Json& envelope) { return Json{{"diagnosticID", envelope.at("diagnosticID")}, {"chainRootReference", envelope.at("chainRootReference")}, {"diagnosticKind", envelope.at("diagnosticKind")}, {"ruleReferences", envelope.at("ruleReferences")}}; }
void requireAppendOnly(const Json& before, const Json& after) {
    if (before.at("auditChainID") != after.at("auditChainID") || before.at("diagnosticChainReference") != after.at("diagnosticChainReference") || before.at("events").size() > after.at("events").size()) throw std::invalid_argument("Audit snapshot rewrites or removes persisted history");
    for (std::size_t i = 0; i < before.at("events").size(); ++i) if (before.at("events").at(i) != after.at("events").at(i)) throw std::invalid_argument("Audit snapshot modifies prior persisted event");
}
}
BaseAuditSink::BaseAuditSink(std::string path, std::shared_ptr<Aeostara::IFileSystem> filesystem)
    : auditPath_(std::move(path)), filesystem_(std::move(filesystem)) {
    if (auditPath_.empty() || !filesystem_) throw std::invalid_argument("Actual audit storage capability/path required");
}
OperationFailure BaseAuditSink::failure(OperationCode code, const std::string& message, const Json& evidence) const {
    OperationFailure result(OperationPhase::Audit, code, message, evidence);
    if (evidence.is_object() && evidence.contains("diagnosticChainReference")) {
        const auto found = diagnosticChains_.find(evidence.at("diagnosticChainReference").get<std::string>());
        if (found != diagnosticChains_.end()) { result.diagnosticChain = found->second.value(); result.diagnosticPersistence = DiagnosticPersistence::NotRequested; result.unavailableEvidence.clear(); }
    }
    return result;
}
OperationAttempt<BaseContracts::DiagnosticReference> BaseAuditSink::registerDiagnosticChain(const BaseContracts::DiagnosticChain& chain) {
    std::lock_guard lock(mutex_);
    try { return retainDiagnosticChain(chain); }
    catch (const std::exception& error) { return failure(OperationCode::InvalidInput, error.what()); }
}
BaseContracts::DiagnosticReference BaseAuditSink::retainDiagnosticChain(const BaseContracts::DiagnosticChain& chain) const {
        const auto& value = chain.value(); const auto id = value.at("chainID").get<std::string>();
        if (id.empty() || value.at("rootDiagnosticID").get<std::string>().empty()) throw std::invalid_argument("Diagnostic chain identity absent");
        std::set<std::string> seen; const Json* last = nullptr;
        for (const auto& envelope : value.at("diagnostics").array_items()) {
            const auto diagnosticID = envelope.at("diagnosticID").get<std::string>();
            if (diagnosticID.empty() || envelope.at("chainRootReference") != Json(id) || seen.contains(diagnosticID)) throw std::invalid_argument("Diagnostic identity duplicate or root mismatch");
            if (!last) { if (!envelope.at("parentDiagnosticID").is_null() || envelope.at("diagnosticID") != value.at("rootDiagnosticID")) throw std::invalid_argument("Diagnostic root is not genuine chain root"); }
            else if (envelope.at("parentDiagnosticID").is_null() || !seen.contains(envelope.at("parentDiagnosticID").get<std::string>())) throw std::invalid_argument("Diagnostic parent does not resolve to earlier actual envelope");
            for (const auto& rule : envelope.at("ruleReferences").array_items()) if (rule.at("sourceReference").get<std::string>().empty() || rule.at("ruleFamily").get<std::string>().empty()) throw std::invalid_argument("Diagnostic actual source/family absent");
            for (const auto& source : envelope.at("sourceReferences").array_items()) if (source.get<std::string>().empty()) throw std::invalid_argument("Diagnostic actual source absent");
            seen.insert(diagnosticID); last = &envelope;
        }
        if (!last) throw std::invalid_argument("Diagnostic chain has no genuine envelope");
        const auto bytes = value.canonical().size();
        if (const auto found = diagnosticChains_.find(id); found != diagnosticChains_.end()) {
            const auto& old = found->second.value(); if (old.at("rootDiagnosticID") != value.at("rootDiagnosticID") || old.at("diagnostics").size() > value.at("diagnostics").size()) throw std::invalid_argument("Registered diagnostic history replaced or truncated");
            for (std::size_t i = 0; i < old.at("diagnostics").size(); ++i) if (old.at("diagnostics").at(i) != value.at("diagnostics").at(i)) throw std::invalid_argument("Registered diagnostic envelope rewritten");
            const auto oldBytes = old.canonical().size();
            if (bytes > JsonLimits::canonicalDocumentBytes - (diagnosticBytes_ - oldBytes)) throw std::length_error("Mandatory audit diagnostic byte capacity exceeded");
            found->second = chain; diagnosticBytes_ = diagnosticBytes_ - oldBytes + bytes;
        } else {
            if (diagnosticChains_.size() >= 64 || bytes > JsonLimits::canonicalDocumentBytes - diagnosticBytes_) throw std::length_error("Mandatory audit diagnostic registry capacity exceeded");
            diagnosticChains_.emplace(id, chain); diagnosticBytes_ += bytes;
        }
        return BaseContracts::DiagnosticReference(referenceFor(*last));
}
BaseContracts::AuditChain BaseAuditSink::decodeSnapshot(const Json& snapshot) const {
    if (!snapshot.is_object() || snapshot.size() != 3 || snapshot.at("storageSchema") != Json("aeostara.audit-snapshot/1")) throw std::invalid_argument("Unsupported audit storage snapshot");
    const BaseContracts::DiagnosticChain supplied(snapshot.at("diagnosticChain")); const auto& value = supplied.value();
    const auto found = diagnosticChains_.find(value.at("chainID").get<std::string>());
    if (found != diagnosticChains_.end() && found->second.value().at("diagnostics").size() >= value.at("diagnostics").size()) {
        const auto& owned = found->second.value(); if (owned.at("rootDiagnosticID") != value.at("rootDiagnosticID")) throw std::invalid_argument("Persisted diagnostic root rewritten");
        for (std::size_t i = 0; i < value.at("diagnostics").size(); ++i) if (owned.at("diagnostics").at(i) != value.at("diagnostics").at(i)) throw std::invalid_argument("Persisted diagnostic envelope rewritten");
    } else (void)retainDiagnosticChain(supplied);
    BaseContracts::AuditChain audit(snapshot.at("auditChain")); validateChain(audit); return audit;
}
void BaseAuditSink::validateReference(const Json& reference) const {
    const auto found = diagnosticChains_.find(reference.at("chainRootReference").get<std::string>());
    if (found != diagnosticChains_.end()) for (const auto& envelope : found->second.value().at("diagnostics").array_items()) if (referenceFor(envelope) == reference) return;
    throw std::invalid_argument("Audit diagnostic reference does not resolve to registered actual envelope");
}
void BaseAuditSink::validateChain(const BaseContracts::AuditChain& chain) const {
    const auto& value = chain.value(); if (value.at("auditChainID").get<std::string>().empty() || !diagnosticChains_.contains(value.at("diagnosticChainReference").get<std::string>())) throw std::invalid_argument("Audit chain identity or active diagnostic root unavailable");
    std::set<std::string> ids;
    for (std::size_t i = 0; i < value.at("events").size(); ++i) {
        const auto& event = value.at("events").at(i); const auto id = event.at("auditEventID").get<std::string>();
        if (id.empty() || !ids.insert(id).second || event.at("eventSequence") != Json(i + 1) || event.at("occurredAt").get<std::string>().empty() || event.at("diagnosticReference").at("chainRootReference") != value.at("diagnosticChainReference")) throw std::invalid_argument("Audit event identity/time/sequence/root is invalid");
        validateReference(event.at("diagnosticReference"));
    }
}
OperationAttempt<BaseContracts::AuditChain> BaseAuditSink::appendEvent(const BaseContracts::AuditChain& chain, const BaseContracts::AuditEvent& event) {
    std::lock_guard lock(mutex_);
    try { validateChain(chain); auto value = chain.value(); value["events"].push_back(event.value()); BaseContracts::AuditChain appended(std::move(value)); validateChain(appended); return appended;
    } catch (const std::exception& error) { return failure(OperationCode::InvalidInput, error.what(), chain.value()); }
}
OperationAttempt<BaseContracts::DiagnosticReference> BaseAuditSink::persistChain(const BaseContracts::AuditChain& chain) {
    std::lock_guard lock(mutex_); std::lock_guard persistence(persistenceMutex());
    bool writeAttempted = false;
    try {
        validateChain(chain); std::string existing;
        if (filesystem_->fileExists(auditPath_)) existing = filesystem_->readFile(auditPath_);
        if (existing.size() > JsonLimits::rawDocumentBytes) throw std::length_error("Audit storage byte limit exceeded");
        std::istringstream input(existing); std::string line; std::map<std::string, Json, UnsignedUtf8Less> latest;
        while (std::getline(input, line)) {
            const auto stored = decodeSnapshot(Json::parse(line)); const auto id = stored.value().at("auditChainID").get<std::string>();
            if (latest.contains(id)) requireAppendOnly(latest.at(id), stored.value()); latest[id] = stored.value();
        }
        const auto id = chain.value().at("auditChainID").get<std::string>(); if (latest.contains(id)) requireAppendOnly(latest.at(id), chain.value());
        if (!existing.empty() && existing.back() != '\n') existing.push_back('\n');
        const auto& linked = diagnosticChains_.at(chain.value().at("diagnosticChainReference").get<std::string>());
        const auto snapshot = Json{{"storageSchema", "aeostara.audit-snapshot/1"}, {"auditChain", chain.value()}, {"diagnosticChain", linked.value()}};
        const auto expected = existing + snapshot.canonical() + "\n";
        if (expected.size() > JsonLimits::rawDocumentBytes) throw std::length_error("Audit storage byte limit exceeded");
        writeAttempted = true; filesystem_->writeFile(auditPath_, expected);
        if (filesystem_->readFile(auditPath_) != expected) throw std::runtime_error("Mandatory audit write/readback mismatch");
        return BaseContracts::DiagnosticReference(chain.value().at("events").at(chain.value().at("events").size() - 1).at("diagnosticReference"));
    } catch (const std::exception& error) {
        auto evidence = chain.value(); evidence["auditStorageWriteAttempted"] = writeAttempted;
        auto result = failure(OperationCode::StorageFailure, error.what(), evidence);
        result.mutationState = writeAttempted ? MutationState::MayHaveMutated : MutationState::NotStarted;
        return result;
    }
}
OperationAttempt<BaseContracts::AuditChain> BaseAuditSink::readChain(const std::string& id) const {
    std::lock_guard lock(mutex_); std::lock_guard persistence(persistenceMutex());
    try {
        if (id.empty()) return failure(OperationCode::InvalidInput, "Audit chain identity absent");
        if (!filesystem_->fileExists(auditPath_)) return failure(OperationCode::Blocked, "Audit storage does not exist");
        const auto bytes = filesystem_->readFile(auditPath_); if (bytes.size() > JsonLimits::rawDocumentBytes) throw std::length_error("Audit storage byte limit exceeded");
        std::istringstream input(bytes); std::string line; std::map<std::string, Json, UnsignedUtf8Less> latest;
        while (std::getline(input, line)) { const auto stored = decodeSnapshot(Json::parse(line)); const auto key = stored.value().at("auditChainID").get<std::string>(); if (latest.contains(key)) requireAppendOnly(latest.at(key), stored.value()); latest[key] = stored.value(); }
        if (!latest.contains(id)) return failure(OperationCode::Blocked, "Requested audit chain is absent from validated storage");
        return BaseContracts::AuditChain(latest.at(id));
    } catch (const std::exception& error) { return failure(OperationCode::StorageFailure, error.what()); }
}
} // namespace Aeostara::BaseBindings
