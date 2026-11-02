# T13 — first unproved callee and frame observations

2026-11-02; baseline `f958a404fdccfda24d991e183ddde5b8b57b39a2`.
**Bounded nonexecuting investigation COMPLETE. Normal return and title-entry
equivalence remain BLOCKED / NO-GO.** No title PPC was generated, executed,
compiled, linked or deployed. No title-executable image was constructed. Production
runtime, loader, SDK locks, protected refs/tags and verified NRO packages are unchanged.
Detailed PCs, words, disassembly, call identities and matrices remain private.

## Target authority and finite coverage

The target is derived from the exact T12 direct-call word. The same entry has an
exact record in the PE Exception DataDirectory, whose packed instruction extent
and prologue length match the source. Both preserved A/B views agree on code and
metadata. The range belongs to the declared code section. Neither a guessed nearest
symbol nor the following padding is used to choose its bounds. PE declarations
and metadata are source authority, not proof of live mapping or executable rights.

| Static evidence | Count / qualification |
| --- | --- |
| Metadata-bound decoded instruction sites | 53 |
| Sites in conditional call-free prefixes | 35 |
| Discovered-only sites beyond unknown calls | 18 |
| Abstract prefix transitions including the bounded counted loop | 45 |
| Unknown direct-call frontiers | 3 |
| Actual original instructions executed | 0 |

The budget is 96 sites, 128 abstract steps per path, 16 paths and zero recursive
callee traversal. Conditional paths require their recorded symbolic guards,
successful memory operations and no exceptional transfer. They are not proven
hardware reachability. The first sequential prefix is separately identified;
post-call continuations never acquire an invented successful-call summary.

An independent checker uses literal bit fields, branch arithmetic and a separate
structural graph cut. It confirms that **every local structural entry-to-return
route crosses an unknown call**. A discovered epilogue therefore cannot establish
normal return. No additional arbitrary game functions were analyzed.

## Concrete observations and remaining contracts

The function saves incoming state, builds another frame, initializes a finite local
buffer, and has a saved-slot restore epilogue. There is no explicit stack-relative
backchain load in these local words. Provider-derived pointer loads could still
alias unknown objects; their absence from the stack-affine classification is not
a nonaliasing proof.

A prior-derived frame-pointer register is saved and conditionally restored here.
Its type follows from cross-fragment dataflow, not from recognizing an integer as
an address. This is distinct from the opaque original root register value saved
earlier. Relative stack/backchain and saved-frame-pointer relationships remain
compatible with a typed renaming relation under no-wrap, ownership, permissions,
matching input and lifetime assumptions.

A local buffer address is explicitly delivered to an unproved direct callee.
Its access extent, pointer comparison/alias semantics, numeric observation,
retention and post-frame lifetime are UNKNOWN. A later global publication of an
unproved call result does not prove either presence or absence of a persistent
stack-pointer escape. No direct incompatible stack-address predicate was proved
in the local code; external observers remain unqualified.

The incoming local LR follows from the verified direct BL continuation. The
epilogue's SP inverse, saved LR width and register reloads can be checked
conditionally. They require calls to return normally, preserve the frame basis
and saved cells, and leave readable owned memory. The original root caller/LR,
full ABI, exceptional transitions and external register effects are not inferred.
Local flags and counter use are recorded explicitly; unreferenced PCR/TLS/FPR
state is not declared irrelevant to the unproved callees.

## New provider and memory dependency

The `XexExecutableModuleHandle` variable-import slot is read and its provider cell
dereferenced before the first data-dependent branch. This is a concrete required
provider, rather than merely an item in the historical import inventory. Its
saved prebinding token and historical SDK pointer are not current native inputs.
Choosing null to simplify a branch would not establish correct initialization.

A guarded route also reaches the identified `RtlImageXexHeaderField` function import.
Export identity and prior SDK declaration evidence do not supply its missing
admitted pointer/lifetime/return contract. Other direct local callees remain opaque;
they are not recursively explored or replaced with synthetic success behavior.

