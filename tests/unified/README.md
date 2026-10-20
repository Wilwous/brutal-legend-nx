# Original unified SDK fixtures

All fixtures are original redistributable test logic. They contain no game functions, instructions or assets. The main matrix uses empty game directories and the [isolated rebuilt SDK](../../tools/sdk/README.md), with a 30-second external child/process-group watchdog for every compile/run. Output paths must be private and new. [Observed results, known failures and supported contracts](../../docs/research/unified-sdk.md).

After the documented SDK build, from the project root:

```sh
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/unified/staged/Debug" --source "$PRIVATE_WORK/unified/sdk" --workdir "$PRIVATE_WORK/tests-Debug"
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/unified/staged/Release" --source "$PRIVATE_WORK/unified/sdk" --workdir "$PRIVATE_WORK/tests-Release"
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/unified/staged/Sanitized" --source "$PRIVATE_WORK/unified/sdk" --workdir "$PRIVATE_WORK/tests-Sanitized" --sanitize
python3 tests/ppc/test_watchdog.py
```

Each main profile reports 33 passing groups, 11/11/10 fault checks and six separately classified BLOCKED observations. The negative typed-vector compile, writable unallocated hole, allocation false-success and exception stubs are **not passing runtime features**. An unexpected exit/timeout or assertion fails the driver. `results.json` preserves detailed process metrics and blockers; private stdout/stderr and per-child JSON contain fault records and CPU/RSS/time diagnostics. Compile commands/source flags are explicit in the runner. Shader/game codegen is never invoked.

- `abi.cpp`: independent arithmetic/byte/stack oracles, selected GPR/FPR/raw-vector context, callbacks/r2/r13 and restoration.
- `faults.cpp`: actual SDK Memory/MMIO/dispatcher; legitimate decoded 32-bit MMIO and protected/unhandled/reentrant faults.
- `errors.cpp`: real initialization exports, allocation/protection failures, explicit false-success/stub observations.
- `emitter.cpp`: actual rebuilt codegen library, original synthetic VMX operands; emits private original fixtures for alias regressions.
- `vector-negative.cpp`: unsupported typed-vector contract, expected compiler diagnostic.
- Existing Phase 5/6 original components/native-memory/export/PCR/XThread/TLS/fault fixtures run against the same SDK. Standalone components need no SDK. No separately linked replacement export unit or old prebuilt runtime is used.

## Optional private XEX loader only

This extra probe requires the owner's unchanged base `default.xex`, checked by SHA-256. Set `PRIVATE_GAME_ROOT` to its private extracted directory. The helper uses a separate empty updates directory and never calls guest launch/preparation APIs. It does not compile or execute generated game code. Raw loader output must stay private.

```sh
python3 tests/unified/loader.py --sdk "$PRIVATE_WORK/unified/staged/Debug" --game-root "$PRIVATE_GAME_ROOT" --workdir "$PRIVATE_WORK/loader-Debug"
python3 tests/unified/loader.py --sdk "$PRIVATE_WORK/unified/staged/Sanitized" --game-root "$PRIVATE_GAME_ROOT" --workdir "$PRIVATE_WORK/loader-Sanitized" --sanitize
```

Observed Debug result: PASS, 274 imports / zero unresolved. **Observed Sanitized result: failure**, UBSan alignment error in `src/system/util/xdbf_utils.cpp:55:60`, then SIGABRT. Preserve the nonzero result; do not disable alignment checks or classify it as a passing game-loader contract. A successful Debug load does not establish entry readiness or playability.
