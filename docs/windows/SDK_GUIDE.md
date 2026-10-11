# Aeostara Windows SDK

The SDK is the `AeostaraCore` static library with curated public headers, relocatable CMake metadata and a native NuGet package. The development CLI remains a source regression consumer. SDK assets contain the lean diagnostic backend in both Debug and Release.

The qualified tuple is Windows x64, Visual Studio 2022 v143, MSVC 19.44.35223, VCToolsVersion 14.44.35207, Windows SDK 10.0.26100.0 and C++20. Use /MDd with Debug or /MD with Release, exceptions, RTTI and the corresponding iterator debug setting. Embedded /Z7 producer debug information avoids a required external compiler PDB. Cross-toolset, x86, ARM64, /MT and mixed Debug/Release use are rejected. This candidate does not promise a compiler-independent C++ ABI.

`find_package(Aeostara CONFIG REQUIRED)` provides `Aeostara::Core`. Set `CMAKE_PREFIX_PATH` to the extracted SDK root. The native NuGet package is `Aeostara.Windows.SDK`; its `build/native` props and targets supply relative include/library paths and validate compatibility. The package has no runtime dependency package. Windows `bcrypt` and `ole32` are transitive system libraries. The host supplies the matching Microsoft runtime and toolchain.

The [installed client](../../examples/sdk-client/README.md) demonstrates both integration routes using public headers and synthetic data. It exercises an authorized repair, rejection before configuration mutation, truthful verified restoration after failure, and paired incident export. Extract the developer kit beside the SDK, or restore the `.nupkg` from a local folder feed. Do not add the producer source checkout or a dependency cache to client search paths.

## Semantic boundary and migration

The [adopted supplement](../semantics/ADOPTION.md) binds the two immutable upstream authorities. `SemanticEnvironment::adoptedDirect()` and `adoptedClaims()` select the explicit adopted domain bindings. A complete semantic input supplies snapshots and hashes, profile/revision, evidence, lifecycle context, recognized-state and fallback registries, invariant and policy identities, and any selected validated witness and inverse rule. Projection, diagnosis and planning own no writer.

The pre-SDK public JSON type is replaced by the owned `Aeostara::Json` value. Serialization functions use that value, strict bounded parsing, exact decimal normalization and canonical ordering. Hosts must migrate third-party JSON values at their own boundary. The SDK neither distributes nor requires the old JSON dependency.

Generic `validate`/`diff` retain structural comparison and invariant reporting. Generic configuration differences alone cannot authorize healing. `healSemantic` requires a complete immutable semantic request and an explicitly selected environment. Legacy CLI healing of ordinary desired JSON therefore fails closed. A semantic request carrier may be used by source callers; the installed client calls the explicit APIs directly.

## Recovery and evidence

Allowed configuration execution requires an independently computed source-bound plan, exact policy authorization, actual target identity/version, an exclusive target lease and a genuinely captured/read-back backup. Verification checks exact permitted postconditions, same-profile nine-coordinate projection, validity/classification and mandatory invariants. A failure after a possible write arms restoration; restored original bytes do not imply successful healing. Primary and secondary failures, mutation state, measured hashes and diagnostic persistence remain separate facts.

Successful ASH provider results expose checked diagnostic references. Resolve them through `IAshBindingProvider::resolveDiagnosticChain`; the provider retains owned, validated chains up to 64 chains and 16 MiB and rejects new chains when that budget is exhausted. Previously returned successful references remain resolvable. A complete generation plan carries its helper and generation chains, so `planArtifactEmission` validates and consumes that plan without rerunning generation or relying on the generating provider's constructor context. Emission planning performs no file writes.

The default target writer protects a held file handle and performs checked in-place writes. It does not claim atomic replacement or crash durability. Scoped atomic publication is a separate native capability with explicit authorization and checked staging/publication; its documented limitations apply. Host adapters must implement their advertised capabilities truthfully. Missing capabilities reject the dependent operation.

Diagnostics retain bounded structured evidence. Lean defaults are 64 events, 4 KiB per event, 256 KiB memory, one-hour retention and a 2 MiB paired export limit. Raw configuration strings and unstructured exception details are excluded from exported evidence. Known counters and unavailable causes are explicit. Export redacts a consistent copy, writes a new staging directory, verifies both files and publishes the complete pair. An incomplete export reports its checked partial locations and actual failure. Hosts own completed bundle retention and access control.

Developer collection is best effort. Collector loss is reported through bounded diagnostic errors and measured health, without changing an otherwise completed business operation. Required canonical diagnostic chains and durable business audit retain their execution gates. Collection loss during a failed operation remains secondary evidence and does not replace its primary failure or restoration outcome.

This release candidate qualifies the assigned host and synthetic/adversarial cases recorded in its manifest. Representative deployment hardware, storage/ACL behavior and crash/reboot recovery remain owner validation. No signing identity is required or asserted for the ordinary static package.
