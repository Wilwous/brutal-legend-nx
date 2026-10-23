# Phase 19 — game entry readiness and blocker priorities

Assessment: 2026-10-23. Baseline `v0.0.18-generated-loop-stop`, commit
`b8a29cf734ca73c30eef9de60f337aac3a6c0c89`. Initial working tree: clean.
**PARTIAL readiness; private game-entry execution NO-GO.** The next original
synthetic milestone is **CONDITIONAL GO** under the acceptance criteria below.
No proprietary instruction, callback or entry point is executed in this audit.

## Evidence and preservation

**VERIFIED:** the original XEX SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The preservation inventory checks 1,455 XEX/configuration/generated-file
SHA-256, size and modification-time fingerprints, 299 extracted-file digests,
and the installed runtime digest. No title update is applied. Existing private
code/configuration is neither rewritten, regenerated nor compiled.

The isolated SDK remains ReXGlue v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with nineteen patches from
[the current lock](../../tools/sdk/xex-binding-lock.json), SHA-256
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
The workflow reconstructs patched source from pristine Git, checks 22 dependency
pins and matching staged headers/runtime fingerprints. No SDK patch or global
installation is changed. The only runtime correction is in the project's
original host stop adapter, described below.

The reports for Phases [9](guest-memory-entry-readiness.md),
[10](generated-memory-instrumentation.md), [11](synthetic-execution.md),
[12](image-ownership-runtime-contracts.md), [13](xex-tls-import-admission.md),
[14](pointer-import-synthetic-dispatch.md),
[15](generated-ppc-end-to-end-dispatch.md), [16](synthetic-xex-binding.md),
[17](image-backed-pointer-shutdown.md) and [18](generated-loop-stop-boundaries.md)
were reviewed before selecting new work. Their results certify their stated
original fixtures, not the unchanged game output. This phase does not reopen
the consumed game-analysis budget or build a general CFG engine.

## Static entry inventory: what is actually known

[The new original auditor](../../tools/audit_readiness.py) reads preserved C++
and writes detailed results only outside the public checkout. It reconciles with
the previous entry/instrumentation audits: **721 partitions, 48,371 bodies** and
the same content digest. Entry identity is cross-referenced privately to the
existing generated mappings and XEX/PE metadata. Addresses, initial edge
identities, instruction locations and detailed per-function inventories stay
private. Diagnostic instruction ordinals are **not guest PCs**: disjoint emitted
blocks make naive sequential address reconstruction unsafe.

| Observation | Result / confidence |
| --- | --- |
| Literal direct closure | VERIFIED lexical count: 108 generated functions + 33 imports; no missing literal symbols |
| Entry body's literal calls | VERIFIED 13 sites / 12 unique symbols; neither branch feasibility nor execution order is proven |
| Calls in the closure | VERIFIED 262 literal sites; includes mutually exclusive/error paths |
| Indirect calls | VERIFIED 21 lexical sites; zero literal constant targets recognized; all 21 remain unresolved by this method |
| Generated memory | VERIFIED 1,693 scalar macro sites in closure; existing checked mediation is zero |
| Exception helpers | VERIFIED three trap-helper sites and a literal exception-import dependency; execution necessity UNKNOWN |
| Labels / gotos | VERIFIED 404 function-qualified label identities / 574 goto sites; these are not a validated basic-block or backedge count |
| Executed sites | UNKNOWN: no game execution or trace was collected |

This closure over-approximates branch-feasible **direct** paths while omitting
unknown indirect targets, callbacks and exception edges. It is not a conservative
superset of *all* game execution. No nontrivial path is promoted to proven
reachable. Existing metadata establishes initial image/stack/TLS requirements;
it does not supply title launch values, TOC policy or arbitrary callback ABI.
The private report retains entry edges, all 108 function inventories, the 21
indirect locations and their unresolved status for follow-up.

Historical loader-only evidence resolved all 274 ordinals, including 261 function
shims. Current source/signature audits recheck the 33 matrix exports against the
pinned patched SDK. Resolution is not semantic admission. Additional imports
outside this direct closure are not proven dead; unresolved flow can enlarge
the dependency set. Filesystem/STFS, graphics and audio requirements cannot be
excluded for later paths merely because they are absent from this closure.

