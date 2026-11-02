# T11 — conditional native startup context equivalence

2026-11-02; baseline `83b474b6aa7a0d6a56e96cf6391ad4d52a751b6e`.
**Bounded investigation complete; actual title-entry equivalence BLOCKED / NO-GO.**
No title PPC was generated, executed, compiled, linked or deployed. Production
runtime, title loader/NativeImagePlan, SDK locks, protected refs/tags and verified
NROs are unchanged. Detailed title instructions, PCs and dependency matrices stay
private. New executable inputs are independently authored synthetic variants.

## Historical state, native state and observer

Historical Xbox entry values are not established by a writable native allocation.
An original native ABI can define its own objects/exit boundary, but that alone
does not prove those choices preserve the title's required observations.

The key distinction is **literal equality versus a typed observation relation**.
Changing a representable incoming stack pointer changes its saved backchain bytes.
Thus literal final guest-byte equality is false even for the relative fixture.
It is possible to preserve an explicitly declared observation under stack-object
renaming. That requires proving which fields are pointers, rather than rewriting
every integer that happens to resemble an address.

## Bounded theorem and limits

The [model](../../tests/context-equivalence/model.py) checks integer-linear
identities for each declared offset: translating both entry pointer and effective
address by the same delta preserves relative offsets, widths and frame geometry.
The six-store union and enclosing span are different; untouched bytes are not
assigned contents by this reasoning. This is an algebraic identity check, not a
full ISA/exception proof or an execution-admission API.

The private title theorem is conditional on:

- both pointers being aligned, representable and nonwrapping, with all spans owned,
  committed, writable and nonaliasing with fixed code/data/PCR/TLS;
- the same qualified code PCs, widths/endianness, fixed data addresses and values;
- matching opaque incoming payload/LR bits and all unchanged state included by
  the observer, unless a separate typed object correspondence is proved;
- a proved type for each renamed pointer field and an appropriate initial-memory
  relation for untouched bytes if those bytes are observed;
- no absolute stack comparison or persistent raw-pointer escape, and a closed
  normal trace with no unproved call, exception, callback or continuation.

Under those preconditions, the restricted normal-prefix observations are related.
This does **not** establish that the historical title state satisfies them, that
the observer is appropriate for the entire game, or that a concrete native title
context may be chosen now. Raw-value observers remain counterexamples.

Private source checks cover the frozen 19 normal sites/eight stores and excluded
boundary without expanding reachability. They find no stack load, absolute stack
comparison or SP store into the two fixed globals in that normal scope. However,
the backchain stores raw SP inside the stack, and SP-derived registers escape the
boundary. Unknown continuation/unwind consumers can observe those values.

| Property | Bounded requirement | Remaining title uncertainty |
| --- | --- | --- |
| Incoming r1 | Relative typed invariant under the stated observer; exact value for raw equality | Applicable observer/continuation contract unproved |
| Saved r28-r31 | Same opaque64-bit values; no heuristic pointer substitution | Concrete inputs and later consumers unknown; saved r31 is later overwritten by a frame pointer |
| Initial LR | Same full bits if the copied register is observed; low32 only for the saved word | Actual caller/return contract unknown; native exit cookie cannot substitute |
| Helper return | Same verified call/continuation identity | No original executable binding or observed hook |
| r2/r13, FPR/CTR/CR/XER | Not consumed normally; unchanged state still needs relation if observed | Full startup/exception/PCR/TLS use unknown |
| Module/thread/PCR/TLS | Correct identity relationships and ownership/lifetime | Native objects are not historical Xbox identities |
| Untouched stack | No read in the prefix; initial relation needed for full-memory observers | Future frame reads and exception observation unknown |

Absence of a normal dependency is not evidence of irrelevance outside the boundary.
Incoming nonvolatile values are consumed as saved payloads, not proved freely
choosable. The fixed helper continuation does not require inventing the root LR.

## Independent synthetic differential tests

Five [original variants](../../tests/context-equivalence/fixture.py) exercise:

1. Relative stack stores and frame/call behavior.
2. PCR-to-TLS association and a slot-derived result/store.
3. A fixed absolute stack-address predicate.
4. A raw stack pointer escaping to a fixed persistent cell.
5. Unexpected indirect control flow, generated but never admitted/executed.

Production ReXGlue generates 80 instructions in 10 functions. Default/custom
function bodies match, generation is deterministic, and instruction/branch/LR/
memory correspondence is checked per function. No generated C++ is manually edited.
The pinned CLI and real SDK headers are used. These are host tests, not Switch
hardware execution or an original title test.

Three actual stack bases and two declared payload/TLS sets produce 24 completed
runs per compiler profile. Payloads remain equal within each differential pair;
one deliberately looks like an address and is never heuristically renamed.

| Observation per profile | Result |
| --- | ---: |
| Related normalized pairs, relative/TLS variants | 12 |
| Counterexample pairs, absolute predicate / persistent escape | 4 / 6 |
| Fixed-predicate pairs which happen not to refute it | 2; not an invariance proof |
| Raw stack byte differences | 24 of 24 pairs |

