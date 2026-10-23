# XEX TLS and scoped import admission

These independently authored XEX2/PE32 fixtures exercise the actual SDK loader,
XThread and export shims. They contain no proprietary input or title instructions.
The new integration does not dispatch any PPC entry point. The unchanged Phase 11
regression separately regenerates and executes its original nine-function module.

Build one coherent SDK with [tls-import-lock.json](../../tools/sdk/tls-import-lock.json)
and `--interposable-runtime` in a private workspace. Run each matching profile:

```sh
python3 tests/tls-import/run.py --sdk "$PHASE13/repro/staged/Debug" --source "$PHASE13/repro/sdk" --workdir "$PHASE13/tls-Debug"
python3 tests/tls-import/run.py --sdk "$PHASE13/repro/staged/Release" --source "$PHASE13/repro/sdk" --workdir "$PHASE13/tls-Release"
python3 tests/tls-import/run.py --sdk "$PHASE13/repro/staged/Sanitized" --source "$PHASE13/repro/sdk" --workdir "$PHASE13/tls-Sanitized" --sanitize
```

Every build/execution child has an external 30-second process-group watchdog.
SDK allocation/native-create failure injection is confined to six selected child
processes. Ordinary tests have no DYLD override. Outputs, runtimes, fixtures,
allocation addresses and diagnostics stay outside this checkout. Never reuse an
existing output directory; preserve failed runs separately.

Thirteen groups cover 13 malformed TLS layouts; BSS-only metadata; two modules,
four XThreads, eight repeated cycles; creation rollback; all 33 imports with 27
denied under all three supported signature shapes; direct/indirect admitted calls;
unknown/mismatched contexts and metadata; actual title-process TLS bitmap,
exhaustion/free and dispatch shutdown. See [evidence and supported scope](../../docs/research/xex-tls-import-admission.md).

The SDK source census checks all 33 identities and fixed declarations. Public CI
runs `python3 tests/tls-import/audit.py --matrix-only` without an SDK; that mode
checks catalog/policy coherence only. Neither mode certifies the Xbox contracts.
`runtime/import_admission.h` is an opt-in host adapter; old generated imports do
not automatically use it. Pointer imports, process-wide slot allocation/free,
waits, fatal operations, stubs and unverified signatures remain denied there.

All Phase 5–12 regression recipes still apply with matching Phase 13 SDK profiles.
Typed-vector argument, return and mixed signatures must continue failing their
existing negative compilation tests. Never run a private game-code loader or
pass game paths to these fixtures.

Phase 14 adds three explicit capability-scoped pointer overloads to the adapter.
This historical suite still exercises the six scalar signatures and 27 refusals
under scalar-only shapes; it does not admit pointer calls implicitly. The current
matrix audit counts six scalar, three pointer and 24 denied contracts. See the
[Phase 14 pointer tests](../pointer-import/README.md) for the separate scope.