## Memory inventory and instrumentation gap

These are lexical **macro occurrences**, not dynamic transfers, unique memory
instructions, alias proofs or exhaustive helper-internal accesses.

| Operation | Whole preserved output | Literal direct closure | Current seam / limitation |
| --- | ---: | ---: | --- |
| Ordinary scalar loads/stores | 791,353 | 1,693 | Candidate for checked U8/U16/U32/U64 seam after reproducible regeneration and capability binding |
| MMIO-hint scalar macros | 49 | 0 | Same checked seam can refuse MMIO; no device implementation |
| Raw full-vector accesses | 26,774 | 0 | Regenerate to named 16-byte vector seams; including a new header alone cannot repair old raw bodies |
| Raw reservation/conditional atomic accesses | 14,402 | 0 | Explicitly reject; no atomic semantics certified |
| Raw partial/element vector accesses | 2,279 | 0 | Explicitly reject; variable/element extent not certified |
| Raw cache-zero accesses | 38 | 0 | Reject raw access; no checked cache-line contract certified |
| Total memory/raw macro occurrences | 834,895 | 1,693 | Excludes fetch, shim-internal pointers and other unrecognized C++ access paths |
| Instrumented existing game sites | 0 | 0 | VERIFIED preserved legacy-output finding, not inferred from macro spelling alone |

The raw categories sum to 43,493, matching the old census. The closure has
**778 reads / 915 writes**: 171 one-byte, 89 two-byte, 1,148 four-byte and 285
eight-byte sites. There are no lexical vector-memory/atomic/MMIO-hint sites
and no vector instruction comments in this limited direct closure. This does
not rule them out through unresolved flow. Across the corpus, 16,719 raw
atomic/partial/cache sites are not eligible for normal checked-copy semantics.
Full-vector transfers have aligned-down PPC effective-address semantics;
scalar runtime alignment, pointer provenance and actual target addresses remain
UNKNOWN. Width does not establish atomicity. The generic scanner's direct-base
pattern count is zero, which is not a C++ sandbox proof.

The supported original layer mediates scalar bytes/big-endian words and reviewed
full-vector transfers, validates complete spans, owner/lease, section/page
permissions and lifecycle, and refuses MMIO/fetch. FP operations using scalar
bit transfers require their separate numeric semantics; the prior VMX special
value limitations are unchanged. A 16-KiB host page cannot enforce all individual
4-KiB guest permissions. Explicit checked ranges remain mandatory; raw pointers,
SDK internals, host corruption and quiescence violations are outside that layer.

**Required reproducible instrumentation:** freeze the intended region and input
hashes; use an isolated manifest derived from the production configuration;
regenerate into a **new private output directory** with the pinned patched CLI
and the checked PCH/import seams; independently reconcile decoded instructions,
all emitted access/call/helper sites and preprocessed macro definitions; reject
unknown/raw helpers before compiling the region. Bind image, stack, PCR/TLS and
ordinary allocations with actual owners/leases. Repeat generation and compare
artifacts/configuration/site policy. Keep the old outputs intact. No such game
regeneration is performed here. Eligibility of 1,693 scalar sites alone proves
neither complete mediation nor correct capability coverage.

## All 33 import contracts

Classification: **A** = admitted for the stated verified SDK subset;
**B** = in the literal closure with insufficient contract evidence;
**C** = potential through unresolved flow; **D** = not currently shown;
**E** = unsupported for the specific reason stated. All 33 have a syntactic
literal path, so this matrix has **A 9 / B 18 / C 0 / D 0 / E 6**. C/D being zero
here says nothing about imports outside this preselected matrix.