Known fixed code/data coordinates do not directly intersect the existing permission
gap or unknown tails. Provider-derived pointees, live rights, aliasing and lifetime
remain unproved. The function's own packed exception flag does not establish absence
of faults, nonlocal transfer or caller unwind requirements. All six prior
exception-metadata-overlapping sites retain existing refusal. The existing title
stop boundary is not moved and no new executable admission is created.

## Minimal generic tooling and validation

[callee_observations.py](../../tools/callee_observations.py) is an independent,
non-admitting static helper around the existing restricted decoder. It adds the
needed unsigned-compare fields, CR-field branches, doubleword update-store and
counted-loop observations. Unsupported arithmetic results remain opaque when the
helper has no proved value model. Metadata mismatch, unsupported format/exception
extent, source changes, unknown transfers, range and budget violations refuse.
Every unproved call is a barrier; source file bytes never stand in for live memory.
Existing runtime/loader admission and the older decoder's policies are unchanged.
This is not a complete taint/alias analyzer: opaque results stay UNKNOWN, and
absence of a stack-affine value is not an indirect nonaliasing proof.

Generic instruction checks use pinned SDK implementation source, with the unsigned
compare and update-store definitions cross-checked against IBM's
[Compare Logical Immediate](https://www.ibm.com/docs/en/aix/7.1.0?topic=set-cmpli-compare-logical-immediate-instruction)
and [Store Double Word with Update](https://www.ibm.com/docs/en/aix/7.1.0?topic=set-stdu-store-double-word-update-instruction)
references. These describe instruction behavior, not Xbox startup or live-provider
contracts. External documentation queries contain only generic instruction names.

```sh
python3 tests/callee-observations/test_static.py
python3 tests/startup-dependencies/test_static.py
python3 tests/bounded-cfg/test_manifest.py
```

The new original static corpus has 21 passing methods. The affected prior static
dependency and CFG suites have 26 and 22 passing methods: **69 methods total**.
Five selected bounded children comprise those three suites, title static analysis
and the independent checker. No unexpected failure or timeout occurred. Earlier
passing development diagnostics are retained privately and excluded from totals.

No production runtime defect was established or patched. No C++/generated runtime
body changed, so local Debug/Release/ASan+UBSan reruns are **not applicable**; the
entire T11/T12 execution campaign was deliberately not repeated. Its verified
checkpoint is preserved as prior evidence, not relabeled as T13 execution. Standard
CI still checks existing SDK-independent components in its configured profiles;
those results are separate from these static Python tests.

## Readiness decision

| Obligation | Result |
| --- | --- |
| A target identity | PROVEN source/metadata identity |
| B normal-path static coverage | PARTIAL: finite guarded prefixes, no call summaries |
| C local frame/backchain observations | CONDITIONALLY COMPATIBLE |
| D external pointer escape/retention | UNKNOWN |
| E incoming register/context completeness | PARTIAL |
| F actual normal-return proof | UNKNOWN/BLOCKED |
| G stack-address renaming | Local relation conditional; continuation BLOCKED |
| H live image/provider permissions | BLOCKED |
| I title entry | BLOCKED / NO-GO |

The 392-byte permission gap, 185,980-byte virtual tail, 1,474,436-byte remainder,
13 variable-provider obligations, original startup state, live image ownership/
lifetime and exception semantics remain. One provider is now a demonstrated
mandatory dependency in this later scope; none is newly admitted. Historical
G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED and native N-G4 BLOCKED / N-G5 PARTIAL /
N-G8 BLOCKED are unchanged.

The single next proof is an owned native `XexExecutableModuleHandle` provider/object
contract: intended module association, live ownership/current rights, slot
publication and the exact consumed header relationship. Do not reuse a historical
snapshot pointer or select a convenient null. Other callee/stack/exception
requirements remain after that proof. T14 is not started.
