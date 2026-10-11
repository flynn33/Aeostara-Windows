// Aeostara
// Copyright (c) 2026 James Daley. All Rights Reserved.
#include "DiagnosticsInternal.h"
#include <stdexcept>
namespace Aeostara {
std::shared_ptr<IDiagnostics> createDefaultDiagnostics() {
    return createDiagnostics(DiagnosticProfileType::Development, DiagnosticLimits::development());
}
namespace {
class DevelopmentEvidenceRetention final : public DiagnosticDetail::IEvidenceRetention {
public:
    Json retain(const Json& evidence, DiagnosticHealth& health) const override {
        return DiagnosticDetail::redactEvidence(evidence, health);
    }
};
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
    std::shared_ptr<const DiagnosticDetail::IEvidenceRetention> retention;
    if (profile == DiagnosticProfileType::Development) retention = std::make_shared<DevelopmentEvidenceRetention>();
    else if (profile == DiagnosticProfileType::Lean) retention = std::make_shared<LeanEvidenceRetention>();
    else throw std::invalid_argument("Invalid diagnostic profile");
    return DiagnosticDetail::makeCollector(profile, limits, std::move(context), std::move(clock), std::move(retention), std::move(identity));
}
}
