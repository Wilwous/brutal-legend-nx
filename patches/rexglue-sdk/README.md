# Focused SDK safety patches

Target: ReXGlue **0.10.0**, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. This is an open-source SDK patch, not a game patch. It contains no proprietary code.

`0001-runtime-safety.patch`:

- Snapshots the complete source vector before float16_4 output writes. The scope prevents temporary-name collisions. It changes aliasing only; numeric conversion, mask warnings and rounding remain unchanged.
- Bounds XThread TLS access against dynamic slot bytes, checks null output/invalid metadata and computes the address without 32-bit multiplication/addition wrap.
- Returns `X_STATUS_NOT_IMPLEMENTED` from unsupported STFS device/control operations. It does not dereference potentially invalid output pointers or fabricate a device. Callers must check failure status; outputs remain unchanged on failure.

Apply only to a separate private source copy:

```sh
git -C "$PRIVATE_SDK_COPY" apply --check "$PROJECT/patches/rexglue-sdk/0001-runtime-safety.patch"
git -C "$PRIVATE_SDK_COPY" apply "$PROJECT/patches/rexglue-sdk/0001-runtime-safety.patch"
```

The runtime test runner verifies source fingerprints, copies the four relevant source files to its private output directory and checks/applies the patch there. The VMX runner recognizes the exact alias-patched builder hash with `--alias-safe`. Whole SDK integration, linking and game behavior remain unvalidated; the installed prebuilt SDK and previous game output are unchanged. This patch has not been submitted upstream.

Patch contributions are **BSD-3-Clause**, following the affected SDK source. Original SDK/Xenia notices and the full conditions are retained in [LICENSE.BSD-3-Clause](LICENSE.BSD-3-Clause). Original standalone foundation/harness code elsewhere in this repository remains GPL-3.0-only. No reference-port implementation was copied.

[Evidence, tests and limitations](../../docs/research/runtime-foundation.md).

`0002-host-fault-and-protect.patch` (apply after 0001) makes an unclaimed POSIX fault fail-stop via its original signal instead of retrying an unchanged instruction, and converts NtProtectVirtualMemory old protection through the existing ToXdkProtectFlags helper.

```sh
git -C "$PRIVATE_SDK_COPY" apply --check "$PROJECT/patches/rexglue-sdk/0002-host-fault-and-protect.patch"
git -C "$PRIVATE_SDK_COPY" apply "$PROJECT/patches/rexglue-sdk/0002-host-fault-and-protect.patch"
```

[Phase 6](../../docs/research/ppc-execution.md) validates selected real PPC exports, real-class TLS methods and isolated dispatcher behavior with original fixtures. A hybrid test executable does not establish complete SDK library integration or guest unwinding. The installed SDK remains unchanged; neither patch has been submitted upstream. Phase 6 contributions retain the same BSD-3-Clause boundary.

## Phase 7 coherent integration

The [isolated builder](../../tools/sdk/README.md) applies all seven patches in the order and with the SHA-256 values in [lock.json](../../tools/sdk/lock.json). It validates pristine source plus patches, builds new runtime/codegen libraries and stages matching headers privately. Never apply this series to an installed SDK or game output. The earlier hybrid commands above remain historical evidence, not the unified build recipe.

- `0003-ppc-callback-arguments.patch`: preserve full GPR integer values and selected 64-bit stack arguments; preserve r2 in CallFrame; use/restorably snapshot the bound context for nested callbacks; reject unbound or undispatched integer-address callbacks explicitly. FPSCR propagation policy stays unchanged. No complete hardware ABI claim.
- `0004-initialization-failures.patch`: return real TLS-free failure through KeTlsFree, handle absent memory heaps, and permit existing null-metadata error returns in Debug. The internal FreeTLS return type changes, requiring coherent headers/library rebuilding. XamAlloc remains unchanged because its proper Xbox failure code is unverified.
- `0005-signal-diagnostics.patch`: fixed-size write-based unhandled/nested fault records, thread guard initialization outside signal context, and explicit nested-fault fail-stop. The existing wider MMIO chain is not proven fully async-signal-safe.
- `0006-darwin-signal-context.patch`: copy the Darwin mcontext pointer by byte offset instead of dereferencing a potentially misaligned ucontext C++ object.
- `0007-defined-pe-header-offset.patch`: use compiler offsetof and assert offset 24, removing a UBSan null-member expression without changing PE layout.

[Final tests and remaining blockers](../../docs/research/unified-sdk.md): clean Debug/Release/sanitized original fixtures pass, but the sanitized private XEX loader stops on an additional XDBF alignment defect. No alignment suppression or resource-parser patch is included. These contributions share the existing BSD-3-Clause retained notices and have not been submitted upstream. No game-derived code is included.

## Phase 8 pre-entry safety series

The first seven patches above are unchanged. [pre-entry-lock.json](../../tools/sdk/pre-entry-lock.json) explicitly orders and hashes the additional four patches. Use the isolated builder's `--lock` option and rebuild every SDK client against matching headers. These patches have the same BSD-3-Clause notice boundary; no game-derived code is included.

