# Phase 7 — Unified SDK and ABI integration

Assessment: **2026-10-20 · v0.0.7-unified-sdk**. Baseline: `v0.0.6-ppc-execution`, commit `0ac8bf8d7b6eacddab8223f31185ee7c4356fb64`.

**VERIFIED:** one pinned, patched SDK builds cleanly in Debug, Release and ASan/UBSan profiles. The original integrated fixture matrix passes 131 supported checks; six watchdog regressions also pass. **PARTIAL:** these are selected host runtime contracts, not complete Xbox ABI conformance. **BLOCKED / Phase 8 NO-GO:** a sanitized private loader probe aborts in XDBF parsing, and allocation, exception, pointer and vector contracts remain unresolved. No generated game code is executed, no game entry is prepared and no full game compilation is attempted.

## Baseline and privacy

The original `default.xex` SHA-256 was checked before work and before each private loader probe:

```text
636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456
```

Final SHA-256/size/nanosecond-mtime comparison matches all **1,455 baseline records**: the executable, previous ReXGlue manifest and 1,453 generated files. No title update was applied and no analyzer was rerun. The old installed runtime hash remains `645325a2372ca77111635d613e3692b766012c3125f9dbc0c05f593b5a973e00`. All SDK copies, compiler outputs, original synthetic generated fixtures, private loader output, raw diagnostics and game files stay outside Git. Only original fixtures/tooling, sanitized findings and narrowly scoped open-source patches are published.

## Reproducible SDK environment

