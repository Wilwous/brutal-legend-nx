# Native module → bounded data → synthetic startup — T17–T28

2026-11-04. Baseline: T16 `5dd9f8d4e6d425804f02f02a76f9e915daa5baa7`.
**VERIFIED bounded generic host contracts; original game entry NO-GO.** The
T17–T28 cycle completes the independent engineering and finite private evidence
audits. Title-dependent acceptance remains PARTIAL/BLOCKED as listed below.
No original PPC is generated, compiled, linked, executed or deployed.

## Stage and readiness decisions

| Stage | Result | Evidence / remaining boundary |
|---|---|---|
| T17 | VERIFIED, bounded | Only the supported metadata word; private original-header data-only replay |
| T18 | VERIFIED, tested interleavings | Retained graph, atomic sibling retirement, generation, rollback and last-reader cleanup |
| T19 | VERIFIED generic / BLOCKED title | Separate exported cell and import slot, exact BE32 pointer depth |
| T20 | VERIFIED selected native lookup | Present/missing/zero/malformed/duplicate distinctions; historical conformance UNKNOWN |
| T21 | VERIFIED generic / PARTIAL title ledger | Selected self-module registry/binder; 13 original providers unadmitted |
| T22 | VERIFIED generic / BLOCKED original rights | Exact logical subranges; no new independent evidence for the original gap |
| T23 | VERIFIED native RO forensic projection / BLOCKED image | 11,896 proven bytes readable, 392 excluded; required original writes not admitted |
| T24 | VERIFIED synthetic / PARTIAL title | Owned stack, module, PCR/TLS and entry checks; actual title state unproved |
| T25 | VERIFIED selected fail-stop / BLOCKED exceptions | Strict PC/call/return/stop policy; no guest exception dispatch or continuation |
| T26 | VERIFIED original synthetic host integration | Actual production generation and unchanged emitted PPC execution |
| T27 | VERIFIED selected host regressions | Debug, Release, combined ASan/UBSan; new Horizon hardware execution UNKNOWN |
| T28 | VERIFIED finite audit | No gate promotion; original game entry NO-GO |

| Readiness area | Status |
|---|---|
| A–E: generic metadata, header, selected variable, lookup, subranges | VERIFIED in the declared contracts |
| F–H: synthetic startup, PC/stop, integrated execution | VERIFIED for the original fixture and tested paths |
| I: title header provenance | VERIFIED, private source-bound nonexecuting replay |
| J: title module provider | PARTIAL mechanism; title binding BLOCKED |
| K: title image admission | BLOCKED |
| L: title startup | PARTIAL evidence; complete context UNKNOWN |
| M: title exceptions | BLOCKED |
| N: original game entry | NO-GO |

The architecture and retained graph contract are specified in
[native module runtime](native-module-runtime.md). `Space`/`ModuleLife` must outlive
readers, and raw management pins require wrapper retention and close retry. This
is not exhaustive concurrency proof or a raw native-code sandbox.

## Exact permission boundary

`Space::reserve_slice` explicitly reserves an exact logical interval. The existing
checked arena backs it; allocation rounding never grants enclosing guest bytes.
Range checks cover the complete transfer. A request crossing the known/unknown
boundary refuses, even if its backing host page exists. Page-wide queries refuse
when the complete page is not a single admitted region. Existing page-aligned
`reserve` behavior is unchanged. All accesses in this fixture use the checked
layer; raw host pointers or unreviewed generated code remain outside this claim.

`DataProjection` validates the existing canonical DATA-only provenance plan and
source hashes, creates a new explicitly native owner, commits each known
in-section interval and seals it read-only. Unknown intervals remain unreserved.
PE flags select provenance intervals for forensic reading; they do not establish
current/historical RW or executable rights. This policy is not an original-title
memory contract. No title bytes receive executable access or a function binding.

The private three-page replay independently compares four source intervals in
both preserved representations: 12,288 bytes total, 11,896 in-section and 392
outside the relevant section. The new RO projection admits only the in-section
intervals. All three profile replays reject the unknown interval's first/last byte
and writes to each mapped interval, and finish with zero resources/PPC entries.
The independent Python concatenation digest agrees with the checked reader.

**UNKNOWN/BLOCKED:** original gap reservation, commitment, owner, current rights,
changes over time and lifetime. Known contents alone cannot supply these facts.
Coarser host protection, including 16-KiB versus 4-KiB pages, cannot independently
isolate all logical guest intervals. The checked path is authoritative; no global
protection bypass or whole-page grant is added. The 185,980-byte virtual tail and
1,474,436-byte SizeOfImage-only remainder remain unresolved. Required original
image-backed writes still refuse; no executable NativeImagePlan is admitted.

## Production synthetic integration

The independently authored NONE XEX contains five functions / 33 instructions.
The pinned ReXGlue v0.10.0 production generator discovers the functions from
original synthetic exception-directory function records, with no manual generated
body changes. Repeated generation produces identical seven-file output hashes.
A separate decoder checks every encoded instruction against emitted comments;
a fixed public oracle checks layout, calls, memory sites and arithmetic.

