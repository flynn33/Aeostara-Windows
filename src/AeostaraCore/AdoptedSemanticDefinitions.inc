// Exact immutable adopted source bytes; owner-adopted supplement SHA98539d83...
static constexpr const char* adoptedDirectBytes = R"ASHDOC({
  "profileID": "AEOSTARA-DEMO-DIRECT-BINARY-V1",
  "revision": 1,
  "status": "proposed_requires_adoption",
  "synthetic": true,
  "productionDefault": false,
  "domainMeaning": "JSON directly carries the canonical nine binary coordinates; no business interpretation is assigned.",
  "canonicalizationRuleID": "JASH-CANON-EXACT-DECIMAL-V1",
  "evaluatorID": "JASH-BINARY-EXACT-V1",
  "recognizedValidCatalogID": "AEOSTARA-DEMO-K-V1",
  "acceptedCarrierShape": {
    "root": "object",
    "pointer": "/state_vector",
    "type": "array",
        "length": 9,
        "lengthRule": "Nine entries required for complete projection; shorter array gives BLOCKED for actual absent Pointers (retain existing coordinate evidence); longer array is FAILED shape; wrong carrier type is FAILED. Do not fail a short array before reporting actual missing evidence.",
    "elementType": "exact JSON numeric 0 or 1",
    "extraRootMembers": "allowed and preserved"
  },
  "dimensions": [
    {
      "dimension": "b0",
      "index": 0,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/0",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b1",
      "index": 1,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/1",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b2",
      "index": 2,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/2",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b3",
      "index": 3,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/3",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b4",
      "index": 4,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/4",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b5",
      "index": 5,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/5",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b6",
      "index": 6,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/6",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b7",
      "index": 7,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/7",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b8",
      "index": 8,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/8",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    }
  ],
  "mutationScope": {
    "operation": "replace",
    "paths": [
      "/state_vector/0",
      "/state_vector/1",
      "/state_vector/2",
      "/state_vector/3",
      "/state_vector/4",
      "/state_vector/5",
      "/state_vector/6",
      "/state_vector/7",
      "/state_vector/8"
    ],
    "precondition": "full canonical/raw observed identity, per-slot old value, adopted immutable profile/catalog/registry/policy identities, generated verification plan, verified backup, exclusive host mutation capability",
    "preserve": "all root members other than changed state_vector slots; no additions, removals, carrier resize, key rename, metadata mutation"
  },
  "inverseRuleID": "JASH-DEMO-CARRIER-INVERSE-V1"
}
)ASHDOC";
static constexpr const char* adoptedClaimsBytes = R"ASHDOC({
  "profileID": "AEOSTARA-DEMO-DIRECT-BINARY-CLAIMS-V1",
  "revision": 1,
  "status": "proposed_requires_adoption",
  "synthetic": true,
  "productionDefault": false,
  "domainMeaning": "JSON directly carries the canonical nine binary coordinates; no business interpretation is assigned.",
  "canonicalizationRuleID": "JASH-CANON-EXACT-DECIMAL-V1",
  "evaluatorID": "JASH-BINARY-EXACT-V1",
  "recognizedValidCatalogID": "AEOSTARA-DEMO-K-V1",
  "acceptedCarrierShape": {
    "root": "object",
    "pointer": "/state_vector",
    "type": "array",
        "length": 9,
        "lengthRule": "Nine entries required for complete projection; shorter array gives BLOCKED for actual absent Pointers (retain existing coordinate evidence); longer array is FAILED shape; wrong carrier type is FAILED. Do not fail a short array before reporting actual missing evidence.",
    "elementType": "exact JSON numeric 0 or 1",
    "extraRootMembers": "allowed and preserved"
  },
  "dimensions": [
    {
      "dimension": "b0",
      "index": 0,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/0",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/0/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/0/coordinate",
            "coordinateEquals": "b0",
            "kindPointer": "/claims/0/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/0/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b1",
      "index": 1,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/1",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/1/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/1/coordinate",
            "coordinateEquals": "b1",
            "kindPointer": "/claims/1/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/1/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b2",
      "index": 2,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/2",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/2/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/2/coordinate",
            "coordinateEquals": "b2",
            "kindPointer": "/claims/2/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/2/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b3",
      "index": 3,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/3",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/3/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/3/coordinate",
            "coordinateEquals": "b3",
            "kindPointer": "/claims/3/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/3/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b4",
      "index": 4,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/4",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/4/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/4/coordinate",
            "coordinateEquals": "b4",
            "kindPointer": "/claims/4/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/4/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b5",
      "index": 5,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/5",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/5/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/5/coordinate",
            "coordinateEquals": "b5",
            "kindPointer": "/claims/5/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/5/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b6",
      "index": 6,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/6",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/6/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/6/coordinate",
            "coordinateEquals": "b6",
            "kindPointer": "/claims/6/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/6/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b7",
      "index": 7,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/7",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/7/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/7/coordinate",
            "coordinateEquals": "b7",
            "kindPointer": "/claims/7/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/7/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    },
    {
      "dimension": "b8",
      "index": 8,
      "sourceBindings": [
        {
          "sourceName": "observed",
          "pointer": "/state_vector/8",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true
        },
        {
          "sourceName": "claims",
          "pointer": "/claims/8/value",
          "evidenceKind": "JSON_BINARY_COORDINATE",
          "required": true,
          "metadataPredicates": {
            "coordinatePointer": "/claims/8/coordinate",
            "coordinateEquals": "b8",
            "kindPointer": "/claims/8/kind",
            "kindEquals": "JSON_BINARY_COORDINATE",
            "sourceReferencePointer": "/claims/8/sourceReference",
            "sourceReference": "logical reference resolved to actual captured immutable snapshot/hash/Pointer/authority", "provenanceResolutionRuleID": "JASH-CLAIM-PROVENANCE-V1", "sourceAuthorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1"
          }
        }
      ],
      "mappingRule": "exact-json-binary",
      "acceptedValues": [
        0,
        1
      ],
      "missing": "BLOCKED",
      "null": "FAILED",
      "malformedValue": "FAILED",
      "unsupportedEvaluatorOrEvidenceKind": "BLOCKED",
      "disagreeingGenuineValues": "AMBIGUOUS",
      "diagnosticRuleIDs": [
        "JASH-003",
        "JASH-004",
        "JASH-005",
        "JASH-006"
      ]
    }
  ],
  "mutationScope": {
    "operation": "replace",
    "paths": [
      "/state_vector/0",
      "/state_vector/1",
      "/state_vector/2",
      "/state_vector/3",
      "/state_vector/4",
      "/state_vector/5",
      "/state_vector/6",
      "/state_vector/7",
      "/state_vector/8"
    ],
    "precondition": "full canonical/raw observed identity, per-slot old value, adopted immutable profile/catalog/registry/policy identities, generated verification plan, verified backup, exclusive host mutation capability",
    "preserve": "all root members other than changed state_vector slots; no additions, removals, carrier resize, key rename, metadata mutation; claims snapshot read-only"
  },
  "inverseRuleID": "JASH-DEMO-CARRIER-INVERSE-V1"
}
)ASHDOC";
static constexpr const char* adoptedCatalogBytes = R"ASHDOC({
  "catalogID": "AEOSTARA-DEMO-K-V1",
  "revision": 1,
  "status": "proposed_requires_adoption",
  "synthetic": true,
  "productionDefault": false,
  "states": [
    "000000000",
    "000000001"
  ],
  "authority": "none: synthetic domain proposal only",
  "binding": "one immutable adopted catalog per domain contract; never arbitrary desired state, request-supplied mutable cache, or inferred from C",
  "sourceInvariantReconciliationRequired": "ASH classify_state_admissibility(state,C) does not expose K and promises same state/C status. Adoption must authorize this fixed recognized-valid catalog and its domain binding explicitly."
}
)ASHDOC";
static constexpr const char* adoptedRegistryBytes = R"ASHDOC({
  "registryID": "AEOSTARA-DEMO-FALLBACK-V1",
  "revision": 1,
  "status": "proposed_requires_adoption",
  "synthetic": true,
  "productionDefault": false,
  "orderingRule": {
    "status": "derived_from_source",
    "keys": [
      "ordering_rank ASC",
      "policy_id ASCII lexicographic ASC"
    ],
    "source": {
      "repository": "flynn33/ASH-Pattern-System",
      "commit": "e123f5d7fdbb381179971f721a3292c31eb1cbc2",
      "path": "specs/registries/fallback-policy-registry.md",
      "symbol": "Deterministic ordering rules"
    }
  },
  "entries": [
    {
      "policy_id": "FALLBACK-STATE-001",
      "ordering_rank": 0,
      "candidate_state_reference": "000000000",
      "applicability_conditions": [
        "diagnostic.admissibility == TRANSFORMATION_INCOMPATIBLE",
        "candidate belongs to adopted fixed K",
        "entry is enabled by immutable registry revision"
      ],
      "validation_requirements": [
        "candidate diagnoses VALID",
        "candidate classifies STABLE in fresh nonterminal validation context"
      ],
      "escalation_on_failure": "TRY_NEXT"
    },
    {
      "policy_id": "FALLBACK-STATE-002",
      "ordering_rank": 1,
      "candidate_state_reference": "000000001",
      "applicability_conditions": [
        "diagnostic.admissibility == TRANSFORMATION_INCOMPATIBLE",
        "candidate belongs to adopted fixed K",
        "entry is enabled by immutable registry revision"
      ],
      "validation_requirements": [
        "candidate diagnoses VALID",
        "candidate classifies STABLE in fresh nonterminal validation context"
      ],
      "escalation_on_failure": "ESCALATE_TO_CONTAINMENT"
    }
  ],
  "availabilityRule": {
    "status": "proposed_requires_adoption",
    "rule": "Before canonical classification, availability is an existence preflight over eligible re-diagnosed candidates using the full diagnostic and immutable registry; it does not recursively call get_fallback_candidates whose precondition is DEGRADED. After classification, canonical selection requires DEGRADED. Execution policy authorization occurs after planning and never turns a valid mapping into a different bit value."
  },
  "idReservation": "IDs shown are synthetic examples; adopting entries requires reserving globally unique identifiers or replacing them in an immutable adopted supplement."
}
)ASHDOC";
static constexpr const char* adoptedAuthorityBytes = R"ASHDOC({
  "authorityID": "AEOSTARA-DEMO-SOURCE-AUTHORITY-V1",
  "revision": 1,
  "allowedSourceNames": [
    "claims"
  ],
  "allowedEvidenceKinds": [
    "JSON_BINARY_COORDINATE"
  ],
  "provenanceRuleID": "JASH-CLAIM-PROVENANCE-V1",
  "status": "proposed_requires_adoption",
  "synthetic": true,
  "productionAuthenticationAuthority": false
}
)ASHDOC";
