# T7 — bounded nonexecuting data contract

2026-10-31 · baseline `3cafdc56939d706461fd2bd7f7e619f73aaeb169`.
**Data-only materialization VERIFIED; guest admission BLOCKED.**
No original title PPC was compiled, linked, dispatched or executed. No executable
title storage, generated game binding, runnable title image or Switch artifact
was created. Existing NativeImagePlan, binder, Xbox loader and SDK locks are unchanged.

## Two distinct contracts

The existing N1 NativeImagePlan requires an executable synthetic entry, supported
origin/initialization, owned regions and startup contracts. It is not an inert-data
API. It continues to refuse the title draft. No synthetic origin, fake code identity,
TLS padding or permissive page mapping was substituted to get it to accept.

[Offline data](../../runtime/offline_image_data.h) is a separate minimal host-only
representation. Guest addresses are labels in a checked source-offset ledger.
It accepts exact verified source intervals and an independently supplied canonical
plan identity, binds original/source/module/generation/interval identities, and
copies those intervals into ordinary vectors. It has no entry, call table, guest
mapping, executable allocation, relocation or provider interface. Its guest-access
request always refuses, including for section-backed bytes. Unknown declaration
evidence remains explicit and produces a protection refusal on guest requests.

Hashes validate caller-supplied evidence identities; they do not authenticate a
provenance assertion by themselves. The private caller verifies the previously
reconciled sources/ledger independently. Source input must remain immutable during
construction; the audit object is used on one host audit thread. Its Owner must
outlive it. Concurrent read/close and whole Xbox ABI correctness are not claimed.

An Owner prevents duplicate live materializations. Closed or foreign views refuse;
a later materialization has a distinct instance identity. Allocation/copy failure
rolls back copied bytes and the owner lease. Source gaps are not allocated or
zero-filled. Audit reads return host copies, never guest pointers or capabilities.
Resource counters describe these data objects and Space allocations/leases, not
all allocator metadata, OS RSS, secure erasure or physical memory isolation.

## Exact private slice

The byte map partitions all three previously selected pages into **four intervals**:
two complete in-section pages, one in-section fragment and the separate out-of-section
fragment. **12,288 bytes** are verified DERIVED and equal in A/B: **11,896 in-section**
and **392 outside a PE section**. Nothing was removed to force a passing result.
Each private row retains guest/source coordinates, whole-source/interval hashes,
section/declaration evidence, original ownership uncertainty, separate host owner,
lifetime and explicit guest refusal.

For the 392-byte fragment, the original XEX descriptor declares DATA and the pinned
SDK requests descriptor-wide read/write protection during loader continuation.
That is recorded as declaration/source mechanics, not ignored. It does not prove
a successful strict load, live owner/commitment/lifetime or an approved native
byte-permission capability. PE section authority is absent. No guest read, write
or execute right is granted to the fragment and no section is expanded.

The private experiment keeps the fragment as known inert data while refusing guest
access. Independently authored synthetic tests also demonstrate exact logical
allocation bounds that exclude a page remainder and reject a cross-boundary access.
Those tests do not admit these title intervals through N1.

## Private materialization result

The generic [data reader](../../tests/native-provenance/data_reader.cpp) contains
no title constants, PPC context, generated functions or guest image constructor.
Private metadata/source/trust files are read as data. Python supplies an independent
canonical identity and an oracle assembled from B; C++ copies A and checks every
output byte and aggregate identity. Four intervals / 12,288 bytes pass in Debug,
Release and ASan+UBSan. Source immutability is rechecked.

Each profile also injects allocation failure after one interval has been copied;
rollback releases the materialized storage and host owner lease. Four guest requests
refuse. Wrong whole-source identity, stale base, offset overflow and missing provenance
refuse separately. A/B selected contents remain equal even though B's whole-source
identity differs from the explicitly bound pre-binding source. B is not called corrupt.

Tracked data buffers/bytes, guest allocations and leases are zero after cleanup.
Temporary audit copies are discarded and read-only input buffers end with the child
process. No title-derived output bytes or layouts enter public Git.

## Synthetic stack/PCR/TLS, kept separate