There are **33/33 whole-instruction hooks**, **12/12 scalar memory sites**
(seven U32 reads, three U32 writes, one U64 read, one U64 write), five generated
function entries and zero SDK function imports. No raw-address, raw arena-base,
unsupported-instruction or trap body is admitted. Code/source/plan/runtime/control
and generated map identities bind through the strict native binder. An explicit
`no_function_imports` mode requires an empty import manifest and code-region
backing for every function. The prior selected SDK-leaf mode remains the default
and retains its original regression contract. Unknown modes refuse.

The workload loads `S → P → M → H`, invokes an **explicit original native hook at
the synthetic query function's pre-BLR boundary**, reads the selected inline word,
direct-calls an add-seven helper, reads the independently seeded PCR/TLS slot,
stores the result and returns through the declared root boundary. The hook is
not a newly implemented/invoked Xbox SDK export or general import ABI. The
independent expected result is `21 + 7 + 3 = 31`. The declared synthetic stack,
r13/PCR association, return target and preserved r14 are checked. r2 is explicitly
seeded but not used as proof of title TOC initialization. Full GPR/FPR/vector ABI,
thread startup and Xbox return/error semantics are not claimed.

Per profile, two successful runs occur (caller thread and joined owned worker),
two selected queries complete, and one loop observes cooperative stop at its
fourth hook. The completed path uses a present field. Its missing-field branch is
statically checked; separate selected lookup/SDK tests cover absence. This is not
full executed-branch coverage. The indirect fixture is negative-only: an unknown
target refuses; no general indirect-call/return contract is inferred.

Twelve expected refusals cover binding/policy/identity, wrong r13, read-only stack,
midfunction/unknown entry, invalid LR, unknown indirect target, closed variable,
and explicit exception/instruction-denial injection. Nine cases assert no
increase in the generated guest-entry counter. Exception and instruction failures
are original host-hook injections, not actual Xbox exception delivery. Fail-stop
unwinds original native scopes; it does not resume unsupported guest behavior.
Provider unbinding, registry withdrawal, guest cleanup, TLS slot release, module
close and image unload finish with zero allocations, leases and registrations.

## Title evidence kept separate

The private import ledger retains all 13 identities/slots and their obligations:
known cell/scalar/structure/string/indirection shape, value semantics, owner,
initialization, permissions and lifetime. One generic selected mechanism is now
available; the other 12 are not implemented. **All 13 remain unadmitted for title
execution.** Synthetic storage is not evidence for original values or consumers.

T9–T13/T15 startup evidence is hash-bound and reviewed without reopening full
reachability: six stack stores / 40 written bytes, conditional address renaming,
unknown original caller r1/LR and root r28–r31, unknown continuation calls and
escaping local pointers. A local BL continuation and symbolic previous-frame
relationship are known only in their selected scope. Complete PCR/TLS/thread
identity, module startup, global write lifetime, computed control flow and return
semantics remain unsupported. Six original exception-overlapping sites remain
unadmitted. The native graph and successful synthetic startup do not fill these
unknowns or establish title stack/context equivalence.

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**.
Native **N-G4 BLOCKED / N-G5 PARTIAL / N-G8 BLOCKED**.
No hardware or authoritative Xbox evidence was acquired during this cycle.

## Reproduce without proprietary inputs

Run from a checkout with fresh external output directories. `$TOOLCHAIN` is the
privately prepared pinned toolchain and `$SDK_SOURCE` the locked isolated SDK
source; neither is modified or installed globally.

```sh
python3 tests/native-module/run.py --workdir "$OUT/module"
python3 tests/variable-imports/run.py --workdir "$OUT/variables"
python3 tests/data-projection/run.py --workdir "$OUT/projection"
python3 tests/module-startup/test_fixture.py
python3 tests/module-startup/run.py --workdir "$OUT/startup" --toolchain "$TOOLCHAIN"
python3 tests/native-image-binding/run.py --workdir "$OUT/old-binding" --toolchain "$TOOLCHAIN" --sdk-source "$SDK_SOURCE"
python3 tests/header-buffer/run.py --workdir "$OUT/header"
python3 tests/native-image-plan/run.py --workdir "$OUT/plan"
python3 tests/native-image-contract/run.py --workdir "$OUT/ownership"
python3 tests/native-provenance/run_data.py --workdir "$OUT/data" --toolchain "$TOOLCHAIN"
python3 tests/pre-entry-stack/run.py --workdir "$OUT/stack" --toolchain "$TOOLCHAIN" --foundation "$FOUNDATION_GENERATED"
python3 tests/stack-geometry/run.py --workdir "$OUT/geometry" --toolchain "$TOOLCHAIN"
python3 tests/stack-provenance/run.py --workdir "$OUT/frames" --toolchain "$TOOLCHAIN"
python3 tests/horizon-foundation/core.py --workdir "$OUT/core"
```

