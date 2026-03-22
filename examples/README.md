# Aeostara Examples

Sample configurations demonstrating Aeostara's three core workflows.

## basic-validate

A config that matches the desired state exactly. Running `validate` produces exit code 0 (no drift).

```powershell
aeostara validate examples/basic-validate/config.json --desired examples/basic-validate/desired.json
```

## drift-and-heal

A config with drift from the desired state. Running `diff` shows the repair plan; running `heal` applies it.

```powershell
# Show drift and repair plan
aeostara diff examples/drift-and-heal/config.json --desired examples/drift-and-heal/desired.json --invariants examples/drift-and-heal/invariants.json

# Apply healing with audit trail
aeostara heal examples/drift-and-heal/config.json --desired examples/drift-and-heal/desired.json --invariants examples/drift-and-heal/invariants.json --audit audit.jsonl
```

## policy-blocked

A config that violates critical invariants. Aeostara detects the violations but the policy blocks auto-remediation because `ssl_enabled` is `false` (critical severity).

```powershell
aeostara heal examples/policy-blocked/config.json --desired examples/policy-blocked/desired.json --invariants examples/policy-blocked/invariants.json
# Exit 1: policy blocked
```
