# Phase 18 — generated PPC loop stop boundaries

Baseline: `v0.0.17-image-pointer-shutdown`. Verification date: 2026-10-23.

**VERIFIED scoped milestone:** all 575 core checks pass across Debug, Release
and ASan/UBSan, with 6/6 scoped backedges instrumented and observed at runtime.
There are zero unexpected signals, timeouts or sanitizer findings in final
regressions. No proprietary game code is executed. Generic title integration
and guest unwind remain PARTIAL or BLOCKED.

## Baseline and reproducible pipeline

The existing isolated ReXGlue v0.10.0 source remains pinned at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. The unchanged nineteen-patch
[XEX-binding lock](../../tools/sdk/xex-binding-lock.json) has SHA-256
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
No new SDK patch, global SDK edit or rebuild is needed. The full workflow checks
patch digests, reconstructs patched source, verifies dependency revisions and
staged headers, and checks the runtime library used by every client.

The original private game XEX remains SHA-256
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The saved 1,455 XEX/config/generated fingerprints and 299 extracted-file hashes
are verified privately. No title update is applied. All game content, previous
game-derived C++, configuration, products and diagnostics remain private.

The [original encoder](../../tests/generated-loop-stop/fixture.py) extends the
independently authored Phase 16 XEX with finite, intentionally infinite, nested,
checked-memory and imported-call loops plus a long straight-line function. The
finite loop is the actual XEX entry; other original function entries are admitted
explicitly by their verified generated mappings. PDATA supplies all nine
function extents without manual function hints. Production `codegen` and forced
regeneration produce unchanged C++/headers, which are hashed and compiled.
Actual SDK loading, XEX-header TLS, native XThreads, checked memory and the
existing real SDK imports execute the result.

The product contains **nine functions / 182 PPC instructions / 45 emitted
memory operations**. Two fixed-base products are built: instrumented A and the
unhooked finite benchmark reference B. Only B's count-bounded memory-loop entry
is exposed in its benchmark execution Scope; its other functions, including the
unhooked infinite loop, are never admitted/executed. Compiler-only namespacing
changes linkage identities, not emitted files or guest instruction bodies.

## Smallest supported instrumentation mechanism

Source inspection of the pinned SDK establishes:

- `src/codegen/config.cpp` parses `[[midasm_hook]]` fields.
- `src/codegen/function_graph.cpp` emits hooks before/after complete PPC
  instructions, after labels and outside instruction-builder fragments.
- `src/codegen/builders/context.cpp` emits the configured native callback.
- `src/codegen/manifest.cpp` loads this configuration from the entrypoint table.

The reproducible encoder emits `[[entrypoint.midasm_hook]]` entries with a
unique callback name per instruction address. No register arguments, early
C++ `return`, jump override or guest import is invented. The original host hook
wrappers call [GeneratedStop](../../runtime/generated_stop.h). The wrappers are
instrumentation callbacks, not substitutes for generated PPC callers. No emitted
C++ file is edited, patched or postprocessed.

| Placement | Sites before deduplication | Purpose |
| --- | ---: | --- |
| Function entry | 9 | Observe a pending request before the first PPC instruction |
| Every backward branch, including self branch | 6 | Bound loop execution between checks |
| Before complete guest-memory instructions | 45 | Avoid cancellation inside a scalar/vector copy or stack update |
| Before generated direct/indirect calls | 6 | Refuse further work once a request is observed |
| After generated direct/indirect calls | 6 | Finish admitted imports/callees before stopping |
| Straight-line stride of 16 PPC instructions | 16 | Cover the original long block and intermediate straight-line regions |

Overlapping reasons yield **67 unique safe-point sites**. The stride describes
straight-line placement in this fixture, not a general CFG or wall-clock latency
guarantee. Each configured hook runs before the whole instruction. A request
arriving during a checked copy waits until a later safe point. A request during
an import waits for that synchronous admitted call and scratch transaction to
finish. The selected imports are the existing three evidence-backed contracts;
the admitted-import matrix remains six scalar plus three pointer signatures,
with 24 denied entry-matrix imports.