`$FOUNDATION_GENERATED` is the preserved original foundation output whose complete
hash table the T8 driver verifies. Toolchain CLI SHA-256 is
`6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c`;
SDK revision is `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`.
Generated synthetic XEX SHA-256 is
`84055836723deb8fc10db9878571cd387993af6b7c0ed2e9c7d75e5e9b8ae25f`.
No original title input is needed for these commands. Output, binaries, generated
sources, hashes, commands, process-group diagnostics and private replay manifests
are kept outside the checkout.

## Final host regression evidence

Each row passes in **Debug / Release / combined ASan+UBSan**, with `-O0`, `-O2`
and `-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all` respectively.
The table reports the suites' raw per-profile counters. Some raw positive
counters also increment for byte equality checks; those are separated below and
must not be counted as independent semantic tests or compatibility facts.

| Suite | Per-profile outcome |
|---|---|
| New metadata/lifetime | 97 positive, 21 expected refusals |
| Selected variable/lookup | 120 positive, 84 expected refusals |
| New exact RO projection | 13 positive, 13 expected refusals; 7,800 separately compared bytes |
| Integrated generated startup | 27 positive, 12 expected refusals; 9 zero-entry checks |
| Existing header + data reader | 115 + 10 positive; 66 + 5 refusals |
| NativeImagePlan | 75 semantic passes, 49 negative refusals |
| Image ownership/rollback | 45 passes, 17 negative refusals |
| Offline data/context | 29 + 26 positive, 38 + 23 refusals; 3 invalid-input processes/profile reject |
| T8 window / generated admission / foundation | 5/16 and 39/28 positive/refusal; 108 foundation passes |
| T10 stack geometry | 87 positive, 72 refusals; 72 zero-entry checks |
| T12 frame/escape differential | 905 positive, 40 refusals; 36 runs, 12 equivalent and 22 counterexample pairs |
| Prior generated native binding | smoke 19/19, repeat 152/152, concurrent 38/38 positive/negative; result 32 |
| Horizon core / checked foundation | 20 core passes / 6 foundation cases |
| Private original header / RO projection | Three-profile data-only byte and refusal checks; zero PPC entries |

New four-suite totals per profile are **237 non-byte positive assertions,
20 byte-equality assertions and 130 expected refusals**, with no failed final
assertion. The raw positive counters sum to 257 because the metadata suite includes
four byte checks and the variable suite includes sixteen. The projection's 7,800
bulk byte comparisons are separate and do not increment its positive counter. The selected final corpus has 159
bounded child processes: 150 exit zero; nine offline-reader invalid-input cases
intentionally exit one. There are zero unexpected faults, timeouts or sanitizer
findings. Expected native errors are exceptions/refusals, not host fault tests.
Summed child wall time is 159.469 seconds; maximum measured child RSS is
429,883,392 bytes (~410 MiB, including compilation). These are host measurements,
not Switch budgets or a wall-clock duration for the entire research cycle.

One additional development run of the new malformed-header tests exited one:
the test expected lookup-time refusal, but T16 validation refused earlier during
setup. The expectation was corrected to require refusal before publication; no
runtime protection was weakened. Its original logs remain private. The final
corpus above excludes that superseded development run explicitly.

Five independent static fixture tests and the unchanged selected T11/T12/T14/T15
models pass. Host generation/SDK integration is local: hosted CI has no private
prepared CLI. CI runs the portable new module, variable, projection and static
fixture suites plus existing public-tree/runtime jobs. Private evidence is not
uploaded. New target-side execution is **UNKNOWN**: no Horizon syscall/backend
changed, so no new NRO or deployment was justified. Preserve the hardware-verified
Stage 15 and N1–N3 artifacts and their prior evidence separately.

## Preservation and next action

The original XEX, both representations, 12 prior checkpoint manifests, 103
external artifacts, 1,118 pinned SDK headers, 35 SDK locks, eight actual SDK source
files, protected main/Xbox branch and 21 tags are verified unchanged. The public
central-memory changes and explicit no-function-import binding mode are reviewed
native-source changes, not SDK patch edits. Original assets/config/generated
outputs remain private and unchanged. Only generic original sources/tests and
sanitized documentation enter the research branch. No completion tag or Release.

A private cycle checkpoint retains source fingerprints, complete commands,
results (including the development failure), import/gap/startup ledgers, public
Git recovery material, publication receipts and exact local restoration commands.
No private plan, title address, header contents, import-value ledger or compiled
title object is published. See the private report for actual paths and identities.

**Single next proof obligation:** define and independently verify a source-bound
native RW owner/commitment/lifetime contract for the required *in-section*
image-backed stores, retaining the 392-byte/tail/remainder exclusions. The RO
forensic projection cannot satisfy those writes. A new contract must state its
native initialization policy explicitly and justify the real consumers; PE RW
flags and synthetic success alone do not suffice. Keep original game execution
NO-GO while startup, providers, continuation and exceptions remain unresolved.
No stage beyond T28 is started.
