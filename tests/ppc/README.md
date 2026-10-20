# Original PPC and host-fault fixtures

These redistributable GPL-3.0-only fixtures contain no game code or assets. They call actual ReXGlue v0.10.0 SDK exports and real Memory/XThread implementations. Locally copied BSD SDK code, objects, logs and outputs must stay outside the checkout. [Results and precise limits](../../docs/research/ppc-execution.md).

Set `PROJECT` to this checkout, `SDK_SOURCE` to an unmodified SDK source tree at commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, `SDK_BINARY` to the matching SDK directory containing `include/` and `lib/`, and `PRIVATE_WORK` to a private output parent outside Git. Each run directory must be new. No game input argument is accepted.

```sh
python3 "$PROJECT/tests/ppc/test_watchdog.py"
python3 "$PROJECT/tests/ppc/run.py" --workdir "$PRIVATE_WORK/ppc-O0" --sdk-source-root "$SDK_SOURCE" --sdk-binary-root "$SDK_BINARY" --faults
python3 "$PROJECT/tests/ppc/run.py" --workdir "$PRIVATE_WORK/ppc-O2" --sdk-source-root "$SDK_SOURCE" --sdk-binary-root "$SDK_BINARY" --optimization 2 --faults
python3 "$PROJECT/tests/ppc/run.py" --workdir "$PRIVATE_WORK/ppc-sanitized" --sdk-source-root "$SDK_SOURCE" --sdk-binary-root "$SDK_BINARY" --sanitize
```

Each build/test child has an external monotonic watchdog capped at 30 seconds and a separate process group. Timeout kills the whole group. Explicit exit codes, RSS, CPU and duration are recorded in private JSON; stdout/stderr are continuously drained and capped at 64 KiB retained per stream. Deliberate fault children disable core dumps only, preserve page protection, and never run on the parent thread.

`--faults` runs finite isolated fault/recovery regressions. To reproduce the two original SDK retry loops **once**, explicitly opt into:

```sh
python3 "$PROJECT/tests/ppc/run.py" --workdir "$PRIVATE_WORK/ppc-original-fault-reproduction" --sdk-source-root "$SDK_SOURCE" --sdk-binary-root "$SDK_BINARY" --reproduce-timeouts
```

Those two children are expected to time out at 30 seconds; their `outcome` remains `timeout` even when the diagnostic expectation passes. Do not repeat them merely because they timed out. Native macOS ARM64 was tested; Linux SDK integration and other hosts are not claimed. The supervisor-only tests run without the SDK and are included in CI.

The runner verifies source hashes, checks/applies both SDK patches in its private copy, compiles real export units, and extracts the exact patched XThread TLS method bodies against the real SDK class. This keeps one prebuilt Create/current-thread binding. It does not substitute the Phase 5 fake backend. Shim ownership is checked with `dladdr`; a full patched SDK library/its internal call closure is a later milestone. The prebuilt runtime remains unsanitized. Intentional signal tests are built without sanitizer instrumentation. No complete Xbox ABI, title TOC, guest unwind, game boot or Switch compatibility is established.