**For every row, initialization necessity is UNKNOWN**, and actual execution
reachability is PARTIAL. Safe deferral requires an independently validated
exclusion from the frozen region or an explicit admitted abort **before** the
call; static absence from one chosen path does not authorize a fallback. All
24 denied imports stay denied. Six scalar plus three pointer-scoped admissions
do not certify full Xbox ABI or semantics. All 33 SDK implementations/stubs exist;
the source declarations are known except that the UI stub/variadic logger do not
establish their actual Xbox signatures. The existing
[full signature/ownership/error matrix](../../tests/image-runtime/import-contracts.json)
remains authoritative; admission has not been widened.

| Import | Class | Pointer contract needed | Verified behavior / specific remaining gate |
| --- | --- | --- | --- |
| DbgPrint | B | Yes | Null case only; variable argument ABI and unbounded format/string access unverified. |
| ExGetXConfigSetting | B | Yes | Selected invalid/short/AV-region results checked; output/required-size bounds and ownership not admitted. |
| HalReturnToFirmware | E | No guest buffer; other effects still gated | Host exit 0/assert path is not a verified title termination contract. |
| KeBugCheck | E | No guest buffer; other effects still gated | Fatal/debugger path has no safe ordinary-return contract. |
| KeBugCheckEx | E | No guest buffer; other effects still gated | Fatal/assert path and parameter contract not admitted. |
| KeDelayExecutionThread | B | Yes | Wait/APC and interval lifetime/error contract unverified. |
| KeGetCurrentProcessType | B | No guest buffer; other effects still gated | SDK scalar declaration known; bound PCR/process-type initialization/output not admitted. |
| KeQueryPerformanceFrequency | A | No guest buffer; other effects still gated | Admitted u32(): positive SDK frequency observed; hardware width/policy partial. |
| KeTlsAlloc | B | No guest buffer; other effects still gated | Selected lifecycle/exhaustion tested; independent-module admission does not define process slot ownership. |
| KeTlsFree | B | No guest buffer; other effects still gated | Selected false/valid/repeated cases; process lifetime and cross-thread clearing not generally admitted. |
| KeTlsGetValue | A | No guest buffer; other effects still gated | Admitted u32(u32): scoped bound thread; invalid index returns zero, ambiguous with valid zero. |
| KeTlsSetValue | A | No guest buffer; other effects still gated | Admitted u32(u32,u32): scoped bounds/isolation and false failure verified. |
| NtAllocateVirtualMemory | B | Yes | Selected lifecycle/OOM/invalid results; pointer outputs, flags and allocation capability adoption unverified. |
| NtClose | B | No guest buffer; other effects still gated | Selected valid/double/concurrent close and INVALID_HANDLE; general object/stale-handle contracts not admitted. |
| NtCreateEvent | B | Yes | Unnamed lifecycle checked; output handle, attributes, names/collision/OOM need contracts. |
| NtFreeVirtualMemory | B | Yes | Selected release errors; pointer outputs, flags and exact-owner lifecycle not admitted. |
| NtQueryVirtualMemory | A | Yes | Admitted OUT 28 bytes, same-owner target/type subset; actual SUCCESS/INVALID_PARAMETER and unchanged failure output verified. |
| NtWaitForSingleObjectEx | B | Yes | Closed-handle INVALID_HANDLE checked; waits/APC/timeout ownership unverified. |
| RtlCompareMemoryUlong | A | Yes | Admitted bounded IN words; actual SDK match-count checked; complete Xbox return convention still partial. |
| RtlEnterCriticalSection | B | Yes | Structure/atomic/contention/wait lifetime and wrong-owner paths unverified. |
| RtlImageXexHeaderField | B | Yes | Selected immediate/missing field checked; raw header bounds and borrowed result lifetime unverified. |
| RtlInitAnsiString | B | Yes | Unchecked strlen and u16 length narrowing block general input; short/null observations are insufficient. |
| RtlInitializeCriticalSection | A | Yes | Admitted INOUT 28 bytes, void return; selected initialized/preserved fields and rollback verified; later locking not certified. |
| RtlInitializeCriticalSectionAndSpinCount | B | Yes | Selected spin checked; spin rounding can wrap, with no validated bounds/error adapter. |
| RtlLeaveCriticalSection | B | Yes | Wrong-owner/recursion assertions and waiter lifetime unverified. |
| RtlNtStatusToDosError | A | No guest buffer; other effects still gated | Admitted u32(u32): success, INVALID_HANDLE and unknown fallback 317 observed; not exhaustive conformance. |
| RtlRaiseException | E | Yes | Guest dispatch/unwind incomplete; no safe compatibility continuation. |
| XGetAVPack | A | No guest buffer; other effects still gated | Admitted u32(): actual SDK constant observed; hardware policy unknown. |
| XGetLanguage | A | No guest buffer; other effects still gated | Admitted u32(): actual English policy observed; title/user policy unknown. |
| XamLoaderTerminateTitle | E | No guest buffer; other effects still gated | Title termination/cleanup and observable nonreturn contract not verified. |
| XamShowMessageBoxUIEx | E | No guest buffer; other effects still gated | False-success UI/completion stub; actual Xbox ABI and asynchronous ownership unknown. |
| XexCheckExecutablePrivilege | B | No guest buffer; other effects still gated | Out-of-range bit shift is undefined; absent-module subset does not validate title metadata/policy. |
| XexGetModuleHandle | B | Yes | Missing-name error/zero output checked; success returns borrowed module identity needing bounds/lifetime contract. |

