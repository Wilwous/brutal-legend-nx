# PPC execution and host-fault diagnostics — Phase 6

2026-10-20 · baseline `v0.0.5-runtime-foundation` (`9a45188fe8897c393c4098ab7e02b3fc9e571540`) · result **PARTIAL**. Original PPC fixtures call real ReXGlue implementations. No game entry, XEX load, game compile, renderer, audio integration or Switch test occurs in this phase.

## Preserved inputs and tool identity

**VERIFIED:** before testing, the original `default.xex` SHA-256 matched `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`. The XEX, Phase 3 manifest and 1,453 generated files were fingerprinted before/after (1,455 records: SHA-256, size, nanosecond mtime). All remain identical. No update was applied and no game-derived source was regenerated or published.

ReXGlue SDK: **v0.10.0**, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. Prebuilt macOS ARM64 `librexruntime.dylib` SHA-256: `645325a2372ca77111635d613e3692b766012c3125f9dbc0c05f593b5a973e00`. Apple Clang 17.0.0, clang-1700.6.4.2, C++23; macOS ARM64. The runner pins six upstream source files by SHA-256, copies them outside Git and checks/applies both published patches there. It never writes installed headers/libraries. [Runner and commands](../../tests/ppc/README.md).

## Host-fault root cause

**VERIFIED:** the Phase 5 hang is a repeated, unclaimed synchronous fault in the SDK POSIX signal dispatcher. It is not evidence of failed page protection or memory pressure.

1. A normal read-only `mmap` page rejects the original fixture's store with **SIGBUS (10)** on this host. A separate signal-capture child records `si_code=1`, fault address and host PC using async-signal-safe `write`, then exits **80**, the explicit expected-fault capture code.
2. Installing an SDK observer that returns `false` reproduces the loop even without Memory or Runtime initialization. Its first three records contain the same fault address, same PC and write operation (2).
3. With real SDK Memory initialized, protecting a system-heap allocation read-only reproduces the Phase 5 path: guest `0x30000000`, host `0x330000000`. The MMIO/access-violation callback declines this virtual-heap access and logs it; the POSIX callback returns without changing the faulting state or terminating. The same store retries.
4. External samples show the main thread running through `_sigtramp`, `ExceptionHandlerCallback`, MMIO, `Memory::AccessViolationCallback` and logging; the timer thread sleeps. `ps` reports runnable state `Rs`, rather than a mutex wait. Low RSS and approximately one busy CPU support the retry diagnosis.

The [pinned POSIX dispatcher](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/core/exception_handler_posix.cpp) falls through after all handlers decline. The [memory callback](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/system/xmemory.cpp) explicitly returns false for an unhandled virtual-heap access. This is a runtime signal-dispatch defect; the old supervisor also lacked group cleanup and compact diagnostics.

| Initial diagnostic child | Outcome | Monotonic seconds | Peak RSS bytes | CPU, user+system / elapsed |
| --- | --- | ---: | ---: | ---: |
| Plain POSIX store | Expected SIGBUS | 0.731626 | 10,223,616 | 8.50% |
| POSIX capture | Expected fault, explicit exit 80 | 0.026867 | 9,666,560 | 32.92% |
| SDK observer declines | TIMEOUT, group SIGKILL | 30.000835 | 9,666,560 | 100.35% |
| SDK Memory + observer declines | TIMEOUT, group SIGKILL | 30.003829 | 20,054,016 | 99.33% |

Only those two deliberate 30-second timeout reproductions were needed. They are diagnostic outcomes, not protection-test passes. Private samples, register/address records and process snapshots are retained; raw logs and private paths are not published. The first Memory diagnostic exposed a large logging flood; only its first 64 KiB was retained. The final supervisor continuously drains both streams, counts all bytes and retains at most 64 KiB per stream, so logging cannot fill a pipe or consume unbounded disk.

**VERIFIED limited correction:** patch `0002-host-fault-and-protect.patch` makes the isolated dispatcher reset an unclaimed signal to `SIG_DFL` and re-raise it. O0/O2 children then terminate with SIGBUS, not timeout. The fixture's claimed recovery handler sets the resume PC to an original assembly label immediately after its store and returns `true`. Both original and patched SDK dispatchers restore that PC and return successfully; exactly one fault occurs and the read-only page still contains its original byte. No global memory protection is disabled. This tests host PC recovery, not guest exceptions or arbitrary native register restoration.

## Original PPC harness and verified exports

`tests/ppc/execution.cpp` constructs Runtime in tool mode with an empty fixture directory, actual Memory/KernelState and real `PPCContext`. It calls these exported shims directly with `(context, virtual_membase)`. Runtime export metadata verifies each name/type/ordinal, and `dladdr` verifies whether the called shim belongs to the prebuilt library or isolated executable.

