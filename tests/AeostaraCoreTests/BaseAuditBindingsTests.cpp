#include "CppUnitTest.h"
#include "AeostaraCore/BaseAuditBindings.h"
#include "AeostaraCore/Diagnostics.h"
#include <iostream>
#include <map>
#include <stdexcept>
using namespace Aeostara;
using namespace Aeostara::BaseBindings;
namespace {
std::size_t assertions = 0;
void check(bool value, const char* message) { ++assertions; if (!value) throw std::runtime_error(message); }
template<class T> T success(OperationAttempt<T> value) { if (auto error = std::get_if<OperationFailure>(&value)) throw std::runtime_error(error->message); return std::get<T>(std::move(value)); }
class AuditStorage final : public Aeostara::IFileSystem {
public:
    std::map<std::string, std::string> files;
    bool failRead = false, failWrite = false, dropWrite = false;
    std::size_t writes = 0;
    std::string readFile(const std::string& path) override { if (failRead) throw std::runtime_error("Injected actual read failure"); return files.at(path); }
    void writeFile(const std::string& path, const std::string& bytes) override { ++writes; if (failWrite) throw std::runtime_error("Injected actual write failure"); if (!dropWrite) files[path] = bytes; }
    bool fileExists(const std::string& path) override { return files.contains(path); }
    bool copyFile(const std::string&, const std::string&) override { return false; }
};
}
int exerciseAuditBindings() {
    try {
        const auto rules = Json::array({Json{{"ruleID", "ASH-STATE-STRUCTURE-001"}, {"ruleFamily", "ASH-STATE"}, {"sourceReference", "https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md"}}});
        const std::vector<std::string> sources{"https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/interfaces/IAuditSink.pseudo.md"};
        const auto bundle = makeDiagnosticBundle("AUDIT", "INFO", "Actual audit test phase evidence", Json{{"state", stateJson(AshState{})}}, rules, sources);
        auto storage = std::make_shared<AuditStorage>(); BaseAuditSink sink("audit.jsonl", storage);
        const auto reference = success(sink.registerDiagnosticChain(BaseContracts::DiagnosticChain(bundle.chain)));
        Json event{{"auditEventID", "event-1"}, {"eventSequence", 1}, {"eventType", "OBSERVE"}, {"diagnosticReference", reference.value()}, {"occurredAt", "2026-10-11T00:00:00Z"}, {"details", Json{{"actualSourceHash", sha256("actual captured bytes")}}}};
        const auto chain = BaseContracts::AuditChain(Json{{"auditChainID", "audit-test-chain"}, {"diagnosticChainReference", bundle.chain.at("chainID")}, {"events", Json::array({event})}});
        check(success(sink.persistChain(chain)) == reference, "Successful persistence reference mismatch");
        check(success(sink.readChain("audit-test-chain")) == chain, "Exact audit readback mismatch");
        BaseAuditSink reopened("audit.jsonl", storage); check(success(reopened.readChain("audit-test-chain")) == chain, "Fresh sink could not reconstruct persisted diagnostic links");
        auto second = event; second["auditEventID"] = "event-2"; second["eventSequence"] = 2; second["eventType"] = "RESULT";
        const auto appended = success(sink.appendEvent(chain, BaseContracts::AuditEvent(second))); check(appended.value().at("events").size() == 2, "Actual append missing event");
        check(std::holds_alternative<BaseContracts::DiagnosticReference>(sink.persistChain(appended)), "Extended audit persistence failed"); check(success(reopened.readChain("audit-test-chain")) == appended, "Latest history did not reconstruct");
        auto skipped = second; skipped["eventSequence"] = 4; check(std::holds_alternative<OperationFailure>(sink.appendEvent(chain, BaseContracts::AuditEvent(skipped))), "Noncontiguous sequence accepted");
        auto foreign = second; foreign["diagnosticReference"]["diagnosticID"] = "foreign"; check(std::holds_alternative<OperationFailure>(sink.appendEvent(chain, BaseContracts::AuditEvent(foreign))), "Unresolvable diagnostic accepted");
        check(std::holds_alternative<OperationFailure>(sink.persistChain(chain)), "Persisted audit history truncation accepted");
        auto corrupted = storage->files.at("audit.jsonl") + "invalid\n"; storage->files["audit.jsonl"] = corrupted; const auto writesBefore = storage->writes;
        check(std::holds_alternative<OperationFailure>(sink.persistChain(appended)) && storage->writes == writesBefore && storage->files.at("audit.jsonl") == corrupted, "Corrupt prior history overwritten");
        check(std::holds_alternative<OperationFailure>(sink.readChain("audit-test-chain")), "Corrupt history silently skipped");
        storage->files.clear(); storage->dropWrite = true; auto dropped = sink.persistChain(chain); check(std::holds_alternative<OperationFailure>(dropped) && std::get<OperationFailure>(dropped).mutationMayHaveOccurred(), "Silent persistence write reported complete");
        storage->dropWrite = false; storage->failWrite = true; auto failed = sink.persistChain(chain); check(std::holds_alternative<OperationFailure>(failed) && std::get<OperationFailure>(failed).mutationMayHaveOccurred(), "Write throw hid possible audit mutation");
        storage->failWrite = false; storage->files["audit.jsonl"] = "original"; storage->failRead = true; const auto beforeReadFailure = storage->writes;
        auto unreadable = sink.persistChain(chain); check(std::holds_alternative<OperationFailure>(unreadable) && storage->writes == beforeReadFailure && !std::get<OperationFailure>(unreadable).mutationMayHaveOccurred(), "Read failure overwrote prior audit history");
        std::cout << "BaseAuditAssertions=" << assertions << " Failed=0 ReopenedChainReconstructed=true\n"; return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

namespace AeostaraCoreTests {
TEST_CLASS(BaseAuditBindingsTests) {
public:
    TEST_METHOD(PersistReopenOrderingAndStorageFailures) { Microsoft::VisualStudio::CppUnitTestFramework::Assert::AreEqual(0, exerciseAuditBindings()); }
};
}
