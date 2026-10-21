# Generated memory instrumentation — Phase 10

2026-10-21 · baseline `v0.0.9-entry-readiness` (`3e3222b380770319cf063c54daa959594ebc0d32`). **VERIFIED original fixture prototype; PARTIAL access coverage; NO-GO for proprietary execution.** No game-derived function, callback or entry point is executed, compiled, rewritten or regenerated. No game addresses/snippets, private paths or generated game files are published.

## Baseline and reproducibility

The original XEX SHA-256 is verified against `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`. XEX, ReXGlue manifest and all 1,453 generated files retain their baseline content, size and nanosecond mtime; extracted assets stay private, no title update is applied, and the installed SDK is unchanged.

The [instrumentation lock](../../tools/sdk/instrumentation-lock.json) retains all thirteen prior patches and pins patch 0014 on ReXGlue v0.10.0, revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. SDK dependencies/tool hashes follow the existing isolated builder. SDK outputs, synthetic emission, objects and logs remain outside public Git. All clients use matching staged headers/libraries. Debug is O0, Release SDK O3/fixture O2, Sanitized SDK O1/fixture O2 with ASan+UBSan and no recovery. System frameworks and handwritten assembly are not fully instrumented; ASan/UBSan are not race detectors.

## Generator analysis and selected seam

**VERIFIED source:** `src/codegen/builders/memory.cpp` and `builders/helpers.h` emit scalar memory operations through `REX_LOAD_U*`/`REX_STORE_U*`, with selected `REX_MM_*` variants. `resources/templates/codegen/pch_h.inja` defines these as volatile host-pointer dereferences plus byte swaps. Address expressions are normally guest u32; the macros explicitly truncate to u32. Typed native loads may be unaligned in C++ and are not a portable alignment-exception model.

Full/partial vector operations and load-reserve/store-conditional helpers use `REX_RAW_ADDR`. Full vectors reverse all sixteen bytes into the SDK register representation. Reservation loads use a raw dereference and store-conditionals a host compare/exchange; these alone do not prove Xbox reservation-granule, invalidation or memory-order semantics. MMIO detection combines generator register tracking/eieio heuristics, an aperture test and runtime callbacks; some default paths ignore a failed CheckLoad/CheckStore. These are not retained as safe fallbacks in checked mode.

Indirect calls normally read the module dispatch table or use the runtime resolver. Direct emissions include ordinary, conditional, jump-table, SEH-finally and register-restore sites. Instruction fetch is separate: executing emitted host functions does not perform a checked guest instruction fetch or establish execute permissions.

**VERIFIED narrow prototype:** patch 0014 adds an opt-in consumer include after memory/dispatch macro definitions, before inline helpers expand them, in both generated-PCH templates. `REX_CHECKED_ACCESS_HEADER` selects the original [checked backend](../../runtime/generated_access.h). Scalar builders remain unchanged. Full vector loads/stores use two named macros; four partial-vector builders emit a guard; all identified direct-call emission sites use the existing call macro. With no opt-in, macros retain default SDK behavior. Game C++ is never edited manually.

The SDK test-PCH template receives matching defaults for the new vector/guard macros. Both default and checked test-template variants compile with original vector functions. This is selected template compatibility, not the complete upstream SDK test suite.

## Checked integration contract

A thread-local, nested RAII scope binds up to 64 opaque Phase 9 allocation handles and a separately authored synthetic call allowlist. Handles authorize bytes, not arbitrary host pointers. Every supported transfer routes through `SdkCheckedMemory` and the real SDK's locked `CopyOwnedMemory`. Requested allocation bounds, guest reservation/commit/protection and lifetime ownership are enforced before copying. Guest pages remain 4 KiB even on the 16-KiB host.

**VERIFIED:** scalar address arguments are evaluated once and truncated to guest u32 before the bounded transfer. The transfer itself cannot wrap at the guest address-space end. Byte codecs preserve big-endian scalars; byte-reversed instructions retain their builder's extra swap. Full vectors copy sixteen bytes transactionally and reverse them to/from the existing register layout. The generated vector builder retains its align-down rule. Scalar unaligned byte copies are accepted by this prototype and tested, but complete Xbox alignment-trap behavior remains **UNKNOWN**.

A failed read does not assign its result; a failed cross-page write cannot modify an earlier page. Backend errors throw a typed host `Fault` with an explicit memory result. Execution must stop at the host boundary. This does not deliver a guest exception, roll back an entire PPCContext, or authorize catch-and-resume. Builders may already have changed LR, CR or FP state before an unsupported operation is refused; restart/precise guest exception semantics remain **BLOCKED**.