SDK source: [ReXGlue v0.10.0](https://github.com/rexglue/rexglue-sdk/tree/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2), revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. The [lock file](../../tools/sdk/lock.json) records all 22 submodule revisions, the SPIRV-Cross and cereal CPM revisions, CPM 0.40.8 bootstrap SHA-256, ordered patch SHA-256 values and tool wheel hashes. This is a pinned source/build recipe; byte-identical output across arbitrary host paths/Xcode installations is not claimed.

Measured host: macOS 15.7.7 (24G720), ARM64, 8 GiB RAM; Apple Clang 17.0.0 (`clang-1700.6.4.2`), Apple Git 2.50.1, macOS SDK 26.2. Isolated Python environment: CMake 3.31.6; Ninja wheel 1.11.1.4, binary `1.11.1.git.kitware.jobserver-1`. No globally installed SDK file or build tool was replaced.

The [builder](../../tools/sdk/README.md) fails on revision, dependency, patch or command mismatch; compares each changed SDK file against pristine Git content plus the ordered patches; rejects extra source edits; uses private CPM/build/staging paths and no install/SDK registration step. It supports a fresh network clone and an optional explicit local Git source cache. The measured build cloned committed source and submodules from such a cache; dirty cache working files were not copied. Clean mode deletes only its owned build/staging/output directories. Tool and library fingerprints are written privately.

The pinned upstream configuration requires a graphics backend even for these targets. Vulkan dependencies were configured, but **only `rexruntime`, `rexcodegen` and `rexglue` targets were built**; the `rexgpu-xenos` rendering plugin was not built or exercised. Native runtime dependencies are compiled without claiming graphics/audio integration. Tracy and performance counters are disabled consistently. `SOURCE_DATE_EPOCH` comes from the pinned commit.

The final seven-patch clean rebuild completed:

| Profile | SDK optimization / instrumentation | Build seconds | Peak RSS bytes |
| --- | --- | ---: | ---: |
| Debug | O0, debug checks | 103.084 | 564,953,088 |
| Release | O3 | 131.541 | 506,675,200 |
| Sanitized | O1, ASan + UBSan, no recovery | 221.642 | 842,186,752 |

Configure took 20.891 seconds. Two compile jobs were allowed. Build peak RSS uses `wait4` accounting for the supervised process/reaped descendants, **not simultaneous aggregate process-tree memory**. Build wall time includes compilation/linking and excludes initial acquisition. C/C++/Objective-C/Objective-C++ runtime sources are instrumented in Sanitized; system frameworks and handwritten assembly are not. This is substantially wider instrumentation than Phase 6's hybrid fixture.

Final runtime SHA-256 values:

| Profile | Actual library | SHA-256 |
| --- | --- | --- |
| Debug | `librexruntimed.dylib` | `7c990f9a1b586eb23df7be61a61849e266b4dd8146e2cc98aefb5f51b47c77a3` |
| Release | `librexruntime.dylib` | `88d4dc7ab9e2b402824eddbd1db144b8e2ce1810c41f4bcfdf52680e71c4f032` |
| Sanitized | `librexruntime.dylib` | `e6919734fa0ea2b71f8f5740019225b9438b87a64941c635c7dda723eb25e2db` |

Every profile exports all **261 baseline function shims**, with zero missing symbols. This establishes symbol presence only. Real runtime shim ownership is checked with `dladdr` against the supplied staged SDK, with environment overrides removed. The Phase 6 fixture now links all its SDK calls to this library; no separately compiled replacement exports or old prebuilt runtime are used. Codegen fixtures use the newly built `rexcodegen` library and matching headers.

## Seven ordered patches and observed defects

[Patch descriptions and license boundary](../../patches/rexglue-sdk/README.md); hashes and order are in the lock file.

| Patch | Verified scope |
| --- | --- |
| 0001 | Existing alias snapshot, XThread TLS bounds/wrap checks and STFS NOT_IMPLEMENTED |
| 0002 | Existing unclaimed POSIX fault fail-stop and old-protection XDK conversion |
| 0003 | Preserve 64-bit GPR/selected stack arguments, r2 and active nested callback context; reject unsupported/unbound callbacks |
| 0004 | Propagate failed TLS-free result, reject missing memory heaps, allow existing null-metadata error returns in Debug |
| 0005 | Fixed-size signal diagnostics, prepared thread-local reentrancy guard and bounded nested-fault fail-stop |
| 0006 | Read Darwin `uc_mcontext` pointer without assuming C++ ucontext alignment |
| 0007 | Replace null-member PE offset calculation with compiler offsetof; assert expected offset 24 |

Before 0003, the initial unified ABI fixture had **5 passing / 4 failing groups**: high GPR bits, 64-bit stack load/store and nested callback/r2 propagation. Original arithmetic, raw big-endian bytes and explicit stack relationships provide test oracles independent of the translation setters. A source comparison with [Xenia shim utilities at 95a5c3e](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/util/shim_utils.h) supports the u64 GPR and selected stack-slot conventions; this is software evidence, **not Xbox hardware validation**.

A deliberately nested fault in a registered exception callback, with the real MMIO chain installed, reproduced a **30.004-second timeout** before 0005, with approximately one CPU core occupied. It was killed by the external watchdog. Final nested faults terminate in 0.025–0.074 seconds with an explicit nested record and exit 139. No additional unbounded reproduction was attempted.

Full SDK UBSan exposed Darwin kernel-provided ucontext alignment differing from the C++ type requirement. Patch 0006 removes that typed misaligned dereference; all final signal fixtures then pass under instrumentation. The optional private loader subsequently exposed the null-member offsetof expression; patch 0007 fixes that exact construct without changing PE interpretation. A later XDBF alignment defect remains open below. No sanitizer finding is suppressed.

## Integrated faults and MMIO

The original [fault fixture](../../tests/unified/faults.cpp) uses the actual `Memory`, registered MMIO range, SDK POSIX dispatcher, MMIO decoder and bound context. Its own assembly performs a single 32-bit load/store. Only the fixture MMIO page is protected NOACCESS; global protections remain enabled. Both explicit MMIO checks and fault-driven load/store verify callback address/value, endian conversion, host register update and resumed PC. Other widths, exclusive/atomic instructions and arbitrary compiler instruction forms remain untested.

| Path | Final expected outcome |
| --- | --- |
| Registered MMIO read/write | Recovered, callbacks and values checked, exit 0 |
| Claimed Phase 6 store fault | Recovered by controlled PC adjustment; page remains read-only |
| Read-only write / NOACCESS read / null guest access | Unhandled SIGBUS, compact fault record, no retry loop |
| Native null access | SIGSEGV, compact fault record |
| Fault inside a registered exception callback | Nested SIGSEGV record; explicit fail-stop exit 139 |
| Declined SDK fault and SDK memory control | Terminate by original signal |
| Independent POSIX capture | Explicit diagnostic exit 80 |

An external monotonic watchdog supervises every compile/test child with a **30-second limit**, kills its entire process group on timeout, and records exit, elapsed time, CPU, RSS and compact logs. The build driver has separate bounded acquisition/configure/build budgets. Six supervision regressions verify cleanup and exit handling. Final fixture runs have **zero timeouts and zero unexpected crashes**. Sanitized excludes only the plain POSIX-default control because ASan owns that default signal disposition; the SDK's actual fault paths still run.

Signal records carry signal/code/nested flag, fault address and host PC. Fast final children finish before stack sampling is needed; stack sampling remains available for a timeout. The guard is initialized outside the handler for SDK-installed/bound threads. **PARTIAL:** the broader upstream MMIO handler still uses locks, OS mapping queries and some diagnostic/decode paths; complete POSIX async-signal safety and unprepared foreign host threads are not established. Nested faults deliberately fail-stop; they do not unwind/recover guest exceptions.

## ABI, memory and TLS contracts

The final ABI fixture has **13 groups per profile**: 64-bit GPR arguments/return, one ninth 64-bit stack argument loaded from independently constructed bytes, callback stack store, two double arguments/return, nested callbacks, r1 16-byte alignment and 0x70 frame nesting, r2/r13 preservation, host-exception context restoration, raw vector registers, pointer round-trip, CallFrame, typed callback and rejected unbound/integral-address callbacks. Phase 6 adds register sentinels and real export calls. Restoration follows the SDK policy of propagating FPSCR; full FPSCR semantics are not independently validated.

**UNKNOWN:** complete mixed integer/FPR/vector ABI, variadics, additional stack cases/FPR spill, guest callback address dispatch/TOC initialization, Xbox exception unwinding and hardware nonvolatile-register conformance. Raw vector register execution passes; typed vector argument marshalling fails to compile and is explicitly BLOCKED. The callback frame alignment check does not validate that every guest stack address is allocated.

Real memory/export regressions cover alignment, allocation/protection/free lifecycle, big-endian access, output preservation, null metadata, oversized allocation failure and missing heap rejection. Protection flag round trips use the real compiled SDK conversion functions. STFS calls return `0xC0000002` without touching valid outputs or dereferencing deliberately invalid output addresses. This is correct explicit rejection, not STFS functionality.

Actual integrated `XThread`/`ThreadState`/PCR tests start two host-backed SDK threads concurrently, verify distinct PCR/r13/TLS state, allocate/use/free process slots, check isolation, clear values and reject invalid/double frees, then join and close handles. Separate real-class boundary tests exercise last valid/one-past/overflow slot, null output and malformed allocation/static/dynamic TLS metadata. These use a small original synthetic process layout; **title-specific launch/TLS initialization remains untested**. Fibers, APCs, cancellation and stressed concurrent slot churn are not claimed.

The newly compiled SDK propagates invalid/double TLS-free failure instead of unconditional success. This is a fail-closed host contract, not independently proven Xbox invalid-input behavior. Invalid nonzero guest addresses remain a separate risk: translation is arithmetic, not a general bounds validator.

## Final fixture matrix and resource measurements

| Profile | Passing groups | Passing fault checks | Unexpected failures / timeouts | Sum of supervised child seconds | Peak execution RSS bytes |
| --- | ---: | ---: | --- | ---: | ---: |
| Debug SDK / O0 fixtures | 33 | 11 | 0 / 0 | 19.392 | 34,799,616 |
| Release SDK / O2 fixtures | 33 | 11 | 0 / 0 | 18.409 | 22,233,088 |
| Sanitized SDK / O2 sanitized fixtures | 33 | 10 | 0 / 0 | 21.858 | 62,078,976 |

The 33 groups are 7 Phase 6 integrated groups, 13 ABI groups, 5 failure groups, 6 Phase 5 standalone groups, 1 native memory group and 1 codegen alias group. The latter executes **3,084 original synthetic invocations per profile** through output from the real rebuilt emitter. It validates alias equivalence only, not special-value numeric hardware truth. The Phase 5 hybrid leaf adapter is superseded by integrated real-export checks; it is not relinked against the old SDK. No baseline source classification is promoted merely because a shim exists.

Totals: **131 supported fixture/fault checks + 6 watchdog checks = 137 passing checks**. Six distinct known blockers are separately observed in every profile and are **not counted as passing functionality**. Compilation contributes to child-time sums; the expected negative compile is excluded from those sums; execution-only sums are 5.137 / 3.921 / 5.079 seconds. Peak compilation/execution child RSS is 317,636,608 / 335,069,184 / 348,504,064 bytes. Profiles were launched concurrently; sums are not elapsed project time or game performance. CPU/resource records and raw diagnostics remain private.

## Optional private loader probe — one pass, one confirmed failure

A bounded loader-only helper uses the hash-checked original XEX, separate empty update directory and the same staged library. It never calls `PrepareModuleLaunch` or `LaunchModule` and never compiles/executes generated game functions. SDK host support threads can exist during runtime initialization.

- **VERIFIED Debug:** load succeeds; 274 imports recognized, zero unresolved, image size 14,090,240 bytes; 0.641 seconds, peak RSS 65,716,224 bytes. This extra loader pass is separate from the 137 original-fixture checks.
- **BLOCKED Sanitized:** aborts with SIGABRT (exit -6) after 0.707 seconds, peak RSS 97,992,704 bytes. Exact sanitized diagnostic, with the private path/address removed: `src/system/util/xdbf_utils.cpp:55:60: runtime error: member call on misaligned address … for type 'rex::endian_store<unsigned long long, std::endian::big>', which requires 8 byte alignment`. This is during `XdbfWrapper::GetEntry`, after import registration. No guest entry ran.

This newly exposed packed-XDBF access needs an alignment-safe parser and bounds review with original malformed/aligned/unaligned resource fixtures. A one-line local suppression or alignment assumption would not establish parser safety. It is intentionally left as an exact reproducible blocker rather than expanding this milestone into resource-parser repair.

## Remaining blockers and Phase 8 decision

| Classification | Evidence / consequence |
| --- | --- |
| BLOCKED | XDBF loader typed endian access violates alignment under UBSan; sanitized original-XEX load does not finish |
| BLOCKED | `XamAlloc` oversized allocation returns success 0 with null output; the appropriate Xbox failure status is not independently established, so no speculative replacement is introduced |
| BLOCKED | `RtlCaptureContext`, `RtlUnwind`, `__C_specific_handler` warn and return without implementing their contracts; sentinel output stays unchanged |
| BLOCKED | Typed native vector marshalling has no matching GetValue specialization; expected compile failure |
| BLOCKED | An unallocated nonzero guest hole is writable unless explicitly protected; TranslateVirtual/GuestPtr are not bounds validation |
| PARTIAL | Tested SDK-thread fault/MMIO chain works; full async-signal safety, decoder coverage and external-thread preparation remain open |
| UNKNOWN | Xbox VMX numeric truth, full ABI, title initialization/exception behavior, Switch runtime/memory/graphics/audio and performance |

**Phase 7 result: PARTIAL. Phase 8 decision: NO-GO for executing private generated game code.** Clean integration and selected ABI fixes are real progress, but neither loader success nor fixture success proves entry readiness.

**Exact next milestone: Phase 8 — loader safety and fail-closed entry prerequisites, before guest execution.** First replace unsafe XDBF packed accesses with bounded alignment-safe decoding and independent malformed-resource regressions; require the unchanged XEX to load with sanitizers. Establish the XamAlloc error contract, guard unsupported exception imports and unchecked guest-pointer boundaries, and review a narrowly defined entry/import allowlist plus title TLS/TOC/dispatch requirements. Only a later explicit readiness review should authorize a limited private generated-code trace. Full game compilation, graphics/audio and Switch development remain separate gates.

## Reproduction

[Exact isolated build commands](../../tools/sdk/README.md) · [Original matrix and optional loader commands](../../tests/unified/README.md). All output arguments must point outside the public checkout. Expected blocker observations and the sanitized loader failure must remain visible; do not reinterpret driver exit 0 as complete runtime compatibility. No GitHub Release is created.
