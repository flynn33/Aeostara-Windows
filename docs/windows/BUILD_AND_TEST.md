# Aeostara Windows — Build and Test Guide

## Prerequisites

| Requirement | Version | Notes |
|-------------|---------|-------|
| Windows | 10 or 11 | x64 |
| Visual Studio | 2022 | C++ Desktop Development workload required |
| CMake | 3.28+ | Bundled with VS2022 or install standalone |
| vcpkg | latest | Set `VCPKG_ROOT` environment variable |

### Setting VCPKG_ROOT

```powershell
# Permanent (user-level)
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\path\to\vcpkg", "User")

# Session-only
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```

## Configure

### Debug (with tests)
```powershell
cmake --preset debug
```
- Generator: Visual Studio 17 2022
- Build type: Debug
- Tests: ON
- vcpkg toolchain: auto-detected from VCPKG_ROOT
- Dependencies: nlohmann-json fetched automatically

### Release (without tests)
```powershell
cmake --preset release
```
- Generator: Visual Studio 17 2022
- Build type: Release
- Tests: OFF

## Build

```powershell
# Debug
cmake --build --preset debug

# Release
cmake --build --preset release
```

### Build Targets
| Target | Type | Output |
|--------|------|--------|
| `AeostaraCore` | Static library | `build/debug/src/AeostaraCore/Debug/AeostaraCore.lib` |
| `aeostara` | Executable | `build/debug/src/AeostaraCLI/Debug/aeostara.exe` |
| `AeostaraCoreTests` | Test DLL | `build/debug/tests/Debug/AeostaraCoreTests.dll` |
| `AeostaraArchitectureTests` | Test DLL | `build/debug/tests/Debug/AeostaraArchitectureTests.dll` |
| `AeostaraCLITests` | Test DLL | `build/debug/tests/Debug/AeostaraCLITests.dll` |

### Compiler Flags
- `/W4` — warning level 4 (high)
- `/WX` — warnings as errors
- `/std:c++20` — C++20 standard

## Test

### Run All Tests
```powershell
cmake --build --preset debug --target RUN_TESTS
```

### Test Suites
| Suite | Files | Coverage |
|-------|-------|----------|
| AeostaraCoreTests | 12 | Contracts, DriftAnalyzer, RepairPlanner, PolicyEvaluator, JsonPath, HealingEngine, Verification, BackupManager, RollbackManager, AuditTrail, JsonConfigAdapter, InvariantParser |
| AeostaraArchitectureTests | 1 | Forsetti isolation, class finality, namespaces, copyright headers |
| AeostaraCLITests | 1 | Command parsing |

### Acceptance Scenarios (HealingEngineTests)
1. Valid config, no drift → validates correctly
2. Invalid JSON → error reported
3. Policy blocked (invariant violation) → no repair
4. Repairable config → full healing flow with backup
5. Verification failure → automatic rollback

### Test Fixtures
Located in `Resources/TestFixtures/`:
- `valid_config.json`
- `desired_state.json`
- `repairable_config.json`
- `policy_blocked_config.json`
- `invalid_config.json`
- `invariants.json`

## Guardrail Verification

### Full Guardrails Check
```powershell
powershell -ExecutionPolicy Bypass -File Scripts/verify-aeostara-guardrails.ps1
```

Checks: Forsetti isolation (domain only), copyright headers, class finality, dependency direction, Python/YAML compliance, required file existence (15 files).

### Fast Architecture Check
```powershell
powershell -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1
```

Checks: Forsetti isolation (domain only), dependency direction, namespace isolation, Python compliance, YAML compliance.

## Expected Results

All of the following should produce zero failures:
```powershell
cmake --preset debug                                           # Configure OK
cmake --build --preset debug                                    # Build OK (0 warnings)
cmake --build --preset debug --target RUN_TESTS                 # 3/3 suites pass
powershell -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1    # PASSED
powershell -ExecutionPolicy Bypass -File Scripts/verify-aeostara-guardrails.ps1  # ALL PASSED
```
