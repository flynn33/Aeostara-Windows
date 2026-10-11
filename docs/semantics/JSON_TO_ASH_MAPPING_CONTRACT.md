# JSON-to-ASH Mapping and Recovery Realization Contract

Document ID: `JSON-TO-ASH-MAPPING-CONTRACT-PROPOSAL-2026-10-11`, revision 1. **REVIEW-READY PROPOSAL — NOT ADOPTED.** The separate hash-bound [SEMANTIC_MAPPING_REVIEW.md](SEMANTIC_MAPPING_REVIEW.md) controls review findings/disposition; this readiness label grants no approval. This is platform-neutral preparation authorized by the owner’s [Semantic Mapping Resolution](../OWNER_SEMANTIC_MAPPING_RESOLUTION_2026-10-11.md). It authorizes no speculative production semantics, dependent API freeze, or release qualification. The machine-readable companion is [JSON_TO_ASH_MAPPING_CONTRACT.json](JSON_TO_ASH_MAPPING_CONTRACT.json). The Windows mechanics belong in [WINDOWS_SEMANTIC_MAPPING_BINDING.md](WINDOWS_SEMANTIC_MAPPING_BINDING.md).

The reviewed sources define the full nine-bit ASH model, class-to-recovery mapping, generic projection boundary and recovery lifecycle. They do not supply a complete JSON evaluator, recognized-valid catalog, inverse materialization rule, or a lawful representation for every unavailable required result field. The concrete demo below makes these missing choices reviewable; it has no existing domain authority and is never a production default.

## Authority and rule provenance

Retain Aeostara base `774b981ef2267a41c1b1a59497b2b0746a86f32b` and ASH `e123f5d7fdbb381179971f721a3292c31eb1cbc2`. Raven Forge `8dd6e75a06336491b249fac3577db75458db8c13` governs interpretation/conflict handling only. Preserve all pinned source files, fixtures and expected bytes. ASH precedence for conflicting fallback ordering is already established by Aeostara product-boundaries rule 4 and the ASH canonical registry; it does not need a new owner preference.

Every rule below is either **derived from source**, **proposed requiring adoption**, or **unresolved**. A source citation attached to a proposal establishes the surrounding boundary, not adoption of the proposed details. JASH identifiers are local proposal traceability identifiers, never replacements for canonical ASH rule IDs.

| Rule | Status | Concrete boundary |
|---|---|---|
| JASH-001 | derived from source | Authority and separation |
| JASH-002 | proposed requires adoption | Complete semantic input tuple |
| JASH-003 | derived from source | All nine coordinate meanings |
| JASH-004 | derived from source | Pointer, null and structure semantics |
| JASH-005 | proposed requires adoption | Concrete binary evaluator |
| JASH-006 | proposed requires adoption | No manufactured full state on incomplete mapping |
| JASH-007 | proposed requires adoption | Recognized valid state catalog and admissibility evidence |
| JASH-008 | derived from source | Canonical classification |
| JASH-009 | derived from source | Canonical class to recovery and blocked transitions |
| JASH-010 | derived from source | Canonical fallback registry procedure |
| JASH-011 | proposed requires adoption | Authorized inverse and concrete operations |
| JASH-012 | proposed requires adoption | Policy, stale, backup and capability gates |
| JASH-013 | proposed requires adoption | Verification postconditions |
| JASH-014 | proposed requires adoption | Truthful failure and rollback |
| JASH-015 | proposed requires adoption | Conservative exact JSON supplement |
| JASH-016 | derived from source | Genuine diagnostics and source references |
| JASH-017 | proposed requires adoption | Independent fixtures and oracle |
| JASH-018 | proposed requires adoption | Frozen DTO applicability and typed unsuccessful operations |

Exact references and symbols are retained in each JSON rule record. The essential source bindings are:

