# Aeostara Windows — Troubleshooting

## Build Issues

### "VCPKG_ROOT is not defined" or "vcpkg not found"

**Cause**: The `VCPKG_ROOT` environment variable is not set.

**Fix**:
```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```
Or set it permanently:
```powershell
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\path\to\vcpkg", "User")
```

### "CMake Error: could not find CMAKE_PROJECT_VERSION"

**Cause**: CMake version is too old. Aeostara requires CMake 3.28+.

**Fix**: Update CMake or use the version bundled with Visual Studio 2022:
```
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe
```

### Build fails with warnings-as-errors (/W4 /WX)

**Cause**: The project enforces zero-warning builds. Any compiler warning fails the build.

**Fix**: Fix the code that produces the warning. Do not disable `/WX` — this is a compliance requirement.

### "Cannot open include file: 'nlohmann/json.hpp'"

**Cause**: vcpkg did not install nlohmann-json during configure.

**Fix**: Re-run configure to trigger vcpkg install:
```powershell
cmake --preset debug
```
Ensure `vcpkg.json` is present at the repo root (it should contain `nlohmann-json` as a dependency).

## Test Issues

### CppUnitTest DLL fails to load

**Cause**: The test DLL depends on the CppUnitTest framework which is part of Visual Studio.

**Fix**: Ensure Visual Studio 2022 is installed with the "Desktop development with C++" workload. Run tests via the MSBuild target:
```powershell
cmake --build --preset debug --target RUN_TESTS
```

### "Test not available without configuration"

**Cause**: Running ctest directly without specifying the build configuration.

**Fix**: Use the MSBuild target instead:
```powershell
cmake --build --preset debug --target RUN_TESTS
```
Or specify the config:
```powershell
cd build/debug && ctest -C Debug --output-on-failure
```

## Runtime Issues

### Exit Code 2 — Parse Error

**Symptom**: `aeostara validate config.json --desired desired.json` exits with code 2.

**Causes**:
- Config file contains invalid JSON (missing comma, trailing comma, unquoted key)
- File path is wrong or file does not exist
- File encoding is not UTF-8

**Fix**: Validate your JSON with a linter (e.g., `python -m json.tool config.json`) and check the file path.

### Exit Code 2 — File Not Found

**Symptom**: Error message indicates a file cannot be opened.

**Fix**: Check that all paths are correct. Use absolute paths if relative paths fail. Ensure the working directory is correct.

### Exit Code 1 — Policy Blocked

**Symptom**: `aeostara heal` exits with code 1 and reports policy violation.

**Causes**:
- An invariant with `severity: critical` is violated
- The invariant has `auto_remediate: false`
- The violation cannot be resolved by the repair plan

**Fix**: Either fix the config manually to satisfy the invariant, or change the invariant's `auto_remediate` to `true` if the violation is acceptable to auto-fix.

### Exit Code 1 — Verification Failed (Auto-Rollback)

**Symptom**: Heal reports that verification failed after repair and rollback was executed.

**Cause**: The repaired config does not match the desired state after applying all repair actions. This can happen if the repair plan is incomplete (e.g., nested object drift that the planner cannot fully resolve).

**Fix**: Check the audit trail (JSONL) for details. The backup file is preserved alongside the config. Manual intervention may be needed for complex drift scenarios.

## Guardrail Issues

### "Forbidden Forsetti include in domain (AeostaraCore)"

**Cause**: A source file in `include/AeostaraCore/` or `src/AeostaraCore/` contains a `#include <Forsetti...>` directive.

**Fix**: Remove the Forsetti include from the domain layer. Forsetti integration is only permitted at the platform layer (AeostaraCLI). Domain contracts must remain Forsetti-independent.

### "Core depends on CLI"

**Cause**: A file in `include/AeostaraCore/` or `src/AeostaraCore/` includes a header from `AeostaraCLI`.

**Fix**: Remove the reverse dependency. The dependency direction is one-way: CLI → Core → nlohmann/json.

## Environment Notes

- **Minimum OS**: Windows 10 (x64)
- **Required for build**: Visual Studio 2022 + CMake 3.28+ + vcpkg
- **Required to run**: Windows 10/11 only (no build tools needed)
- **Supported shells**: PowerShell, Command Prompt, Windows Terminal