## CFG and indexed site identity

**VERIFIED feasibility, scoped evidence:** Phase 18 already uses production
`[[entrypoint.midasm_hook]]` configuration to emit 67 distinct callbacks at
original function entries, six backedges, pre-memory, pre/post-call and bounded
straight-line sites. Actual emitted code is regenerated identically across the
three profiles, compiled unchanged and executed. This evidence is sufficient
for the feasibility audit; no new indexed-dispatch engine is needed here.

Pinned implementation: `src/codegen/config.cpp` parses hooks,
`src/codegen/manifest.cpp` loads entrypoint configuration,
`src/codegen/function_graph.cpp` places labels and before/after-instruction hooks,
and `src/codegen/builders/context.cpp` emits native callbacks. The production
map is keyed by PC: duplicate addresses override earlier configuration entries.
They cannot represent independent event records without explicit coalescing.

| Site category | Current evidence / prerequisite |
| --- | --- |
| Function entry | Original generated entries checked; hook follows generated host prologue, not a pre-prologue native trap |
| Basic-block / forward joins | Source labels are available; no validated general block partition/dominance or complete forward CFG policy exists |
| Direct branch / backedge | Six original backward branches independently decoded and reconciled; private labels/gotos are only lexical observations |
| Indirect branch / return | Exact original import gates tested; private dynamic targets and exception returns unresolved |
| Long paths | Original 16-instruction stride tested; not a bound on host scheduling, SDK calls or lock acquisition |
| Import boundaries | Original before/after checks preserve synchronous adapter completion and leases; not cancellation inside SDK instructions |
| Memory sites | Original checked accesses and adjacent hooks audited; private output remains untouched |
| Indexed identity | Feasible design, not implemented/benchmarked: canonical manifest plus deterministic index, verified PC/context and immutable policy |

A candidate identity namespace must bind image/code hash, exact CLI revision,
patch-lock hash, full codegen settings and site-policy digest. Within it, use
function/range identity, actual PC, before/after phase and an explicitly
coalesced set of events. Canonically sorted records can receive dense indices;
indices are not stable across changed inputs/policies. Verify index→PC/context
and compiled manifest at admission; reject duplicate/conflicting PCs, shared
function tails without disambiguation, missing hooks and unexpected control
edges. Never infer a guest PC from the auditor's ordinal.

Codegen/local-register/exception settings can alter emitted structure and
state visibility. Compiler optimization may inline or reorder surrounding work;
callback observability is established only in the measured builds, not as a
portable instruction scheduling theorem. Revision/settings drift requires new
identity validation. Unknown indirect edges prevent a whole-region stop proof.
The current 256-site stop-policy, 64-call checked Scope and 32-function explicit
synthetic binder limits are intentional bounds; the 108-function closure is
not admitted by simply enlarging them. Indexed lookup is an optional later
optimization, not a prerequisite for a correct small region.

## Narrow stop-race correction discovered by regressions