- **JASH-001:** [flynn33/aeostara:specs/architecture/product_boundaries.md — Non-negotiable rules, rule 4](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/architecture/product_boundaries.md); [flynn33/aeostara:specs/architecture/ash_authority_and_aeostara_conformance.md — ASH authority and platform-neutral adaptation](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/architecture/ash_authority_and_aeostara_conformance.md); [flynn33/ASH-Pattern-System:specs/core/ash-state-space.pseudo.md — State space and full-state invariants](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md).
- **JASH-003:** [flynn33/ASH-Pattern-System:specs/core/ash-state-space.pseudo.md — F2^9 and full-state semantics](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/ash-state-space.pseudo.md); [flynn33/aeostara:specs/algorithms/semantic_projection.pseudo.md — coordinate ordering](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/algorithms/semantic_projection.pseudo.md).
- **JASH-004:** [flynn33/aeostara:specs/algorithms/json_pointer_operations.pseudo.md — Pointer resolution](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/algorithms/json_pointer_operations.pseudo.md); [flynn33/aeostara:specs/contracts/JsonPointer.schema.json — pointer,tokens,pointsToMissing,pointsToNull](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/JsonPointer.schema.json).
- **JASH-008:** [flynn33/ASH-Pattern-System:specs/core/system-state-classification.pseudo.md — classify_system_state](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/system-state-classification.pseudo.md); [flynn33/ASH-Pattern-System:specs/core/state-validity-diagnostics.pseudo.md — diagnose_state](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/state-validity-diagnostics.pseudo.md).
- **JASH-009:** [flynn33/ASH-Pattern-System:specs/core/recoverability-semantics.pseudo.md — classify_recoverability and Blocked recovery conditions](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/core/recoverability-semantics.pseudo.md).
- **JASH-010:** [flynn33/ASH-Pattern-System:specs/registries/fallback-policy-registry.md — get_fallback_candidates, select_fallback_candidate, Candidate eligibility, Deterministic ordering, Escalation](https://github.com/flynn33/ASH-Pattern-System/blob/e123f5d7fdbb381179971f721a3292c31eb1cbc2/specs/registries/fallback-policy-registry.md).
- **JASH-018:** [flynn33/aeostara:specs/interfaces/ISemanticProjector.pseudo.md — explicit C++ error results, schema-valid returned contracts, no fabricated empty state](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/interfaces/ISemanticProjector.pseudo.md); [flynn33/aeostara:specs/contracts/MappingResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/MappingResult.schema.json); [flynn33/aeostara:specs/contracts/BackupResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/BackupResult.schema.json); [flynn33/aeostara:specs/contracts/ExecutionStepResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/ExecutionStepResult.schema.json); [flynn33/aeostara:specs/contracts/RollbackResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/RollbackResult.schema.json); [flynn33/aeostara:specs/contracts/FallbackDecision.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/FallbackDecision.schema.json); [flynn33/aeostara:specs/contracts/VerificationResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/VerificationResult.schema.json); [flynn33/aeostara:specs/contracts/HealResult.schema.json — required](https://github.com/flynn33/aeostara/blob/774b981ef2267a41c1b1a59497b2b0746a86f32b/specs/contracts/HealResult.schema.json).

## Complete immutable input boundary — JASH-002 (proposal)

The shared evaluator consumes an explicit semantic input tuple, with typed “unavailable” values where a phase has not produced evidence. No default state, empty policy, implied fallback registry, machine-specific discovery, fixture label, or expected assertion supplies missing meaning.

| Input | Required identity and evidence | What it may affect |
|---|---|---|
| Observed snapshot | Exact source/document identity, original UTF-8 bytes, raw-byte SHA-256, strict parsed document and canonical SHA-256 | The actual projected state and mutation preconditions |
| Desired snapshot | Independent immutable document identity/bytes/hashes; partial-intent declaration | Intent and proposed target; never automatically recognized-valid or authorized |
| Mapping profile | Adopted profile ID, revision, source authority, content hash, evaluator ID, all nine source bindings | Exactly which genuine evidence determines each coordinate |
| JSON rule | Immutable canonicalization/Pointer/type policy ID, revision and hash | Parsing, equality, canonical identities and Pointer resolution |
| Invariants | Applicable invariant bundle ID/revision/hash, complete definitions and read evidence | Validity of a proposed repair and its verified postconditions; no undefined bit meaning |
| Evidence snapshots | Named immutable source references, bytes/hashes, declared evidence kinds and provenance | Only those claims explicitly consumed by the profile |
| ASH authority | Fixed ASH pin, fixed C identity and full nine-coordinate state | Canonical diagnosis and transformation witnesses |
| Recognized-valid catalog | One immutable adopted domain catalog K and its authority/revision/hash | Recognized valid states and orbit witnesses; unresolved authority today |
| Lifecycle context | Explicit safe-halt, containment and boundary-breach evidence; external-authority reachability; recovery phase | Canonical terminal/contained precedence and later escalation |
| Correction context | Actual complete witness, known target/path evidence and selected authorized target reference, or explicit unavailable reason | Whether a specific correction path is known and whether it can be materialized |
| Recovery registry | Actual immutable registry revision/content hash; all predicates, entries and candidate state identities | Fallback availability/selection, deterministic order and escalation |
| Realization authority | Adopted inverse rule and target/scope authorization identities | Which concrete configuration operations are justified |
| Execution inputs | Actual policy bundle/decision, verification plan, backup record/evidence and explicit host capabilities | Whether planned mutation can begin, succeed or be restored |

Semantic determinism compares the complete semantic tuple and semantic payload. Event IDs, one-based sequence numbers, timestamps and host I/O outcomes are explicitly supplied or recorded outside that comparison; they cannot change projection or classification covertly. A different class must have a documented difference in diagnostic/context inputs. K cannot be a mutable per-request cache: the ASH `classify_state_admissibility(state,C)` invariant promises the same status for the same state/C, and adopting a domain-bound catalog requires an authoritative reconciliation of this currently implicit recognized-valid state source.

Proposed identity convention: canonicalize each semantic document using the adopted JSON rule, lower-case SHA-256 its UTF-8 bytes, and hash a canonical structured input manifest containing every semantic dependency identity. Hash original bytes separately. A raw file hash includes its exact formatting and final newline; it is **not** the canonical semantic hash. Distinct dependency revisions invalidate plan/cache/precondition identity even when their visible projected bits happen to agree. Expected outputs and fixture IDs are excluded from this manifest and evaluator access.

## Nine coordinates and concrete demo profile — JASH-003/005

**Source-derived meaning:** each of b0 through b8 is an equal binary coordinate of F2^9, in that exact order. No coordinate carries a supplied business meaning. Canonical C containing only b8=0 is a property of transformations; b8 remains part of every state, diagnosis and verification.

**Proposed direct carrier:** `AEOSTARA-DEMO-DIRECT-BINARY-V1`, revision 1, treats the observed JSON object’s exact `state_vector` array of nine entries as direct numeric coordinates. Other root members are permitted and protected. It is a synthetic domain example, not an inferred existing application model. [Exact proposed direct profile](semantic-mapping/proposed_direct_profile.json), [claims profile](semantic-mapping/proposed_claims_profile.json), [K catalog](semantic-mapping/proposed_recognized_valid_catalog.json) and [fallback registry](semantic-mapping/proposed_fallback_registry.json) are independently byte-hashed for fixture references.

| Coordinate | Existing meaning | Required direct source Pointer | Accepted value | Rule/diagnostic references |
|---|---|---|---|---|
| b0 | Binary coordinate 0 of full F2^9 | observed `/state_vector/0` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b1 | Binary coordinate 1 of full F2^9 | observed `/state_vector/1` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b2 | Binary coordinate 2 of full F2^9 | observed `/state_vector/2` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b3 | Binary coordinate 3 of full F2^9 | observed `/state_vector/3` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b4 | Binary coordinate 4 of full F2^9 | observed `/state_vector/4` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b5 | Binary coordinate 5 of full F2^9 | observed `/state_vector/5` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b6 | Binary coordinate 6 of full F2^9 | observed `/state_vector/6` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b7 | Binary coordinate 7 of full F2^9 | observed `/state_vector/7` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |
| b8 | Binary coordinate 8 of full F2^9 | observed `/state_vector/8` | Exact JSON numeric 0 or 1 | JASH-003/004/005/006; genuine source refs |

All coordinates use the same explicit evaluation rules. Desired is projected independently when a target is needed; it never supplies an absent observed bit. No generic surface difference, changed-key count, dotted path, boolean truthiness, distance heuristic or checksum is a coordinate evaluator.

| Evidence condition | Proposed coordinate outcome | Retained evidence |
|---|---|---|
| Every required supported source yields one exact numeric 0, or every such source yields one exact numeric 1 | MAPPED with that bit | Snapshot identity/hash, literal Pointer, actual extracted numeric value and evaluator rule |
| Required snapshot unavailable or valid Pointer absent (including short array) | BLOCKED | Missing source/Pointer and exact availability reason |
| Unsupported evaluator or declared evidence kind | BLOCKED | Actual profile/evidence identity and unsupported kind; no guess |
| Explicit null; bool/string; nonbinary number; wrong carrier type; invalid metadata/provenance; malformed JSON | FAILED | Actual present value/type or parsing/source error |
| Two or more explicitly bound supported genuine sources disagree between 0 and 1 | AMBIGUOUS | Every genuine claim and candidate bit, never a manufactured majority winner |
| Mixture of different failing coordinates | FAILED before BLOCKED before AMBIGUOUS (proposed precedence) | All coordinate outcomes/faults remain present; precedence never erases them |

Carrier shape and per-coordinate resolution are separate evidence: a short array reports BLOCKED for real absent indices while retaining present coordinate evidence; more than nine entries is FAILED shape; a null entry remains present-null and fails; a non-array carrier fails type validation. Nine entries are required for complete projection, but a short-array shape check never preempts the specific missing-Pointer outcomes. A full projected state exists only when nine unique genuine bits have been obtained. No zero padding or guessed binding is allowed.

The explicit `AEOSTARA-DEMO-DIRECT-BINARY-CLAIMS-V1` profile requires a second named immutable `claims` snapshot, with `/claims/i/value` for each coordinate. Each record has `coordinate: bN`, `kind: JSON_BINARY_COORDINATE`, actual captured `value` and a logical `sourceReference` resolved under JASH-CLAIM-PROVENANCE-V1; the profile binds and validates the corresponding metadata Pointers and expected source authority. The original observed carrier and this second source must agree. This separate profile makes ambiguity reproducible. Supplemental claims are never read or overridden implicitly by the direct profile. Unknown declared evidence kinds block; malformed/mismatched value/metadata types fail.

**Proposed genuine source/evidence resolution — JASH-CLAIM-PROVENANCE-V1:** a nonempty reference string is never sufficient. Complete input includes immutable captured source snapshots, an evidenceReferenceManifest and a source-authority definition plus explicitly registeredAuthorityRef. The authority’s semantic hash canonicalizes exactly authorityID/revision/allowedSourceNames/allowedEvidenceKinds/provenanceRuleID; raw locator/status metadata is excluded. The expected authority comes from adopted profile/host source capability, not from the claim itself. Demo source authority [proposed_source_authority.json](semantic-mapping/proposed_source_authority.json) is explicit synthetic test registration and makes no production authentication claim.

The manifest is `{manifestID,revision,authorityRef,references}`. Each unique record binds logical reference, authorityID, sourceName, actual snapshotID, computed canonicalDocumentHash, exact Pointer, coordinate and evidenceKind. Resolve every claim reference to the exact actual source snapshot/hash/Pointer and adopted registered authority, then extract the value from the snapshot; never accept a detached asserted value instead. Missing source/manifest/authority or unresolvable logical reference is BLOCKED. Malformed, duplicate/conflicting, mismatched or tampered provenance is FAILED. Two supported fully resolved genuine extracted values that disagree remain AMBIGUOUS. Synthetic manifests are computed from actual fixture source documents and proposed resolver authority, never expected outputs. Production source authority/attestation still needs an immutable adopted supplement.

Proposed per-coordinate retained record: coordinate, outcome, all actual source names/document references, source raw/canonical hashes, Pointer, evidence kind, actual value/type where available, fault reason and genuine linked diagnostics. This is a supplemental unsuccessful-attempt evidence model, not an unauthorized extra field in frozen `SemanticDimensionBinding`. Conversion to frozen bindings occurs only with genuinely complete schema-valid fields and an adopted return/error interpretation.

## Exact JSON boundary — JASH-004/015

The existing source establishes Pointer addressing, null/missing distinction, UTF-8 lexicographic object-key order, array-order preservation, deterministic numeric/string representation and SHA-256 canonical hashing. It does not fully define numeric lexical identity, duplicate-member handling, invalid Unicode, resource limits or every array index spelling. The following complete precision policy is **proposed, unapproved** under `JASH-CANON-EXACT-DECIMAL-V1`:

- UTF-8 JSON without BOM, comments, trailing commas, NaN/Infinity, invalid UTF-8 or unpaired surrogates. Preserve original bytes for actual rollback.
- JSON number grammar `-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)?`, ASCII digits, exact signed decimal coefficient/exponent. No binary-float conversion. Mathematical equality makes 1, 1.0 and 1e0 equal; bool is never numeric. Normalize all numeric zero spellings, including negative zero, to 0.
- Emit exact expanded decimal numbers without exponent, plus sign, insignificant fractional zeros or trailing decimal point. Examples: 1.0→1, 1e3→1000, 1e-3→0.001, -0.0→0. The integer 9007199254740993 remains exact. A direct bit accepts every lexically valid exact numeric zero/one under this proposed rule, not strings or bool.
- Reject duplicate decoded keys before constructing lookup maps, including `a` and `\u0061`. No first/last-wins handling. Unicode scalar sequences receive no normalization or casefolding; paired escaped surrogates encode their scalar, unpaired ones fail.
- Canonical strings emit UTF-8; escape quote/backslash; use `\b\t\n\f\r` for those controls and lowercase `\u00xx` for the other U+0000..001F; leave slash and all remaining scalars literal. Object keys sort by unsigned UTF-8 byte order. Arrays retain their exact order. Canonical output has no whitespace.
- Pointer root is empty, `/` denotes an empty object key, dots are literal. Decode `~0/~1` once; any other tilde escape fails. Array reads require ASCII `0|[1-9][0-9]*`; no leading zeros, signs, Unicode decimal digits or `-`. Bounds checks do not wrap. Missing valid index/key blocks; present null fails this binary evaluator; wrong container type fails. Write pointers in the demo are the nine fixed existing slots only.

| Proposed limit | Bound |
|---|---|
| Raw bytes per document | 8,388,608 |
| Aggregate complete-input bytes / source documents | 67,108,864 / 32 |
| Canonical bytes per document | 16,777,216, checked before unbounded expansion |
| Depth / nodes per document | 64 (root depth 1) / 100,000 (each value/container one node) |
| Decoded UTF-8 bytes per key/string | 1,048,576 |
| Significand coefficient digits | 1,024 counted before insignificant-zero removal |
| Written exponent magnitude | 4,096 |
| Effective exponent magnitude | 5,120 after subtracting fraction length and removing coefficient trailing zeros |
| Pointer tokens | 64 |

Limit violation is an explicit typed input/canonicalization failure with diagnostics; no truncated value or guessed hash. Implementation must bound expansion and aggregate input before allocating the full output. Limits apply to profile/invariant/registry/evidence documents as well as observed/desired JSON.

**Reference compatibility remains an adoption decision.** The pinned `ci/common_checks.py::CanonicalJson` uses Python JSON serialization and its parsed numeric representation; reference loaders currently accept duplicate-last-wins and `JsonPointerEvaluator` uses broader `isdecimal()` array indices. Those observed mechanisms do not silently become normative precision rules, and this proposal cannot silently replace them. Adoption requires an immutable clarification and supplementary exact lexical/Unicode/duplicate/index vectors; existing pinned reference and expected bytes remain intact. Agreement on the simple existing canonical JSON fixture does not prove agreement for all numeric inputs.

## Recognized-valid states, witnesses and classification — JASH-007/008/009

Canonical C is the fixed 16-member set in the JSON companion, copied from ASH `codeword-set.pseudo.md`. C is a transformation set, not the recognized-valid catalog.

For the **synthetic** domain, propose fixed `AEOSTARA-DEMO-K-V1 = {000000000,000000001}`. This is a complete finite example chosen to demonstrate both b8 values. It has no existing authority; it is not offered as the real project’s valid states. After an immutable domain catalog is adopted, proposed evaluation is:

1. A genuine full x in fixed K diagnoses VALID / ALREADY_VALID.
2. Otherwise enumerate the fixed complete K and canonical C. A genuine witness `x xor k = c, c in C` proves transformation compatibility and retains all nine bits, k, c and source identities.
3. No witness with complete evaluable catalog/C proves transformation incompatibility. Actual inability to evaluate established canonical evidence produces UNCLASSIFIED. Missing catalog authority is an unresolved input/contract boundary, not permission to assert all states invalid or invent a catalog.
4. **Proposed concrete `correction_path_is_known` predicate:** true only when complete correction context explicitly selects an immutable-source-bound witness and validation proves its sourceState equals the actual full observed state, targetState belongs to fixed K, every listed codeword belongs to pinned C, and the ordered XOR chain reaches that target. Exhaustive orbit existence supplies NORMALIZABLE evidence; it does not silently select a specific correction witness. Explicit `not_selected` context makes known-path false, so compatible+NORMALIZABLE yields UNSTABLE. Validated `selected` context yields CORRECTABLE. This distinction is a proposed domain rule requiring adoption, not a new claim about existing authority.

Proposed selected context fields are `status: selected`, `sourceState[9]`, `targetState[9]`, `codewords[1..16][9]`, canonical-content `witnessID`, immutable platform-neutral `evidenceSourceReference`, semantic `profileRef`/`catalogRef` and pinned `codewordSetRef` as defined below. Validate every shape, source/hash identity, full state and codeword step. Malformed/noncanonical/out-of-K/mismatched witnesses fail before classification; a claimed boolean is insufficient. `not_selected` explicitly carries no codewords and no target in the supplemental tagged-absence shape; it never modifies a frozen DTO. `not_evaluated` is allowed before diagnosis but blocks a compatible classification that requires the missing context. Materialization authorization is a later gate and cannot be inferred from selected witness existence.

**Proposed exact platform-neutral identity projection — JASH-SEMANTIC-IDENTITY-V1:**

- Profile definition hash canonicalizes exactly `profileID, revision, canonicalizationRuleID, evaluatorID, recognizedValidCatalogID, acceptedCarrierShape, dimensions, mutationScope, inverseRuleID`. Catalog definition hash canonicalizes exactly `catalogID, revision, states`. The codeword definition hash canonicalizes exactly the JSON companion’s `{coordinateOrder:[b0..b8],members:[the sixteen ordered nine-bit strings]}` object.
- Semantic `profileRef` is exactly `{profileID,revision,semanticDefinitionHash}`; `catalogRef` is exactly `{catalogID,revision,semanticDefinitionHash}`. `codewordSetRef` is exactly `{repository,commit,path,semanticDefinitionHash}`, where path is the pinned repository-relative `specs/core/codeword-set.pseudo.md`, never a local transport path.
- Construct the witness identity payload with exactly `witnessRuleID: JASH-CORRECTION-WITNESS-ID-V1`, actual `sourceState`, `targetState`, ordered `codewords`, immutable logical `evidenceSourceReference`, and the three semantic refs above. SHA-256 the canonical UTF-8 payload; exclude witnessID itself. Evidence source reference is a platform-neutral immutable logical authority/source ID or content-addressed URN, never a local path.
- Exclude local locator/path/volume/host, raw file-byte hash/formatting/newline, display/review/adoption status, notes and timestamps from these semantic identity projections. Retain raw transport/provenance evidence separately, including actual raw snapshot identity for execution preconditions. Relocation or a review-label change cannot alter this witness identity; semantic dependency/state/path changes do. These identity projections are proposed and require adoption, like the evaluator.

The source algorithm’s recognized-valid set is implicit and not enumerated. Its determinism invariant names state/C, not a free mutable K argument. An adopted supplement must fix catalog ownership/domain scope and reconcile that invariant. Merely allowing arbitrary host-provided K would change semantics.

The canonical classification procedure is applied only after genuine full-state diagnostics exist:

1. Safe-halt context → SAFE_HALT; otherwise containment context → CONTAINED.
2. VALID + ALREADY_VALID → STABLE.
3. TRANSFORMATION_COMPATIBLE + NORMALIZABLE → CORRECTABLE if the specific correction path is known, otherwise UNSTABLE.
4. TRANSFORMATION_INCOMPATIBLE → DEGRADED when fallback is available, otherwise FAILED.
5. Remaining/UNCLASSIFIED → DEGRADED.

Preserve this immediate class separately from later blocked recovery/escalation. Failed JSON projection is not a full-state diagnosis and cannot be classified by supplying an invented all-zero vector. The source names correction/fallback helper predicates without completing the domain evidence/availability rules; the JSON proposal explicitly records those dependencies.

| Canonical class | Canonical recovery category |
|---|---|
| STABLE | NO_ACTION |
| UNSTABLE | NORMALIZE_STATE |
| CORRECTABLE | APPLY_CORRECTION |
| DEGRADED | FALLBACK_REQUIRED |
| CONTAINED | CONTAINMENT_REQUIRED |
| FAILED | ESCALATION_REQUIRED |
| SAFE_HALT | TERMINAL_NO_RECOVERY |

The table is already source-defined and independently implementable/testable with explicit class input. Blocked NORMALIZE_STATE→containment; blocked APPLY_CORRECTION→fallback; no fallback registry/candidates→containment; containment breach→terminal; unreachable external authority during escalation→terminal. Do not introduce another severity scale or re-label an earlier diagnostic to make a later fixture pass.

## Fallback registry — JASH-010

**Source-derived:** actual immutable entries have stable globally unique `FALLBACK-{DOMAIN}-{SEQUENCE}` IDs; all applicability predicates must pass; rank ascending then policy ID lexicographic order; candidate must diagnose VALID and reclassify STABLE; validation failure invokes TRY_NEXT or ESCALATE_TO_CONTAINMENT. No valid candidate produces a recorded containment escalation. No distance/cache/majority heuristic or guessed fallback state is permitted.

**Synthetic proposal:** `AEOSTARA-DEMO-FALLBACK-V1` has FALLBACK-STATE-001 rank 0 / zero state / TRY_NEXT and FALLBACK-STATE-002 rank 1 / 000000001 / ESCALATE_TO_CONTAINMENT. Entries apply to a full transformation-incompatible diagnostic with candidate in fixed K and entry enabled by immutable registry revision. The entry contents/IDs require adoption or replacement/reservation; ASH ordering already has authority.

The undefined `fallback_is_available` helper receives a proposed existence preflight over actual immutable registry contents and genuine eligible, re-diagnosed candidates before canonical classification. It does not recursively call `get_fallback_candidates`, whose source precondition is already DEGRADED. Actual canonical selection occurs after DEGRADED. Policy execution authorization remains after planning: availability of a known-good semantic candidate is not permission to mutate. Missing/inaccessible registry, empty entries or no eligible validated candidate is explicitly unavailable. A flag alone cannot manufacture or hide candidates; a context flag saying “fallback available” while providing an empty registry is inconsistent. The synthetic unavailable case supplies a separately hashed empty registry variant.

## Concrete repair realization — JASH-011/012/013/014

Propose `JASH-DEMO-CARRIER-INVERSE-V1`: every justified operation replaces an existing changed `/state_vector/i` numeric slot and carries its actual old value, authorized new bit, whole-document precondition identity and linked rule/evidence. It never adds/removes/resizes a carrier, mutates claims, renames keys or overwrites unrelated metadata. A changed bit and a recovery category supply no execution authority by themselves.

**Proposed invariant boundary:** each applicable immutable invariant declares a complete predicate/evidence and exactly one role: MUTATION_PRECONDITION, LIFECYCLE_SUCCESS_POSTCONDITION, RESTORATION_SAFETY or DIAGNOSTIC_ONLY, with explicit mandatory/diagnostic disposition. Pure evaluation records PASS/VIOLATION/UNAVAILABLE and genuine source/value evidence within observation/normalization/policy/verification; undefined role/predicate/evidence blocks, never becomes an empty-policy pass. It never silently changes projected bits or ASH classification. Mandatory mutation preconditions gate execution. Repair postcondition violations may trigger an independently authorized canonical plan, but are not automatically mutation preconditions that make the repair impossible; they must pass after repair before HEALED.

For **STABLE/NO_ACTION**, preserve that canonical class/category and zero mutation steps. Evaluate all mandatory lifecycle-success postconditions against the actual observed snapshot before any overall healthy/success result. A real violation or unavailable predicate blocks product lifecycle success and records linked diagnostics/typed unsuccessful outcome; it cannot be hidden by no-drift/no-action, changed into another ASH class or repaired by a generic diff. Diagnostic-only invariants remain truthful diagnostic evidence unless adopted policy expressly makes them a gate.

NORMALIZE_STATE/APPLY_CORRECTION require a genuine canonical codeword path to an explicitly authorized fixed-K target. Desired supplies intent only; invalid/unrecognized desired is not a target. FALLBACK additionally requires the canonically selected eligible validated registry entry and allowed realization. NO_ACTION has zero mutation steps even when surface metadata differs. CONTAINMENT/ESCALATION/SAFE_HALT invoke specifically bound host capabilities and diagnostics; they do not justify invented JSON edits.

If inverse/target permission or policy authorization is absent, product materialization/execution is blocked while preserving the genuine mathematical witness, canonical class and recovery category. A known validated path does not become “path not computable” merely because concrete realization or permission is unavailable. Invoke a canonical path-not-computable escalation only when its actual source condition holds; execution rejection alone cannot automatically authorize fallback. Any later fallback requires its own justified recovery category, canonical registry selection, target/inverse permission and policy/backup gates. No automatic nearest target, lexicographic K target, arbitrary desired target or generic desired-subset merge is proposed. A complete synthetic repair fixture must explicitly bind its authorized target and witness.

| Phase | Proposed required evidence/gate | Required outcome when unavailable or false |
|---|---|---|
| Pure preparation | Immutable snapshots, full projection/diagnostic, canonical class/category, concrete plan and scope | No configuration mutation; typed unsuccessful outcome |
| Policy gate | Actual adopted policy bundle, ALLOW decision and resolvable references | BLOCK/ESCALATE prevents mutation; no empty-policy success |
| Verification planning | Full target, invariants, exact permitted/protected scope and checks generated before mutation | Execution blocked |
| Preconditions | Exclusive host mutation capability; current raw/canonical bytes and all profile/catalog/registry/policy identities match plan | Stale attempt blocked before writes; fresh observation/plan/authorization/backup needed |
| Backup | Unique genuine storage record; confirmed byte copy/write and verified source bytes/hash | No mutation; false/throw/absent artifact never claims BackupCreated |
| Execution | Pre-execution diagnostics/audit confirmed, recovery armed before first possible write, explicit allowed operations only | Retain primary failure, read actual state where possible, attempt authorized recovery |
| Verification | Re-read actual bytes, parse/project/diagnose all nine bits with same immutable dependencies; authorized target, STABLE where required, every invariant, intended changes and unchanged protected scope | No HEALED/success; authorized recovery and canonical escalation |
| Restore | Actual authorized backup and confirmed restore plus exact byte equality/raw hash to captured original and explicit mandatory restoration-safety invariants | SUCCEEDED restoration only with real confirmation; preserve original repair-trigger invalidity/violations and failed healing outcome; otherwise FAILED with both initiating/recovery faults |
| Post-failure | Genuine diagnostics, audit/persistence outcome, declared fallback/containment/halt boundary | No success-on-exception, silent fallback, guessed state/hash or unverified resume |

False adapter returns, partial write exceptions, unreadable post-state, verification failure and diagnostic/audit persistence failure are distinct recorded outcomes. Restore call completion is not successful restoration. Once execution may have mutated, preserve that fact until actual verification proves exact restoration. Successful restoration returns the captured original preimage and satisfies separately declared restoration-safety predicates; it need not make that originally invalid configuration pass the unmet repair postconditions. Preserve its preexisting violations and the original failed healing outcome. Restoration success never implies VALID/STABLE, HEALED or automatic resume; restore failure never erases the initiating failure. Terminal state remains terminal. Native I/O details and dependency ownership belong in the separate Windows binding.

## Unsuccessful attempts and frozen DTOs — JASH-006/018

The exact conflict is bounded: frozen `MappingResult` requires a genuine `projectedState` and exactly nine dimension bindings for AMBIGUOUS/BLOCKED/FAILED; `ISemanticProjector::project` requires schema-valid returns, blocked contract results, explicit native error results, and prohibits fabricated empty states. Source support for a C++ result/error mechanism does **not** automatically prove an outer failure meets its promised frozen DTO return contract. No existing authorized complete representation has been established for absent/ambiguous coordinates.

Propose supplemental `OperationAttempt<T> = Value(T) | Failure(OperationFailure)`. The value branch is complete, truthful and frozen-schema-valid. The failure branch retains actual phase, typed stable error, coordinate outcomes, genuine available evidence, explicit field/reason for unavailable evidence, actual mutation state, primary fault, ordered secondary recovery/export/persistence faults and genuine available diagnostic chain. These proposal-only fields must not be added to frozen T silently. The failure itself is never serialized as a schema-valid T.

| Frozen DTO | Required genuine fields relevant to conflict | Honest negative DTO where evidence exists | Needs adopted failure boundary when evidence absent |
|---|---|---|---|
| MappingResult | `projectedState`, `dimensionBindings[9]` | Only when a genuine complete state and all bindings already exist and reported negative status meaning is explicitly authorized. | Any coordinate missing/ambiguous/malformed or no genuine unique state; use proposal, not invented zero. |
| BackupResult | `backupRecord` | An actual attempted record may exist even if storage failed, provided fields truthfully denote attempted source/storage and do not claim persisted artifact. | No real record/source/storage reference exists; applicability clarification required. |
| ExecutionStepResult | `observedHashAfterStep` | Negative/blocked/skipped attempt may return genuine current hash after independently reading actual state. | Post-attempt state cannot be read/hash verified; unknown is not empty or pre-write hash. |
| RollbackResult | `backupRecordID`, `restoredHash` | Only if adopted wording permits actual observed post-restore-attempt hash with FAILED status; SUCCEEDED additionally requires verified restoration. | No backup ID or no actual readable restored/post-attempt bytes; NOT_REQUIRED without a restoration hash needs same clarification. |
| FallbackDecision | `selectedCandidateID` | BLOCKED may refer to a genuinely selected/attempted candidate if status semantics explicitly allow it. | UNAVAILABLE with no selected candidate cannot invent ID or empty sentinel. |
| VerificationResult | `expectedSemanticState`, `actualSemanticState` | FAILED may contain real expected and observed complete states with mismatch/invariant violation. | Mapping/read failure leaves no full actual state; do not invent expected/actual. |
| HealResult | `recoveryPlan`, `auditChain`, `diagnosticChain` | Complete real precomputed plan and chains may support BLOCKED/ESCALATED/SAFE_HALT result. | Projection/planning/persistence prevents required real plan or chain; absence/failure must be typed, not manufacture recoveryPlan. |

This table lists fields driving the unavailable-evidence conflict, not a substitute for each schema’s full required list. IDs/status/diagnostic references and every other required field remain necessary. A FAILED status is not universally unreturnable: genuine attempted references or actual complete observed states/hashes can exist. Conversely, a string schema accepting an empty string syntactically does not authorize an empty ID/hash sentinel or misleading `restoredHash` meaning.

Two precise adoption routes remain:

1. **ERRATUM-OUTER-FAILURE:** immutable authoritative interface clarification defines when native non-return errors satisfy failed operation reporting, restricts frozen DTO return to complete truthful required evidence, and defines per-operation wire/export error applicability.
2. **VERSIONED-RESULT-UNION:** a new immutable supplemental schema version discriminates a truthful existing DTO branch from a typed unsuccessful operation branch, expressly representing unavailable evidence in that new branch. Frozen schemas and pins remain unchanged; compatibility/serializer migration must be identified.

Neither is adopted. No optional/null `projectedState`, empty zero state, fabricated backup record, pre-write hash masquerading as post-write hash, invented selected candidate, or fake plan/chain is authorized. Production dependent APIs remain unresolved until an immutable source disposition adopts a lawful route.

## Fixture consistency and independent test obligations — JASH-017

Preserve original fixture bytes/hashes and assertions. New proposal fixtures reference exact immutable demo inputs and have separate assertion files. Actual evaluators may receive complete inputs only; no fixture ID, expected class/category, assertion or label enters runtime computation.

- The two projection negatives declare identical observed/desired empty objects but expect different classes/categories. Proposed distinguishing mapping cases are explicit two-source conflict versus missing required source/Pointer; both honestly retain unsuccessful projection and **no full state/class**. Retaining their existing DEGRADED/FAILED class assertions requires an authoritative genuine full diagnostic/context/lifecycle supplement or expectation erratum. There is no evidence to choose either interpretation for a green test.
- The four admissibility cases declare identical empty inputs. Proposed supplements need actual full states, fixed recognized-valid authority, path evidence and fallback/context differences. UNCLASSIFIED’s immediate canonical class is DEGRADED; a later FAILED expectation needs a documented subsequent transition or correction.
- Scenario 2 supplies observed and desired all-one nine-bit vectors. The synthetic K demonstrates genuine correction witness `111111111 xor 000000001 = 111111110 in C`, preserving b8. It does not establish the original desired vector as valid/authorized, satisfy that original scenario, or prove that K belongs to the product.
- Pinned `FixtureSuiteEvaluator::_compute_actual` seeds values from expected and `_compute_projection` compares supplied coordinate names/count. Existing structural checks remain useful; their success is not independent semantic proof.

Required independent test design includes identical complete-input determinism; a single explicit selected-witness/registry difference giving a justified different canonical outcome; all nine bit evaluations; missing/null/malformed/unsupported/ambiguous evidence and mixed failure; exact Pointer/numeric/duplicate/Unicode/limits; every canonical classifier/class→category branch with explicit diagnostic/context input; terminal/blocked escalation; no writes in pure/dry-run/rejected phases; explicit target/inverse scope; stale/policy/backup gates; read/verify/partial-write failures; truthful restore false/throw/unreadable cases; and paired diagnostics without expected-driven evaluation. Integrated UNSTABLE/CORRECTABLE fixtures must differ in the explicit not-selected versus selected validated witness context defined above; an expected label is never that input.

The separate [semantic-mapping test preparation](semantic-mapping/TEST_DESIGN.md), proposed input/assertion vectors and source-rule-test traceability record document which assertions are proposed/unexecuted and which independent checks actually ran. Already-defined explicit-class mapping tests are separate from the proposed JSON evaluator and release acceptance.

## Bound adoption decisions and implementation gate

The following complete proposals are available for adoption/replacement by immutable authoritative disposition; this is not a questionnaire or an implied default selection:

| Disposition | Bound proposal | Missing authority |
|---|---|---|
| ADOPT-PROFILE | JASH-002/005; AEOSTARA-DEMO-DIRECT-BINARY-V1 and optional explicit CLAIMS-V1 | Immutable domain mapping supplement adopts exact profile, evidence shapes, evaluator outcomes and mixed precedence, or replaces them with another fully specified profile. |
| ADOPT-K | JASH-007; AEOSTARA-DEMO-K-V1 | ASH-compatible recognized-valid catalog authority and same-state/C determinism reconciliation. No inference from C or desired. |
| ADOPT-CONTEXT | JASH-008/010; explicit immutable correction witness and fallback-availability preflight | Adopt or replace the explicit contextPredicateProposal: selected source-bound validated witness is known, independently of orbit existence; availability derives from actual eligible validated registry contents. |
| ADOPT-REGISTRY | AEOSTARA-DEMO-FALLBACK-V1 | Adopt actual domain entries and reserve IDs; canonical rank/policyID order itself already source-derived. |
| ADOPT-INVERSE | JASH-011/012/013/014; JASH-DEMO-CARRIER-INVERSE-V1 | Adopt allowed target authority, exact Pointer scope, witnesses, policy/invariant/backup/stale/verification/restore guarantees and host capabilities. |
| ADOPT-JSON | JASH-015; JASH-CANON-EXACT-DECIMAL-V1 | Adopt exact precision/limits and reconcile reference numeric, duplicate, Unicode and Pointer behavior with immutable supplementary vectors. |
| ADOPT-ERRORS | JASH-006/018; ERRATUM-OUTER-FAILURE or VERSIONED-RESULT-UNION | Adopt lawful failure channel and per-operation negative DTO semantics/exports; fixed schemas unchanged. |
| ADOPT-FIXTURE-ERRATA | JASH-017 plus separate proposed fixtures/traceability | Supply complete authoritative distinguishing inputs/lifecycle traces or corrected expectation errata, preserving original frozen bytes and identification. |

A separate reviewer assesses source fidelity, complete-input determinism, schema compatibility, fixture consistency and recovery safety. The **REVIEW-READY PROPOSAL — NOT ADOPTED** readiness label identifies this prepared artifact; the separate hash-bound review records actual findings/disposition and controls whether revisions remain. Independent review cannot itself amend either source pin or establish the missing catalog/profile/inverse/error semantics.

Continue authorized independent build/package/diagnostic work and tests of already-defined canonical rules. Do not freeze a dependent public contract, implement this synthetic domain as production behavior or qualify the SDK release while the disposition remains unadopted.