The runtime validates the current PPCContext and exact sorted allowed site list,
counts each reached point, loads the thread-safe stop state and throws a private,
owner-bound `Stopped` object when it consumes the request. No context register
is passed to or altered by the zero-argument hook. Benchmark observers are empty;
diagnostic observers are trusted bounded host callbacks receiving only a PC.

## Static and runtime coverage

[The independent census](../../tests/generated-loop-stop/audit.py) decodes the
original PPC words and separately walks actual emitted instruction comments,
callback locations and gotos. It requires exact function/instruction counts,
expected call identities, memory-operation counts and backward-branch targets. Missing,
duplicated, misplaced or unknown points fail before compilation/execution.
Unscoped/unsupported control flow and raw/atomic/partial/unimplemented paths are
refused. Portable negative controls verify that removing a backedge hook or
changing its address/target fails the audit.

**Instrumented scope A: six reachable backward branches, six instrumented,
zero rejected/unsupported branches — 100%.** These are the cancellation scope;
the deliberately unhooked, finite benchmark control is reported separately.

| Function | Backward branch PCs | Runtime oracle |
| --- | --- | --- |
| Finite | `0x82010118` | Natural count 10, early count 0 and cancellation before completion |
| Infinite | `0x82010120` | Repeated in-function observation, then cooperative cancellation |
| Nested | `0x82010144`, `0x82010150` | Four outer × three inner iterations and cancellation |
| Checked memory | `0x8201017C` | Checked increments, fault refusal and equivalent benchmark outputs |
| Imports | `0x820101B8` | Two iterations through the actual generated caller and three SDK contracts |

Runtime counters measure **reached branch safe points**, including final untaken
conditional branches; they are not a taken-branch instruction trace. Natural
loop result/count oracles establish repeated loop behavior, while the infinite
loop asynchronous test reaches at least 128 checks before requesting stop.
Per-profile reached-site counts and runtime coverage are recorded below. This is not private game CFG or memory coverage.

## Stop state, import completion and ownership

The existing atomic CooperativeStop lifecycle is retained:

`running → requested → generated boundary → controlled C++ exit → exited marker
→ actual native completion/join → final reclamation → module unload`.

Multiple requests change state only once; redundant requests have no further
side effect. Eight concurrent host requesters exercise four separate XThreads.
The executing thread alone consumes its request at a generated safe point.
The typed signal unwinds ordinary generated C++ to the matching execution Scope;
a different scope's signal is not accepted as successful cancellation. Active
scratch/call leases finish before the post-import hook can throw.

The trusted host restores its saved PPC checkpoint after catching cancellation
or a supported checked-access/callback error. This restores r1/r2/r13/LR and the
context for native cleanup. Canceled guest frames never resume. This is a
**host execution cancellation contract**, not guest exception dispatch/unwind,
an Xbox exception code, a guest function return or complete ABI certification.
Natural returns separately check selected register and result preservation.

The import-stop test pauses the admitted operation at `sdk_returned`; adapter-stop
pauses at scratch-ready. The host requests stop, releases the observer, and the
actual import/commit returns before the emitted post-call check throws. These
are controlled admitted-call boundaries; timing inside SDK instruction bodies
is not independently controlled or established. No forced cancellation of SDK
instructions is claimed.

The unload-import test closes module admission while the pointer/image leases
are active. Existing leased copies finish; fresh import admission is refused,
and the next generated check exits before later imports. Concurrent callers
also stop with unload pending. Module pages, XThread/PCR/TLS and pointer birth
leases remain live until their legitimate owners finish. After native join,
handle/final caller release reclaims TLS/PCR; binding/capability release permits
actual SDK unload. Final image and ordinary-heap page counts return to baseline
after metadata/module references leave scope.

The only binding extension is a bounded explicit original native-function policy
in [XexBinding](../../runtime/synthetic_xex_binding.h). Empty policy preserves the
Phase 16 three-function contract. An explicit policy permits at most 32 unique,
aligned functions within the verified original code bytes, with exact mapping
membership/count. Image/entry/code/section/TLS/import checks are unchanged;
duplicate policy PCs are rejected. This is not a generic title loader or ABI
inference mechanism.

