// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#pragma once
#include "AeostaraCore/Json.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Aeostara {
enum class DiagnosticProfileType { Lean, Development };
enum class DiagnosticPhaseType { Observation, Normalization, Projection, Diagnosis,
    Recoverability, Policy, Planning, Backup, Execution, Verification, Recovery,
    Containment, Completion, Export };
enum class DiagnosticOutcomeType { Started, Passed, Blocked, Failed, Restored, RecoveryFailed };
enum class DiagnosticErrorType { None, Input, Mapping, Policy, Storage, Stale, Execution,
    Verification, Recovery, Audit, Internal };
struct DiagnosticError final {
    DiagnosticErrorType kind = DiagnosticErrorType::None;
    std::optional<std::int64_t> nativeCode;
    std::vector<DiagnosticError> causes;
};
struct DiagnosticEvent final {
    std::string operationID;
    DiagnosticPhaseType phase = DiagnosticPhaseType::Observation;
    DiagnosticOutcomeType outcome = DiagnosticOutcomeType::Started;
    std::string objectHash;
    std::vector<std::string> ruleReferences;
    std::optional<DiagnosticError> primaryError;
    std::vector<DiagnosticError> secondaryErrors;
    Json evidence = Json::object();
};
struct DiagnosticLimits final {
    std::size_t eventCount = 64;
    std::size_t eventBytes = 4096;
    std::size_t memoryBytes = 262144;
    std::uint64_t retentionMilliseconds = 3600000;
    std::size_t exportBytes = 2097152;
    static DiagnosticLimits development() noexcept;
};
struct DiagnosticHealth final {
    std::uint64_t dropped = 0;
    std::uint64_t truncated = 0;
    std::uint64_t redacted = 0;
    std::uint64_t captureFailures = 0;
    std::optional<std::uint64_t> exportFailures;
};
class IDiagnosticClock {
public:
    virtual ~IDiagnosticClock() = default;
    virtual std::string utcNow() const = 0;
    virtual std::uint64_t monotonicMilliseconds() const = 0;
};
class IDiagnosticIdentity {
public:
    virtual ~IDiagnosticIdentity() = default;
    virtual std::string sessionIdentity() const = 0;
};
struct DiagnosticSnapshot final {
    Json document;
    std::size_t exportByteLimit = 2097152;
};
class IDiagnostics {
public:
    virtual ~IDiagnostics() = default;
    virtual void capture(const DiagnosticEvent& event) noexcept = 0;
    virtual DiagnosticSnapshot snapshot() const = 0;
    virtual DiagnosticProfileType profile() const noexcept = 0;
    virtual DiagnosticHealth health() const noexcept = 0;
};
struct DiagnosticExport final {
    std::string json;
    std::string markdown;
};
struct DiagnosticExportResult final {
    bool complete = false;
    std::string stagingPath;
    std::string jsonPath;
    std::string markdownPath;
    std::string error;
    std::optional<DiagnosticError> primaryError;
};
struct DiagnosticBundle final {
    Json envelope;
    Json reference;
    Json chain;
};
struct AshDiagnosticContext final {
    std::string kind;
    std::string stage;
    std::string disposition;
    std::string subjectReference;
    std::vector<std::string> notes;
};
class IDiagnosticExportWriter {
public:
    virtual ~IDiagnosticExportWriter() = default;
    virtual void write(const std::string& stagingDirectory, const std::string& filename,
                       const std::string& bytes) = 0;
};
std::shared_ptr<IDiagnostics> createDiagnostics(DiagnosticProfileType profile,
    DiagnosticLimits limits, Json buildContext = Json::object(),
    std::shared_ptr<IDiagnosticClock> clock = {}, std::shared_ptr<IDiagnosticIdentity> identity = {});
std::shared_ptr<IDiagnostics> createDefaultDiagnostics();
DiagnosticExport makeDiagnosticExport(const DiagnosticSnapshot& snapshot);
DiagnosticExportResult exportDiagnosticBundle(const DiagnosticSnapshot& snapshot,
    const std::string& destinationDirectory, IDiagnosticExportWriter* writer = nullptr) noexcept;
DiagnosticBundle makeDiagnosticBundle(const std::string& kind, const std::string& severity,
    const std::string& message, const Json& evidence, const Json& actualRuleReferences,
    const std::vector<std::string>& actualSourceReferences,
    const std::string& chainRoot = {}, const std::optional<std::string>& parent = {});
DiagnosticBundle appendDiagnosticBundle(const Json& resolvedChain, const std::string& kind,
    const std::string& severity, const std::string& message, const Json& evidence,
    const Json& actualRuleReferences, const std::vector<std::string>& actualSourceReferences);
DiagnosticBundle makeAshDiagnosticBundle(const AshDiagnosticContext& context,
    const std::string& severity, const std::string& summary, const Json& evidence,
    const Json& actualRuleReferences, const std::vector<std::string>& actualSourceReferences,
    const Json* resolvedChain = nullptr);
void validateDiagnosticChain(const Json& resolvedChain, bool requireCanonicalAsh = false);
} // namespace Aeostara