The initial Release `latency` child exited 1 with an unexpected host
`generated execution after stop boundary` error. **VERIFIED source defect:**
GeneratedStop read the atomic state twice. A request between a first `running`
read and a second `requested` read reached the terminal-state error path.
The observed diagnostic is consistent with that interleaving; the exact
instruction-level schedule was not traced. This was a false rejection, not
evidence of arbitrary memory access or a game-code failure.

[The correction](../../runtime/generated_stop.h) uses one state snapshot.
A request after a running snapshot is handled at the next poll; a requested
snapshot still must transition exactly once and throw the owned typed stop.
Terminal states still reject. No check location, admission or memory rule is
removed. The added `request-race` child runs 256 original synchronized
request/poll lifecycles, accepts either legal initial scheduling outcome,
requires a stop by the next poll after requester join, verifies signal ownership
and rejects a terminal-state poll. Its oracles are deterministic; exact thread
interleaving is not. Existing generated infinite-loop/latency tests rerun.
ASan/UBSan are not race detectors, and no exhaustive concurrency proof follows.

## Minimum contract before any future private experiment

Every item is mandatory **for the frozen proposed region**, or needs an
independent proof that the dependency is excluded. No region satisfying all
items is currently established.

1. **Region and control:** independently validated actual instruction/site map,
   entry/exit boundaries, direct edges and all admitted indirect targets;
   unknown targets refuse before transfer. No jump to an unchecked continuation.
2. **Memory:** every generated access/helper in the region is checked or rejects
   before access, including stack/argument slots, TLS/PCR, image data and any
   adapter outputs. Reconcile instruction, emitter, preprocessor and runtime
   counters; supply exact owners, full-span permissions and lifetimes.
3. **Operations:** every potentially reached operation is supported for the
   chosen inputs or explicitly aborts before unsupported effects. Raw atomics,
   partial vectors/cache operations, MMIO/fetch and nonlocal helpers cannot
   silently bypass the checked layer. Dynamic instruction fetch is not authorized
   by a readable page; fixed compiled dispatch needs separate exact binding.
4. **Imports and ABI:** actual XEX identity/membership, signature, pointer direction,
   length/alignment, output/return/error obligations and ownership per call.
   Denied/unverified calls produce observable host refusal, never invented Xbox
   success. Typed-vector, variadic/mixed/aggregate ABI stays rejected.
5. **Binding:** verified image/code/section/import identity and unique compiled
   function mappings; retain image leases across execution and each admitted
   call. No generic title admission through the synthetic-only binder.
6. **Initialization:** verify every used initial GPR/FPR/vector/CR/LR/FPSCR,
   stack/linkage and r2/TOC assumption; bind actual XThread/PCR/r13/TLS and
   required process policy. Preservation tests do not establish initial values.
7. **Stop:** audited entry/block/backedge/long-path and pre/post-import points,
   with no uncovered cycle or unbounded admitted synchronous API. Snapshot
   requests, typed host exit, native join and final-owner reclamation must hold.
8. **Containment:** dedicated process, external monotonic deadline plus calendar
   ceiling, process-group kill on timeout, shorter rendezvous/join limits and
   fail-stop before unsafe cleanup. No global protection bypass/native sandbox
   claim; raw SDK operations/reset/restore require quiescence.
9. **Observability:** distinguish natural return, deliberate unsupported-path
   abort, checked refusal, host exception, native signal and timeout. Record
   actual site identity, attempted/completed accesses, active import and owners
   privately. An expected early abort is not successful title initialization.
10. **Evidence and review:** reproducible private generation, unchanged inputs,
    original synthetic analogues and Debug/Release/sanitizer tests, negative
    coverage/binding cases, explicit output audit and a new readiness decision.
    This report authorizes no proprietary execution.

Full guest unwind, renderer/audio/input/saves, broad DLL loading, hardware vector
conformance and performance optimization can be deferred only when excluded
from the chosen region. General exceptions may remain unsupported if the region
has an independently validated abort boundary; they cannot be allowed to resume
as if Xbox unwinding succeeded. The same conditional rule applies to the 24
denied imports: implementing all of them is not a proven first-experiment need.