[Context fixtures](../../tests/native-provenance/context_contract.cpp) take no
input files. They use the actual existing GuestThread/Space/Slots/Scope implementation
with the pinned real SDK PPCContext header, independently authored TLS data and
synthetic module identities. They never dispatch their original callback, execute
PPC or combine their context with title data. The header matches pristine SDK
revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`; all prepared headers are
fingerprinted privately and preserved. No SDK runtime library or code generator
is linked/run for these tests.

Verified subset: stack allocation/alignment and logical bounds; PCR TLS/self
fields; static TLS bytes and independent per-context dynamic slots; identity and
module checks; pinning; cleanup denial while active/leased; three deterministic
arena-exhaustion points, oversized TLS rollback and stale-token refusal. The
synthetic 32-byte template and existing register defaults are not title startup values.

**Observed limitation:** GuestThread::Execution validates context metadata and pins
allocations but does not itself require stack write permission. A read-only stack
passes that metadata guard; a checked write/view refuses. Both observations are
asserted by the fixture. This is PARTIAL entry readiness, not complete permission
admission or a reason to weaken checked memory. Runtime behavior was not changed
in T7. Complete thread startup, typed-vector ABI, Xbox XThread/APC/exception behavior
and title PCR/TLS/context remain unproven.

## Static PC/stop obligations

The private manifest rechecks **19 normal instruction sites plus one excluded
candidate stop site**, one direct helper call/matched return and eight memory sites
(six symbolic stack writes, two constant globals). Both source representations
agree. The fixed stores remain in known in-section content; the gap is not required
by those selected normal words. This is not a proof of full runtime reachability.

The manifest records exact source/site identities, relative stack requirements,
unknown full-startup/computed targets, original-context uncertainty, exceptional
transitions and required PC/stop/fault binding. Five read-only manifest mutations
are rejected. No executable binding or stop hook was generated. A candidate stop
does not prove fault-time containment or guest exception continuation.

## Tests and limits

All selected children use external monotonic 30-second process-group watchdogs.
Counts per Debug/Release/ASan+UBSan profile:

| Fixture | Positive semantic assertions | Expected refusals | Separate byte comparisons |
| --- | ---: | ---: | ---: |
| New inert-data contracts | 29 | 38 | 32 |
| New synthetic context | 26 | 22 | 32 |
| Existing N1 ownership/rollback | 26 | 49 | 16,384 |
| Existing overhanging-section model | 28 | 17 | 327,680 |

The original generic data-reader fixture additionally checks 32 copied bytes,
three guest denials, one rollback and three failing input controls per profile.
The private reader checks 12,288 bytes, four guest denials, one partial failure and
four failing controls per profile. The unchanged actual N1 reader separately
reports two expected origin/module/lifetime refusals per profile, with no guest image.
The legacy Horizon core reports 20 combined assertions per profile. Metadata
regressions retain 77 unit methods; private PC obligations add five refusals.
Byte counts are not added to independent semantic test counts.

Final selected 68 compile/test/audit children total **36.173 seconds of measured
child wall time**. Peak RSS including compilation: **274,628,608 bytes**;
peak noncompile child RSS: **135,938,048 bytes**. Final unexpected exits, signal
faults, timeouts and sanitizer findings: **0**. Twenty-one failing input-control
exits are expected and retained separately.

Initial failed fixtures/diagnostics are preserved. A raw read immediately after
the synthetic stack was incorrectly expected to fail, but that byte belongs to
the next separately owned PCR. A bounded diagnostic identified the assertion;
the fixture now verifies a two-byte access crossing stack/PCR ownership is denied.
No runtime check was removed or weakened. A read-only stack's metadata admission
limitation is reported explicitly above, not hidden as a successful permission proof.

## Reproduction and CI scope

```sh
python3 tests/native-provenance/run_data.py --workdir "$FRESH_DATA_OUTPUT"
python3 tests/native-provenance/run_data.py --workdir "$FRESH_CONTEXT_OUTPUT" --toolchain "$VERIFIED_PREPARED_TOOLCHAIN"
python3 tests/native-image-plan/run.py --workdir "$FRESH_N1_OUTPUT"
python3 tests/native-image-contract/run.py --workdir "$FRESH_TAIL_OUTPUT"
python3 tests/horizon-foundation/core.py --workdir "$FRESH_CORE_OUTPUT"
python3 tests/native-provenance/test_intervals.py
python3 tests/native-provenance/test_initialization.py
python3 tests/bounded-cfg/test_manifest.py
python3 tests/residency-evidence/test_residency.py
```

Use fresh directories outside Git. Public CI runs the SDK-independent data suite
and existing regressions. The real-header context matrix was run locally in all
three profiles; without the verified prepared toolchain, the runner explicitly
reports context NOT RUN and does not substitute a mock context. Private manifests,
source intervals and experiment receipts remain outside the public tree.

## Readiness decision

| Obligation | T7 result |
| --- | --- |
| Three-page source identity | VERIFIED; four exact intervals, A/B equal |
| Private inert materialization | VERIFIED; complete copy/rollback/cleanup |
| Guest-visible admission | BLOCKED; no mappings or executable image |
| 392-byte fragment rights/owner | UNKNOWN native authority; guest access refused |
| Generic stack/PCR/TLS preparation | VERIFIED selected mechanics; entry permission contract PARTIAL |
| Title startup/context | UNKNOWN / BLOCKED |
| PC/stop proof | PARTIAL static obligations; runtime binding absent |
| Variable providers | 0/13 admitted; old targets not reused |
| Original unresolved content | 185,980 tail + 1,474,436 outer bytes unchanged |
| Cleanup | VERIFIED tracked data/guest resources and leases |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED** and native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED** remain unchanged. Explicit zero-fill remains zero.
Successful data copying closes none of these title gates. No T8, Release, merge,
tag, NRO or deployment is part of T7.

**Single next engineering action:** tighten and independently test the generic
pre-entry stack permission/ownership contract on synthetic fixtures, starting
with the observed write-permission admission gap. This does not authorize title
execution; source authority, scoped guest admission, startup and PC/fault binding
still require their own complete proofs.

The subsequent [T8 targeted fix](pre-entry-stack-admission.md) closes that generic
pre-entry stack gap and updates the live synthetic context fixture to require
early refusal. The historical T7 observations/counts above and all title gates
remain evidence of their original checkpoint; no title admission is implied.
