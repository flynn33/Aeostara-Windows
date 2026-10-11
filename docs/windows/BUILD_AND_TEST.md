# Build and test the Windows SDK

Install Visual Studio 2022 with native C++ development and test tools, qualified v143 MSVC 19.44.35223 (VCToolsVersion 14.44.35207), Windows SDK 10.0.26100.0, and CMake 3.28 or later. The qualified release host used CMake/CTest 4.4.2 and VSTest 17.14. A standalone native NuGet CLI is needed only for package construction and local-folder restore. No vcpkg dependency is fetched.

`debug` and `release` configure the shared source development build with tests enabled and the DEVELOPMENT diagnostic profile. Build/test presets select the requested configuration explicitly:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
cmake --build --preset release
ctest --preset release
```

`sdk` configures the separate lean installation build. Both distributed configurations use the LEAN backend:

```powershell
cmake --preset sdk
cmake --build --preset sdk-debug
ctest --preset sdk-debug
cmake --build --preset sdk-release
ctest --preset sdk-release
cmake --install build/sdk --config Debug --prefix C:/SDK/Aeostara
cmake --install build/sdk --config Release --prefix C:/SDK/Aeostara
```

The producer requires Windows x64 and the exact qualified compiler. /W4 /WX, C++20, /EHsc, /GR and /Z7 are explicit. /MDd and /MD are selected by configuration. Compiler-independent ABI, alternate toolsets and mixed CRT/configuration use are unsupported and rejected by package/header checks.

Native Visual Studio CppUnitTest suites cover Core, architecture and CLI. CTest invokes VSTest with configuration-specific TRX results under `TestResults/Debug` and `TestResults/Release`. Additional native executables evaluate all adopted semantic vectors from input-only producer results, controlled lifecycle failures and diagnostic behavior. Expected values enter only after evaluation. Do not infer semantic correctness from a successful schema comparison or build.

Run the repository guardrails after changes:

```powershell
pwsh -File Scripts/verify-aeostara-guardrails.ps1
pwsh -File Scripts/check-architecture.ps1
```

The [SDK client](../../examples/sdk-client/README.md) builds against installed public headers and libraries through CMake or native NuGet. Release qualification also checks moved package paths with spaces/Unicode, source-unavailable clients, incompatible consumers, every public header in isolation, archive object composition, repeated staging and a second clean compiler build. Actual results belong in the release verification record; these commands are instructions, not a statement that an unexecuted gate passed.

`Scripts/package-sdk.ps1` creates the five prerelease assets from a lean Debug/Release build and an existing NuGet CLI. A dirty intermediate build requires `-AllowDirtyCandidate` and is labeled ineligible for publication. Publication staging requires a clean source and an actual exact-source verification record. The script uses explicit file allowlists, stable ZIP ordering/timestamps and content hashes. It does not publish or create a tag.