## Ranked blockers and minimum actions

“First” means blocks the first experiment now, or conditionally if the dependency
cannot be excluded. “Later only” is not a claim of gameplay implementation scope.
Complexity describes the minimum proposed action, not a schedule or completion
percentage. Dependency IDs define order, not a serial implementation mandate.

| Rank / ID | Category; evidence and reachability confidence | Severity | Dependencies | Minimum action | Validation | Complexity | First experiment | Later only |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 / B01 | Region/control: 21 unresolved indirect sites; direct closure is lexical, actual path UNKNOWN | CRITICAL | None | Freeze smallest meaningful entry prefix/region; independently recover actual site/edge identities; stop before unsupported transfer | Original branching analogue, independent decode/emitted-map checks, negative unknown-target cases | MEDIUM | Yes | No |
| 2 / B02 | Memory: 1,693 direct-closure scalar sites, zero mediated; actual addresses/aliases UNKNOWN | CRITICAL | B01 | Reproducible new private output with complete checked seams and all required capabilities; preserve old output | Instruction/emitter/preprocessor census, cross-page/RO/freed/owner failures, attempt/completion reconciliation | LARGE | Yes | No |
| 3 / B03 | Image/code binding: only fixed original layouts verified; private maps/sections not admitted; synthetic capacity limits | CRITICAL | B01 | Design bounded title-region binder with exact hash/range/map/lease checks, or prove a scope fits an independently justified policy | Original mismatched code/section/import/entry and shared-tail/capacity refusals | MEDIUM | Yes | No |
| 4 / B04 | Initial ABI/thread/TLS: preservation/header fixtures verified; title initial values/process policy UNKNOWN | CRITICAL | B01, B03 | Enumerate only registers/stack/TLS/process state consumed by region; independently justify values or prove unused | Original header/TLS/thread twins, bad r1/r13/TOC/bounds/lifetime cases and final reclamation | MEDIUM | Yes for consumed state | No |
| 5 / B05 | Import contracts: 9 scoped admissions, 24 denied; all 33 literal candidates, dynamic necessity UNKNOWN | HIGH | B01, B04 | Select genuinely required calls; add one evidence-backed contract at a time or prove exclusion/pre-call abort | Actual SDK signature/pointer/error/ownership tests from generated original callers; false-success refusal | UNKNOWN | Conditional per selected call | No |
| 6 / B06 | Stop/site policy: original 67 hooks/6 backedges verified; title CFG coverage UNKNOWN | CRITICAL | B01, B03 | Derive and independently check complete stop coverage of chosen region; preserve import completion and watchdogs | Original forward/backward/long-path tests, missing/duplicate hook rejection, concurrent stop and join containment | MEDIUM | Yes | No |
| 7 / B07 | Exceptions/traps: three direct-closure trap helpers plus exception import; necessity UNKNOWN; guest unwind incomplete | HIGH | B01, B05, B06 | Exclude or explicitly abort at a validated host boundary; never continue via guessed guest unwind | Original trap/nonlocal/exception-demand refusal and unchanged owner cleanup | SMALL for explicit exclusion/abort; LARGE for guest unwind | Conditional; no unhandled continuation | No |
| 8 / B08 | Additional operations/ABI: atomics, partial vectors, cache-zero, MMIO/fetch and typed signatures unsupported; not shown in direct closure, indirect reachability UNKNOWN | HIGH | B01, B02 | Keep explicit rejection; expand semantics only if selected region requires them | Actual generated negative fixtures and no raw-access fallback; hardware oracle for any claimed new semantics | UNKNOWN | Conditional | No |
| 9 / B09 | General relocation/DLL/process TLS, asynchronous callbacks/waits: outside fixed synthetic guarantees; necessity UNKNOWN | HIGH | B01, B04, B05 | Prove absent from first region; otherwise split a specific original contract milestone | Original relocation/callback/wait/TLS-index fixtures with bounded failure paths | UNKNOWN | Conditional | No |
| 10 / B10 | Graphics/audio/streaming/input/saves and Horizon integration: no execution evidence | LATER | Separate platform/runtime evidence | Defer from a host-only bounded CPU experiment; establish independent target probes later | Original target probes, then separately authorized integration/hardware tests | LARGE | No for the proposed host-only CPU scope | Yes relative to that scope |

