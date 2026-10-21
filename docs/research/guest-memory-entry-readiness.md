# Guest memory and entry readiness — Phase 9

2026-10-21 · baseline `v0.0.8-pre-entry-safety` (`30ba5dfcf64a9d995e3ddc4b40ed55ca613fa512`). **PARTIAL implementation coverage; NO-GO for any proprietary execution.** This milestone validates original host fixtures and audits existing private output statically. No game-derived function, entry point, callback or boot is executed; no full game compilation, graphics/audio or Switch integration occurs.

## Preserved baseline and isolated SDK

The unmodified base `default.xex` SHA-256 is **VERIFIED**:
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
No title update is applied. XEX, manifest and 1,453 generated files are fingerprinted by SHA-256, size and nanosecond mtime before/after. Extracted assets, earlier patches and installed SDK remain private and unchanged.

ReXGlue **v0.10.0**, `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, is rebuilt in a separate private workspace. [entry-readiness-lock.json](../../tools/sdk/entry-readiness-lock.json) preserves the eleven Phase 8 patches and adds two BSD-3-Clause patches. Matching runtime/codegen headers/libraries are staged for Debug/O0, Release/O3 and Sanitized/O1 (ASan+UBSan, no recovery). Original optimized fixtures use O2. Apple Clang 17.0.0, macOS ARM64; system frameworks and handwritten assembly are not fully instrumented. Historical locks remain unchanged. No global SDK files are patched.

## Checked access contract

[The original adapter](../../runtime/sdk_checked_memory.h) owns opaque allocation handles backed by actual SDK virtual-heap reservations. Patch 0012 adds `BaseHeap::CopyOwnedMemory`; the adapter routes all its supported reads/writes through it. This is integration into the real allocator, not the separate Phase 5 arena.

**VERIFIED supported subset:**

- 4-KiB ordinary virtual guest pages, independently of 16-KiB host permissions. Adapter allocations use host-aligned reservation groups, enforce the exact requested byte size and bound allocation size to 1 MiB, retained lifetime records to 4,096 and each transfer to 64 KiB.
- Subtraction/wide range checks precede narrowing; empty, overflowing and out-of-object spans are rejected. The SDK additionally checks guest address-space and heap bounds.
- Each page must belong to the exact owner-tagged reservation and be reserved, committed and readable; writes additionally require guest write permission. All pages are checked before any byte is copied. Rejected reads leave outputs unchanged; rejected writes cannot partially alter earlier pages.
- Global-before-heap lock ordering and the heap lock cover validation and copying against ordinary allocation, release, decommit and protect operations. The adapter serializes its lifecycle/read/write operations. Distinct retained tokens identify reservation lifetimes; stale handles cannot authorize a replacement reservation, even when addresses repeat.
- Host-buffer copies expose no host pointer into guest backing. Big-endian 32-bit helpers use explicit bytes; arbitrary byte copies preserve their input order.
- Masked MMIO is refused without invoking device callbacks. The complete intersecting host pages are scanned, including padding outside the requested range: MMIO protection is applied to whole host pages. This is deliberately conservative and bounded, with a throughput cost.

**BLOCKED fetch:** the pinned guest page table has read/write/cache flags but no execute-permission bit. Fetch requests validate range, ownership and page accessibility, then return `unsupported_fetch` without copying. Read permission is never treated as execute permission. No instruction fetch or code dispatch is authorized.

**Unsupported contracts:** physical aliases/watch callbacks, XEX/large-page heaps, raw host buffers aliasing guest backing, direct page-table mutation, runtime reset/restore/teardown during access, concurrent MMIO registration and persisted handles. Host buffers must be valid, disjoint host-owned spans whose lifetime is maintained by the caller. The SDK object must outlive the adapter; adapter destruction must be quiescent. SDK configuration is trusted. Arbitrary external `mprotect`/unmapping is not mediated. Ownership records are not serialized. The bare SDK token API trusts internal callers; only the adapter provides private per-allocation tokens and byte bounds.

**PARTIAL ownership:** this does not solve same-owner ABA for ordinary XAM pointers, low-level object-table handle reuse, object lifetimes in other subsystems, unmediated guest pointers or save-state restoration. ASan/UBSan are not race detectors; concurrent fixtures demonstrate selected serialization outcomes, not exhaustive concurrency correctness.

## Coverage and cross-page protection

| Access path | Coverage / finding |
| --- | --- |
| Adapter read/write/load32be/store32be | VERIFIED mediated: each supported operation reaches the SDK locked copy |
| Adapter fetch | BLOCKED: explicit unsupported result, no copied bytes |
| 4-KiB RO beside RW in a 16-KiB host page | VERIFIED checked write denied; neighboring writable page remains usable |
| RW→RO, RO→RW, committed→uncommitted spans | VERIFIED full-span rejection; no partial write or output modification |
| Free, foreign manager, raw release plus foreign reuse, adapter release plus replacement | VERIFIED owner/lifetime rejection within this adapter |
| Concurrent readers/writers, real SDK Protect and adapter release | VERIFIED selected bounded operations; allowed outcomes checked |
| SDK `TranslateVirtual`, `TranslateRelative`, `GuestPtr`, mapped/ppc pointer marshalling | UNMEDIATED; pointer conversion is not a lifetime/bounds check |
| Existing memory/string/critical-section/import implementations | UNMEDIATED except calls explicitly made through the adapter; no automatic interposition |
| Generated loads/stores, indirect calls, stack argument slots, atomics, vector accesses | UNMEDIATED; no game output rewritten or regenerated |
| MMIO device operations, physical views, XEX fetch and Horizon mappings | BLOCKED/UNKNOWN for this layer |

Static counts are **VERIFIED lexical observations**, not executed coverage: all 48,371 generated bodies contain **464,272 load + 327,081 store sites** (791,353 total), plus **22,203 indirect-call sites**. The direct entry closure contains **778 load + 915 store sites** (1,693), plus **21 indirect-call sites**. **Zero existing generated sites are mediated.** That denominator excludes helper-internal accesses, pointers, atomics and memory accesses not expressed by those macros. It cannot establish a complete access census or dynamic percentage.

## Static entry dependency matrix

[The static auditor](../../tools/audit_entry.py) reads the original private output only. Three original parser tests exercise cycles, comments, unreachable calls, absent entry and duplicate definitions. The existing manifest/metadata maps `xstart` to entry `0x827F1498`. A literal direct-call closure contains **141 nodes: 108 generated functions and 33 imported functions**, with no missing literal symbols. All 721 emitted body partitions are scanned. Branch feasibility is not evaluated; 21 indirect sites, callbacks and table-driven exceptions remain unresolved. The closure overestimates feasible direct paths and underestimates dynamic reachability; it is not an execution trace or a complete dependency graph.

| Dependency | Confirmed evidence | Conservative requirement / gate |
| --- | --- | --- |
| Memory image | VERIFIED metadata: base `0x82000000`, loaded allocation `0xD70000`; prior loader-only success | PARTIAL: relocations/section permissions/import variables and raw access coverage must be validated for the chosen scope |
| Stack/TOC | VERIFIED header stack `0x40000`; existing fixtures exercise aligned r1 and r2 preservation | UNKNOWN title TOC/r2 and actual entry stack values; no launch preparation here |
| TLS/PCR | VERIFIED header: 64 TLS slots, 28 raw bytes; four TLS imports in closure | PARTIAL: real XThread, bound ThreadState, PCR/r13, title TLS initialization and teardown are required; synthetic fixtures are not title initialization |
| Callbacks/dispatch | VERIFIED 21 indirect sites in closure; selected original nested callbacks pass | UNKNOWN targets, actual callback signatures, TOC/FPSCR/dispatch requirements |
| Exceptions | VERIFIED direct closure includes RtlRaiseException; prior scan found context/unwind calls elsewhere and 106 shared SEH-funclet functions | BLOCKED guest dispatch/unwind; failure paths must not continue after host rejection/debug breaks |
| Filesystem/STFS | VERIFIED no literal filesystem/STFS import in this direct closure; prior module loader uses host VFS | UNKNOWN indirect/later file/content requirements; STFS exports reject with NOT_IMPLEMENTED, not functional support |
| Vector ABI | VERIFIED generated functions use context/base signatures; no typed ABI inference follows from that | BLOCKED typed vector argument/return/mixed marshalling; indirect signatures remain UNKNOWN |
| Import status | VERIFIED 33 literal imported dependencies inspected below; 274 ordinals/261 function symbols recognized historically | PARTIAL: binding does not prove semantics; all required signatures/errors must be validated before entry |

## Import and error contracts

All 33 direct imports are covered by the following source audit against the pinned SDK plus locked patches. “Observed” means an original bounded fixture, not hardware ABI conformance. Pointer arguments in these exports still use unchecked SDK marshalling. No unsafe argument is deliberately passed to them; adversarial memory cases use the checked layer instead.

| Import | Arguments / result / error and unsupported behavior | Evidence |
| --- | --- | --- |
| DbgPrint | r3 format pointer, variadic stack/GPR formatting; null→success; non-null format parsing uses raw pointers and unconditional status success | VERIFIED null case; PARTIAL formatting/error behavior |
| ExGetXConfigSetting | u16 category/setting/size, raw buffer, optional u16 required-size output; helper status propagated, required-size initialized to zero | VERIFIED source; PARTIAL selected settings, no complete output/bounds proof |
| HalReturnToFirmware | u32 routine; asserts reboot case then exits host with 0 | VERIFIED source; BLOCKED safe entry termination/observable guest outcome |
| KeBugCheck | u32 code, forwards bug check | VERIFIED source; BLOCKED debugger/assert termination contract |
| KeBugCheckEx | code + four u32 values; debug break/assert | VERIFIED source; BLOCKED guest fatal error reporting |
| KeDelayExecutionThread | mode, alertable, raw signed interval; current XThread and optional APC delivery; Delay status returned | VERIFIED source; PARTIAL wait/APC/callback behavior |
| KeGetCurrentProcessType | no args; dereferences bound XThread/PCR/r13/current-thread record | VERIFIED source; PARTIAL valid initialization, unsafe without binding |
| KeQueryPerformanceFrequency | no args; guest clock frequency narrowed to u32 | VERIFIED source; PARTIAL Xbox width/clock contract |
| KeTlsAlloc | no args; returns slot or OUT_OF_INDEXES, zeroes current slot | VERIFIED selected real-XThread lifecycle/exhaustion fixtures |
| KeTlsFree | slot; returns actual free boolean through patched FreeTLS | VERIFIED invalid, repeated and valid lifecycle fixtures |
| KeTlsGetValue | slot→u32; initializes local output, invalid→0 (ambiguous with valid zero) | VERIFIED selected bounds/isolation; PARTIAL complete Xbox semantics |
| KeTlsSetValue | slot,value→boolean; patched thread bounds/wrap checks | VERIFIED selected bounds/isolation; PARTIAL complete Xbox semantics |
| NtAllocateVirtualMemory | base/size output pointers, type/protection/flags; explicit existing statuses and allocation failure; raw output marshalling | VERIFIED selected lifecycle/errors; PARTIAL all flag combinations/pointer contracts |
| NtClose | u32 handle→object-table status; patch 0013 rejects empty or exhausted entries before decrement | VERIFIED valid, double and concurrent close; PARTIAL handle reuse/pseudo-handles/all object types |
| NtCreateEvent | output handle, optional attributes, type/state; success/name-exists writes handle; invalid-type collision error leaves output unchanged | VERIFIED unnamed event lifecycle; PARTIAL names/OOM/invalid pointer behavior |
| NtFreeVirtualMemory | base/size pointers,type,unknown flags→status; heap/lifetime checks on selected paths | VERIFIED selected release/error fixtures; PARTIAL all flag/pointer cases |
| NtQueryVirtualMemory | address, 28-byte output, region type; invalid type/absent heap return INVALID_PARAMETER before writing; success fills fields | VERIFIED those error sentinels and committed RW result; PARTIAL all regions/pointers |
| NtWaitForSingleObjectEx | handle,mode,alertable,optional timeout; invalid handle→INVALID_HANDLE; waits/APCs otherwise | VERIFIED closed-handle failure only; PARTIAL blocking/APC semantics |
| RtlCompareMemoryUlong | source,length,pattern; unaligned→0; mapped `rex::be<u32>` array, counts matching words; the source FIXME is stale because the actual `as_array` type is endian-aware | VERIFIED source; PARTIAL counting/compatibility until independent oracle |
| RtlEnterCriticalSection | raw structure; current XThread, atomic lock/spin/wait | VERIFIED source; PARTIAL guest atomic/wait/register contracts |
| RtlImageXexHeaderField | raw header pointer,field→value; initialized local zero, helper result not propagated separately | VERIFIED source; PARTIAL invalid headers/error ambiguity |
| RtlInitAnsiString | raw destination/source; source strlen narrows to u16; null source resets 8-byte structure | VERIFIED null output; BLOCKED unchecked/unbounded non-null input |
| RtlInitializeCriticalSection | raw structure→void; fills SDK lock fields | VERIFIED source; PARTIAL pointer/ownership contracts |
| RtlInitializeCriticalSectionAndSpinCount | raw structure,spin→SUCCESS; `(spin+255)` can wrap before saturation | VERIFIED source; PARTIAL range/compatibility, no speculative correction |
| RtlLeaveCriticalSection | raw structure; ownership assertions, decrements/wakes | VERIFIED source; PARTIAL wrong-owner/error behavior |
| RtlNtStatusToDosError | u32 status→mapped error/fallback 317 | VERIFIED source; PARTIAL exhaustive mapping/compatibility |
| RtlRaiseException | raw exception record; thread-name and C++ branches, general unwind TODO, debug break | VERIFIED source and static dependency; BLOCKED guest exception/unwind correctness |
| XGetAVPack | no args→constant 6 | VERIFIED source; PARTIAL actual AV policy |
| XGetLanguage | no args→English from hard-coded region | VERIFIED source; PARTIAL title/user-region policy |
| XamLoaderTerminateTitle | no args→TerminateTitle, no ordinary return promised | VERIFIED source; PARTIAL callback/thread teardown/observable termination |
| XamShowMessageBoxUIEx | zero-argument stub warns once and returns 0, no real outputs/completion | VERIFIED source; BLOCKED false-success; actual Xbox signature/output obligations UNKNOWN |
| XexCheckExecutablePrivilege | u32 bit→boolean, absent module→0; shift occurs before module check and is undefined for bit≥32 | VERIFIED absent-module cases 0/6/31; BLOCKED unvalidated indices, title metadata cases UNKNOWN |
| XexGetModuleHandle | optional module string + output; missing→NOT_FOUND and zero output, success writes unretained handle pointer | VERIFIED absent executable/missing synthetic name; PARTIAL string/output/lifetime handling |

Source definitions are in `src/kernel/xboxkrnl/{xboxkrnl_strings,xboxkrnl_xconfig,xboxkrnl_hal,xboxkrnl_debug,xboxkrnl_threading,xboxkrnl_memory,xboxkrnl_ob,xboxkrnl_rtl,xboxkrnl_error,xboxkrnl_modules}.cpp`, `src/kernel/xam/{xam_info,xam_ui}.cpp` and `src/system/util/object_table.cpp` of the pinned SDK. [Pinned upstream source tree](https://github.com/rexglue/rexglue-sdk/tree/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src). [Earlier complete import audit](runtime-audit.md) retains symbol/ordinal provenance. Baseline IMPLEMENTED/PARTIAL/STUB labels are historical source categories, not current ABI certifications.

**VERIFIED new defect and limited correction:** the first new import fixture failed at assertion 80 when a second NtClose did not return INVALID_HANDLE. Source review shows LookupTable can return a slot whose object is null; ReleaseHandle decremented its zero count and returned success. Patch 0013 extends the existing invalid-handle guard to null objects/nonpositive counts, under the existing global lock. It uses an already-established SDK status, not an invented Xbox value. Final single/double/concurrent close checks must pass on the unified build. The initial exit-1 diagnostic is retained privately; it was not a host fault or timeout. Other object-table operations and stale handles after slot reuse are not thereby validated.

## ABI readiness

| Contract | Status / supported evidence |
| --- | --- |
| Selected GPR32/GPR64/return | VERIFIED original fixtures and real selected exports; no complete hardware ABI claim |
| GPR spill/64-bit stack values | VERIFIED selected original slots through patched translator; mixed/classification/variadic behavior PARTIAL |
| FPR double f1/f2 and return | VERIFIED selected original fixture; mixed/overflow/FPSCR behavior UNKNOWN |
| r1 alignment, r2/r13, preservation | VERIFIED synthetic aligned stack, sentinels, bound PCR and selected nested callbacks; title TOC/actual launch values UNKNOWN |
| XThread/TLS | VERIFIED synthetic per-thread isolation, slot lifecycle and patched failures; title initializer/APC/fiber behavior PARTIAL |
| Typed vectors (args/returns/mixed) | BLOCKED expected compiler failures; raw-context vector fixture is not typed-vector ABI support |
| Callback address dispatch and exceptions | BLOCKED/UNKNOWN for game targets, table handlers and unwind |

There is no proposed safe game-entry subset: its already observed memory accesses bypass the layer and its import/exception/dynamic-call contracts are incomplete. The minimum supported experiment remains original host-owned fixture allocations and explicitly tested scalar shim signatures, with no guest address dispatch.

## Regressions and measurements

Final measurements are recorded below after all thirteen patches are built and verified. Native memory fault children retain the existing 30-second external process-group watchdog; expected fault/recovery outcomes are counted separately from ordinary passing groups. Raw MMIO/signal tests are synthetic. No known failing operation is retried without a bounded fixture and a correction or explicit diagnostic purpose.

| Final profile | Passing groups / failed | Expected fault/recovery checks / failed | Execution seconds (sum of children) | Maximum execution-child RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 43 / 0 | 23 / 0 | 12.297323 | 34,766,848 |
| Release | 43 / 0 | 23 / 0 | 6.524311 | 22,265,856 |
| Sanitized | 43 / 0 | 22 / 0 | 12.507125 | 62,013,440 |

**VERIFIED final core:** 129 passing groups + 68 expected fault/recovery checks = **197**, zero unexpected faults, zero timeouts, zero ASan/UBSan findings. There are 56 expected signal-terminated children: 44 SIGBUS, 3 SIGSEGV and 9 SIGABRT; the other 12 checks recover or exit with their specified diagnostic status. Sanitized excludes the single plain default-signal case owned by ASan, as in Phase 8; no test silently becomes a success. Nine negative vector compile observations and three accessible raw-host-slack observations remain BLOCKED features, outside the passing feature count.

Per profile: the existing unified suite contributes 37 groups; pre-entry contributes four (including 32,961 arithmetic cases, 7,658 XDBF assertions and 480 runtime assertions); Phase 9 contributes two groups with 2,074 memory assertions (2,000 are benchmark-loop success checks), 83 import assertions, 6,400 scheduled concurrent operations and eight selected real exports. Counted operations are not unique behavior coverage.

Separately, **27 historical component/source-control groups**, **three VMX characterization suites** (212,068 original synthetic invocations each), **six watchdog checks** and **three static-audit unit tests** pass. Source-control/VMX inputs retain the exact fingerprints used before the final internal handle correction; these consume no old SDK runtime. VMX special-value hardware equivalence remains UNKNOWN. All 261 imported function symbols are present in each final library, without proving their behavior.

| Clean thirteen-patch build | Seconds | Peak build-child RSS bytes | Runtime SHA-256 |
| --- | ---: | ---: | --- |
| Debug | 100.433785 | 590,462,976 | `bbf224a9dac5b6e3fc0db7dc706a2c7051960c625a3413b4c5864966f44d4e6f` |
| Release | 129.099061 | 518,471,680 | `be8b648b071cc383febc481a087721f1f29f58b02ec280ea23679fb64f4e65de` |
| Sanitized | 223.692987 | 847,101,952 | `192e8a4e3ec3cfe8ffaba66d48faed301d5e4f180de97f852ac5ced5da8ec9c0` |

The clean builds follow exact pristine-plus-ordered-patch verification. Each build has a 1,200-second external ceiling; two parallel compiler jobs are used. Existing unused-parameter/ARM FP-asm warnings remain visible in private logs; they are not evidence of full FP correctness.

| 2,000 original 8-byte timing iterations | Checked reads, total ns | Host-local copy reference, total ns |
| --- | ---: | ---: |
| Debug | 685,782,083 | 7,542 |
| Release | 38,623,709 | 2,500 |
| Sanitized | 3,041,255,542 | 2,208 |

These are single wall-clock samples including validation/oracle work, optimization, scheduling and possible watchdog sampling/parallel-build interference. The local copy can be optimized heavily and is not an equivalent guest workload. They establish substantial diagnostic overhead, not a stable slowdown ratio, throughput promise or game performance. No timing threshold is treated as a correctness test.


No sanitizer finding is suppressed. Negative typed-vector compilations and raw host-page slack are retained as BLOCKED observations rather than passing features. The initial import failure above and earlier development runs are excluded from the final pass count, not hidden or deleted. One orchestration attempt requested the Sanitized SDK before staging completed: it exited 1 on a missing metadata file before compiling/executing a test; its empty output directory and diagnostic were retained separately, and staging readiness was required before resuming. RSS is per-child peak, not aggregate process-tree/game memory. Timing is a local sample, not Switch performance. MMIO host-page scans deliberately add significant overhead; the API is a diagnostic safety boundary, not a production CPU-memory backend.

## Reproduction and publication

[New fixture commands](../../tests/guest-memory/README.md) and [isolated builder](../../tools/sdk/README.md) specify fresh private outputs and the explicit Phase 9 lock. After building, repeat these against each matching profile (add `--sanitize` only for Sanitized):

```sh
python3 tests/unified/run.py --sdk "$SDK" --source "$SDK_SOURCE" \
  --workdir "$PRIVATE/unified-$PROFILE" --pre-entry