MMIO and shared host-page padding are refused without device callbacks. Unknown/unowned pointers are refused. No raw guest backing pointer, string view into guest backing, dispatch-table pointer or permissive memory fallback is returned. Raw-pointer operations, atomics, partial vectors and borrowed strings fail explicitly; lookup-table access is refused. Reset/restore/teardown and MMIO registration must remain quiescent, host buffers valid/disjoint, and the SDK must outlive the adapter/scope. Physical aliases, XEX/large-page heaps and persistent capability restoration remain unsupported.

## Original PowerPC fixtures and independent oracles

[The emitter](../../tests/instrumentation/emitter.cpp) independently encodes original PPC words, decodes them with the actual pinned SDK decoder, asserts every expected mnemonic, invokes the matching real instruction builders and renders the real embedded PCH templates. This tests decoder→builder→template→compiler→execution on selected instructions; it does not run the complete ReXGlue scanner/analysis pipeline on a title.

| Selected decoded variants | Outcome / oracle |
| --- | --- |
| lbz, lhz, lha, lwz, lwa, ld; stb, sth, stw, std | VERIFIED selected unsigned/sign-extension results, independently folded bytes and guard bytes |
| lhbrx, lwbrx, sthbrx, stwbrx | VERIFIED selected reversed-endian bytes/results |
| lfs, lfd, stfs, stfd | VERIFIED selected finite exact values and bits; FP exceptions, rounding/denormals and complete FPSCR remain UNKNOWN |
| lwz/stw with synthetic MMIO register-tracking hints | VERIFIED ordinary-memory fallback uses the same checked codec; device access is refused |
| lvx, stvx | VERIFIED align-down, byte reversal, preserved neighboring bytes; no typed-vector ABI claim |
| lwarx, ldarx, stwcx., stdcx. | BLOCKED explicit rejection before any atomic/backing-memory operation |
| lvlx, lvrx, stvlx, stvrx | BLOCKED explicit guard, including zero-length/no-op cases; no partial-vector emulation |
| bl, bctrl | VERIFIED selected allowlisted synthetic context calls and explicit rejection, not general guest call correctness |

Final mnemonic assertions were independently rerun on all three SDK profiles; the emitted body and both rendered headers were byte-identical to the executed fixture inputs. This does not add duplicate runtime passes.

There are **32 decoded cases**: 22 supported memory variants, eight refused memory variants and two policy-guarded call variants. Original checks cover unaligned/cross-page access, RO/RW neighbors, full-span failure without partial writes, reserved/uncommitted pages, freed handles, width overflow and MMIO boundaries. Four original host threads perform 1,000 generated read/write operations with independent whole-value expectations. Checked execution passes a null base pointer, so the generated data path cannot rely on the normal arena base. This is a useful regression guard, not protection against arbitrary malicious native C++.

## Coverage and bypass review

**VERIFIED fixture evidence:** 24 selected memory/call macro definitions resolve to the checked backend or its explicit rejection helpers after actual PCH preprocessing. The emitted corpus has no literal direct calls, direct base-pointer expressions or nonlocal/exception helpers according to the lexical fixture gate. Six raw-pointer expressions remain in emitted unsupported bodies, but their macro never returns a pointer; four partial-vector guards execute before their bodies. Lexical checks and compile failures are not a C++ sandbox or a proof covering unknown compiler intrinsics/helpers.

**PARTIAL global coverage:** SDK `GuestPtr`, `TranslateVirtual`, `TranslateRelative`, mapped pointers, stack argument marshalling, arbitrary hooks, physical memory, atomics outside this seam, inline assembly and guest exception helpers remain outside the checked backend. The default build is intentionally uninstrumented. Removing the opt-in or calling unreviewed host helpers bypasses this prototype; the controlled fixture build asserts its actual macro expansion and source census instead of assuming universal coverage.

## Call and ABI safety

The scope permits only host-authored original functions of the exact `void(PPCContext&, uint8_t*)` type. Direct and indirect calls require explicit allowlist entries; indirect dispatch never reads guest table slots or falls back to the SDK resolver. Unknown, unaligned and unregistered targets fail. Nested scopes restore the previous scope; a synthetic nested callback's exception propagates to the original outer host handler. State changes preceding the callback failure are observable and not rolled back.

