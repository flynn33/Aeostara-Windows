// Aeostara
// Copyright (c) 2026 James Daley, Raven Forge Software.
// SPDX-License-Identifier: MIT
#include "AeostaraCore/BaseContracts.h"
#include <array>

namespace Aeostara::BaseContracts {
namespace {
struct Field final { std::string_view name; std::size_t node; bool required; };
struct Descriptor final {
    unsigned typeMask;
    int reference;
    std::vector<Field> fields;
    int item;
    std::vector<std::string_view> enumStrings;
    std::vector<int> enumIntegers;
    std::size_t minimumItems;
    int maximumItems;
    bool additionalProperties;
    int pattern;
    int constantBoolean;
};
// Native structural constraints derived from the 59 frozen contracts at
// flynn33/aeostara 774b981ef2267a41c1b1a59497b2b0746a86f32b.
// No schema files are loaded or distributed as runtime dependencies.
const std::vector<Descriptor>& descriptors() {
    static const std::vector<Descriptor> values{
        {64, -1, {{"resultID", 1, true}, {"stateReference", 2, true}, {"status", 3, true}, {"orbitReference", 4, true}, {"diagnosticReference", 5, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"VALID", "TRANSFORMATION_COMPATIBLE", "TRANSFORMATION_INCOMPATIBLE", "UNCLASSIFIED"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"emissionPlanID", 7, true}, {"generationPlanID", 8, true}, {"artifacts", 9, true}, {"traceability", 11, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 10, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 12, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"ashSemanticStateID", 14, true}, {"ashState", 15, true}, {"admissibilityResult", 16, true}, {"realmIdentity", 17, true}, {"diagnosticReference", 18, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 0, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 242, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"stateID", 20, true}, {"coordinates", 21, true}, {"sourceReference", 23, true}, {"canonicalHash", 24, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 22, {}, {}, 9, 9, true, 0, -1},
        {8, -1, {}, -1, {}, {0, 1}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"auditChainID", 26, true}, {"diagnosticChainReference", 27, true}, {"events", 28, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 29, {}, {}, 1, -1, true, 0, -1},
        {127, 30, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"auditEventID", 31, true}, {"eventSequence", 32, true}, {"eventType", 33, true}, {"diagnosticReference", 34, true}, {"occurredAt", 35, true}, {"details", 36, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {8, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"OBSERVE", "NORMALIZE", "PROJECT", "DIAGNOSE", "CLASSIFY", "PLAN", "POLICY_GATE", "BACKUP", "EXECUTE", "VERIFY", "ROLLBACK", "FALLBACK", "CONTAINMENT", "SAFE_HALT", "RESULT"}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"axiomID", 38, true}, {"diagnosticEnvelope", 39, true}, {"evaluationResult", 40, true}, {"evidence", 41, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 90, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"PASS", "FAIL", "BLOCKED"}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"backupRecordID", 43, true}, {"sourceDocument", 44, true}, {"sourceHash", 45, true}, {"storageReference", 46, true}, {"diagnosticReference", 47, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 175, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"backupResultID", 49, true}, {"status", 50, true}, {"backupRecord", 51, true}, {"escalationDecision", 52, false}, {"diagnosticReference", 53, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SUCCEEDED", "FAILED", "SKIPPED"}, {}, 0, -1, true, 0, -1},
        {127, 42, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 125, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"codewordSetID", 55, true}, {"codewords", 56, true}, {"sourceReference", 59, true}, {"canonicalHash", 60, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 57, {}, {}, 16, 16, true, 0, -1},
        {32, -1, {}, 58, {}, {}, 9, 9, true, 0, -1},
        {8, -1, {}, -1, {}, {0, 1}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"transformationID", 62, true}, {"inputState", 63, true}, {"codeword", 64, true}, {"outputState", 66, true}, {"diagnosticReference", 67, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 65, {}, {}, 9, 9, true, 0, -1},
        {8, -1, {}, -1, {}, {0, 1}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"containmentDecisionID", 69, true}, {"boundary", 70, true}, {"allowedOperations", 71, true}, {"restrictedOperations", 73, true}, {"diagnosticReference", 75, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 72, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 74, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"desiredStateID", 77, true}, {"desiredSystemIntent", 78, true}, {"evidenceOnly", 79, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 80, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"desiredSystemIntentID", 81, true}, {"documentReference", 82, true}, {"partialIntent", 83, true}, {"diagnosticReference", 84, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 175, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"chainID", 86, true}, {"rootDiagnosticID", 87, true}, {"diagnostics", 88, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 89, {}, {}, 1, -1, true, 0, -1},
        {127, 90, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"diagnosticID", 91, true}, {"chainRootReference", 92, true}, {"parentDiagnosticID", 93, true}, {"diagnosticKind", 94, true}, {"severity", 95, true}, {"message", 96, true}, {"ruleReferences", 97, true}, {"sourceReferences", 99, true}, {"evidence", 101, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {17, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"STATE_VALIDITY", "RECOVERY", "POLICY", "BACKUP", "EXECUTION", "VERIFICATION", "ROLLBACK", "FALLBACK", "CONTAINMENT", "SAFE_HALT", "AUDIT", "MAPPING", "JSON_SEMANTICS"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"INFO", "WARNING", "ERROR", "CRITICAL"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 98, {}, {}, 1, -1, true, 0, -1},
        {127, 294, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 100, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"diagnosticID", 103, true}, {"chainRootReference", 104, true}, {"diagnosticKind", 105, true}, {"ruleReferences", 106, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"STATE_VALIDITY", "RECOVERY", "POLICY", "BACKUP", "EXECUTION", "VERIFICATION", "ROLLBACK", "FALLBACK", "CONTAINMENT", "SAFE_HALT", "AUDIT", "MAPPING", "JSON_SEMANTICS"}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 107, {}, {}, 1, -1, true, 0, -1},
        {127, 294, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"driftEventID", 109, true}, {"surfaceDifferences", 110, true}, {"evidenceOnly", 112, true}, {"diagnosticReference", 113, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 111, {}, {}, 1, -1, true, 0, -1},
        {127, 325, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"dryRunResultID", 115, true}, {"wouldMutate", 116, true}, {"recoveryPlan", 117, true}, {"verificationPlan", 118, true}, {"diagnosticChain", 119, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 254, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 356, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 85, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"encodedStateID", 121, true}, {"encoding", 122, true}, {"ashState", 123, true}, {"evidenceOnly", 124, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"decisionID", 126, true}, {"fromCategory", 127, true}, {"toCategory", 128, true}, {"reason", 129, true}, {"diagnosticReference", 130, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 247, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 247, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"executionContextID", 132, true}, {"recoveryPlanID", 133, true}, {"policyDecision", 134, true}, {"backupResult", 135, true}, {"verificationPlan", 136, true}, {"diagnosticReference", 137, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 235, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 48, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 356, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"executionStepResultID", 139, true}, {"stepID", 140, true}, {"status", 141, true}, {"observedHashAfterStep", 142, true}, {"diagnosticReference", 143, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SUCCEEDED", "FAILED", "SKIPPED", "BLOCKED"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"fallbackDecisionID", 145, true}, {"registryReference", 146, true}, {"selectedCandidateID", 147, true}, {"status", 148, true}, {"diagnosticReference", 149, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SELECTED", "UNAVAILABLE", "BLOCKED"}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"planID", 151, true}, {"planElements", 152, true}, {"semanticInputs", 154, true}, {"diagnosticReferences", 155, true}, {"planHash", 157, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 153, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 156, {}, {}, 1, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"healResultID", 159, true}, {"status", 160, true}, {"recoveryPlan", 161, true}, {"auditChain", 162, true}, {"diagnosticChain", 163, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"NO_ACTION", "HEALED", "BLOCKED", "ESCALATED", "SAFE_HALT"}, {}, 0, -1, true, 0, -1},
        {127, 254, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 25, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 85, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"invariantID", 165, true}, {"conformanceCategory", 166, true}, {"sourceReference", 167, true}, {"diagnosticRule", 168, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 294, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"ruleID", 170, true}, {"objectKeyOrdering", 171, true}, {"arrayOrdering", 172, true}, {"numberEncoding", 173, true}, {"hashAlgorithm", 174, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"LEXICOGRAPHIC_UTF8"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"PRESERVE_INPUT_ORDER"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"JSON_NUMBER_CANONICAL"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SHA-256"}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"documentID", 176, true}, {"role", 177, true}, {"canonicalHash", 178, true}, {"sourceReference", 179, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"OBSERVED", "DESIRED", "CANONICALIZED", "BACKUP"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"operationID", 181, true}, {"op", 182, true}, {"path", 183, true}, {"value", 184, false}, {"preconditionHash", 185, true}, {"diagnosticReference", 186, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"test", "add", "replace", "remove"}, {}, 0, -1, true, 0, -1},
        {127, 187, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"pointer", 188, true}, {"tokens", 189, true}, {"pointsToMissing", 191, true}, {"pointsToNull", 192, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 1, -1},
        {32, -1, {}, 190, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"ambiguityID", 194, true}, {"dimensionID", 195, true}, {"candidateValues", 196, true}, {"evidence", 198, true}, {"blockingRule", 200, true}, {"diagnosticReference", 201, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8"}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 197, {}, {}, 2, -1, true, 0, -1},
        {8, -1, {}, -1, {}, {0, 1}, 0, -1, true, 0, -1},
        {32, -1, {}, 199, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 294, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"mappingResultID", 203, true}, {"status", 204, true}, {"projectedState", 205, true}, {"dimensionBindings", 206, true}, {"ambiguities", 208, true}, {"diagnosticReferences", 210, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"MAPPED", "AMBIGUOUS", "BLOCKED", "FAILED"}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 207, {}, {}, 9, 9, true, 0, -1},
        {127, 304, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 209, {}, {}, 0, -1, true, 0, -1},
        {127, 193, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 211, {}, {}, 1, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"moduleManifestID", 213, true}, {"aeostaraVersion", 214, true}, {"modules", 215, true}, {"diagnosticReference", 217, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 216, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"observedStateID", 219, true}, {"observedSystemState", 220, true}, {"evidenceOnly", 221, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 222, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"observedSystemStateID", 223, true}, {"documentReference", 224, true}, {"surfaceDifferences", 225, true}, {"diagnosticReference", 227, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 175, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 226, {}, {}, 0, -1, true, 0, -1},
        {127, 325, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"policyBundleID", 229, true}, {"sourceReferences", 230, true}, {"rules", 232, true}, {"bundleHash", 234, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 231, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 233, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"policyDecisionID", 236, true}, {"decision", 237, true}, {"policyBundleID", 238, true}, {"reasons", 239, true}, {"diagnosticReference", 241, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"ALLOW", "BLOCK", "ESCALATE"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 240, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"realmID", 243, true}, {"stateReference", 244, true}, {"encoding", 245, true}, {"diagnosticReference", 246, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"NO_ACTION", "NORMALIZE_STATE", "APPLY_CORRECTION", "FALLBACK_REQUIRED", "CONTAINMENT_REQUIRED", "ESCALATION_REQUIRED", "TERMINAL_NO_RECOVERY"}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"outcomeID", 249, true}, {"status", 250, true}, {"recoveryCategory", 251, true}, {"escalationDecision", 252, false}, {"diagnosticReference", 253, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SUCCEEDED", "BLOCKED", "FAILED", "ESCALATED", "NO_ACTION"}, {}, 0, -1, true, 0, -1},
        {127, 247, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 125, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"recoveryPlanID", 255, true}, {"planDisposition", 256, true}, {"diagnosticReference", 257, true}, {"stateClass", 258, true}, {"recoveryCategory", 259, true}, {"requiresPolicyGate", 260, true}, {"requiresBackup", 261, true}, {"recoverySteps", 262, true}, {"planHash", 264, true}, {"createdAt", 265, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"NO_ACTION", "MUTATION_REQUIRED", "ESCALATION_REQUIRED", "TERMINAL"}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 332, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 247, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 263, {}, {}, 0, -1, true, 0, -1},
        {127, 266, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"stepID", 267, true}, {"stepType", 268, true}, {"description", 269, true}, {"jsonMutationOperations", 270, true}, {"diagnosticReference", 272, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"NORMALIZE_STATE", "APPLY_CORRECTION", "SELECT_FALLBACK", "ENTER_CONTAINMENT", "ENTER_SAFE_HALT", "ROLLBACK", "VERIFY", "ESCALATE"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 271, {}, {}, 0, -1, true, 0, -1},
        {127, 180, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"repairActionID", 274, true}, {"recoveryStep", 275, true}, {"actuatorParameters", 276, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 266, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"repairPlanID", 278, true}, {"recoveryPlan", 279, true}, {"repairActions", 280, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 254, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 281, {}, {}, 0, -1, true, 0, -1},
        {127, 273, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"rollbackPlanID", 283, true}, {"backupRecordID", 284, true}, {"triggerVerificationResultID", 285, true}, {"diagnosticReference", 286, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"rollbackResultID", 288, true}, {"status", 289, true}, {"backupRecordID", 290, true}, {"restoredHash", 291, true}, {"escalationDecision", 292, false}, {"diagnosticReference", 293, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"SUCCEEDED", "FAILED", "NOT_REQUIRED"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 125, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"ruleID", 295, true}, {"sourceReference", 296, true}, {"ruleFamily", 297, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 2, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"safeHaltDecisionID", 299, true}, {"terminal", 300, true}, {"blockedTransitions", 301, true}, {"diagnosticReference", 303, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, 1},
        {32, -1, {}, 302, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"dimensionID", 305, true}, {"sourcePointers", 306, true}, {"mappingRule", 308, true}, {"evidence", 309, true}, {"diagnosticReference", 311, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {"b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8"}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 307, {}, {}, 1, -1, true, 0, -1},
        {127, 187, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 310, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"projectionSpecID", 313, true}, {"dimensions", 314, true}, {"version", 316, true}, {"sourceReferences", 317, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 315, {}, {}, 9, 9, true, 0, -1},
        {127, 304, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 318, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"stateValidityDiagnosticID", 320, true}, {"diagnosticEnvelope", 321, true}, {"admissibilityStatus", 322, true}, {"normalizationStatus", 323, true}, {"recoverabilityRelevance", 324, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 90, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"VALID", "TRANSFORMATION_COMPATIBLE", "TRANSFORMATION_INCOMPATIBLE", "UNCLASSIFIED"}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"ALREADY_VALID", "NORMALIZABLE", "NOT_NORMALIZABLE", "BLOCKED"}, {}, 0, -1, true, 0, -1},
        {127, 247, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"differenceID", 326, true}, {"path", 327, true}, {"differenceKind", 328, true}, {"observedDocument", 329, true}, {"desiredDocument", 330, true}, {"diagnosticReference", 331, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 187, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"MISSING", "ADDED", "REMOVED", "REPLACED", "TYPE_CHANGED", "ARRAY_CHANGED"}, {}, 0, -1, true, 0, -1},
        {127, 175, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 175, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"STABLE", "UNSTABLE", "CORRECTABLE", "DEGRADED", "CONTAINED", "FAILED", "SAFE_HALT"}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"topologyID", 334, true}, {"rootState", 335, true}, {"nodes", 336, true}, {"edges", 338, true}, {"lineage", 340, true}, {"diagnosticReferences", 341, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 337, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 339, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 342, {}, {}, 1, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"transitionID", 344, true}, {"codewordReference", 345, true}, {"preconditions", 346, true}, {"diagnosticRules", 348, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 347, {}, {}, 1, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 349, {}, {}, 1, -1, true, 0, -1},
        {127, 294, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"resultID", 351, true}, {"transitionID", 352, true}, {"inputState", 353, true}, {"outputState", 354, true}, {"diagnosticReference", 355, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"verificationPlanID", 357, true}, {"checks", 358, true}, {"expectedSemanticState", 360, true}, {"generatedBeforeMutation", 361, true}, {"diagnosticReference", 362, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {32, -1, {}, 359, {}, {}, 1, -1, true, 0, -1},
        {64, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {2, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {64, -1, {{"verificationResultID", 364, true}, {"verificationPlanID", 365, true}, {"status", 366, true}, {"expectedSemanticState", 367, true}, {"actualSemanticState", 368, true}, {"diagnosticReference", 369, true}}, -1, {}, {}, 0, -1, false, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {16, -1, {}, -1, {"PASSED", "FAILED", "BLOCKED"}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 19, {}, -1, {}, {}, 0, -1, true, 0, -1},
        {127, 102, {}, -1, {}, {}, 0, -1, true, 0, -1},
    };
    return values;
}
constexpr std::array<std::size_t, 59> roots{0, 6, 13, 19, 25, 30, 37, 42, 48, 54, 61, 68, 76, 80, 85, 90, 102, 108, 114, 120, 125, 131, 138, 144, 150, 158, 164, 169, 175, 180, 187, 193, 202, 212, 218, 222, 228, 235, 242, 247, 248, 254, 266, 273, 277, 282, 287, 294, 298, 304, 312, 319, 325, 332, 333, 343, 350, 356, 363};
constexpr std::array<std::string_view, 59> names{"AdmissibilityResult", "ArtifactEmissionPlan", "AshSemanticState", "AshState", "AuditChain", "AuditEvent", "AxiomDiagnostic", "BackupRecord", "BackupResult", "CanonicalCodewordSet", "CodewordTransformation", "ContainmentDecision", "DesiredState", "DesiredSystemIntent", "DiagnosticChain", "DiagnosticEnvelope", "DiagnosticReference", "DriftEvent", "DryRunResult", "EncodedState", "EscalationDecision", "ExecutionContext", "ExecutionStepResult", "FallbackDecision", "GenerationPlan", "HealResult", "Invariant", "JsonCanonicalizationRule", "JsonDocumentReference", "JsonMutationOperation", "JsonPointer", "MappingAmbiguity", "MappingResult", "ModuleManifest", "ObservedState", "ObservedSystemState", "PolicyBundle", "PolicyDecision", "RealmIdentity", "RecoveryCategory", "RecoveryOutcome", "RecoveryPlan", "RecoveryStep", "RepairAction", "RepairPlan", "RollbackPlan", "RollbackResult", "RuleReference", "SafeHaltDecision", "SemanticDimensionBinding", "SemanticProjectionSpec", "StateValidityDiagnostic", "SurfaceDifference", "SystemStateClass", "TopologyPlan", "TransitionDefinition", "TransitionResult", "VerificationPlan", "VerificationResult"};

std::string token(std::string_view text) {
    std::string result;
    for (const char c : text) {
        if (c == '~') result += "~0";
        else if (c == '/') result += "~1";
        else result += c;
    }
    return result;
}
bool rule_pattern(std::string_view text) {
    if (!text.starts_with("ASH-")) return false;
    std::size_t position = 4;
    for (int segment = 0; segment < 2; ++segment) {
        const auto start = position;
        while (position < text.size() && text[position] >= 'A' && text[position] <= 'Z') ++position;
        if (position == start || position == text.size() || text[position++] != '-') return false;
    }
    if (text.size() - position != 3) return false;
    for (; position < text.size(); ++position) if (text[position] < '0' || text[position] > '9') return false;
    return true;
}
[[noreturn]] void violation(ContractKind kind, ContractViolation rule, const std::string& pointer, const char* message) {
    throw ContractError(kind, rule, pointer, message);
}
void validate_node(ContractKind kind, std::size_t index, const Json& value, const std::string& pointer) {
    const auto& descriptor = descriptors().at(index);
    if (descriptor.reference >= 0) {
        validate_node(kind, static_cast<std::size_t>(descriptor.reference), value, pointer);
        return;
    }
    unsigned actualType = value.is_null() ? 1U : value.is_boolean() ? 2U : value.is_number() ? 4U :
        value.is_string() ? 16U : value.is_array() ? 32U : 64U;
    if (value.is_number_integer()) actualType |= 8U;
    if ((descriptor.typeMask & actualType) == 0)
        violation(kind, ContractViolation::Type, pointer, "Frozen contract type mismatch");
    if (!descriptor.enumStrings.empty()) {
        bool found = false;
        if (value.is_string()) for (const auto choice : descriptor.enumStrings) found |= choice == value.string_value();
        if (!found) violation(kind, ContractViolation::Enum, pointer, "Frozen contract enum mismatch");
    }
    if (!descriptor.enumIntegers.empty()) {
        bool found = false;
        if (value.is_number()) for (const auto choice : descriptor.enumIntegers) found |= value == Json(choice);
        if (!found) violation(kind, ContractViolation::Enum, pointer, "Frozen contract numeric enum mismatch");
    }
    if (descriptor.constantBoolean >= 0 && (!value.is_boolean() || value.get<bool>() != (descriptor.constantBoolean != 0)))
        violation(kind, ContractViolation::Constant, pointer, "Frozen contract constant mismatch");
    if (descriptor.pattern != 0) {
        const auto& text = value.string_value();
        const bool matched = descriptor.pattern == 1 ? text.empty() || text.front() == '/' : rule_pattern(text);
        if (!matched) violation(kind, ContractViolation::Pattern, pointer, "Frozen contract pattern mismatch");
    }
    if (value.is_object()) {
        for (const auto& field : descriptor.fields) {
            if (!value.contains(field.name)) {
                if (field.required) violation(kind, ContractViolation::Required, pointer + "/" + token(field.name), "Frozen required field is absent");
            } else validate_node(kind, field.node, value.at(field.name), pointer + "/" + token(field.name));
        }
        if (!descriptor.additionalProperties) for (const auto& [key, child] : value.object_items()) {
            (void)child;
            bool found = false;
            for (const auto& field : descriptor.fields) found |= field.name == key;
            if (!found) violation(kind, ContractViolation::AdditionalProperty, pointer + "/" + token(key), "Frozen contract rejects additional field");
        }
    }
    if (value.is_array()) {
        if (value.size() < descriptor.minimumItems) violation(kind, ContractViolation::MinimumItems, pointer, "Frozen contract array too short");
        if (descriptor.maximumItems >= 0 && value.size() > static_cast<std::size_t>(descriptor.maximumItems))
            violation(kind, ContractViolation::MaximumItems, pointer, "Frozen contract array too long");
        if (descriptor.item >= 0) for (std::size_t i = 0; i < value.size(); ++i)
            validate_node(kind, static_cast<std::size_t>(descriptor.item), value.at(i), pointer + "/" + std::to_string(i));
    }
}
} // namespace

ContractError::ContractError(ContractKind kind, ContractViolation rule, std::string pointer, std::string message)
    : std::runtime_error(std::move(message)), kind_(kind), violation_(rule), pointer_(std::move(pointer)) {}
std::string_view contract_name(ContractKind kind) {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= names.size()) throw std::invalid_argument("Unknown frozen contract kind");
    return names[index];
}
void validate_contract(ContractKind kind, const Json& value) {
    const auto index = static_cast<std::size_t>(kind);
    if (index >= roots.size()) throw std::invalid_argument("Unknown frozen contract kind");
    value.validate();
    validate_node(kind, roots[index], value, "");
}
} // namespace Aeostara::BaseContracts