The [C++ oracle](../../tests/context-equivalence/probe.cpp) uses manually specified
byte offsets/payloads, not decoder or generated-code expectations. Its typed
normalizer changes only the proved backchain field. A separate
[Python verifier](../../tests/context-equivalence/verify.py) reads actual binary
snapshots, independently constructs expected bytes using a different serializer,
and recomputes the normalized results/counterexamples. It imports neither the
fixture encoder nor generated C++/C++ normalizer. Persistent cells and opaque
payloads are not normalized. All three profiles produce identical snapshot hashes.
This is independent observation-oracle implementation, not a second full ISA
engine or independent evidence of Xbox loader/startup semantics.

## Explicit native ABI and refusal

The [fixture contract](../../tests/context-equivalence/contract.h) owns stack/cell/
PCR/TLS/module leases and binds a thread, module, entry, source tag, complete
payload declarations and verified object associations. It retains full-stack RW
admission and validates every span. The mandatory generated prologue binds an
exact Scope, rechecks after the trusted controller and refuses missing declarations,
wrong object/thread/TLS, absent input bits, insufficient/misaligned stack, invalid
return cookie, unknown indirect flow, revoked rights and stale capabilities.
The compiled tag is not a general image authenticator; full SHA256 bindings are
recorded separately. Production runtime policies are unchanged.

The synthetic return cookie is an explicitly declared logical native-exit token,
not an invented original return address or mapped guest-fetch target. Actual root
return to the native caller and its register/stack postconditions are checked.
The helper continuation is checked against the generated direct call. No original
title LR, caller or full Xbox return-dispatch rule is inferred.

Absolute-predicate/escape variants execute only in explicit safe counterexample
experiments with owned memory. A request claiming they satisfy the relocation
contract refuses before PPC. The indirect variant always refuses before entry.
There is no unsafe fallback. Two concurrent native tasks validate separate PCR/
TLS roles. Cooperative stop before helper effects, active-lease refusal, rollback
at three creation stages and cleanup are checked. Stop has one valid prior root
entry; it is not reported as a zero-instruction run.

## Validation and evidence boundary

Per Debug / Release / ASan+UBSan:

| Metric | Count |
| --- | ---: |
| Positive semantic assertions / expected refusals | 486 / 34 |
| Refusal checks with no additional PPC entry | 34 |
| C++ literal-oracle byte comparisons | 425,984 |
| Differential normalized-stack byte comparisons | 393,216 |
| Independent Python snapshot oracle comparisons | 393,216 |
| Actual generated entries | 53: 48 matrix, 4 concurrent, 1 stopped root |
| Outstanding allocations / leases | 0 / 0 |

Byte comparisons are separate from semantic counts. Eight new model/input methods
and 26 context methods run once, not as three compiler profiles. T8/T10 generated
stack admission, N1 ownership, Horizon core, identity/lifetime and cleanup/rollback
regressions pass. The final selected 54 generation/build/test/analysis children sum
to 52.353 child wall seconds; peak RSS 364,363,776 bytes including compilers and
46,497,792 noncompile. Unexpected final failures/faults/timeouts/sanitizer findings: 0.

Two initial harness failures remain private: a negative case did not restore r1;
and a process-global entry delta was incorrectly used as a per-thread counter.
They were corrected with explicit restoration, per-Scope checks and an aggregate
entry check after join. Earlier diagnostics and source snapshots were retained,
not suppressed. Every selected child has an external monotonic 30-second watchdog.

```sh
python3 tests/context-equivalence/test_model.py
python3 tests/context-equivalence/run.py --workdir "$FRESH_PRIVATE_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN"
```

Use a fresh external directory and the T8/T10 pinned toolchain. The runner creates
its own original inputs; it has no title-input or Switch-build mode. CI checks the
public model/inputs and existing SDK-independent suites. Local real-generator/
SDK/snapshot verification is separate evidence, not a mocked CI replacement.

## Readiness decision

| Obligation | Decision |
| --- | --- |
| A native stack mechanism | VERIFIED in the declared synthetic host ABI |
| B bounded relocation | PROVEN conditional address/typed-observer relation; PARTIAL title applicability; raw equality disproven |
| C title incoming-register completeness | PARTIAL static coverage; required values/equivalence unproved |
| D title thread/PCR/TLS startup | BLOCKED / actual identities and contract UNKNOWN |
| E original return/stop completeness | PARTIAL helper correspondence; caller/stop/exception binding BLOCKED |
| F overall native title entry | BLOCKED / NO-GO |

The 392-byte page gap, 185,980-byte virtual tail, 1,474,436-byte remainder, 13 unverified
variable providers, image lifetime and unsupported exception/unknown continuation
constraints remain. No title byte receives guest executable rights. Historical
G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED and native N-G4 BLOCKED / N-G5 PARTIAL /
N-G8 BLOCKED are unchanged.

Smallest next proof: a bounded consumer/observer contract for saved backchain and
SP-derived frame pointers at the excluded continuation, establishing whether it
respects typed stack renaming rather than raw historical addresses. Other opaque
input, image and exception obligations remain. No such continuation was executed
or expanded here; no further stage is started.