Three compile-negative fixtures refuse typed vector return, vector argument and mixed integer/FP/vector signatures. Existing SDK typed-vector negative fixtures remain separate blockers. A context/base function type says nothing about the underlying guest ABI: it must not be used to claim support for arbitrary vectors, variadics, mixed arguments, import marshalling, TOC changes, title callbacks or exceptions. Register/stack/XThread contracts validated in earlier phases remain limited to those original fixtures.

## Private generated-code assessment

The [static census](../../tools/audit_instrumentation.py) reads existing private source without executing, compiling or rewriting it. Its digest matches the Phase 9 input. Counts are lexical observations, not dynamic coverage or a complete pointer/alias proof:

| Existing private output | Whole output | Literal direct entry closure |
| --- | ---: | ---: |
| Ordinary scalar macro sites | 791,353 | 1,693 |
| MMIO-hint macro sites | 49 | 0 |
| Raw-address macro sites | 43,493 | 0 |
| Literal direct call sites | 201,282 | 262 |
| Indirect call sites | 22,203 | 21 |
| Exception/nonlocal helper observations | 1,243 | 3 |
| Actually instrumented sites | **0** | **0** |

The corpus contains 48,371 bodies across 721 partitions. The simple direct-base-pointer pattern count is zero; that does not eliminate the raw-macro, SDK-helper or indirect bypasses above. The direct closure omits dynamic targets and overestimates branch-feasible paths.

**PARTIAL compatibility estimate:** scalar/MMIO-hint sites are candidates for routing after reviewed regeneration and correct capability binding; actual operand ranges, module/stack/TLS ownership and device needs remain UNKNOWN. Old raw full-vector bodies require regeneration to use the new full-vector macros; old partial/atomic bodies must be refused. Old literal calls do not acquire the new call policy merely by including a new header. A newly rendered opt-in PCH alone is therefore insufficient for the existing private output. The pinned SDK can render the new templates and generate the original test bodies; complete title regeneration has **not** been attempted. No title analysis budget is reopened and the original game configuration is unchanged.

The entry closure still has unverified import contracts, nonlocal helpers, unresolved indirect targets, no checked instruction-fetch authorization and image/stack/TLS lifetime gaps. Its absence of lexical raw-address macros does not make it safe. Current checked allocations cover ordinary virtual heaps, not the loaded title image.

## Bounded regressions and measurements

| Final profile | Passing groups / failures | Expected fault/recovery checks / failures | Execution-child seconds, summed | Peak execution-child RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 45 / 0 | 23 / 0 | 12.504051 | 34,619,392 |
| Release | 45 / 0 | 23 / 0 | 6.531493 | 22,200,320 |
| Sanitized | 45 / 0 | 22 / 0 | 9.910953 | 61,997,056 |

**VERIFIED:** 135 ordinary passing groups + 68 expected fault/recovery checks = **203 core checks**. Zero final unexpected faults/timeouts or ASan/UBSan findings. Expected signal children: 44 SIGBUS, three SIGSEGV, nine SIGABRT; the remaining twelve checks recover or use their expected diagnostic exits. The plain default-signal case owned by ASan remains excluded only in Sanitized, as documented in Phase 9.

Per profile the new runtime comparison has **387 default + 471 checked assertions**, 32 original decoded cases and 1,000 concurrent generated accesses. The second PCH template compiles in both modes (six compatibility compilations total). Three typed-vector forms per profile are expected compile-negative observations, not supported ABI features. Existing vector negatives and raw host-page slack remain BLOCKED observations rather than passing features.

Separately: **27 historical component/source-control groups**, **three VMX characterization suites** (212,068 synthetic invocations each), **six watchdog checks**, and **six static-audit unit tests** pass. Source-leaf controls are not an old SDK runtime loaded alongside the unified library. VMX numeric hardware equivalence remains UNKNOWN. All 261 baseline imported function symbols are present in every final library; symbol presence is not conformance.

| Same original generated 64-bit load, median of five 256-load samples | Default ns | Checked ns | Observed ratio |
| --- | ---: | ---: | ---: |
| Debug | 750 | 66,603,959 | 88805.3× |
| Release | 375 | 6,661,000 | 17762.7× |
| Sanitized | 209 | 17,494,333 | 83704.9× |

The checksums match between default/checked executions. Ratios are large and apply only to this tiny diagnostic loop: dispatch, locks, range/ownership checks and the Phase 9 per-host-page MMIO scan dominate an otherwise very small native load. Scheduling, concurrent build/test work, watchdog sampling and compiler optimization affect the samples. They are not a production throughput guarantee or game slowdown estimate. No performance threshold is used to hide a correctness failure.