R01, the false stop rejection, is corrected and regression-tested; it is not
silently removed from development history. Indexed lookup performance is not a
higher-priority safety blocker than B01–B06. No date, FPS or completion percentage
is inferred from these counts.

## Dependency-driven Phase 20–22 proposal

**Phase 20 — freeze and validate a bounded region manifest (B01/B06).** Start
with a privately identified candidate ending before its first unsupported
call/helper/transfer. Verify actual PCs from decoder/metadata, function extents,
forward joins, cycles and dynamic-transfer refusals. Build an original synthetic
analogue and deterministic site manifest with conflict/omission tests. Exit gate:
independent coverage evidence and a meaningful region, or an explicit finding
that no useful prefix fits; no game execution and no speculative full CFG engine.

**Phase 21 — close only that region's runtime contracts (B03/B04/B05/B07/B08).**
Validate exact binding and consumed initial state with original images/threads.
Select only necessary import signatures, pointer lifetimes and errors. Keep
unsupported operations and unrelated imports denied. Exit gate: the original
generated analogue executes and fails safely under all three profiles, with
complete memory/call/stop accounting and final-owner cleanup.

**Phase 22 — reproducible private instrumentation and a fresh readiness gate
(B02 plus integrated B01–B09).** In a separately bounded generation-only task,
produce new private outputs/configuration/site manifest without overwriting
historical products; account for every access and transfer in the frozen region,
validate compiled image/code binding and repeat negative original regressions.
Any required fresh title analysis needs its own explicit finite budget; Phase 3
is not silently rerun. Exit gate: all mandatory contract evidence, containment
and publication review independently pass. Reassess a later separately authorized
private experiment; Phase 22 does not automatically grant execution permission.

## Readiness decisions

**A — CONDITIONAL GO for Phase 20 original synthetic/static engineering.** There
is evidence for production hook placement and the checked original pipeline.
Keep the manifest/CFG work bounded, preserve all existing memory/import/stop
checks, validate forward/indirect failures and retain the fixed SDK lock. A
whole-title CFG engine or ABI expansion is not justified by this audit.

**B — NO-GO for private game-entry execution.** Required evidence to change this
is a frozen region with independent control/site mapping, full checked access
coverage, exact image/code/owner binding, justified consumed initial state,
verified required imports or explicit pre-call aborts, validated indirect refusal,
complete stop coverage and successful bounded original analogues in all profiles.
The current 108-function/33-import closure alone supplies none of that combined
proof. Synthetic success, import resolution and code generation do not prove
boot, compatibility or Nintendo Switch execution.

## Regression and publication evidence

Final measured results and retained failed attempts are recorded below after
verification. All detailed output, game identities and diagnostics remain private.

**VERIFIED selected final matrix: 578 core passes / zero final failures.**
The Phase 5–18 suites run against matching unified SDK profiles; the loop suite
adds the request-race group, for 20 groups per profile. The two interrupted Debug
suites are replaced only by their successful bounded revalidation, not inferred
passes. Every compiled source hash matches; the loop README was subsequently
updated as documentation only. Pinned SDK runtime/codegen inputs are unchanged.

| Profile | Ordinary groups | Fault/capture/recovery | Total | Core child seconds | Peak individual core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 170 | 23 | 193 | 156.545654 | 360,824,832 |
| Release | 170 | 23 | 193 | 127.156586 | 378,306,560 |
| ASan/UBSan | 170 | 22 | 192 | 156.702627 | 452,247,552 |

The selected final runs contain **zero unexpected signals, zero timeouts and
zero emitted sanitizer findings**. They retain 56 expected native signal exits:
44 SIGBUS, three SIGSEGV and nine SIGABRT; twelve other fault checks capture or
recover with specified diagnostics. Nine separate expected join fail-stops exit
70 and demonstrate process containment, not orderly reclamation. The historical
ASan-owned plain default-signal case remains excluded only from Sanitized.
Negative typed-vector compilation remains a refusal, not supported ABI.

