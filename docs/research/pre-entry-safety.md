# Phase 8 — Pre-entry safety and defensive regressions

Assessment: **2026-10-21 · v0.0.8-pre-entry-safety**, continuing `v0.0.7-unified-sdk` (`b8ebaa0e85c27a7085f2d39e1f0b189743ee374f`). No proprietary code is executed, no guest entry point is prepared and no game build or boot is attempted.

This report separates tested host safety contracts from Xbox compatibility. **Phase 9 execution recommendation: NO-GO** while unchecked raw guest accesses, typed-vector ABI and guest exception requirements remain unresolved. Completing the safety regression milestone does not close those compatibility gates.

## Preservation and reproducibility

The original `default.xex` remains SHA-256 `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`. All 1,455 previous XEX/configuration/generated-output records retain SHA-256, size and modification time. All **299 extracted base/DLC/update files** also match their original extraction SHA-256 inventory. No title update is applied, analyzer rerun, generated game file changed or proprietary content published. The old installed SDK runtime remains SHA-256 `645325a2372ca77111635d613e3692b766012c3125f9dbc0c05f593b5a973e00`.

The SDK stays pinned to ReXGlue v0.10.0, revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. [pre-entry-lock.json](../../tools/sdk/pre-entry-lock.json) retains the seven Phase 7 patches unchanged, adds three existing Phase 8 patches and the completion correction 0011, and records every ordered SHA-256. Dependency pins, tool wheel hashes and compiler settings are inherited from [Phase 7](unified-sdk.md). Source verification reconstructs pristine Git content plus all eleven patches before building; unknown source changes or patch failures stop the builder. All SDK sources, dependencies, staging, executables and diagnostics stay in private directories. No global installation runs.

[Exact build/test commands](../../tests/pre-entry/README.md). Explicitly select `--lock tools/sdk/pre-entry-lock.json`; the builder's default lock preserves Phase 7 reproduction. Clean Debug, Release and Sanitized profiles use the same patched source and matching headers/libraries. Only `rexruntime`, `rexcodegen` and `rexglue` targets are built; no GPU plugin, full game compilation or Switch integration is added.

## Saved failure and confirmed allocation defect

The interrupted expanded Debug contract fixture previously exited with **SIGBUS (-10)** after 0.342 seconds, without a timeout. Its compact signal record, binary and logs remain private. It was not automatically rerun on resumption. Review first revalidated the source/patch fingerprints and tested parser, ABI and selected memory denial paths separately.

An arithmetic-only probe then confirmed the defect without making a large allocation:

1. `BaseHeap::AllocSystemHeap` rounded a `uint32_t` size using `(value + multiple - 1) / multiple * multiple`.
2. Requests `0xffffffff` and `0xfffff001` overflowed the 32-bit addition and became zero.
3. `get_page_count` rounded zero with the helper's minimum-one-page policy and obtained one page.
4. `Memory::SystemHeapAlloc` could therefore allocate that page and call `Zero(address, size)` with the original nearly 4 GiB size.

The integer wrap itself is defined unsigned arithmetic, so ordinary UBSan need not diagnose it. The resulting undersized allocation and oversized write are an SDK correctness defect, not evidence that alignment sanitization should be disabled. This source path explains the saved fault mechanism; the old record has a host PC but no complete symbolic stack, so the exact faulting instruction in that historical run is not independently proven.

Patch **0011** introduces `CheckedAllocationRoundUp`: it computes the rounded result in `uint64_t`, rejects values above `UINT32_MAX`, and leaves the output unchanged on failure. Heap allocation entry points validate size/alignment before modulo, truncation, host mapping or page-table mutation. `get_page_count` rejects unrepresentable rounding; `AllocFixed` also rejects a one-past table end. Range alignment uses a wide intermediate. SystemHeapAlloc checks capacity and the actual returned region size before zeroing. Ordinary zero-sized SDK reservations retain their prior minimum-page policy; XamAlloc's unsupported zero-size path remains explicit rejection.

A deterministic fully occupied **16 KiB range** now returns allocation failure in Debug as well as optimized builds; the old heap-exhaustion assertion is removed from this expected OOM path. No success may silently substitute a smaller allocation for the request.

Before any retry of the formerly failing sizes, the new standalone arithmetic fixture passed **32,961 cases per O0/O2/sanitized profile**, using a separate quotient/remainder oracle. It allocates no guest memory. The integrated fixture orders that arithmetic gate before allocation contracts. Only after the correction and gate passed were the saved boundary sizes tested in externally supervised children against the rebuilt SDK.

## XamAlloc and XamFree contracts