python3 tests/pre-entry/run.py --sdk "$SDK" --workdir "$PRIVATE/pre-entry-$PROFILE"
python3 tests/runtime/run.py --sdk-source-root "$PRISTINE_SOURCE_CONTROLS" \
  --workdir "$PRIVATE/components-$PROFILE" --optimization "$OPT"
python3 tests/vmx128/run.py --sdk-source "$SDK_SOURCE/src/codegen/builders/vector.cpp" \
  --sdk-include "$SDK/include" --workdir "$PRIVATE/vmx-$PROFILE" \
  --alias-safe --optimization "$OPT"
python3 tests/ppc/test_watchdog.py
python3 tests/guest-memory/test_audit.py
```

`PRISTINE_SOURCE_CONTROLS` contains only the four hash-pinned files described by the historical runtime runner. Those leaf comparisons are source controls, not a second SDK loaded into the unified integration tests. The VMX corpus uses original operands; no game-derived code is compiled. Phase 8 loader-only results remain historical; this phase does not repeat XEX loading or entry preparation.

**VERIFIED publication review:** all 21 changed files were reviewed. The index contains 97 explicitly admitted public text files (86 previous + 11 new); local-link, text/binary/size, private-path/credential-pattern and all three patch-lock checks pass. Changed content is original adapter/audit/test source, two retained-notice SDK patches, a dependency lock, CI metadata and documentation. No XEX, asset, generated game-derived C++, private path, raw log or temporary artifact is staged. Useful diagnostics, the initial import failure and the orchestration error stay private. The 1,455 original XEX/config/generated fingerprint records and all 299 extracted file hashes match; all eleven old patch hashes and the installed SDK library hash are unchanged. This bounded text/allowlist review is not a universal copyright or secret-detection proof. No GitHub Release is created.

## Decision and next exact milestone

**NO-GO for proprietary execution**, including a narrow entry trace. Critical coverage gaps (zero mediated generated sites), unsupported fetch, unverified required imports, raw pointer/exception/dispatch behavior and typed-vector ABI prevent the readiness gate from passing. Successful original fixtures and code generation do not establish game execution or playability.

**Phase 10 / R2j: original generated-access mediation and fail-closed dispatch policy.** First build original scalar/stack/vector/atomic/indirect-call emit fixtures routed through a checked backend; reject unsupported fetch/dispatch/import signatures explicitly. Establish executable-section authorization from validated metadata and an allowlist of independently tested import contracts. Audit every remaining access path and lifetime; keep exception/unwind and title TLS/TOC failures observable. Only a later review, with no unresolved critical defect in a defined scope and external watchdogs, can reconsider private execution. No game execution, full game compile, graphics/audio or Switch work is authorized by this milestone.
