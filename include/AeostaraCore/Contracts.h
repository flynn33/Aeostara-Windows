// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "AeostaraCore/Json.h"

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace Aeostara {

// ---------------------------------------------------------------------------
// 1. ObservedState — snapshot of real system config at observation time
// ---------------------------------------------------------------------------
struct ObservedState final {
    std::string sourceFile;
    Json data;
    std::string timestamp;  // ISO 8601

    bool operator==(const ObservedState&) const = default;
};

void to_json(Json& j, const ObservedState& s);
void from_json(const Json& j, ObservedState& s);

// ---------------------------------------------------------------------------
// 2. DesiredState — declared desired configuration
// ---------------------------------------------------------------------------
struct DesiredState final {
    Json data;
    std::string source;

    bool operator==(const DesiredState&) const = default;
};

void to_json(Json& j, const DesiredState& s);
void from_json(const Json& j, DesiredState& s);

// ---------------------------------------------------------------------------
// 3. EncodedState — flattened canonical representations for drift comparison
// ---------------------------------------------------------------------------
struct EncodedState final {
    std::map<std::string, Json> observed;
    std::map<std::string, Json> desired;

    bool operator==(const EncodedState&) const = default;
};

void to_json(Json& j, const EncodedState& s);
void from_json(const Json& j, EncodedState& s);

// ---------------------------------------------------------------------------
// 4. DriftEvent — a single detected drift between observed and desired
// ---------------------------------------------------------------------------
enum class DriftType {
    ValueChanged,
    KeyAdded,
    KeyRemoved
};

std::string to_string(DriftType type);
DriftType driftTypeFromString(const std::string& str);

void to_json(Json& j, DriftType type);
void from_json(const Json& j, DriftType& type);

struct DriftEvent final {
    std::string keyPath;
    DriftType type;
    Json observedValue;  // null if KeyAdded
    Json desiredValue;   // null if KeyRemoved
    std::string description;

    bool operator==(const DriftEvent&) const = default;
};

void to_json(Json& j, const DriftEvent& e);
void from_json(const Json& j, DriftEvent& e);

// ---------------------------------------------------------------------------
// 5. RepairAction — a single remediation step
// ---------------------------------------------------------------------------
enum class RepairActionType {
    Set,
    Add,
    Remove
};

std::string to_string(RepairActionType type);
RepairActionType repairActionTypeFromString(const std::string& str);

void to_json(Json& j, RepairActionType type);
void from_json(const Json& j, RepairActionType& type);

struct RepairAction final {
    std::string keyPath;
    RepairActionType actionType;
    Json fromValue;
    Json toValue;
    std::string rationale;

    bool operator==(const RepairAction&) const = default;
};

void to_json(Json& j, const RepairAction& a);
void from_json(const Json& j, RepairAction& a);

// ---------------------------------------------------------------------------
// 6. RepairPlan — complete repair sequence
// ---------------------------------------------------------------------------
struct RepairPlan final {
    std::string planID;
    std::vector<RepairAction> actions;
    std::string timestamp;
    bool requiresBackup = true;

    bool operator==(const RepairPlan&) const = default;
};

void to_json(Json& j, const RepairPlan& p);
void from_json(const Json& j, RepairPlan& p);

// ---------------------------------------------------------------------------
// 7. VerificationResult — outcome of post-repair verification
// ---------------------------------------------------------------------------
struct VerificationResult final {
    bool success = false;
    std::vector<std::string> failedChecks;
    std::string verifiedAt;  // ISO 8601

    bool operator==(const VerificationResult&) const = default;
};

void to_json(Json& j, const VerificationResult& r);
void from_json(const Json& j, VerificationResult& r);

// ---------------------------------------------------------------------------
// 8. RollbackPlan — recovery procedure
// ---------------------------------------------------------------------------
struct RollbackPlan final {
    std::string planID;
    std::string backupFilePath;
    std::string originalFilePath;

    bool operator==(const RollbackPlan&) const = default;
};

void to_json(Json& j, const RollbackPlan& p);
void from_json(const Json& j, RollbackPlan& p);

// ---------------------------------------------------------------------------
// 9. AuditEvent — audit trail record
// ---------------------------------------------------------------------------
enum class AuditEventType {
    ValidationPerformed,
    DiffGenerated,
    HealStarted,
    BackupCreated,
    RepairApplied,
    VerificationSucceeded,
    VerificationFailed,
    RollbackExecuted,
    PolicyBlocked,
    NoDrift
};

std::string to_string(AuditEventType type);
AuditEventType auditEventTypeFromString(const std::string& str);

void to_json(Json& j, AuditEventType type);
void from_json(const Json& j, AuditEventType& type);

struct AuditEvent final {
    std::string eventID;
    AuditEventType type;
    std::string timestamp;
    std::string configFile;
    Json details;

    bool operator==(const AuditEvent&) const = default;
};

void to_json(Json& j, const AuditEvent& e);
void from_json(const Json& j, AuditEvent& e);

// ---------------------------------------------------------------------------
// 10. ModuleManifest — Aeostara module metadata (NOT Forsetti's)
// ---------------------------------------------------------------------------
struct ModuleManifest final {
    std::string moduleID;
    std::string displayName;
    std::string version;
    std::string description;
    std::vector<std::string> supportedConfigTypes;

    bool operator==(const ModuleManifest&) const = default;
};

void to_json(Json& j, const ModuleManifest& m);
void from_json(const Json& j, ModuleManifest& m);

} // namespace Aeostara
