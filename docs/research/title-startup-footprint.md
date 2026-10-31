# T9 — bounded title startup dependency proof

2026-10-31; baseline `3ab473500a8686e127da22b1323e329c1fd4456e`.
**Bounded static investigation complete. Title admission BLOCKED; execution NO-GO.**
No title PPC was run, compiled, linked, generated, mapped or deployed. Runtime,
NativeImagePlan, SDK locks, protected branches, NROs and previous private
checkpoints are unchanged. This is not a hardware or loader observation.

## Scope and method

Fresh original XEX/A/B fingerprints agree with preserved identities. The frozen
scope contains **19 normal instruction sites**, **eight memory sites**, one direct
helper call/matched return and **one excluded stop instruction**. All 20 source
identities agree with T3/T7 and A/B bytes; all have DERIVED content and PE code
declarations. These declarations do not establish live executable rights/ownership.

Instructions, PCs, addresses, detailed context/dependency/stop manifests and exact
symbolic intervals remain private. Public tests use independently authored toy
words, never a title prefix or renamed title fixture.

The new [static analyzer](../../tools/startup_dependencies.py) propagates constants
and symbolic expressions on one explicit bounded normal route. It records reads
before definition, register writes, low-32 effective addresses, store-before-update
ordering and matched calls/returns. It refuses unknown computed/conditional paths,
repeated sites, escaped calls, unproved returns and source/coordinate mismatches.
It never invokes instructions, generates code or admits memory. Partial register
precision remains explicit; low-32 updates do not invent zero upper bits.

Pinned SDK emitters provide implementation evidence for the selected pipeline,
not original Xbox live startup. Independent field/arithmetic checks verify all
eight memory effects against the old graph. Ledger/section checks bind the fixed
stores. No full decoder, ABI, exceptional-path or reachability proof is claimed.

## Context and stack

Seven incoming symbols appear in reads before definition after canonical nop
normalization; six affect memory effects. A separate private per-site encoding
ledger also records the nop's identity register read/write without inventing a
value requirement. One other identity read is overwritten before any memory
consumer. No concrete
incoming register value is proved. Initial stack pointer/ownership and saved
GPR/root-LR payloads remain unknown. The helper return is fixed by its preceding
call; initial root LR is saved, not used as an indirect target in this scope.

The stack has **six stores, 40 distinct written bytes** in two disjoint symbolic
intervals, with a 492-byte envelope. Its 452-byte hole is not a write/initialization
requirement. Initial address, live size/rights, page placement and lifetime remain
UNKNOWN. A declared default stack size is metadata, not a live allocation.

The T8 64-byte leaf window misses 12 required write bytes and cannot serve as this
title contract. A whole-allocation RW check alone also does not prove that every
instruction-relative access lies inside that allocation: incoming pointer,
no-wrap arithmetic and footprint containment need separate evidence. This is a
caller contract obligation, not a demonstrated T8 runtime defect. T8 continues
to enforce its declared span and actual checked accesses.

All accesses need full-span commitment, suitable alignment under the proposed
contract, current write permission and retained independent ownership. Page
crossings depend on the unknown pointer. No physical guard isolation or synthetic
startup constant is borrowed as title evidence.

No FPR, CTR, CR/XER, r2 or r13/PCR/TLS value is consumed by this normal route.
Their complete startup/exception/native-runtime contracts remain UNKNOWN. Absence
of a direct read does not validate arbitrary PCR/TLS preparation.

## Memory and imports

| Sites | Static evidence | Live native admission |
| --- | --- | --- |
| Six stack stores | Symbolic incoming-stack offsets and payloads | BLOCKED: no independent owner, pointer, bounds or rights |
| Two fixed scalar stores | DERIVED bytes equal in A/B, inside PE-declared RW data | BLOCKED: no live mapping, current rights, owner or lifetime |

The fixed spans do not intersect the 392-byte gap, either unresolved tail or
historical variable-import slots. The normal scope has no loads or import calls.
**Zero of the 13 historical variable imports is required here**. No provider was
added and no historical pointer reused. Expanded startup use remains UNKNOWN.

Their containing data page nevertheless extends into the 392-byte gap. Exact
in-section stores do not prove whole-page permissions or satisfy the strict
page-based NativeImagePlan. That indirect page-admission dependency remains
BLOCKED; no page is expanded to make validation pass.