- `0008-xdbf-byte-parser.patch`: bounded big-endian byte decoding, deterministic status, table/payload validation and naturally aligned decoded achievements; immutable borrowed-buffer contract.
- `0009-guest-memory-ownership.patch`: default NOACCESS mappings, owned commitment/protection reconciliation, checked logical guest ranges, and physical recovery restricted to current committed permissions. Host-page slack remains an explicit limitation.
- `0010-allocation-and-exception-failures.patch`: explicit host exceptions for unsafe allocation arguments/OOM and three unsupported guest exception exports. No Xbox failure value or guest unwind is invented.
- `0011-checked-allocation-ownership.patch`: wide checked size/alignment arithmetic before allocation, capacity/actual-size validation before zeroing, nonfatal expected heap exhaustion, and heap-maintained subsystem owner tokens checked atomically by XamFree. Release/reset/new reservations erase ownership.

[Root causes, original regressions and remaining execution blockers](../../docs/research/pre-entry-safety.md). Owner tokens do not solve same-owner raw-pointer ABA, guest object bounds, concurrent pointer lifetime, save-state serialization or complete Xbox error/ABI conformance. No game entry or boot is validated.

## Phase 9 checked accesses and closed handles

[entry-readiness-lock.json](../../tools/sdk/entry-readiness-lock.json) preserves patches 0001–0011 and adds:

- `0012-checked-guest-copy.patch`: owner-tagged, heap-locked, bounded full-span copies on the 4-KiB virtual heap. Check reservation/commit/protection before copying; conservatively reject MMIO and shared host-page padding. Fetch is explicitly unsupported; physical/XEX/large-page heaps and raw pointers are not covered.
- `0013-closed-handle-failure.patch`: reject empty/exhausted object-table entries in ReleaseHandle using its existing INVALID_HANDLE status before decrement. Original single/double/concurrent-close regressions use real NtClose exports.

These retain the BSD-3-Clause notice boundary. The original GPL adapter/fixtures are separate. [Supported contracts and execution NO-GO](../../docs/research/guest-memory-entry-readiness.md). This is not transparent mediation of generated game accesses, a complete handle-lifetime fix, guest execute permissions or Xbox ABI support.

## Phase 10 optional generated-access seam

[The instrumentation lock](../../tools/sdk/instrumentation-lock.json) retains all prior hashes and adds `0014-generated-access-seam.patch` under the existing BSD-3-Clause notice boundary. Both PCH templates support an explicit consumer include after memory/dispatch macros and before inline helper expansion. Full vectors use two macros, partial vectors a default no-op guard, and all identified direct-call emission sites the existing call macro. Default behavior is retained; the original GPL checked consumer opts in and refuses unsupported operations. No game output is rewritten. Custom/older templates require review and matching regeneration. [Contracts and NO-GO gate](../../docs/research/generated-memory-instrumentation.md).

Phase 11 [0015](0015-checked-virtual-stack-copy.patch) permits the existing locked, owner-checked copy primitive on ordinary virtual 64-KiB allocations, including XThread stacks. Page bounds, reservation identity, commitment, protection and MMIO checks are retained; XEX/physical heaps and instruction fetch remain unsupported. It follows the fourteen existing patches in [synthetic-lock.json](../../tools/sdk/synthetic-lock.json). Non-owning scope loans are original host-fixture code, not a guest ownership API or automatic title-memory adoption. [Evidence](../../docs/research/synthetic-execution.md).

Phase 11 recovery [0016](0016-xthread-owned-exit.patch) fixes the confirmed Mac exit-reference leak with an owned C++ completion boundary and matched native cleanup handle release. It balances failed native creation, delays guest process linkage until native success, and frees unconsumed callback/semaphore resources. Self termination is cooperative; foreign asynchronous termination is refused before mutation. This does not supply Xbox guest exceptions or validate other platforms. The [sixteen-patch lock](../../tools/sdk/synthetic-lock.json) retains all fifteen previous digests. Original destructor/reference/allocation/factory failure regressions validate this scope. [Ownership and results](../../docs/research/synthetic-execution.md#xthread-lifetime-root-cause-and-recovery).

### 0017 — transactional XEX ownership

Adds exclusive, tagged XEX reservation/commit with local rollback; removes whole-heap reset and makes load/retry/continuation/destruction release only recorded owned ranges. Continuation failure clears owned metadata and mappings; unsupported variable imports refuse success. UserModule owns its guest header through final legitimate reference release; failed file loading reclaims partial resources. Explicit host image leases retain UserModule and pin mappings while unload closes admission. Header/PE/import span checks bound selected failure paths. XamFree and historical allocation APIs are unchanged. Patch growth is owner-tracked with unpublished-target rollback; patch-format/application compatibility is not certified. See [Phase 12 evidence and limitations](../../docs/research/image-ownership-runtime-contracts.md).

### 0018 — header TLS, thread image lifetime and slot bounds

The [eighteen-patch lock](../../tools/sdk/tls-import-lock.json) adds bounded TLS metadata copying/validation before XEX continuation success, explicit/default module-bound XThread creation and retained image lifetime through final PCR/TLS release. Selected pre-native creation failures reclaim partial resources; late host exceptions with a native callback fail-stop. Process TLS free checks advertised slot capacity before bitmap mutation, preserving its existing boolean error. Slot-count alignment and 1 MiB total are conservative host admission policies, not Xbox format claims. Patch 0018 retains the upstream BSD notice boundary. [Original synthetic evidence and remaining contracts](../../docs/research/xex-tls-import-admission.md).
