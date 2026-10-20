# Pre-entry safety fixtures

Original synthetic tests only; no proprietary inputs, game execution or generated game-code compilation. Use the isolated Phase 8 SDK recipe below. [Findings and readiness gate](../../docs/research/pre-entry-safety.md).

From the project root, choose a **new private workspace outside Git**, then install the hash-locked CMake/Ninja wheels as described in [the SDK builder](../../tools/sdk/README.md). The Phase 7 default lock is retained for historical reproduction; Phase 8 explicitly selects a separate eleven-patch lock:

```sh
python3 tools/sdk/build.py --lock tools/sdk/pre-entry-lock.json --workspace "$PRIVATE_WORK/pre-entry" --cmake "$PRIVATE_WORK/tools/bin/cmake" --ninja "$PRIVATE_WORK/tools/bin/ninja" --clean
```

An optional `--source-cache "$PINNED_SDK_CHECKOUT"` reuses committed Git objects, never cache working-tree edits. Patches 0001–0007 are unchanged. The private build records dependency revisions, ordered patch hashes, library hashes and resources; no global SDK install runs.

Run original Phase 5–7 integrated regressions and the new fixtures:

```sh
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Debug" --source "$PRIVATE_WORK/pre-entry/sdk" --workdir "$PRIVATE_WORK/regression-Debug" --pre-entry
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Release" --source "$PRIVATE_WORK/pre-entry/sdk" --workdir "$PRIVATE_WORK/regression-Release" --pre-entry
python3 tests/unified/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Sanitized" --source "$PRIVATE_WORK/pre-entry/sdk" --workdir "$PRIVATE_WORK/regression-Sanitized" --pre-entry --sanitize
python3 tests/pre-entry/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Debug" --workdir "$PRIVATE_WORK/safety-Debug"
python3 tests/pre-entry/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Release" --workdir "$PRIVATE_WORK/safety-Release"
python3 tests/pre-entry/run.py --sdk "$PRIVATE_WORK/pre-entry/staged/Sanitized" --workdir "$PRIVATE_WORK/safety-Sanitized" --sanitize
python3 tests/ppc/test_watchdog.py
```

The unified runner's `--pre-entry` option changes the former false-success expectations into explicit failure-contract regressions and requires the old writable hole to fault. It does not weaken the assertions or mix SDK libraries. The six original dependency-free foundation groups and real SDK native memory, exports, XThread/TLS, ABI, MMIO, signal and codegen tests all use the current profile. Historical hybrid scripts remain available for their original SDK series; they are not evidence for the coherent library.

`xdbf.cpp` constructs all bytes independently: valid resources at 16 input alignments, every truncation, malformed table/payload cases and 2,000 seeded mutations. `allocation-boundaries.cpp` checks 32,961 arithmetic cases without guest allocations and runs first. `contracts.cpp` checks real Xam allocation/free, including formerly overflowing sizes, foreign/stale ownership and bounded exhaustion, logical page permissions and explicit exception rejection. `memory-faults.cpp` isolates unowned/reserved/decommitted/released/physical read-only accesses and uncaught exception-export calls. Every compilation/execution has the external 30-second child/group watchdog.

`vector-context.cpp` tests an **explicit original raw-context convention**, including vector lanes, endian byte access, alignment, mixed scalar state and preserved sentinels. It does not implement or establish a typed Xbox ABI. Two negative typed-signature compilations, plus the existing typed-vector argument negative probe, explicitly record unsupported argument/return/mixed contracts. Accessible host-page slack on a 16 KiB host is a BLOCKED observation, never a passing ownership contract.

## Optional private loader gate

The owner's unchanged XEX is hash-checked. `--pre-entry` additionally requires the SDK's title XDBF parser to accept the resource; silently discarding a resource cannot pass this helper. SDK library hashes/ownership are checked. All raw output remains private.

```sh
python3 tests/unified/loader.py --sdk "$PRIVATE_WORK/pre-entry/staged/Debug" --game-root "$PRIVATE_GAME_ROOT" --workdir "$PRIVATE_WORK/loader-Debug" --pre-entry
python3 tests/unified/loader.py --sdk "$PRIVATE_WORK/pre-entry/staged/Release" --game-root "$PRIVATE_GAME_ROOT" --workdir "$PRIVATE_WORK/loader-Release" --pre-entry
python3 tests/unified/loader.py --sdk "$PRIVATE_WORK/pre-entry/staged/Sanitized" --game-root "$PRIVATE_GAME_ROOT" --workdir "$PRIVATE_WORK/loader-Sanitized" --pre-entry --sanitize
```

Neither helper prepares nor executes the guest entry point. Loader success, parser safety and complete runtime readiness remain separate claims. No suppression of alignment or other sanitizer checks is used.

## Historical standalone/source comparisons

The unified matrix already runs the original foundation/native-memory/real-export checks. To reproduce the additional historical source-only negative controls, first materialize the four pristine pinned SDK files in a private directory, without resetting the patched checkout:

```sh
python3 - "$PRIVATE_WORK/pre-entry/sdk" "$PRIVATE_WORK/source-controls" <<'PY'
from pathlib import Path
import subprocess, sys
source, target = map(Path, sys.argv[1:])
paths = ['src/system/xthread.cpp', 'src/kernel/xboxkrnl/xboxkrnl_io.cpp',
         'src/kernel/xboxkrnl/xboxkrnl_memory.cpp', 'src/codegen/builders/vector.cpp']
for name in paths:
    output = target / name
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(subprocess.check_output(['git', 'show', 'HEAD:' + name], cwd=source))
PY
python3 tests/runtime/run.py --sdk-source-root "$PRIVATE_WORK/source-controls" --workdir "$PRIVATE_WORK/legacy-O0"
python3 tests/runtime/run.py --sdk-source-root "$PRIVATE_WORK/source-controls" --workdir "$PRIVATE_WORK/legacy-O2" --optimization 2
python3 tests/runtime/run.py --sdk-source-root "$PRIVATE_WORK/source-controls" --workdir "$PRIVATE_WORK/legacy-sanitized" --optimization 2 --sanitize
python3 tests/vmx128/run.py --sdk-source "$PRIVATE_WORK/pre-entry/sdk/src/codegen/builders/vector.cpp" --sdk-include "$PRIVATE_WORK/pre-entry/staged/Debug/include" --workdir "$PRIVATE_WORK/vmx-O0" --alias-safe
python3 tests/vmx128/run.py --sdk-source "$PRIVATE_WORK/pre-entry/sdk/src/codegen/builders/vector.cpp" --sdk-include "$PRIVATE_WORK/pre-entry/staged/Release/include" --workdir "$PRIVATE_WORK/vmx-O2" --alias-safe --optimization 2
python3 tests/vmx128/run.py --sdk-source "$PRIVATE_WORK/pre-entry/sdk/src/codegen/builders/vector.cpp" --sdk-include "$PRIVATE_WORK/pre-entry/staged/Sanitized/include" --workdir "$PRIVATE_WORK/vmx-sanitized" --alias-safe --optimization 2 --sanitize
```

These compare the pinned baseline with known source corrections and characterize VMX numeric disagreements; they do not substitute mock objects for integrated SDK claims. No installed SDK library is used. Preserve the reported infinity/overflow/NaN disagreements and hardware-equivalence UNKNOWN result.