| Exports | Decimal ordinal | Tested contract |
| --- | --- | --- |
| RtlUpperChar / RtlLowerChar | 330 / 306 | Selected ASCII argument and integer return |
| RtlFillMemoryUlong | 294 | Mapped guest pointer, 16-byte fill, big-endian pattern and guard bytes |
| RtlCompareMemory | 282 | Equal buffer and one-byte mismatch counts; this describes observed SDK behavior |
| NtAllocateVirtualMemory | 204 | Reserve+commit, 4 KiB alignment, 64 KiB rounding, zero fill, zero-size/bad-type errors and unchanged failure outputs |
| NtProtectVirtualMemory | 225 | Read-only/read-write lifecycle and old-protection output |
| NtFreeVirtualMemory | 220 | Release and subsequent free-region state |
| KeTlsAlloc / KeTlsFree | 338 / 339 | Allocation, exhaustion, free, reuse, cross-thread clearing for valid slots |
| KeTlsGetValue / KeTlsSetValue | 340 / 341 | Zero initialization, values, isolation and patched invalid-slot failure |
| StfsCreateDevice / StfsControlDevice | 601 / 602 | Baseline false-success reproduced; patched NOT_IMPLEMENTED, unchanged output sentinel |

**VERIFIED limited register contracts:** each integer/void shim call snapshots the state after installing arguments. All GPRs except result r3, all 32 FPRs, all 128 vector registers, LR, CTR, MSR and eight CR fields are checked for preservation. Distinct deterministic register patterns are used. The ordinary fixture has an aligned allocated r1 stack and r2/r13 sentinels. Threaded TLS calls instead use the real bound thread context and PCR. These tests do not execute translated PPC instructions.

Two original functions also exercise the SDK's header marshaller: ten 32-bit integer arguments (r3–r10, then big-endian stack slots r1+0x54/r1+0x5c), and two double arguments in f1/f2 with a double result in f1. These functions are **not Xbox exports**. They confirm this SDK marshaller's selected layout, not independently validated hardware ABI conformance. The tested SDK exports do not take FP/vector arguments.

**PARTIAL / UNKNOWN:** no title TOC/r2 initialization, auto-isolating callback wrapper, 64-bit integer argument ABI, variadic calls, FPR overflow/stack passing, vector parameters/results, FPSCR rounding/exception semantics, atomics/reservations, guest indirect dispatch, fibers, APCs or guest unwind is validated. In particular, preserving an r2 sentinel during a direct shim call does not validate callback TOC propagation. `TranslateVirtual` remains an unchecked translation primitive, not an ownership/bounds validator. Guard-byte tests only cover explicitly allocated fixture ranges.

## Memory, TLS and lifecycle

**VERIFIED:** memory operations above execute through actual PPC export shims and actual SDK heaps. Failure status is checked explicitly; no failed fixture operation is treated as success. Native protected writes are confined to fault children.

**VERIFIED:** two original `ExecutionThread` bodies derive from real SDK XThread and use its unmodified Create/bind/stack/PCR/Exit lifecycle. They are marked guest threads so the SDK's process-wide FreeTLS loop includes them; their bodies contain only original host fixture calls. No guest address is dispatched. SDK XHostThread is marked non-guest, so it is unsuitable for asserting this guest-thread clearing contract.

Without an XEX, tool-mode initialization leaves the title TLS bitmap unconfigured. The fixture explicitly supplies **synthetic** process metadata for 64 slots (first two bitmap words free), not game headers. Threads receive the SDK's default 1,024-slot per-thread allocation. They validate bound ThreadState, aligned r1, r13==PCR, big-endian PCR[0]==static TLS pointer, distinct PCR/TLS blocks, shared-slot per-thread values, free clearing both threads, reuse, all 64 process slots and OUT_OF_INDEXES on the 65th allocation.

The Phase 5 bounds patch is also exercised on a real SDK XThread object with actual Memory and a synthetic 28-byte static prefix + 64 dynamic slots. Slot 63 succeeds; slot 64, `0x40000000`, and null output fail without changing the output sentinel or static prefix. Patched PPC KeTlsSetValue also rejects slot 1,024 and multiplication-wrap input on running threads. No mock memory backend or substitute thread layout is used.

Fixture objects follow SDK reference/handle ownership, close handles and wait before destruction. A development fixture using a stack-owned XThread violated the object table lifetime and was corrected; it is not an additional SDK fault finding. Replacing the entire XThread translation unit beside the prebuilt Mach-O library also duplicated thread-local binding/CVar state. The final harness retains one prebuilt Create/current-thread implementation and compiles only the exact two patched TLS method definitions against the real SDK class.

## Patch integration boundary

