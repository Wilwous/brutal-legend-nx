# T12 — stack pointer consumers and bounded renaming

2026-11-02; baseline `46e7f151246cc9c59dc5cd2d104f287240942140`.
**Bounded investigation COMPLETE; original title equivalence PARTIAL/BLOCKED;
original entry NO-GO.** No title PPC was generated, executed, compiled, linked or
deployed, and no runnable title image was created. Production runtime, loader,
SDK locks, protected refs/tags and hardware-verified packages are unchanged.
Detailed source coordinates, words, symbolic mappings and matrices stay private.

## Finite source boundary and known consumers

T9–T11's frozen source identities were rechecked against both preserved A/B
representations and the original XEX fingerprint. The excluded direct call was
decoded as data. Its directly addressed callee has a three-word straight-line
prefix before another direct call. That prefix saves the newly defined link and
creates another backchain edge. It has no memory read or absolute stack predicate.
The nested target was not followed. Separately, the direct call's fallthrough has
one straight-line word before a different call; its reachability is conditional
on a normal callee return, which is not proved. It is not appended to the original
normal trace. Each prefix had a fixed 32-word ceiling and stopped at its first
transfer; no discovery/whole-game traversal was performed.

Twenty prior source identities and six additional identities were compared A/B;
new direct-call targets/continuations and prologue fields were independently
checked with literal bit-field arithmetic, separately from the existing scalar
dependency analyzer. This is static encoding/dataflow correspondence, not proof of
live residency, normal return, execution permissions or exception behavior.

There are **no known backchain readers in these observed prefixes**. This does
not establish that there are no readers. The stack pointer escapes into stack
memory, frame-derived registers and the next callee's incoming context. The next
callee, original root caller, future restoration/unwind/debug observers and
exceptional transitions remain unknown. The already verified helper call/return
is not a proof of those boundaries. The initial LR remains an opaque saved input;
the callee's new known link value does not eliminate the saved old value's unknown
consumers. No SP payload enters the known fixed global stores, but no conclusion
is drawn about unexamined providers or future stores.

## Five different equivalence claims

| Claim | Result and scope |
| --- | --- |
| Bytewise equality | False for distinct stack bases because stored backchain values differ |
| Normalized memory equality | Verified on original fixtures after checking and renaming only explicitly typed pointer fields |
| Pointer-graph equality | Verified for selected fixture nodes/aliases; conditional local title store edges, without actual ownership/lifetime proof |
| Bounded observational equivalence | Verified only for the declared relative/TLS synthetic observers; title continuation BLOCKED |
| Full original title-entry equivalence | BLOCKED; no gate promotion |

The [generic model](../../tests/stack-provenance/model.py) names related allocation
objects, extents, live epochs and rights. A stack pointer at object offset `o` maps
to the corresponding object offset, with representability and bounds checked.
It is a declaration/check of a relation, not evidence that historical objects
actually exist. Real checked-memory capabilities supply the synthetic ownership
evidence separately.

The relation requires aligned no-wrap layouts, equal extents, valid ownership,
commitment/RW rights, nonaliasing with fixed objects, typed frame/backchain fields,
matching opaque values, corresponding active lifetimes and stable identity through
every participating call/return. An external provider must explicitly honor the
same relation; delivery of a raw pointer does not automatically qualify it.

Selected interior-pointer aliases and order within one object are preserved.
Cross-object order is refused without another proof. Even one-past equality at
adjacent object boundaries is not automatically preserved: the model contains an
explicit counterexample when relocation removes that adjacency. Pointer-to-integer
observation remains raw scalar observation. No address-looking integer is guessed
to be a pointer, and no persistent cell is silently normalized. Missing consumers
prevent closing an observational-equivalence proof.

## Original production-generated nested-frame fixtures

[Seven independently authored variants](../../tests/stack-provenance/fixture.py)
pass through pinned production ReXGlue `recompile-tests`: 207 instructions in
21 functions. The variants share three nested frames, create/read three backchain
links, copy frame pointers, preserve a scalar payload, read through aliases, store
a temporary pointer within the leaf frame, consume it there, and restore the SP
from each backchain. These original offsets/register choices are not title code.

The relative and PCR/TLS-read variants use declared native observers. Deliberate
non-equivalent variants publish a raw pointer to a persistent cell, observe an
address as a scalar integer, return a dead-frame address as a raw result, or compare
against a fixed absolute predicate. An indirect-call variant is generated but
always refused before any PPC entry. Requests claiming the non-equivalent variants
satisfy the relocation contract are also refused before entry. Safe counterexample
execution is a separate explicit fixture mode; it never invokes unknown targets
or dereferences stale pointers.

Default/custom generated function bodies match; regeneration is deterministic.
Every instruction, memory width, direct-call continuation and branch is reconciled
against independently decoded original input coordinates. Generated C++ is never
manually edited. A mandatory fixture prologue binds exact Scope/context/module/
source/function identities. The compiled source tag is not a general image
authenticator; full hashes and pinned CLI identity are recorded privately.