Without an independent no-alias stack capability, symbolic stack spans cannot
be assigned image locations or declared incapable of overlapping forbidden
ranges. The gap and tails retain their status; nothing is expanded or zero-filled.

## PC and stop proof

Static correspondence covers 19 normal sites plus the excluded boundary, one
call and its matched return. No unknown computed target occurs on that normal
route; transfers beyond the boundary remain unsurveyed/UNKNOWN. The next initializer
stays excluded. Six normal sites intersect existing exception metadata; scalar
interpretation does not resolve dispatch, unwind, continuation or fail-stop.

The private manifest requires exact source/PC/function/context binding before
all 19 normal sites and refusal before the excluded instruction, mediation of all
eight stores and denial of unproved/exceptional transitions. These are **20 proof
obligations**, not installed or observed hooks. Generated capability exists in
previous synthetic scopes only; no title hook was generated or observed. Eight
changed private manifests refuse against externally pinned bytes/coordinates.
Full runtime stop coverage remains UNKNOWN.

Admission fails for five concrete reasons: missing owned stack/start pointer;
missing title context/native handoff; absent live data capabilities; absent code/
PC/stop binding; unresolved exception/fail-stop behavior. Fixing one does not
discharge the others or authorize a smaller fragment automatically.

## Regression evidence

The [original static fixtures](../../tests/startup-dependencies/test_static.py)
cover old-base update order, signed/address construction, exact unions with holes,
dead reads, unknown addresses, trusted binding, refused control flow and excluded
instructions. A new-analyzer JSON tuple/list mismatch was reproduced on the toy
and corrected with JSON-stable dependencies; stored private proof now revalidates.
This was a new analysis-tool defect, not a pre-existing runtime/loader defect.
Failing diagnostics/red regression remain private. A private launch-path error
before any regression ran was corrected and retained separately.

| Suite | Per-profile evidence where applicable |
| --- | --- |
| New static analyzer | 26 Python methods, once; eight private proof-mutation refusals |
| Existing CFG / provenance / initialization | 22 / 8 / 27 Python methods |
| T8 owned window | 5 positive / 16 refusals / 0 byte comparisons |
| T8 production-generated original entry | 39 positive / 28 refusals; 28 zero-entry deltas; 0 byte comparisons |
| Foundation replay | 108 combined checks; original generated bodies unchanged |
| T7 inert data / real SDK context | 29 positive / 38 refusals / 32 bytes; 26 positive / 23 refusals / 32 bytes |
| N1 plan ownership | 26 positive / 49 refusals / 16,384 byte comparisons |

C++ suites pass Debug, Release and ASan+UBSan. Python methods are not counted as
three compiler profiles. Replays use the preserved production-generated original
Foundation fixture and pinned real SDK headers, with no new generation. Seven
valid leaf entries and three stop/start/join cycles per profile leave allocations/
leases zero. T7 reader controls also refuse malformed input as expected.

Final selected evidence: 57 compile/test/analysis children, 41.108 seconds summed
child wall time; peak RSS including compilers 315,441,152 bytes, noncompile
43,319,296 bytes. Nine malformed-reader exit-1 results are expected. Final unexpected
failures/faults/timeouts/sanitizer findings: zero. Earlier diagnostics were resolved,
not suppressed. Selected children use external monotonic 30-second process-group
watchdogs. Source identities bind the final analyzer and unchanged runtime tests.

```sh
python3 tests/startup-dependencies/test_static.py
```

For host runtime/context reproduction use fresh-output commands in the
[T8 report](pre-entry-stack-admission.md). Exact private inputs/commands/recovery
paths stay in the private report. CI runs new static and existing SDK-independent
tests; it does not replace local real-SDK/generated-body evidence.

## Decision

Historical G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED and native N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED are unchanged. Original execution remains NO-GO.

Smallest useful next proof: independently owned, committed, nonwrapping writable
stack capability bound to the intended thread/entry and initial pointer, containing
all six spans. Validate it without executing title instructions. Data ownership,
code/stop and exception obligations still prohibit execution afterwards.
T9 stops here; no subsequent stage starts.
