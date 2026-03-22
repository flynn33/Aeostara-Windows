# Aeostara Windows Release Artifact — v0.1.0

## Artifact Type

Standalone executable with documentation and examples in a zip archive.
No installer required — the operator extracts and runs.

## Artifact Name

`aeostara-windows-v0.1.0.zip`

## Folder Layout

```
aeostara-windows-v0.1.0/
├── aeostara.exe                          # CLI binary (Release build)
├── README.md                             # Quick-start guide
├── LICENSE.md                            # Proprietary license
├── CHANGELOG.md                          # Release notes
├── docs/
│   └── OPERATOR_GUIDE.md                 # Full operator documentation
└── examples/
    ├── README.md                         # Example index
    ├── basic-validate/
    │   ├── config.json                   # Config matching desired state
    │   └── desired.json                  # Desired state
    ├── drift-and-heal/
    │   ├── config.json                   # Config with drift
    │   ├── desired.json                  # Desired state
    │   └── invariants.json               # Policy rules
    └── policy-blocked/
        ├── config.json                   # Config violating critical policy
        ├── desired.json                  # Desired state
        └── invariants.json               # Critical invariant (blocks healing)
```

## What Ships

| Item | Included | Purpose |
|------|----------|---------|
| `aeostara.exe` | Yes | CLI binary |
| `README.md` | Yes | Quick-start |
| `LICENSE.md` | Yes | Legal |
| `CHANGELOG.md` | Yes | Release notes |
| `docs/OPERATOR_GUIDE.md` | Yes | Full documentation |
| `examples/` | Yes | Runnable example scenarios |

## What Does NOT Ship

| Item | Reason |
|------|--------|
| Source code (`src/`, `include/`) | Proprietary — not distributed |
| Tests (`tests/`) | Development only |
| Build files (`CMakeLists.txt`, `CMakePresets.json`) | Build from source only |
| CI workflows (`.github/`) | Repo automation only |
| Guardrail scripts (`Scripts/`) | Development compliance only |
| Specs (`specs/`) | Internal architecture reference |

## Operator Prerequisites

- Windows 10 or Windows 11
- No Visual Studio or build tools required to run the binary
- The executable is self-contained (statically linked)

## Audit Output

When `--audit` is specified, Aeostara writes a JSONL file (one JSON object per line) in the working directory. The default filename is `aeostara-audit.jsonl`. This file is created by the operator's run — it is not included in the release artifact.

## Version Naming

- Version string: `0.1.0`
- Git tag (when created): `v0.1.0-windows`
- Artifact zip: `aeostara-windows-v0.1.0.zip`

## Known Limitations (v0.1.0)

- Windows-only (macOS and iOS targets are in development)
- JSON configuration format only (no YAML, no TOML)
- Single-file healing only (no directory-recursive mode)
- No GUI — CLI only
- No installer — manual extraction from zip
