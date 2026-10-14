// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#pragma once
#include "AeostaraCore/Diagnostics.h"
#include <memory>
namespace Aeostara::DiagnosticDetail {
class IEvidenceRetention {
public:
    virtual ~IEvidenceRetention() = default;
    virtual Json retain(const Json& evidence, DiagnosticHealth& health) const = 0;
};
Json redactEvidence(const Json& evidence, DiagnosticHealth& health);
std::shared_ptr<IDiagnostics> makeCollector(DiagnosticProfileType profile,
    DiagnosticLimits limits, Json context, std::shared_ptr<IDiagnosticClock> clock,
    std::shared_ptr<const IEvidenceRetention> retention, std::shared_ptr<IDiagnosticIdentity> identity);
}