**VERIFIED host safety; Xbox failure return convention remains UNKNOWN.** The current XamAlloc success path returns 0 with a live, zeroed, at least 32-byte-aligned guest allocation. Null, unaligned, unowned or read-only output storage is not accepted. Unsupported flags/zero size raise `std::invalid_argument`; failed allocation clears a previously validated output slot and raises `std::bad_alloc`. These are explicit host failures, not invented Xbox HRESULTs. A future guest execution boundary must stop on them rather than catch and resume with stale PPC return registers.

The primary software comparisons [Xenia at 95a5c3e](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xam/xam_info.cc) and [Canary at e6cd2e4](https://github.com/xenia-canary/xenia-canary/blob/e6cd2e4bc998609ca25e24a24c8e32b66c5908ad/src/xenia/kernel/xam/xam_info.cc) both retain success-with-null behavior on allocation failure. They do not establish a reliable Xbox OOM status. No speculative status is introduced here.

Review confirmed that a live allocation base is insufficient to authorize XamFree: the initial Phase 8 implementation would accept another subsystem's committed virtual block. Patch 0011 records an opaque subsystem owner in the actual `BaseHeap`, using the runtime's XamModule instance as the XAM token. SystemHeapAlloc registers the token before returning; registration failure rolls back the allocation. XamFree uses `ReleaseOwned`, checking and releasing under the existing global-before-heap lock order. Foreign, null, interior and already freed addresses raise explicit host rejection.

Owner records are erased on heap release, reset and new reservation, including direct heap releases. Thus a stale XAM address reused by an untagged/foreign allocation cannot authorize its release. Tests cover a live ordinary SDK block, a block tagged to a different owner, immediate double-free, interior pointer, valid XAM release, and stale address reuse by a foreign fixed allocation. The foreign allocation remains live after rejection.

**Limits:** these are host-side ownership tokens, not guest capabilities or allocation-size bounds on every pointer. A bare stale pointer whose numeric address is later reused by a new XAM allocation is indistinguishable from that new pointer. Owner records are not a save-state serialization format; reset/restore, module recreation and arbitrary concurrent low-level memory mutation are not claimed compatible. The logical guest-range check is not an allocation lifetime pin.

## Guest memory reservation and protection

Patch **0009** starts guest views NOACCESS rather than granting broad read/write mapping permissions, removes blanket physical precommit, and grants access through owned heap commitments. Decommit/release reconcile host permissions for large as well as small guest pages. Missing/overflowing ranges and a one-past page-table query are rejected. Physical fault recovery requires current committed ownership and current permissions; original allocation permissions cannot resurrect a decommitted or read-only alias.

Before that recovery correction, original isolated physical decommit/read-only probes both returned 86 after an invalid write, demonstrating that the handler had restored access. Final denial tests must terminate through SIGBUS for unowned virtual/physical holes, reserve-only regions, virtual and physical decommit/release, and physical read-only writes. Real MMIO callbacks remain separately tested by the unified regression suite; ordinary checked pointer access excludes MMIO ranges. No global protection bypass is introduced.

**PARTIAL / critical execution gate:** this Mac has 16 KiB host pages and the SDK models 4 KiB guest pages. One host page can only have one effective permission, so the union needed by committed neighbours permits direct native access to some uncommitted guest-page slack. A controlled original probe confirms this; it is recorded as BLOCKED and excluded from passing ownership checks. `IsRangeAccessible` rejects that logical guest range, but raw `TranslateVirtual`/generated loads and stores do not automatically call it. Host protection alone cannot provide complete guest isolation, suballocation bounds or use-after-free protection within an accessible page. Graphics alias/watch behaviour, all instruction widths, concurrent lifetime changes and Horizon mappings remain outside the verified contract.

## XDBF parsing and sanitizer evidence

The Phase 7 failure was reproduced once before correction with the saved library and stack reporting enabled. UBSan reports `src/system/util/xdbf_utils.cpp:55:60: member call on misaligned address ... for type rex::endian_store<unsigned long long, std::endian::big>, which requires 8 byte alignment`.

The symbolic SDK stack is `XdbfWrapper::GetEntry → GetStringTableEntry → XdbfGameData::GetExistingLanguage → KernelState::LoadAchievementsData → SetExecutableModule → Runtime::LoadXexImage`. It is a typed overlay on packed wire data and therefore genuine undefined behaviour in the SDK. It is not an unsupported sanitizer interaction or incorrect sanitizer configuration. No alignment check or other sanitizer finding is suppressed.

Patch **0008** loads bytes explicitly in big-endian order, checks table capacities/used counts with subtraction before multiplication, validates entry/free offsets and lengths, and checks known XSTR/XACH/XSTC payload headers/counts/string lengths before exposing getters. Invalid construction returns a deterministic `Status` and safe empty getters. Achievement records are decoded into naturally aligned host integer fields rather than packed endian objects. The buffer is borrowed and must remain alive and immutable.

The independently constructed fixture covers 16 input alignments, all 177 truncation lengths, 21 malformed cases and 2,000 seeded mutations. Each profile makes **7,658 assertions**. Mutation acceptance is not an independent format oracle; the tests establish bounded reads and explicitly specified valid/malformed cases. Unknown metadata remains opaque bounded content; this is not a general parser for every XDBF dialect or every nested resource.

Format decisions use the reverse-engineering project's [XDBF description](https://free60.org/System-Software/Formats/XDBF/) for the 24-byte big-endian header, version `0x10000`, 18-byte entries and terminal free-space descriptor, and its [SPA description](https://free60.org/System-Software/Formats/SPA/) for known metadata payload sizes excluding the four-byte magic. The initial draft's incorrect version/length assumptions were corrected before final validation. Synthetic bytes are original; no game-derived XDBF data is published.

## ABI, exception and runtime limits

The existing 13-group ABI fixture validates selected GPR64/stack/FPR arguments, r1/r2/r13, callbacks, nested callbacks, host-exception restoration and pointer/endian conventions against original software oracles. The additional raw vector fixture uses an explicitly chosen original context convention to test lanes, alignment, mixed scalar state and preserved sentinels. **It is not evidence for an inferred typed Xbox ABI.** Typed vector argument, return and mixed signatures remain expected compiler failures and are counted as BLOCKED observations, not successful features. Complete Xbox ABI and hardware numeric equivalence remain UNKNOWN.

Patch **0010** makes the three reviewed warning-only exports `RtlCaptureContext`, `RtlUnwind` and `__C_specific_handler` raise explicit unsupported host exceptions instead of returning to guest callers with stale outputs/disposition. Catching fixtures confirm unchanged sentinels and an exceptional exit; isolated uncaught fixtures terminate with SIGABRT. This is a fail-closed boundary, not guest context capture, dispatch, handler execution or unwinding. Other exception paths, including `RtlRaiseException`/`RtlRaiseStatus`, retain their prior incomplete/debug/stub limitations and require a reviewed entry/import policy before execution.

## Final regression evidence

Final eleven-patch clean SDK builds on macOS ARM64, Apple Clang 17.0.0 (`clang-1700.6.4.2`), CMake 3.31.6 and Ninja `1.11.1.git.kitware.jobserver-1`:

| SDK profile | Build seconds | Peak build RSS bytes |
| --- | ---: | ---: |
| Debug / O0 | 107.009 | 581,304,320 |
| Release / O3 | 139.816 | 505,118,720 |
| Sanitized / O1, ASan+UBSan | 228.546 | 885,882,880 |

Configure took 20.682 seconds; two compile jobs were allowed. RSS is wait4 accounting, not the sum of simultaneous process-tree residents. C-family SDK sources are instrumented in Sanitized with no recovery; system frameworks and handwritten assembly are not instrumented. Existing upstream FPCR operand-width compiler warnings remain visible; no warning-free toolchain claim is made.

| Final fixture profile | Passing groups | Passing fault-path checks | Expected signal-terminated children | Unexpected failures / timeouts | Child-seconds including compilation | Peak execution RSS bytes |
| --- | ---: | ---: | ---: | --- | ---: | ---: |
| Debug SDK / O0 fixtures | 41 | 23 | 19 | 0 / 0 | 31.136 | 35,078,144 |
| Release SDK / O2 fixtures | 41 | 23 | 19 | 0 / 0 | 32.019 | 22,528,000 |
| Sanitized SDK / O2 sanitized fixtures | 41 | 22 | 18 | 0 / 0 | 32.707 | 62,210,048 |

Each profile combines 37 unified Phase 5–7 groups with 4 Phase 8 groups. The latter contain the 32,961-case arithmetic oracle, 7,658-assertion XDBF corpus, **480 real allocation/ownership/failure assertions**, and explicit raw-vector context test. Direct heap tests also reject overflowing size/alignment, one-past/cross-end fixed ranges and full bounded-range exhaustion. This ensures the SystemHeapAlloc front-end guard is not the only checked allocator path.

Totals: **123 passing groups + 68 passing fault-path checks = 191 core checks**, plus **6 passing watchdog regressions**. Of the fault-path checks, 56 children terminate by the expected signal (44 SIGBUS, 3 SIGSEGV, 9 SIGABRT); the remaining 12 are controlled recovery/diagnostic exits, including explicit nested-fault exit 139. These are child/test counts, not a count of every faulting load/store. Sanitized omits only the independent POSIX-default signal control owned by ASan, as documented in Phase 7. Final runs have **zero unexpected faults, zero timeouts and zero ASan/UBSan findings**.

There are **9 expected negative typed-vector compiler observations** across the three profiles, and **3 host-page-slack BLOCKED observations**. Neither is counted as passing ABI/isolation functionality. The longest supervised child in the final core matrix was 2.734 seconds, below its 30-second limit. Peak child RSS including compilers was 349,847,552 / 335,986,688 / 390,889,472 bytes. Profile runs overlap; summed child time is not elapsed project time or game performance.

Additional historical regression runs pass **9 component/source-comparison groups per profile (27 total)**. The legacy alias-safe VMX characterization runs **212,068 synthetic invocations per profile** with the prior infinity/overflow/NaN disagreements preserved and hardware equivalence UNKNOWN. The new actual-codegen alias fixture adds 3,084 synthetic invocations per core profile. These supplementary runs are reported separately rather than inflating the 191 core count.

Final private loader-only checks pass Debug, Release and Sanitized, require `xdbf_valid=true` / status kOk, recognize all **274 imports with zero unresolved**, and never call entry preparation/launch. Execution times are 0.515 / 0.503 / 0.474 seconds; peak RSS is 65,880,064 / 53,641,216 / 98,271,232 bytes. This closes the characterized XDBF sanitizer load failure within this host probe; it does not execute guest functions. All 261 baseline function-shim symbols exist in each new runtime, with zero missing. A complete generated-game link remains untested.

Runtime SHA-256 values:

| Profile | SHA-256 |
| --- | --- |
| Debug (`librexruntimed.dylib`) | `4fda3f2bbc8ea30b5b96afbb6d7f4081cbac990b24668fedc1f9acc44d259079` |
| Release (`librexruntime.dylib`) | `72f5e56c1cdcf328fea9887e4b7691d3576bc6015a68a9dd45d7d7372a386fba` |
| Sanitized (`librexruntime.dylib`) | `cc08d12976c7c0f327c005e718e5168b04215e47c6c47c61ae6ae2c555cb580f` |

**Phase 8 safety regression milestone: complete within the stated host contract.** The runtime compatibility and Phase 9 execution gates below remain open.

Every dangerous child has the external 30-second process-group watchdog. Expected SIGBUS/SIGABRT, controlled diagnostic exits, compiler-negative observations and unexpected failures are reported separately. The saved pre-fix SIGBUS is retained as a resolved regression history, never removed from the record.

The unified runner exercises real exports, Memory, MMIO, dispatcher, XThread/PCR/TLS and codegen from the newly built SDK. Historical fake leaf source comparisons run separately as negative controls against pristine pinned SDK files and patch 0001; they do not load an old SDK library or substitute for coherent integration. The six standalone foundation groups are intentionally repeated inside the unified matrix. Legacy VMX numeric characterization uses the current pinned alias-patched source and new SDK headers; special-value disagreements remain documented, not hidden or promoted to hardware truth.

## Phase 9 readiness gates

| Gate | Decision | Evidence boundary |
| --- | --- | --- |
| Allocation overflow | PASS after final regressions | Checked arithmetic, real SDK failure propagation and preserved valid allocations |
| XamFree ownership | PASS for tested host lifecycle | Foreign/double/interior/stale-to-foreign releases rejected; raw-pointer ABA/save-state limits remain |
| XamAlloc Xbox status compatibility | UNKNOWN | Explicit host failure is tested; Xbox failure values are not established |
| XDBF parsing | PASS for bounded immutable SPA scope | Synthetic alignment/truncation/adversarial corpus; other dialects/nested formats unclaimed |
| Guest memory ownership/isolation | PARTIAL | Tested page-sized denial and logical range checks pass; raw host-page slack remains accessible |
| Typed-vector ABI | FAIL / unsupported | Compile-time rejection; raw-context fixtures do not establish this ABI |
| Exception failure behaviour | PARTIAL | Three exports fail closed; complete guest dispatch/unwind and other stubs remain incomplete |
| Runtime error propagation | PARTIAL | Tested paths are observable; a production guest-entry failure boundary is not validated |

**Phase 9 NO-GO for proprietary execution.** The exact next milestone is checked guest-access integration and a fail-closed entry/import contract review, using original fixtures: address the 4 KiB/16 KiB access boundary, define which unsupported ABI/exception paths are unreachable or explicitly refused, and establish needed Xbox error contracts. A later, explicit readiness decision must precede any private generated-code trace. No game execution, boot or Switch development follows automatically from this milestone.

## Publication boundary

Only original GPL-3.0-only fixtures/tooling/documentation, the dependency/patch lock and narrowly scoped BSD-3-Clause SDK patches are intended for Git. The SDK/dependencies themselves are not vendored. Publication checks cover the complete intended working tree and local links after staging; private binaries/logs, absolute local paths, credentials and generated game-derived code are excluded. Original diagnostic artifacts remain private. No GitHub Release is created.
