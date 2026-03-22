# Aeostara Operator Guide — Windows

Aeostara is a deterministic JSON configuration drift detection and healing CLI tool.

## Prerequisites

- Windows 10 or Windows 11
- To **run** the prebuilt binary: no additional tools needed
- To **build from source**:
  - Visual Studio 2022 (with C++ Desktop workload)
  - CMake 3.28+ (bundled with VS2022 or standalone)
  - vcpkg (set `VCPKG_ROOT` environment variable)

## Installation from Source

```powershell
# Clone the repository
git clone https://github.com/flynn33/aeostara.git
cd aeostara
git checkout platform/windows

# Configure (downloads nlohmann/json via vcpkg)
cmake --preset debug       # for debug build
cmake --preset release     # for release build

# Build
cmake --build --preset debug
cmake --build --preset release

# The executable is at:
# Debug:   build/debug/src/AeostaraCLI/Debug/aeostara.exe
# Release: build/release/src/AeostaraCLI/Release/aeostara.exe
```

## CLI Reference

### Commands

```
aeostara validate <config> --desired <desired> [--invariants <invariants>]
aeostara diff     <config> --desired <desired> [--invariants <invariants>]
aeostara heal     <config> --desired <desired> [--invariants <invariants>] [--audit <audit.jsonl>]
```

### validate

Parse and validate a config file against a desired state. Reports whether drift or invariant violations exist.

### diff

Show drift events and a proposed deterministic repair plan without modifying any files.

### heal

Execute the full healing flow: back up the config, apply repairs, verify the result, and roll back on failure. Optionally writes an audit trail.

### Options

| Option | Short | Description |
|--------|-------|-------------|
| `<config>` | — | Path to the live configuration JSON file (positional) |
| `--desired` | `-d` | Path to the desired state JSON file (required) |
| `--invariants` | `-i` | Path to the invariant rules JSON file (optional) |
| `--audit` | `-a` | Path for JSONL audit trail output (default: `aeostara-audit.jsonl`) |
| `--help` | `-h` | Show help message |

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success — config is valid, no drift detected, or heal completed |
| 1 | Business logic result — drift detected, policy blocked, or verification failed |
| 2 | Error — invalid JSON, file not found, or unexpected failure |

## Example Workflows

### 1. Validate a Config (No Drift)

```powershell
aeostara validate config.json --desired desired.json
# Exit 0: config matches desired state
```

### 2. Detect Drift

```powershell
aeostara diff config.json --desired desired.json --invariants invariants.json
# Exit 1: drift events listed, repair plan proposed
```

### 3. Heal with Audit Trail

```powershell
aeostara heal config.json --desired desired.json --invariants invariants.json --audit audit.jsonl
# Exit 0: config repaired, backup created, audit written
# Exit 1: policy blocked or verification failed (auto-rollback)
```

## Output Formats

### Repair Plan (JSON)

When `diff` or `heal` runs, Aeostara generates a deterministic repair plan containing:
- `planId` — FNV-1a hash of all repair actions (deterministic, reproducible)
- `actions` — ordered list of field-level repairs with `path`, `from`, `to` values
- `timestamp` — ISO 8601 timestamp

### Audit Trail (JSONL)

The `--audit` option writes one JSON object per line for each healing event:
- `HEALING_STARTED` — session began
- `DRIFT_DETECTED` — individual drift event
- `REPAIR_APPLIED` — field repaired
- `VERIFICATION_PASSED` / `VERIFICATION_FAILED` — post-repair check
- `ROLLBACK_EXECUTED` — automatic rollback on failure
- `HEALING_COMPLETED` — session ended

### Invariant Rules (JSON Array)

Each invariant object contains:
- `invariant_id` — unique identifier (e.g., `INV-001`)
- `expression` — dot-path comparison (e.g., `database.port == 5432`)
- `severity` — `critical`, `high`, `medium`, `low`
- `auto_remediate` — whether Aeostara should auto-fix violations

## Troubleshooting

### Build Failures

**"vcpkg not found" or "VCPKG_ROOT not set"**
Set the `VCPKG_ROOT` environment variable to your vcpkg installation:
```powershell
$env:VCPKG_ROOT = "C:\path\to\vcpkg"
```

**"CMake version too old"**
Ensure CMake 3.28+ is installed. The version bundled with VS2022 is sufficient.

**Warnings as errors (/W4 /WX)**
All warnings are treated as errors. If a build fails due to warnings, the code must be fixed — do not disable /WX.

### Runtime Errors

**Exit code 2 with "Failed to parse config"**
The input JSON file is malformed. Validate it with a JSON linter first.

**Exit code 2 with "File not found"**
Check that all file paths are correct and accessible.

**Exit code 1 with "Policy blocked"**
An invariant with `severity: critical` was violated and `auto_remediate` is false, or the violation cannot be resolved by the repair plan.

### Rollback Behavior

If verification fails after repair, Aeostara automatically restores the original config from backup. The backup file is created alongside the config with a `.backup` extension and timestamp.

## Architecture

```
aeostara.exe (CLI)
  └── AeostaraCore (static library)
        └── nlohmann/json (vcpkg)
```

- **AeostaraCore**: 5 interfaces, 11 contracts, 15-step healing engine
- **aeostara.exe**: CLI shell wiring up the engine via dependency injection
- One-way dependency: CLI → Core → nlohmann/json (no reverse imports)
- All classes `final`, all I/O via interfaces, deterministic by design