No live XThread is force-deleted. No unsafe longjmp is used. A deliberately
failed join still uses isolated process exit 70 before unsafe owner unwinding;
it proves containment, not cooperative completion or orderly in-process
reclamation. A normal child exiting this way fails the suite. The external
monotonic process-group budget is 30 seconds per child; native joins are bounded
at 15 seconds and observer rendezvous at 10 seconds.

## Performance and stop latency

The benchmark executes the same eleven PPC memory-loop instructions with the
same count (200), zero initial word and same opaque heap buffer on the same
native XThread. The control does not use module TLS/imports. Both products retain
the checked-memory layer; no raw host-memory shortcut is used. Five alternating
pairs are timed around only the generated body call, excluding host setup and
assertions. Each call performs 200 checked reads and 200 writes. The instrumented
call reaches **601 checks (3 per iteration + entry)**; the control reaches zero.

| Profile | Unhooked median (ms) | Hooked median (ms) | Median ratio | Arithmetic delta |
| --- | ---: | ---: | ---: | ---: |
| Debug | 107.178000 | 107.025583 | 0.998578 | -0.142% |
| Release | 10.689458 | 10.583000 | 0.990041 | -0.996% |
| Sanitized | 29.099875 | 29.415583 | 1.010849 | +1.085% |

| Profile | Unhooked five-sample range (ms) | Hooked five-sample range (ms) |
| --- | ---: | ---: |
| Debug | 106.141250–108.280042 | 106.523666–108.715458 |
| Release | 10.492750–11.958083 | 10.383208–11.220750 |
| Sanitized | 29.007375–29.351042 | 28.969250–29.928333 |

The ranges overlap in every profile. These five small samples establish neither
a stable slowdown nor a speedup. In particular, negative arithmetic deltas are
not evidence that polling improves performance. The checked-copy work dominates
this finite memory-loop fixture; pure arithmetic loop costs and game performance
remain UNKNOWN. No FPS, Switch performance or general overhead claim follows.

Each hook currently performs a bounded site lookup among 67 PCs, a hit-counter
increment, atomic state reads and an optional observer. Observers are empty in
the benchmark. A candidate optimization is an independently validated indexed
site identity that retains every check and its PC/context verification. Counters
show its check frequency, not a measured cost breakdown; profile/benchmark that
candidate before changing policy. Reducing required check coverage is not an
accepted optimization.

Latency is measured with a monotonic first-request-invocation timestamp,
published before the requested state and after all readiness rendezvous. Eight
concurrent requesters share the first timestamp without overwriting it. The end
point is the matching controlled catch after generated C++ unwinds; checkpoint
restoration and final join follow. Caller scheduling/dispatch is included, while
waiting for other workers to become ready is excluded.

| Profile | Maximum across generated stops (ms) | Ungated asynchronous infinite-loop probe (microseconds) |
| --- | ---: | ---: |
| Debug | 0.580541 | 29.458 |
| Release | 0.105709 | 27.125 |
| Sanitized | 0.225583 | 55.500 |

These are observed host latencies, not hard deadlines. Synchronous import work,
checked-copy locks, diagnostics and OS scheduling can delay the next point.
Gated tests prove lifetime/order; the separate asynchronous infinite-loop probe
requests stop while generated execution continues without a blocking observer.

## Full regressions and resource accounting

All **575 core groups/checks** pass: 507 ordinary groups, including nine expected
join fail-stop children, plus 68 native fault/capture/recovery checks. Expected
native exits are 44 SIGBUS, three SIGSEGV and nine SIGABRT, plus 12 controlled
capture/recovery outcomes. Final unexpected signals, deadlocks, timeouts,
tracked resource leaks and sanitizer findings: **zero**.

| Profile | Ordinary groups | Fault/capture/recovery | New Phase 18 groups | Failures |
| --- | ---: | ---: | ---: | ---: |
| Debug | 169 | 23 | 19 | 0 |
| Release | 169 | 23 | 19 | 0 |
| Sanitized | 169 | 22 | 19 | 0 |

