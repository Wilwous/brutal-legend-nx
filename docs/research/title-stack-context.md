# T10 — owned synthetic stack and title-context provenance

2026-10-31; baseline `8ccc34c3b110e7bb2951559431ce13e011461734`.
**Synthetic mechanism VERIFIED in the selected host scope. Original title NO-GO.**
No original PPC was generated, run, compiled, linked, mapped or deployed. No NRO
was created. Production runtime, title NativeImagePlan, Xbox loader, SDK locks,
protected branches, tags and previous checkpoints are unchanged.

## Three separate stack quantities

Private fresh-word checks independently reproduce the six original stores from
T9, their payload dependencies, store-before-base-update ordering and matched
helper call/return. Exact title PCs, disassembly and per-site intervals stay private.

| Quantity | Finding |
| --- | --- |
| Touched-byte union | Six stores, 40 distinct written bytes in two intervals |
| Minimum-to-maximum envelope | 492 bytes; a 452-byte hole is not accessed |
| Original live stack allocation | UNKNOWN; not inferred from either quantity |
| New independent synthetic stack | Existing explicitly owned 16-KiB GuestThread allocation |

The 64-byte leaf window remains insufficient. A wider allocation is useful only
with a verified incoming pointer, alignment, no-wrap arithmetic and containment
of every required span. The whole-allocation RW policy is retained. No smaller
permission window is automatically inferred or installed in production.

## Original synthetic oracle and production generation

The new [fixture](../../tests/stack-geometry/fixture.py) has **14 instructions in
two functions**, six stores of mixed widths, one direct helper call and root/helper
returns. Its source registers, store order, payloads and split frame construction
differ from the title. It restores its own declared stack pointer/LR and preserves
the selected input payload registers. These synthetic values are not title inputs.

Pinned ReXGlue recompile-tests processes the original binary/map/assembly inputs.
Default and custom-template production function bodies compare exactly; repeated
generation is deterministic. All 14 instruction comments, six store widths and
the call/LR anchor agree with independently decoded input. Generated C++ is never
manually edited. The pinned CLI revision/hash and real SDK headers are verified.

The [host oracle](../../tests/stack-geometry/probe.cpp) independently constructs
expected big-endian bytes using literal payloads/offsets, not generated C++ or
the parser's ordered store table. It compares the full stack allocation, including
every untouched byte. Nine completed calls per profile cover ordinary/raw/direct/
indirect invocation, low/high incoming-pointer bounds and repeated fresh contexts.
They produce 18 actual generated entries. Two interrupted runs each admit a valid
root but refuse/stop its helper before any store, producing two additional entries.
The total is **20**, not a claim that every failure run had zero prior instructions.
The root continuation is the native C++ caller. Its LR is an explicitly supplied,
saved/restored synthetic payload, not an admitted guest instruction-fetch target
or proof of Xbox return dispatch. The helper LR continuation is checked separately.

## Explicit admission, ownership and lifetime

[Admission](../../tests/stack-geometry/admission.h) is a separately declared
**test-fixture policy**, not a new production or title admission API. It binds:

- the exact synthetic stack handle/Space owner, GuestThread and module instance;
- the selected root/source tag and closed two-function call requirement;
- explicit incoming r1, 16-byte alignment, checked signed offsets and full spans;
- complete allocation RW and each store's containment/current rights;
- module/stack lifetime pins, per-entry rechecks and ordered root/helper identity.

The source tag is a compiled fixture identity, not a general image authenticator;
the runner separately records full SHA-256 source/generated bindings. Unknown
nested calls or changed footprint declarations refuse. The two functions share
the same incoming pointer at their entries; frame mutation occurs after the helper,
then is restored before return. No unknown future nested-stack requirement is assumed.
Existing T8 margins are retained; no 492-byte allocation or complete Xbox ABI is
inferred from the envelope.

The first observer-only version could be omitted by constructing an ordinary
Scope. A new original regression reproduced that omission: the valid toy ran when
the test required refusal. That red diagnostic/source state is preserved privately.
The final production-generated configuration now requires the fixture declaration
and its exact BoundScope association in the prologue. BoundScope always validates
after the trusted test controller and before Scope increments the guest-entry
counter. Ordinary Scope even with a declaration, raw invocation without the bound
scope, and out-of-order helper entry refuse. Incoming r1, LR and payload snapshots
are rechecked; controlled mutations before entry also refuse.
The production runtime itself was not changed, and no pre-existing T8 defect is
claimed: the new fixture-specific requirement was made mandatory in its own template.

Scope/GuestThread and the fixture delegate to the same authoritative checked Space/
Memory operations. Pins prevent decommit/free, not protect(). Permission revocation
after construction, after Scope creation, and after the declared check but before
the production prologue recheck refuses before the next instruction. Actual stores
remain checked. No whole-body permission freeze is claimed.

## Controlled limitations and failure paths

The backend commits/protects one logical allocation at a time. It cannot model
partial commitment or independent guard subpages inside the owned stack. Separate
adjacent committed/uncommitted and no-access regions demonstrate refusal when
the declared footprint escapes its one allocation. These are logical checked
fixtures, not host mprotect/hardware guard-page isolation evidence.

