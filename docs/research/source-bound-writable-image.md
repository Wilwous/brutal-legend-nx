# Source-bound writable native regions — T29–T40

2026-11-04. Baseline `3d32db201476dc4200205aac2b6dc89d0c73ed95`.
**VERIFIED bounded original synthetic contracts; title RW draft BLOCKED.**
No original title PPC is generated, compiled, linked or executed. No title-derived
executable image, new NRO, hardware deployment, SDK patch or loader relaxation.

## Contract and implementation

`writable_image::Source` binds module identity and instance, full source digest,
exact source offset/length and interval digest, exact guest destination and explicit
native authority. Its canonical digest must match the trusted descriptor supplied
by the native caller. Bounds are checked before narrowing; source is limited to
64 MiB, one region to 1 MiB. There is no synthesized zero-fill or relocation.

The only admitted RW policy is `original_synthetic`. `provenance_only` explicitly
refuses before allocation: content identity and PE declarations cannot authorize
RW. Authority is a trusted native declaration, not cryptographic proof of source
ownership or original Xbox behavior. No title authority was invented in this cycle.

`Region` retains ModuleLife, reserves the exact logical slice, commits staging
write-only, copies the exact bytes, changes to read-only for a copy-digest check,
pins the allocation and publishes RW only after validation. Its typed reference
and borrow are unavailable until initialization completes. Seven injected failure
boundaries and real bounded-arena exhaustion verify rollback. There is no backing
pointer escape. Raw Space management remains trusted native infrastructure;
it is not a hostile-host sandbox or module isolation for arbitrary host code.

Borrows validate module/space, object/allocation generation, complete span and
current rights. Read-only borrows cannot write. `Borrow::lend` checks the real
execution scope's module and Space before lending its exact handle. Existing
borrows retain memory during wrapper teardown; closing denies new admission.
Leases refuse free/decommit, but permit owner-driven protection revocation, which
is rechecked on every access. A copied write view cannot retain revoked rights.
The native Space/ModuleLife must outlive borrows; unrelated raw management pins
require retaining the wrapper and retrying close after pins drain, as in T16–T28.

## Logical permissions and unknown ranges

Existing Space/checked-memory code already validates exact byte spans and one
allocation before copying. It is unchanged. The new tests cover first/last allowed
bytes, one-byte under/overflow, cross-boundary access, mixed RO/RW slices, reserved
uncommitted storage, stale generation, wrong module, revoked rights, rollback and
worker access through wrapper teardown. A modeled 392-byte unknown interval stays
unreserved; first/last probes and cross-boundary reads/writes refuse. Adjacent
regions in the same logical 16-KiB page do not receive enclosing-page permissions.
Cross-owner transfers refuse even if both sides are readable.

The checked arena is a private vector, not mprotect-backed per-guest-page storage.
Logical checks are independent of host page size. This cycle does not experimentally
prove 4-KiB/16-KiB hardware page isolation or physical aliases. No guest alias API
is exposed by this contract, and no page-rounded loan is created. Generated accesses
must use scoped checked operations; direct native Space authority remains trusted.
PPC atomics, MMIO, partial-vector/fetch paths and guest exceptions are not added.
The generated stack update-form store uses the existing checked seam; no new atomic
or reservation semantics are claimed.

## Original title write proof and private draft

A separate bounded raw-word audit independently checks the two fixed stores against
the preserved T9 instruction words, computes their constants from the authored
static sequence without executing it, checks source-offset mapping, compares A/B
bytes, and checks section boundaries and initial interval digests. Both are BE32
stores; their union is eight consecutive in-section bytes, disjoint from the known
392-byte out-of-section interval. Payloads are defined constants in the selected
prefix. Detailed words, addresses, section coordinates and values remain private.

No target read occurs in the selected 19 normal sites. This says nothing about
pre-entry initialization, unknown continuation callees or aliases. Incoming stack
identity is incomplete, so the six other stores must not be assumed disjoint from
all image/gap ranges merely because these two fixed targets are known. Complete
write coverage, alias behavior, required persistence and future consumers remain
UNKNOWN. PE RW is a declaration; current title permissions, commitment, native
owner/lifetime and historical loader behavior remain unproved.

The private eight-byte candidate records exact source initialization and the
required module/plan/rights/lifetime obligations. Three-profile data-only attempts
use `provenance_only` authority and refuse with zero allocated or mapped RW bytes,
zero PPC entries and no title import binding. Status: **DRAFT/BLOCKED**.
A valid generic native allocation design is not an admitted title-compatible RW
object, and neither is full executable NativeImagePlan admission.

The 392 bytes remain inaccessible under the proposed policy. The 185,980-byte
virtual tail and 1,474,436-byte SizeOfImage remainder remain unknown, not zero-fill.
No title permission was derived from byte provenance or a synthetic default.

## Original production-generated integration

