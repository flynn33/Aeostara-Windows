# Aeostara Windows Release Checklist — v0.1.0

## Prerequisites
- [ ] Visual Studio 2022 with C++ Desktop workload installed
- [ ] CMake 3.28+ available (bundled with VS2022 or standalone)
- [ ] vcpkg installed and `VCPKG_ROOT` set
- [ ] Repository cloned and `platform/windows` branch checked out

## Configure and Build
```powershell
cmake --preset release
cmake --build --preset release
```
- [ ] Configure completes without errors
- [ ] Build completes with zero warnings (/W4 /WX)
- [ ] `aeostara.exe` produced at `build/release/src/AeostaraCLI/Release/aeostara.exe`

## Tests
```powershell
cmake --preset debug
cmake --build --preset debug
cmake --build --preset debug --target RUN_TESTS
```
- [ ] AeostaraCoreTests: PASS
- [ ] AeostaraArchitectureTests: PASS
- [ ] AeostaraCLITests: PASS
- [ ] 100% tests passed, 0 failures

## Guardrails
```powershell
powershell -ExecutionPolicy Bypass -File Scripts/verify-aeostara-guardrails.ps1
powershell -ExecutionPolicy Bypass -File Scripts/check-architecture.ps1
```
- [ ] All guardrail checks pass
- [ ] Architecture check passes (5/5)

## Acceptance Scenarios
- [ ] Valid config, no drift → exit 0
- [ ] Invalid JSON → exit 2
- [ ] Policy blocked → exit 1, no mutation
- [ ] Repairable config → exit 0, config healed, backup created
- [ ] Verification failure → automatic rollback

## Release Artifact
- [ ] Release build produced (`cmake --preset release`)
- [ ] Artifact layout assembled (see RELEASE_ARTIFACT.md)
- [ ] Examples included and runnable
- [ ] Operator guide included
- [ ] License included

## Release Decision
- [ ] All checks above pass
- [ ] No blocking issues remain
- [ ] Version consistent across CMakeLists.txt, vcpkg.json, CHANGELOG.md
- [ ] PLATFORM_MANIFEST.json status updated to `release-candidate`
