// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "DiagnosticsInternal.h"
#include <stdexcept>
namespace Aeostara {
std::shared_ptr<IDiagnostics> createDefaultDiagnostics() {
    return createDiagnostics(DiagnosticProfileType::Lean, DiagnosticLimits{});
}
namespace {
class LeanEvidenceRetention final : public DiagnosticDetail::IEvidenceRetention {
public:
    Json retain(const Json& evidence, DiagnosticHealth& health) const override {
        if (!evidence.empty()) ++health.redacted;
        return Json::object();
    }
};
}
std::shared_ptr<IDiagnostics> createDiagnostics(DiagnosticProfileType profile,
    DiagnosticLimits limits, Json context, std::shared_ptr<IDiagnosticClock> clock, std::shared_ptr<IDiagnosticIdentity> identity) {
    if (profile != DiagnosticProfileType::Lean)
        throw std::invalid_argument("Development diagnostics is absent from this SDK binary");
    return DiagnosticDetail::makeCollector(profile, limits, std::move(context), std::move(clock),
        std::make_shared<LeanEvidenceRetention>(), std::move(identity));
}
}
