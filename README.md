# Aeostara (Windows Native Realization)

**Downstream ASH-based healing platform** - Windows C++20 implementation

This branch is the Windows native realization of Aeostara. Semantic authority is upstream in ASH; this branch implements execution mechanics and native runtime behavior on Windows.

Copyright (c) 2026 James Daley. All Rights Reserved.
Proprietary and Confidential.

## Branch Role

- Semantic authority: ASH upstream + Aeostara downstream conformance specs from `main`
- Branch responsibility: native Windows implementation of those semantics
- Conflict rule: semantics align to `main`/ASH; Windows branch updates implementation details only

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

## Branch Alignment

This branch is validated against `platform_windows` profile contract:

- `branch_profiles/platform_windows.profile.json`
- `python3 ci/branch_alignment_checker.py . --profile platform_windows`

## Release Documentation

- [Build and Test Guide](docs/windows/BUILD_AND_TEST.md)
- [CLI Operator Guide](docs/windows/CLI_OPERATOR_GUIDE.md)
- [Release Checklist](docs/windows/RELEASE_CHECKLIST.md)
- [Release Artifact Spec](docs/windows/RELEASE_ARTIFACT_SPEC.md)
- [Troubleshooting](docs/windows/TROUBLESHOOTING.md)

## License

Proprietary. All rights reserved. See [LICENSE.md](LICENSE.md).
