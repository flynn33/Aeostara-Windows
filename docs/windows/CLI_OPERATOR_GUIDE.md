# Aeostara Windows development CLI

The published SDK contains a native static library, headers, package metadata, and a separate developer kit. The source-built `aeostara.exe` remains a development CLI. See [SDK_GUIDE.md](SDK_GUIDE.md) for the supported application integration and [BUILD_AND_TEST.md](BUILD_AND_TEST.md) for source builds.

```powershell
git clone https://github.com/flynn33/Aeostara-Windows.git
cd Aeostara-Windows
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

The executable is `build/development/src/AeostaraCLI/Debug/aeostara.exe`. Its version comes from `SDK_VERSION`.

```text
aeostara validate <config> --desired <desired> [--invariants <invariants>]
aeostara diff <config> --desired <desired> [--invariants <invariants>]
aeostara heal <config> --desired <desired> [--invariants <invariants>] [--audit <audit.jsonl>]
```

`validate` checks captured JSON differences and supplied legacy invariants. `diff` reports informational differences and any checked planning failure. An ordinary desired JSON document does not authorize mutation. The default CLI composition has no explicitly bound semantic environment, so `heal` rejects recovery. The SDK client in [examples/sdk-client](../../examples/sdk-client/README.md) demonstrates complete adopted semantic inputs, explicit environment binding, native target capture, policy authorization, backup, verification, and measured restoration.

Output is JSON on stdout. Checked failures include an outer `failure` record with the actual phase, code, mutation state, and available diagnostic evidence. This failure record remains distinct from a valid projected ASH state. The CLI returns:

| Code | Meaning |
|---|---|
| 0 | Completed success or successful validation without differences |
| 1 | Differences, blocked or ambiguous planning, rejected policy, stale input, or verification failure |
| 2 | Invalid input, storage failure, restoration failure, or unexpected error |

An empty difference list does not turn a blocked plan into success. A restore receipt is successful only after the original bytes were independently reread and verified. Inspect the actual failure and mutation state before deciding whether intervention is needed.

The optional `--audit` argument names business audit storage; it does not grant configuration mutation authority. Developer diagnostics and explicit JSON/Markdown exports are separate SDK facilities. No CLI executable is shipped in the SDK packages.
