# Aeostara examples

[sdk-client](sdk-client/README.md) is the installed-package example for this SDK release. It exercises a permitted recovery, a policy rejection without mutation, and a forced verification failure with verified restoration using complete adopted semantic inputs and public native interfaces.

The older `basic-validate`, `drift-and-heal`, and `policy-blocked` directories contain ordinary configuration and legacy invariant examples. They remain useful for informational validation and difference reporting with the source development CLI. Their desired JSON values do not supply semantic witnesses, lifecycle context, or policy authorization. The current default CLI rejects recovery for these examples.

```powershell
aeostara validate examples/basic-validate/config.json --desired examples/basic-validate/desired.json
aeostara diff examples/drift-and-heal/config.json --desired examples/drift-and-heal/desired.json --invariants examples/drift-and-heal/invariants.json
```

Use the SDK client instructions for complete recovery integration and checked failure records to distinguish blocked planning, failed verification, and measured restoration.