Separately pass: 27 original component/source-control groups; 47 Python units
(including seven new auditor units); three VMX characterizations with 212,068
invocations each; current 33-import source/signature audits; and the Debug
reference trace `[2,2,2,2,1]` followed by one destructor. VMX hardware equivalence
remains UNKNOWN. Watchdog units intentionally exercise a timeout/kill separately
from the final core matrix. No failure expectation is weakened.

The new race group has 26 main assertions, 1,052 worker observations and 256
request/poll cycles per profile. The private read-only audit takes 29.640095
monotonic seconds / 47,333,376 peak child RSS bytes under a 90-second budget.
No dynamic game coverage is inferred from its output. Timings sum supervised
children (including generation/compilation where present), not project elapsed
time. RSS is maximum wait4 usage for an individual child, not simultaneous-tree
or game/Switch memory. Clients use O0/O2/O2+ASan+UBSan; SDK Sanitized uses O1
without recovery. System frameworks/CommonCrypto/assembly are not fully
instrumented, and ASan/UBSan do not prove race freedom or complete isolation.

**Retained attempts:** the initial Release latency failure is described above.
The first full orchestration later encountered ENOSPC and ended with exit 120;
missing/incomplete Sanitized results are not counted. A second orchestration
completed most suites but killed Debug instrumentation and unified groups at
their 300-second calendar ceiling: approximately 935/927 calendar seconds
versus 16/15 monotonic seconds. The cause of the clock discrepancy is UNKNOWN;
host sleep is not established. Those two timeout records remain failures of
that attempt. Only those suites were revalidated after resource availability
improved, with unchanged budgets/assertions; final selected results above pass.
The incorrect initial relative path for the new static-audit launcher also
remains a private orchestration failure, superseded by its absolute-path run.

To reduce transient storage, the private orchestration discarded only completed
fresh synthetic Mach-O/objects and repeated synthetic runtime-directory XEX
copies after recording SHA-256/size. Sources, generated original fixture inputs,
logs, results and pruning journals remain private. No pre-existing proprietary
or historical diagnostic file was removed. The plain public workflow below can
retain all products when sufficient storage is available.

```sh
# All variables are private directories outside this checkout.
# Outputs must be new; no command executes the game or edits its output.
python3 tools/audit_readiness.py --generated "$GAME_GENERATED" --entry xstart --output "$AUDIT/entry-readiness.json"
python3 tests/readiness/test_audit.py
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$TESTS"
# Exact targeted revalidation, only if a suite was interrupted:
python3 tests/instrumentation/run.py --sdk "$SDK_DEBUG" --source "$SDK_SOURCE" --workdir "$AUDIT/revalidated-instrument-Debug"
python3 tests/unified/run.py --sdk "$SDK_DEBUG" --source "$SDK_SOURCE" --pre-entry --workdir "$AUDIT/revalidated-unified-Debug"
python3 tools/check_public_tree.py
```

The static auditor produces no stdout dump; detailed identities remain private.
Use external 90-second supervision for this static scan, 300-second suite
supervision and the existing 30-second child/process-group watchdog. The existing
[workflow](../../tests/xex-binding/workflow.py) verifies SDK/source/dependencies
and contains the per-child original tests; it adds the new auditor units.
[SDK reconstruction](../../tools/sdk/README.md) and the
[loop recipe](../../tests/generated-loop-stop/README.md) document matching profiles.

Publication is restricted to reviewed original source/tests, CI/allowlist,
project metadata and sanitized English documentation. No game addresses,
extracted instruction sequences, private generated code/configuration, binary,
absolute local path or raw diagnostic is added. Detailed entry/access/import
inventories and useful failed-run logs remain in the established private audit
workspace. Public-tree checks cover text/size/path/link and eight patch-lock
integrity rules, not universal copyright or secret detection. CI checks portable
public units/components and documentation; it does not rerun local SDK-linked
Mac integration or establish title compatibility. No GitHub Release is created.
