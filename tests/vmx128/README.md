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

Phase 5 additionally tests the exact alias-only [SDK patch](../../patches/rexglue-sdk/README.md). Use its privately patched `vector.cpp` and add `--alias-safe` to the existing commands. The runner requires patched hash `ad512025255671a98c7501d3881ab95923659d0eca1ca3ebacc45d97c407e890` and requires zero aliased-vector differences. The numeric oracle differences remain characterization results; no numeric/rounding correction is made.

`compare_evidence.py` separately compiles the actual `float_to_xenos_half` helper from Xenia Canary commit `c28af1125d7a50c1cdfc4ba0c7e6c1df5b00f295` (`src/xenia/base/math.h`, checked SHA-256 `2e61856a7901842d494ddef4f0bc09f1fd246533d3ae56d205b179d9d5be9a0e`) and compares 13 original inputs with the synthetic SDK subject:

```sh
python3 tests/vmx128/compare_evidence.py --canary-math "$CANARY_MATH_HEADER" \
  --sdk-subject "$PRIVATE_VMX_OUTPUT/subject" --workdir "$PRIVATE_TEST_OUTPUT/emulator-evidence"
```

No Canary implementation is vendored. The local extracted helper remains SDK-derived licensed material outside Git. Results are emulator evidence, never a hardware oracle. [Phase 5 findings](../../docs/research/runtime-foundation.md).