| Original emitted functions plus reachable helpers | Default object `__text` bytes | Checked object `__text` bytes | Default / checked full object bytes |
| --- | ---: | ---: | ---: |
| Debug | 7,568 | 11,180 | 92,024 / 529,696 |
| Release | 1,964 | 4,232 | 79,192 / 505,152 |
| Sanitized | 11,392 | 14,944 | 109,400 / 541,784 |

The same 6,761-byte original emitted body source is compiled in both modes: no per-site source rewriting occurs between modes. Object `__text` includes reachable backend helper code; full object sizes also include debug/type metadata. Harness executable sizes are recorded privately but are not used as an isolated generated-code size claim. No title-size extrapolation is justified.

| Final serialized clean SDK build | Monotonic seconds | Peak build-child RSS bytes |
| --- | ---: | ---: |
| Debug | 107.629391 | 597,639,168 |
| Release | 127.423940 | 547,864,576 |
| Sanitized | 214.686842 | 1,030,897,664 |

The final `clean-build-results.json` verifies the complete fourteen-patch source and matching staged outputs; it supersedes all draft build records. Final build calendar/monotonic deltas are below 0.004 seconds. RSS is per-child peak, not process-tree/game memory. Existing SDK unused-parameter/ARM FP-asm warnings remain visible; no suppression was introduced for this phase.


Each compilation/execution uses an external 30-second monotonic process-group watchdog with core suppression, explicit status, capped private output and wait4 timing/RSS. SDK builds have separate 1,200-second monotonic plus calendar ceilings and at most two compiler jobs. Final tests must use all completed matching SDK profiles; unfinished staging is refused before testing. No sanitizer diagnostic is suppressed. Expected native faults in the older suites are reported separately from new host exceptions and from unexpected faults/timeouts.

**Preserved development history:** original fixture compile errors (private graph API, name-overload selection and CTR assignment) were corrected against the actual SDK types. An early regression orchestration attempt stopped before tests because Sanitized staging was absent. An overlapping draft SDK rebuild was an orchestration error; the draft verification included a recorded Sanitized calendar-watchdog failure. One retained record shows 255.481 monotonic seconds versus 1,489.742 calendar seconds. The discrepancy's cause is UNKNOWN (host suspend/clock change is not established). Logs were retained privately; those outputs were rejected for final verification, followed by one serialized clean rebuild. None of these exploratory failures is hidden or counted as a passing test. The saved Phase 8 allocation SIGBUS was not rerun on an unfixed SDK.

## Reproduction and publication

[Fixture recipe](../../tests/instrumentation/README.md) · [isolated SDK builder](../../tools/sdk/README.md). Use the explicit Phase 10 lock, fresh private directories and matching profiles. Static assessment commands emit only counts; raw diagnostics and code remain private. Existing Phase 5–9 runners are retained and run against the unified patched library where applicable; historical source controls are not an old runtime loaded alongside it.

**VERIFIED publication review:** every changed file was reviewed; the index admits 109 public text files (97 prior + twelve new), comprising original source/tests, the six-file BSD SDK patch, its lock, CI metadata and English documentation. Public-tree checks cover text/binary/size limits, local links, private-path/credential patterns and all four patch locks. No game-derived file, snippet, address, asset, private absolute path, raw log or temporary artifact is staged. All 1,455 original input/config/generated fingerprints and 299 extracted hashes match; thirteen old patch hashes and the installed SDK library hash are unchanged. Useful diagnostics and interrupted-build history remain private. This bounded allowlist/text review is not a universal copyright or secret-detection proof. No GitHub Release is created.

## Phase 11 readiness

**NO-GO for proprietary execution**, including a narrow entry trace. Instrumentation demonstrates a selected original host-memory/call boundary, not safe title execution. Existing private output remains uninstrumented; module image/stack/TLS capability binding, imports, dynamic targets, instruction-fetch authorization, atomic/partial-vector behavior, typed ABI and exception/unwind gates remain unresolved. Diagnostic overhead also requires further engineering before any performance claim.

**Exact next milestone: Phase 11 — fail-closed synthetic scope admission and image/stack/TLS capability validation.** Validate an original mini-module's section permissions and lifetimes, test stack/TLS accesses through the checked backend, and reject unsupported emitted helpers/imports before dispatch. Establish an independently tested import allowlist and complete access census for that synthetic scope. Optimize the conservative MMIO scan only with preserved correctness evidence. A later, separate readiness review must precede any proposal to execute private game functions. No game execution, full game compile, renderer/audio or Switch deployment is authorized by this result.
