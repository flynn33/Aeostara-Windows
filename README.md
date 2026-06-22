# Aeostara (Windows Native Realization)

**Downstream ASH-based healing platform** - Windows C++20 implementation

This repository is the Windows native realization of Aeostara. Semantic authority is upstream in ASH and the pinned Aeostara base release; this repository implements execution mechanics and native runtime behavior on Windows.

Copyright (c) 2026 James Daley. All Rights Reserved.
Proprietary and Confidential.

## Branch Role

- Semantic authority: ASH upstream and `flynn33/aeostara` release `v1.0.0`
- Branch responsibility: native Windows implementation of those semantics
- Conflict rule: semantics align to the pinned Aeostara release and ASH; Windows changes update implementation details only

## Native Stack

- C++20
- MSVC (Visual Studio 2022)
- CMake 3.28+
- vcpkg + `nlohmann/json`

## Build

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

## Commands

```text
aeostara validate <config> --desired <desired> [--invariants <invariants>]
aeostara diff    <config> --desired <desired> [--invariants <invariants>]
aeostara heal    <config> --desired <desired> [--invariants <invariants>] [--audit <audit.jsonl>]
```

## Execution Guarantees

- Deterministic execution
- Policy gating before mutation
- Backup before mutation
- Verification after execution
- Rollback/escalation on verification failure
- Audit trail for all decision-critical actions

## Base Pin and Migration

- [Aeostara baseline reference](AEOSTARA_BASELINE_REFERENCE.md)
- [Migration provenance](MIGRATION_PROVENANCE.md)

## Release Documentation

- [Build and Test Guide](docs/windows/BUILD_AND_TEST.md)
- [CLI Operator Guide](docs/windows/CLI_OPERATOR_GUIDE.md)
- [Release Checklist](docs/windows/RELEASE_CHECKLIST.md)
- [Release Artifact Spec](docs/windows/RELEASE_ARTIFACT_SPEC.md)
- [Troubleshooting](docs/windows/TROUBLESHOOTING.md)

## License

Proprietary. All rights reserved. See [LICENSE.md](LICENSE.md).
