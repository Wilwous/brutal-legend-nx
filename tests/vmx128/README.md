# Isolated VMX128 tests

These original GPL-3.0-only tests consume only the pinned open-source SDK builder and fmt headers. They never read game files, invoke the analyzer, link the runtime or compile a game translation unit.

Required: Python 3.9+, a C++20 Clang compiler, the existing ReXGlue 0.10.0 include directory, and `src/codegen/builders/vector.cpp` from SDK commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. The runner verifies the source SHA-256 before extracting the case. Obtain that public source from the [pinned SDK](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/builders/vector.cpp); no package installation is necessary with the existing local SDK.

Set `$SDK_SOURCE`, `$SDK_INCLUDE` and `$PRIVATE_TEST_OUTPUT` to local paths; output must be outside this checkout:

```sh
python3 tests/vmx128/run.py --sdk-source "$SDK_SOURCE" --sdk-include "$SDK_INCLUDE" \
  --workdir "$PRIVATE_TEST_OUTPUT/O0" --optimization 0
python3 tests/vmx128/run.py --sdk-source "$SDK_SOURCE" --sdk-include "$SDK_INCLUDE" \
  --workdir "$PRIVATE_TEST_OUTPUT/O2" --optimization 2
python3 tests/vmx128/run.py --sdk-source "$SDK_SOURCE" --sdk-include "$SDK_INCLUDE" \
  --workdir "$PRIVATE_TEST_OUTPUT/sanitized" --optimization 0 --sanitize
```

Each output directory contains the extracted BSD SDK case, synthetic non-game subject, executable and `results.json`. SDK attribution and license conditions continue to apply to any redistribution of those SDK-derived artifacts; they are not committed here. Only the original adapter/oracle are public. The runner compiles the actual emitter case through a minimal context, then compiles and executes its synthetic output. The independent oracle enumerates mathematically defined half values and searches numerically, rather than repeating the emitter algorithm.

`PASS characterization; hardware equivalence UNKNOWN` means the finite in-range/mask/lane checks passed and known special-value/alias gaps reproduced. It does **not** mean the SDK agrees with the IEEE oracle in every category or with Xbox hardware. See [methodology, exact counts, expected/observed results and limitations](../../docs/research/vmx128-validation.md).

The fixture requires a little-endian host. It tests C++20 at O0/O2 and optionally ASan/UBSan; FPSCR/flush-mode integration, guest traps and hardware behavior are outside the adapter. Each subprocess times out after 30 seconds. Failures include compiler stderr or assertion diagnostics. Public-tree CI checks publication boundaries only; it does not fetch SDK dependencies or claim these tests ran on a different compiler/platform.