**VERIFIED in the isolated harness:** both Phase 5 STFS exports return `0xc0000002`, preserve the output sentinel, and no longer report success. Exact patched TLS methods enforce boundaries. A new one-line correction converts NtProtectVirtualMemory's old internal protection with the existing SDK `ToXdkProtectFlags` helper: baseline returns internal 3 for read/write, corrected output is Xbox PAGE_READWRITE 4. [Pinned memory export source](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_memory.cpp).

The isolated executable compiles complete I/O, memory and threading export translation units, plus the exact patched TLS methods. It links the unchanged prebuilt runtime for Memory, KernelState, XThread lifecycle and other dependencies. This is real implementation testing, but **not a complete SDK rebuild**. Mach-O internal library calls are not assumed to interpose on executable symbols. The isolated host dispatcher is tested separately, including on an SDK-allocated protected page; closure with the prebuilt MMIO handler chain is **PARTIAL**, not a validated rebuilt-runtime integration. No patched library replaces the installed one.

The unchanged Phase 5 vector patch is applied in the private copy but not regenerated or numerically revalidated here; its prior alias-only results remain historical evidence. **Remaining failure-contract blocker:** KeTlsFree can return 1 for other invalid indices because its underlying FreeTLS returns void. That separate issue is not silently treated as valid behavior or patched without a defined Xbox contract. STFS remains unsupported functionality even when it correctly rejects a request.

## Final deterministic checks and measurements

| PPC profile | Baseline groups pass/fail | Patched groups pass/fail | Baseline / patched seconds | Baseline / patched peak RSS bytes |
| --- | --- | --- | --- | --- |
| O0 | 6 / 0 | 7 / 0 | 0.706257 / 0.265064 | 22,626,304 / 23,576,576 |
| O2 | 6 / 0 | 7 / 0 | 0.226585 / 0.226482 | 22,691,840 / 22,822,912 |
| O0 + ASan/UBSan | 6 / 0 | 7 / 0 | 0.967234 / 0.372186 | 46,333,952 / 47,841,280 |

**39 PPC groups pass, zero fail**. Sanitizers instrument the original harness, exact TLS methods and compiled export units; the prebuilt SDK/allocator is **not instrumented**. No sanitizer diagnostic was emitted in final runs. The existing SDK FPCR asm operand-width warning remains; this work does not validate its numeric semantics.

**12 final fault regressions pass, zero unexpected crash/timeout**: six each at O0/O2 (plain fault, captured fault, original dispatcher recovery, patched unclaimed-page fault, patched SDK-page fault, patched recovery). Original dispatcher recovery returns in 0.027955/0.031015 s. Patched SDK-page faults terminate in 0.133942/0.149421 s with peak RSS 20,955,136/21,053,440 bytes. Fault fixtures are intentionally unsanitized because ASan owns signal handling. Six watchdog regressions pass: explicit exit, signal, capped output, group timeout, descendant cleanup after leader exit, invalid budget. **Total: 57 final groups/checks, zero failure**, excluding the two expected diagnostic timeout reproductions. Separately, the existing Phase 5 O0 regression runner was re-run: six original groups, three source-leaf groups and one native Memory probe all passed (10 checks); these are not added to the 57 Phase 6 count.

The external Python parent uses `time.monotonic`, a deadline of at most 30 seconds, a new child session/process group, group SIGKILL and wait4/reaping. It records explicit status, user/system CPU, elapsed time, peak child RSS (Darwin bytes/Linux KiB converted), snapshots and optional bounded sample. RSS is the observed leader's process peak, not an aggregate process-tree or game-memory budget. Sampling/cleanup adds small scheduling/reaping overhead. Suspend-inclusive analysis-budget guarantees and clock-discrepancy diagnosis from Phase 3 remain separate unresolved work; no analyzer is rerun.

## Decision and next milestone

**VERIFIED:** expected faults, selected host recovery, a fail-stop correction, original PPC/export calls, selected register preservation, allocation/TLS integration and isolated patch behavior.

**PARTIAL:** SDK patch integration and selected ABI contracts. **BLOCKED:** game runtime correctness while guest exceptions/unwind, unchecked pointers, failure contracts and VMX numeric evidence remain unresolved. **UNKNOWN:** game boot, full game link, Horizon behavior and performance.

Next exact task: **R2g / Phase 7 — one coherent, isolated SDK runtime build and ABI/failure conformance**. Build the pinned runtime with reviewed patches so one dispatcher, MMIO chain and current-thread binding are used; rerun these fixtures against that library with instrumentation where supported. Add explicit r2/callback and 64-bit argument probes, establish invalid-TLS-free behavior, and validate fail-stop/recovery through the real MMIO chain. Acceptance requires bounded tests and documented contracts; no game entry, full game compilation or graphics/audio integration is implied. VMX hardware evidence and guest unwind remain independent gates.