Per profile, the eighteen normal Phase 18 children report exactly:

- 683 main assertions and 1,071 worker observations.
- 42 root invocations: 22 generated cancellations, 18 natural returns and two
  explicit host errors (invalid checked pointer and original callback error).
- 22 first successful external requests and idempotent redundant requests.
- 2,130 read attempts + 2,107 writes; 4,236 completed accesses and **one refused
  read** in the intentional invalid-pointer fixture. No failed copy is counted
  as completed and no signal is needed for that refusal.
- 27 TLS/PCR lifetimes and descriptor buffers, all reclaimed after join.
- 16 actual SDK returns, 16 scratch allocations/reclaims and 16 call-image
  leases/reclaims; 42 pointer birth capabilities/reclaims.
- 48 successful image unloads, followed by final-owner header/module cleanup
  and baseline image/ordinary-heap page counts.

The nineteenth new child is expected join exit 70 and has no orderly-reclamation
counters. Across Phase 16–18 there are nine such isolated fail-stops, excluded
from cooperative-stop and normal reclamation counts. All measured normal
allocation/lease balances are zero outstanding; SDK internal lease counters are
not directly exported, and successful final unload independently requires zero
active image leases. No forced counter reset or live-object deletion is used.

| Profile | Total safe-point hits | Branch-site hits: finite / infinite / inner / outer / memory / imports |
| --- | ---: | --- |
| Debug | 3497 | 13 / 159 / 13 / 4 / 1009 / 2 |
| Release | 5509 | 13 / 2171 / 13 / 4 / 1009 / 2 |
| Sanitized | 3582 | 13 / 244 / 13 / 4 / 1009 / 2 |

Infinite-loop counts vary with host scheduling; the other loop-result and branch
site oracles remain deterministic. Every one of the six sites is observed in
each profile. No dynamic count is substituted for the independent static audit.

| Profile | Generate + regenerate A/B (s) | Compile generated + harness (s) | New runtime children (s) | Runtime peak RSS (bytes) |
| --- | ---: | ---: | ---: | ---: |
| Debug | 0.536020 | 12.366158 | 3.600005 | 30,883,840 |
| Release | 0.127679 | 12.887404 | 3.992139 | 24,690,688 |
| Sanitized | 0.589110 | 15.443407 | 2.773308 | 68,354,048 |

Durations are sums of independently supervised child wall times. New-suite
compiler-inclusive peak RSS is 367,509,504 / 392,544,256 / 420,872,192 bytes;
selected final core children total 143.692804 / 129.301288 / 164.390125 seconds.
The independent debugger reference control peaked at 945,487,872 bytes, which is
not guest runtime RSS.

The full Phase 5–18 workflow passed. The Phase 18 suite was then rerun in all
three profiles after moving the request timestamp past every readiness wait.
Only the test timer changed; runtime/source/SDK hashes for the retained Phase
5–17 regressions are unchanged. Final source and artifact verification selects
those regressions and the three corrected Phase 18 runs. Earlier coordination-
inclusive timings remain private and are excluded from final latency results.
There were no unexpected native faults or failed runtime groups in these runs.

Separate passing controls are not added to the 575 count: 27 foundation/SDK-leaf
groups, 40 Python unit tests, 33 import-source audit records, the 33-entry
signature matrix and debugger reference transitions `[2, 2, 2, 2, 1]` with one
destructor. Three VMX profiles each characterize 212,068 scalar/vector cases;
known infinity/NaN/overflow differences remain, with hardware equivalence UNKNOWN.
The watchdog unit tests include one deliberate timeout/process-group cleanup,
one expected SIGTERM and an explicit nonzero-exit control; these are separate
from zero unexpected core timeouts/signals.

