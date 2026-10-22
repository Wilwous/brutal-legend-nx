# Original synthetic execution pipeline

**Current status: Phase 11 synthetic scope PASS; Phase 12 private execution NO-GO.** The original reclamation assertions now pass with sixteen ordered SDK patches. [Ownership model, evidence and remaining limitations](../../docs/research/synthetic-execution.md#xthread-lifetime-root-cause-and-recovery).

No game input, game code or assets. The original [encoder](fixture.py) creates 144 big-endian PPC bytes, relative nm symbols, an assembly listing and a section/relocation sidecar in a **fresh private directory**. Nine complete functions / 36 instructions are processed by the actual pinned CLI `recompile-tests`. Its generated function translation unit and PCH are compiled unchanged with the checked-access opt-in. The generated Catch2 case file is not executed; independent [host oracles](contracts.cpp) exercise the emitted functions through the real patched runtime.

The upstream raw-test path does not ingest the sidecar or load an XEX/ELF. The [original fixture loader](module.h) independently maps code/RO/RW sections, applies its single supported ABS32BE data-pointer relocation and admits declared function entries. This is **end-to-end raw-test generation and synthetic execution**, not end-to-end executable-container loading, normal title launch, arbitrary relocation or instruction fetch. Analysis PCs and runtime virtual addresses are separate coordinates. Code is position independent except for relative direct calls resolved to native functions; LR retains analysis PCs. Runtime indirect targets use explicit loaded-address registrations.

From the repository root, set private absolute paths appropriate to your machine:

```sh
export SDK_WORK="${PRIVATE_WORK:?choose a private directory}/synthetic-sdk"
python3 tools/sdk/build.py --lock tools/sdk/synthetic-lock.json \
  --workspace "$SDK_WORK" --cmake "$TOOLS/cmake" --ninja "$TOOLS/ninja" \
  --source-cache "$PINNED_SDK_CHECKOUT"
# Wait for this single builder to complete all three matching profiles.
for profile in Debug Release Sanitized; do
  flags=()
  cli=rexglue
  if [ "$profile" = Debug ]; then cli=rexglued; fi
  if [ "$profile" = Sanitized ]; then flags+=(--sanitize); fi
  python3 tests/synthetic/run.py --sdk "$SDK_WORK/staged/$profile" \
    --rexglue "$SDK_WORK/sdk/out/mac-arm64/$profile/$cli" \
    --workdir "$PRIVATE_WORK/synthetic-$profile" "${flags[@]}"
done
python3 tests/synthetic/test_fixture.py
```

The loop uses zsh/bash array syntax. SDK tools/dependency prerequisites are in the [build recipe](../../tools/sdk/README.md). Existing locks are historical; Phase 11 requires all sixteen patches. Do not install globally. Each generation, compilation, preprocessing and execution child has the external 30-second process-group watchdog, core suppression, capped output, wait4 timing/RSS and explicit status. The six original execution children are `loading`, `memory`, `calls`, `threads`, `unsupported`, `loans`. Fourteen additional original lifetime children cover normal/cooperative exits, callbacks, owners, failures, repeated cycles and simultaneous threads. The runner builds a small original injection dylib and loads it with a child-local environment only for two native-failure cases; do not set this injection globally. Fresh output is required; logs, `.bin`, generated C++ and binaries stay private even though these particular bytes are original and redistributable.

Run previous suites against the **same new SDK** per profile (append `--sanitize` only for Sanitized):

```sh
python3 tests/unified/run.py --sdk "$SDK" --source "$SDK_SOURCE" --pre-entry --workdir "$PRIVATE_WORK/unified-$PROFILE"
python3 tests/pre-entry/run.py --sdk "$SDK" --workdir "$PRIVATE_WORK/pre-entry-$PROFILE"
python3 tests/guest-memory/run.py --sdk "$SDK" --workdir "$PRIVATE_WORK/guest-$PROFILE"
python3 tests/instrumentation/run.py --sdk "$SDK" --source "$SDK_SOURCE" --workdir "$PRIVATE_WORK/instrument-$PROFILE"
```

These retain prior expected fault children and compile-negative ABI observations. Historical component/source controls, VMX characterization and watchdog/static-audit units have their own linked recipes. [Measured results, exact limitations and Phase 12 gate](../../docs/research/synthetic-execution.md).


The reference-count trace requires the matching Debug SDK, LLDB and local developer-debugger access. It only reads the object's private atomic via debugger symbols; it does not change object memory or reference counts:

```sh
python3 tests/synthetic/reference-trace.py --sdk "$SDK_WORK/staged/Debug" \
  --workdir "$PRIVATE_WORK/reference-trace"
```

Expected corrected counts are `[2, 2, 2, 2, 1]` at construction/Create/execution/join/handle-close, then one destructor on final caller release. The uncorrected SDK instead reports `[2, 2, 2, 3, 2]` and no destructor; that is a failing reproduction, not a passing contract. Pre-fix SDK reconstruction can use a **private** copy of the lock with only its first fifteen patches; never truncate or overwrite the public final lock. The unchanged original Phase 11 fixture assertions must continue to pass after any further lifetime change.