The [C++ oracle](../../tests/stack-provenance/probe.cpp) specifies literal expected
stack bytes and registers without encoder/decoder tables. The separate
[Python observer](../../tests/stack-provenance/verify.py) reads actual snapshots,
uses a different serializer, checks raw registers/bytes and reconstructs the typed
graph. It imports neither the fixture encoder nor generated C++/C++ normalizer.
It is an independent observation oracle, not an independent Xbox loader or full
ISA engine. All profiles produce identical observation-file hashes.

## Context, cleanup and lifetime limits

Each native fixture retains stack/cell/PCR/TLS/module leases and an exact
thread/source association. Entry order, stack depth, scalar inputs, parent-derived
frame values, native exit cookie and direct-call LR are checked after the trusted
controller and before each actual generated entry. The exit cookie is an explicitly
declared native return boundary, not a fabricated original Xbox return address.

Negative cases cover insufficient/misaligned stack, wrong ownership, thread,
source, TLS, return or scalar dependency, unsupported control flow, revoked rights,
stale views, and cleanup/decommit attempts with live execution leases. Two native
tasks verify independent PCR/TLS associations. Three allocation-failure stages
check rollback; stop before leaf effects leaves the two earlier frames observable
and joins before cleanup. That stop has two prior generated entries, not zero.

**An allocation lifetime is not a logical frame lifetime.** Popped-frame memory
remains readable while its backing allocation lives. The frame-policy model denies
a returned dead-frame pointer dereference; the production memory backend does not
implement arbitrary per-frame capabilities. After stack destruction the stale
capability and unallocated address refuse; after address reuse an old capability
still refuses, but the same raw number can address the new allocation. Thus these
tests do not prove universal detection of stale raw guest pointers or general
use-after-return safety. The returned-address variant is an explicit counterexample,
never an admitted frame-lifetime contract.

## Validation

Per compiler profile (Debug `-O0`, Release `-O2`, ASan+UBSan with no recovery):

| New generated test measure | Count |
| --- | ---: |
| Positive semantic assertions | 905 |
| Expected refusals / no-additional-entry checks | 40 / 40 |
| Completed differential runs | 36: six executable variants, two payload/TLS sets, three stack bases |
| Equivalent normalized observer pairs | 12 |
| Intentional counterexample pairs | 22: persistent 6, integer 6, returned address 6, absolute predicate 4 |
| Nonrefuting absolute pairs | 2; not an invariance proof |
| Raw stack differences / checked pointer-graph pairs | 36 / 36 |
| C++ literal byte-oracle comparisons | 638,976 |
| Differential byte comparisons / independent Python byte comparisons | 589,824 / 589,824 |
| Actual generated entries | 119: 108 differential, 3 returned-address lifetime, 6 concurrent, 2 before stop |
| Outstanding allocations / leases | 0 / 0 |

Regressions also pass in all three profiles. Counts are deliberately separated:

| Suite | Positive / expected refusals per profile |
| --- | --- |
| T8 window | 5 / 16 |
| T8 generated entry | 39 / 28 |
| T8 foundation replay | 108 combined checks; no separate refusal count |
| T10 | 87 / 72; 180,224 byte comparisons |
| T11 | 486 / 34; 12 equivalent pairs and 10 intended counterexamples |
| NativeImagePlan | 26 / 49; reported semantic total 75 includes the 49 refusals |
| Horizon core | 20 combined checks |

Eleven new model/input/observer-parser methods, eight T11 model methods and
26 context methods run once (45 methods, not three compiler profiles). The selected
67 generation/build/test/analysis children total 61.811 child wall seconds. Peak
RSS is 342,310,912 bytes including compilation and 41,795,584 outside compilation.
Each selected child has an external monotonic 30-second process-group watchdog.
Unexpected failures, faults, timeouts and sanitizer findings: **0**. Earlier passing
development runs remain private and are excluded from final totals; no failing
diagnostics were discarded.

```sh
python3 tests/stack-provenance/test_model.py
python3 tests/stack-provenance/run.py --workdir "$FRESH_PRIVATE_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN"
```

The runner requires a fresh external output directory and the pinned T8–T11
toolchain. It creates its own original PPC inputs and has no title-input or Switch
deployment mode. CI covers the public model/inputs and existing SDK-independent
suites; local real-generator/SDK execution is separate recorded evidence.

## Decision and smallest missing proof

Native stack mechanism and the declared original fixture contracts are VERIFIED.
The observed title stores support a **CONDITIONALLY PROVEN** local typed pointer
relation. Actual title applicability is **PARTIAL**, and title continuation
observational equivalence and entry remain **BLOCKED / NO-GO**.

The 392-byte permission gap, 185,980-byte virtual tail, 1,474,436-byte SizeOfImage
remainder, 13 unverified variable providers, opaque startup registers, original
PCR/TLS/thread identities, image ownership/lifetime and exceptional transitions
remain. Six prior exception-metadata-overlapping sites retain existing refusal.
No title byte gains executable rights. Historical G4 BLOCKED / G5 UNKNOWN /
G8 BLOCKED and native N-G4 BLOCKED / N-G5 PARTIAL / N-G8 BLOCKED are unchanged.

Smallest next obligation: a finite summary of the first still-unproved direct
callee's incoming frame-pointer/backchain observations and normal return. Stop at
the first unsupported observer rather than assuming it preserves renaming. No
return, provider or exception behavior is invented. T13 is not started.
