# Aeostara — Windows Platform Target

## Status: Reference Implementation (v0.1.0)

This branch contains the full Windows native implementation of Aeostara.

## Requirements

- Windows 10/11
- Visual Studio 2022 (MSVC C++20)
- CMake 3.28+
- vcpkg (for nlohmann/json)

## Build

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## Architecture

```
AeostaraCLI (exe) → AeostaraCore (static lib) → nlohmann/json (vcpkg)
```

- **AeostaraCore**: 18 headers, 15 source files, 11 contracts, 5 interfaces
- **AeostaraCLI**: CommandParser, CLIRunner, Main.cpp
- **Tests**: 14 CppUnitTest files + TestHelpers.h with MockFileSystem

## Compliance

- C++20, /W4 /WX (warnings as errors)
- JSON-only configuration scope
- No Python in product source
- No YAML in product source
- All concrete types `final`
- Namespace: `Aeostara`
- Copyright: James Daley

## Verification

```powershell
powershell -ExecutionPolicy Bypass -File Scripts/verify-aeostara-guardrails.ps1
powershell -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1
```