An active lease cannot become stale through permitted release/decommit. A saved
view after decommit/recommit and a retired allocation handle are explicitly refused.
Wrong owner/thread/entry/source, malformed alignment, arithmetic overflow, missing
footprint bytes, undersized/forged region and unknown nested-call contracts refuse.
The 72 zero-entry-delta checks per profile include ownership/lease refusals after
valid execution, not 68 distinct startup mechanisms. All tested invalid initial
entry contexts add no generated entries.

Three deterministic partial-allocation failures roll back. Three fresh-context
allocation/execution/cleanup cycles complete. Cooperative stop and concurrent
module shutdown at a controlled helper boundary preserve untouched stack bytes
and release resources after join. Shutdown denial propagates through one expected
native callback-error result per profile. No guest exception continuation is faked;
register rollback after an already admitted root is not claimed.

## Original title context evidence

Fresh read-only XEX optional-header checks agree with saved entry/default-stack/
TLS declarations. Private fields use the requested classifications:

| Field | Classification and limit |
| --- | --- |
| Entry, default stack declaration, TLS template metadata | PROVEN BY TITLE METADATA; not a live allocation/context |
| Relative footprint and matched helper continuation | DERIVABLE WITH VERIFIED RULE; symbolic, not actual startup values |
| Original r1, live stack base/extent/owner and thread identity | UNKNOWN |
| Saved GPR/root-LR payloads; r2/r13/PCR identities | UNKNOWN actual values/handoff |
| Live TLS initialization, entry ABI and initialization order | PARTIAL metadata/mechanics; actual native binding absent |
| Original PC/stop and exceptional-transition admission | BLOCKED |

Pinned SDK source shows dynamic stack allocation/guard setup, ThreadState r1/r13
assignment, image/TLS ownership and suspended-thread/APC/argument/dispatch ordering.
These are implementation rules, not an independently applicable observation of
original Xbox title startup. No SDK XThread or original title dispatch was run.
No available applicable platform contract proves concrete title register values.

The selected normal path does not read FPR/CTR/CR/XER/r2/PCR/TLS directly. That does
not prove their complete handoff or exception obligations. Title saves are known;
later reads/restores and exceptional consumers beyond the stop remain UNKNOWN.
Choosing a different r1 changes addresses/backchain/register state; saved GPR/LR
values also affect observable bytes. There is no proven equivalence permitting
arbitrary replacements for the actual title. None was selected to make admission pass.

The two fixed image stores still lack independent live permissions/commitment/
ownership. Their containing page includes the inaccessible 392-byte gap. The
185,980-byte section tail and 1,474,436-byte remainder stay UNKNOWN. Six normal
sites still intersect exception metadata; static normal flow is not exception
safety. Original runtime stop hooks remain unobserved.

## Tests and reproduction

Per Debug / Release / ASan+UBSan profile, the new fixture passes **87 positive
semantic assertions, 72 expected refusals and 180,224 byte comparisons**, with
allocations/leases zero. Byte comparisons are not included in semantic counts.
One expected callback error is checked; unexpected native faults/timeouts and
sanitizer findings are zero in the final matrix.

Ten geometry/parser and 26 existing context/dataflow Python methods pass once;
they are not three compiler profiles. Targeted regressions also pass in all three:
T8 window5 positive/16 refusal and generated entry39/28; Foundation108 combined;
N1 plan26 positive/49 refusal/16,384 bytes; Horizon core20 combined; T7 data29/38/
32 bytes and SDK context26/23/32 bytes. Existing T8 stop/start/join checks remain.

The selected final set has 69 generation/compile/test/analysis children, 61.319
summed child wall seconds, peak RSS314,818,560 bytes including compilers and
44,744,704 bytes noncompile. Nine malformed-reader exit1 controls are expected.
The deliberately failing optional-contract red run and earlier successful versions
remain separate. Two BoundScope namespace/stop-owner compile errors from its first
refactor were corrected; those failed-build diagnostics are retained as well.
The controller is trusted fixture code, not a security boundary for arbitrary
native callbacks or unreviewed register corruption. Every selected child has an external monotonic 30-second
process-group watchdog; no failure or sanitizer report was suppressed.

```sh
python3 tests/stack-geometry/test_fixture.py
python3 tests/stack-geometry/run.py --workdir "$FRESH_PRIVATE_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN"
```

Use fresh external directories and the pinned toolchain from the T8/T9 reports.
This runner has no Switch-build/deployment mode. Public CI runs original parser
and existing SDK-independent regressions. Local real-generator/SDK matrices are
separate evidence; CI does not silently substitute a mock SDK/code generator.

## Separate decisions

- **A — Generic synthetic stack mechanism: VERIFIED**, within the declared host fixture.
- **B — Original title entry stack provenance: UNKNOWN.**
- **C — Original title startup context: UNKNOWN**, with metadata/mechanics PARTIAL.
- **D — Original title memory admission: BLOCKED.**
- **E — Original executable entry: NO-GO.**

Historical G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED and native N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED are unchanged.

The smallest remaining title-entry proof is an independently applicable incoming
context provenance/equivalence contract: concrete initial r1 bound to the actual
intended live owned stack/thread, with justified saved register/LR inputs. Safe
synthetic allocation mechanics alone do not supply that title binding. Image/page,
PC/stop and exception blockers remain afterwards. T10 stops without starting a new stage.

The subsequent [T11 conditional-equivalence investigation](native-startup-equivalence.md)
distinguishes historical numeric values from typed native observation invariants.
It does not establish actual title context equivalence or promote these gates.
