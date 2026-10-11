# Aeostara Windows SDK

Aeostara Windows is a native C++20 realization of the pinned Aeostara base and ASH semantics. The SDK is the `AeostaraCore` static library, curated public headers, relocatable `Aeostara::Core` CMake target and `Aeostara.Windows.SDK` native NuGet package. The CLI remains a development consumer.

SDK version is defined once in `SDK_VERSION`. This release candidate qualifies Windows x64, Visual Studio 2022 v143, MSVC 19.44.35223 (VCToolsVersion 14.44.35207), Windows SDK 10.0.26100.0 and C++20. Debug uses /MDd; Release uses /MD. Both installed configurations contain the lean diagnostic backend. The source development profile uses the development collector. The SDK has owned strict JSON and no external runtime dependency package; `bcrypt` and `ole32` are Windows system dependencies.

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
cmake --preset release
cmake --build --preset release
ctest --preset release
```

For installed SDK builds, use `sdk`, `sdk-debug` and `sdk-release` presets and run both SDK test presets before installation. The [build guide](docs/windows/BUILD_AND_TEST.md) documents configuration and test evidence. The [SDK guide](docs/windows/SDK_GUIDE.md) and [public client](examples/sdk-client/README.md) describe CMake and native NuGet integration, migration and recovery limits.

The upstream pins remain Aeostara `774b981ef2267a41c1b1a59497b2b0746a86f32b` and ASH `e123f5d7fdbb381179971f721a3292c31eb1cbc2`. The owner explicitly [adopted the reviewed supplemental contract](docs/semantics/ADOPTION.md), SHA-256 `98539d83ad6199cecc26eb7b9fcb92993073a2e511ebf70191330cd46ba35dc7`. It supplies opt-in domain profiles and concrete recovery realization while preserving platform-neutral ASH meanings. Projection and planning own no writer. Missing evidence uses truthful typed failures. Ordinary observed/desired JSON differences grant no mutation permission.

The original arbitrary-difference healing path is replaced by complete semantic requests, actual policy evaluation, measured target identity/version, checked backup, protected execution and independent verification. Failed healing remains failed after verified restoration. Diagnostic exports provide bounded redacted JSON/Markdown evidence with explicit unavailable causes and partial-export failure status. Representative target-machine storage, ACL and crash behavior remains deployment validation.

This project is licensed under [Apache License, Version 2.0](LICENSE). Copyright 2026 James Daley. See [third-party notices](THIRD_PARTY_NOTICES.md) and [contribution policy](CONTRIBUTING.md). Outside contributions are not accepted.
