# Aeostara Windows SDK troubleshooting

Use [BUILD_AND_TEST.md](BUILD_AND_TEST.md) for exact build commands and [SDK_GUIDE.md](SDK_GUIDE.md) for the qualified compiler, runtime, ownership, and recovery boundaries.

| Observed failure | Check |
|---|---|
| CMake rejects the SDK compiler or platform | Use Windows x64, v143, MSVC 19.44.35223.0, and the documented Windows SDK. Other tuples are not qualified by this release. |
| Header rejects runtime or configuration | Match Debug with `/MDd` or Release with `/MD`; keep exceptions and RTTI enabled and use C++20. |
| `LNK2038` reports a mismatch | Check configuration, CRT, compiler, and SDK identity in every linked object. Preserve the package checks. |
| An installed header cannot be found | Link `Aeostara::Core` through the installed CMake package or import the native NuGet props/targets; inspect the actual package payload. |
| CppUnitTest DLL cannot load | Source tests require Visual Studio's native C++ test framework and runner. This framework is excluded from installed SDK payloads. |
| CTest cannot select a configuration | Run the documented test preset or specify `-C Debug`/`-C Release` for a Visual Studio build. |
| Planning or recovery returns `BLOCKED` | Check complete adopted inputs, revisions, witnesses, lifecycle context, and the explicit environment. Differences and recovery categories alone do not grant mutation permission. |
| Recovery returns `STALE_STATE` | Recapture the actual target identity, version, and snapshots; prepare a fresh authorized request. |
| Audit or backup persistence fails | Inspect the owning phase and measured native error. Required business audit and verified backup failures prevent configuration mutation. |
| Verification or restoration fails | Inspect primary and secondary errors, mutation state, verified raw hashes, and the preserved backup. Restoration requires successful readback. |
| Diagnostic export is incomplete | Inspect measured paths and native errors; retry into a new destination. An incomplete pair is not a completed export. |

The SDK uses its owned JSON implementation and Windows system libraries `bcrypt` and `ole32`. It does not require vcpkg, an upstream executable, an external schema corpus, or a runtime fixture directory. Frozen schemas are enforced by native checked DTO boundaries. Test vectors remain source test assets.

Guardrails enforce the Core-to-platform dependency boundary. Forsetti is not selected for this release. The package contains the lean diagnostic backend; development retention is built only for source development verification.

Qualification records describe execution on the actual build host. Representative deployment on the owner's physical target remains a separate acceptance activity. Static SDK archives are unsigned; this release does not assert executable signing, hardware power-loss testing, or cross-process compare-and-swap guarantees.