An independently authored XEX extends the prior original module workload with a
second output store to a source-backed eight-byte region. Production ReXGlue
v0.10.0 generates five functions / 34 instructions / 34 whole-instruction hooks;
13/13 scalar memory sites use checked macros. Repeat generation hashes match;
no generated C++ body is edited. A separate decoder checks instruction
correspondence and a public fixed-word oracle verifies both original store encodings
and expected BE bytes `00 00 00 1f 00 00 00 1f`.

The workload retains the RO header/metadata/export/import graph, executes the
explicit original native query hook, uses stack/PCR/TLS, direct calls/returns,
source-bound initialization, controlled stop and final unbinding/teardown. It
returns 31 in two successful runs per profile (caller and joined worker). Two
additional runs complete the query before expected memory refusal, so native
queries=4, completed runs=2. The query hook is not an Xbox SDK export.

A crossing output layout permits the first store and refuses the second. The
first effect remains visible. Revocation at the first generated output-store hook
refuses without publishing that store. Individual checked transfers are bounded;
the entire PPC sequence is not an atomic transaction or guest rollback/unwind.
A real foreign-module Scope is refused when lending the region. Eleven refusals
assert no additional generated entry where pre-entry rejection is mandated.
Dynamic memory denials occur after entry and are reported separately.

## Reproduction and host validation

From a checkout, choose new external directories; no proprietary input is needed:

```sh
python3 tests/writable-image/run.py --workdir "$OUT/region"
python3 tests/writable-startup/test_fixture.py
python3 tests/writable-startup/run.py --workdir "$OUT/startup" --toolchain "$TOOLCHAIN"
```

The prepared toolchain is pinned to SDK
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, CLI SHA-256
`6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c`.
Templates reuse the original module-startup checked seam; outputs stay external.
Every build/run uses a monotonic external 30-second process-group watchdog.
Debug is O0, Release O2, Sanitized O1 with ASan+UBSan and no sanitizer recovery.

| New suite, per profile | Non-byte positive assertions | Expected refusals | Separate byte comparisons |
|---|---:|---:|---:|
| RW region/subrange/lifecycle | 17 | 44 | 16 |
| Generated module/image integration | 33 | 16 | 8 |
| Total | 50 | 60 | 24 |

All three profiles pass with zero outstanding allocations, leases or registrations,
zero unexpected faults/timeouts/sanitizer findings. Assertion counts are not
independent Xbox compatibility facts. The private provenance-only draft also
refuses in all three profiles with zero PPC entries/RW mappings.

Applicable existing NativeImagePlan, image ownership, header, module graph, variable
binding, offline data/context, T8/T10/T12 generated stack/escape/stop and six checked
foundation cases pass in all profiles. Static T11/T12/T14/T15 models also pass.
The final private inventory records exact child exits, expected invalid-input
processes, timing, RSS, commands and logs. No new pre-existing runtime defect was
confirmed; checked-memory and all existing runtime headers remain unchanged.
Hosted CI runs portable RW and original instruction-oracle tests with existing
public jobs; production CLI integration remains local because CI has no private
prepared toolchain. No new Horizon-specific backend/syscall changed, so no new
hardware candidate is justified by this cycle; new target execution is UNKNOWN.

## Stage and gate decisions

| Area / stages | Decision |
|---|---|
| T29 exact obligations, T32 finite fixed-write audit | VERIFIED within selected static prefix |
| T30/T34 generic logical subranges/gap isolation | VERIFIED checked-path scope; hardware page isolation UNKNOWN |
| T31/T35 source-bound transaction and ownership | VERIFIED original synthetic native contract |
| T33 private bounded title RW object | DRAFT/BLOCKED, no RW title admission |
| T36/T37 production synthetic writes/module integration | VERIFIED declared host workload |
| T38 startup reassessment | PARTIAL; no new title state/continuation proof |
| T39 host tests/publication, T40 readiness audit | Completed within reported scope |

T40 A–D generic RW/subranges/lifetime/generated integration: VERIFIED bounded.
E/F title source-byte provenance and two fixed writes: VERIFIED finite scope.
G/H title current RW/commitment/owner/lifetime: UNKNOWN/BLOCKED.
I private bounded RW admission and J full image: BLOCKED.
K title context: PARTIAL; L exceptions: BLOCKED; M game execution: NO-GO.

Historical G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED.
Native N-G4 BLOCKED / N-G5 PARTIAL / N-G8 BLOCKED.
No gate is promoted. All 13 original variable providers remain unadmitted.
Original r1/r28–r31/LR, full PCR/TLS/thread provenance, stack escape, unknown
continuations/computed targets and six exception-overlapping sites remain blockers.

The single next proof remains the applicable title-specific native RW authority
and initialization/lifetime contract for these exact eight bytes. If that is
established independently, the next relevant continuation obligation is the first
unproved callee's consumer/persistence/alias and return summary; no full-game
reachability expansion is justified. Do not begin T41 or execute original PPC.

Private ledgers, data-only refusal commands, diagnostics, preservation identities
and recovery material are retained in a new checkpoint. Original inputs, SDK locks,
protected branches/tags and prior hardware-verified artifacts are unchanged. Only
original generic sources, fixtures and sanitized documentation are public.