Host: macOS 15.7.7 ARM64, Apple clang 17.0.0 (clang-1700.6.4.2), CMake 3.31.6,
Ninja 1.11.1.git.kitware.jobserver-1 and Git 2.50.1. Clients use O0 Debug, O2
Release, and O2 ASan+UBSan without recovery; the reused Sanitized SDK uses O1.
System frameworks, CommonCrypto and handwritten assembly are outside full
sanitizer instrumentation. No ThreadSanitizer or general race-freedom claim is
made. Existing FPCR operand-width compiler warnings remain recorded.

| Product | SHA-256 |
| --- | --- |
| Original synthetic XEX A | `b478d6e13ca172aad7220e29fa4c4a0c25dc03c0d285d835ac42ef1642e32b37` |
| Emitted body A | `de0ad1ecbdf75d5bb648d0466840cdc135896604af341183fb7a39a1c255b83d` |
| Original synthetic XEX B | `4d2134d3391be9fadb42ff1ca59d96fa5d581baca782ee1a96ad01067c03e01c` |
| Emitted body B | `9cec651f66d8bd5111fc35354240c24d7aa022e9cf55150b9655064f0240872b` |
| Pinned Debug CLI | `88ec5114eaebed3f7a89e52fb661e3452a73a66917b7126bfa8236cc74fb779f` |
| Pinned Release CLI | `c8e1e6056a1793377b750745279af4dbe1f5ef1c02fdb4ec26642eea8baf97e3` |
| Pinned Sanitized CLI | `a2cd6771a92cfdd8a4889ef83194891b8ba383ef58ce6a59db33046a8c15be8a` |

All profiles and forced regenerations agree on emitted source hashes. Actual
objects, binaries, manifests, fixtures, source files and SDK libraries are
verified against final private records; binary object bit reproducibility is
not claimed. No proprietary input is consumed by a runtime test.

## Remaining limitations and readiness

**VERIFIED:** production configuration-level safe points execute inside original
generated functions and stop an infinite loop without host-driver return,
forced termination or manual C++ replacement. Coverage claims apply only to the
nine explicitly audited original functions and their six backward branches.

**PARTIAL:** policy generation is specialized to the original fixture decoder;
there is no automatic general-title CFG instrumentation, arbitrary indirect
control-flow proof, interruptible SDK execution, guest exception unwind or title
scheduler. The 16-instruction stride is not a bound on host scheduling, locked
memory copies or synchronous SDK latency. Native C++ is not sandboxed. The
4-KiB guest / 16-KiB host raw-access limitation remains; these XEX image pages
are 64 KiB and checked copies do not provide complete guest isolation.

**A — CONDITIONAL GO for broader original synthetic integration.** Next validate
CFG-derived stop policies and indexed site identities against the same complete
loop/permission/lease/error contracts, with comparative measurement. Preserve
all check locations and reject missing coverage before execution. No optimization
should remove a required check or silently widen the supported ABI.

**B — NO-GO for private game-code execution.** The 24 denied imports,
uninstrumented private generated-memory paths, unsupported atomics/partial
vectors, MMIO/fetch, typed-vector ABI, guest exceptions, general relocation,
title TOC/TLS/import/stop contracts and Nintendo Switch Horizon runtime remain
unresolved. Full Xbox hardware equivalence, game compatibility and Switch
behavior are UNKNOWN. No game execution or boot is attempted.

## Reproduction and publication

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$TESTS"
python3 tests/generated-loop-stop/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# Add --sanitize only for a Sanitized SDK.
python3 tests/generated-loop-stop/test_fixture.py
python3 tools/check_public_tree.py
```

[The suite recipe](../../tests/generated-loop-stop/README.md) explains private
output paths and finite budgets. The full workflow checks the nineteen-patch
SDK and runs Phase 5–18 plus separate original controls across all three
profiles. Private results retain versions, source/artifact/library hashes,
regeneration checks, exit codes, counts, sample timings and RSS. Generated files
remain unchanged. No binary-object bit reproducibility claim is made.

Only reviewed original source/tests, English documentation and project metadata
are published. SDK-linked suites run locally; public CI checks the allowlist,
documentation, original encoders/census/import matrix/watchdog and standalone
foundation O0/O2/Sanitized tests. CI does not fetch an SDK, game data or generated
game code. No GitHub Release or game build is created.
