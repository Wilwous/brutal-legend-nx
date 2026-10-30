# Title image forensics — T4–T6

2026-10-30–31; continued from `6ed43211949a3e1b54665a69aedf5ffa1b049e02`.
**Offline research complete within available evidence. Title admission BLOCKED;
game execution NO-GO.** No title PPC was compiled, linked or executed. No loader,
binder, SDK patch lock, Switch application or historical Xbox gate was changed.

## Recovery and independent checks

The interrupted work contained two pending generic source/test files and a private
baseline, with no T4–T6 commit. Existing work was retained. The prior checkpoint's
336 recorded files and 103 preservation comparisons had passed; input identities
were checked again on resume. The original XEX and both previously reconciled
representations remain unchanged. Source A is the pre-binding decoded image;
B differs only at the known 13 historical variable bindings.

New [DATA-only helpers](../../tools/image_initialization.py) and
[original fixtures](../../tests/native-provenance/test_initialization.py) check
coordinate bounds, initialization accounting, selected PNG structure/CRC,
conventional relocation shape, binding identities and logical section holes.
They never map guest memory, apply relocations or admit executable code.
Detailed title coordinates, original bytes, import values and drafts stay private.

## T4 — resource and relocation interpretation

The PE Base Relocation Directory and final-section declaration agree on a start
and extent. A separate PE header parser confirms those header values. Independently
decoded original XEX resource records identify an XDBF resource in the same flat
image. Validated XDBF table/entry extents locate an image entry containing that
directory start. It falls inside the PNG's **IDAT payload**, not its header.

The checked PNG has six chunks. Signature, IHDR fields, contiguous IDAT, IEND,
chunk boundaries and every CRC pass. A separate sequential walk and bit-at-a-time
CRC implementation agree with the generic zlib-CRC parser. Bounded IDAT inflate
reaches EOF with the exact scanline extent and valid row-filter selectors. No
asset file was extracted or published. These checks follow the
[PNG chunk/CRC specification](https://www.w3.org/TR/2025/REC-png-3-20250624/#5Chunk-layout);
they are content/structure evidence, not relocation authority.

The coordinate chain is explicit: original XEX resource-record file offset →
declared guest VA → decoded offset using VA minus image base → XDBF content/entry
offset → PNG chunk offset. The PE directory's RVA plus the same image base reaches
the identical byte. PE raw-file pointers are a separate coordinate and were not
used as initializers. The encrypted/framed compressed payload has no established
one-to-one original-file offset for each decoded byte. No such mapping is invented.

| Candidate interpretation | Finding |
| --- | --- |
| A: incorrectly identified relocation range | **SUPPORTED narrowly:** treating this decoded directory start as a conventional relocation-block table is incorrect. The original declaration itself was read correctly. |
| B: address conversion error | No error found in the checked same-image equations. |
| C: resource interpretation error | No error found in checked records, container/entry extents or PNG checks. Other resource payload grammars were not all audited. |
| D: documented legitimate dual-role overlap | **UNKNOWN:** no applicable conversion/loader contract proves that interpretation. |
| E: insufficient remaining evidence | **UNKNOWN:** why the PE declaration persists, any alternative relocation encoding, and the absent tail's role/lifetime. |

The full declared relocation span exceeds the successful decoded image; the
available prefix fails conventional block shape in both byte orders. A raw-pointer
diagnostic also fails and supplies no alternate guest mapping. Verified applicable
relocations: **0**; actual unseen/alternative relocation count: **UNKNOWN**; fixups
applied: **0**. A valid PNG does not prove that the full title never relocates or
that every byte of the named section is discardable at the relevant time.

## Compression and zero-fill applicability

Actual metadata selects encrypted NORMAL/LZX, not BASIC compression. Original
descriptor arithmetic, security size and successful decoded size agree on
14,090,240 bytes; all 215 descriptor content-chain checks pass for A. Existing
framing receipts and compressed-stream identity were reused; decryption and
previous failed oversized decoder experiments were not repeated. This remains
a shared LZX implementation, not a second independent decoder or RSA validation.

BASIC block zero counts are a different union member and cannot initialize a
NORMAL image. Decoder/allocator clearing outside successful output is not source
authority. TLS raw and total template sizes are equal, so that metadata supplies
no additional zero extension. Observed zero bytes within A remain DERIVED.

The disputed section's raw length is greater than its virtual length. Even the
ordinary PE raw-smaller-than-virtual padding condition does not justify this tail.
PE raw offsets and discardable flags also do not establish this converted XEX's
initialization/lifetime contract. The ordinary fixed-base relocation condition
does not resolve title self-initialization or alternative encoding. See the
[PE format contract](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format).

## Thirteen variable imports

Original import records, variable-only tokens, public SDK export identities and
the A/B differences were independently matched. All 13 changed words are **32-bit
guest addresses** produced by the historical SDK provider, not transferable native
host pointers or initialized objects contained in the image. All targets lie
outside these saved image bytes; the target objects themselves were not captured.
All 50 differing byte positions remain explained, with no new discrepancy.

Source-level owner classes: four physical system-heap video allocations, two
object-type subobjects of KernelGuestGlobals, and seven kernel system-heap globals.
The private per-slot table records identity, both values, target type/size, source
lines, module/thread dependencies, lifetime uncertainty and rebinding obligations.
It distinguishes cells, structured objects, strings, object-type descriptors and
a periodically updated time bundle. No variable is assumed to be a generic pointer
cell, and no emulator-selected version/frequency/hardware contents are adopted.

The pinned source shows binding before final page protection, plus later module
handle publication. Object-type subobjects have a KernelState owner; several other
allocations lack proven per-export reclamation in the module's default destructor.
Timer lifetime and outstanding references require separate handling. These are
SDK-source mechanics, not independent Xbox observations. Relevant primary sources:
[kernel providers](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_module.cpp),
[video providers](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_video.cpp).

Native reuse denied: **13/13**. Native title provider contracts admitted: **0/13**.
A replacement requires fresh owned guest storage/subobjects, independently valid
contents/layout, module+ordinal identity, endian-correct slot publication, retained
provider lifetime, synchronization where needed, and failure/rollback semantics.
Current evidence does not establish those title consumer contracts. No placeholder
binding or fabricated success was added. None is consumed by the frozen normal
startup slice; full startup use remains unknown.

## T5 — demand is narrower than declared image size

The prior 19-instruction normal path was reconfirmed directly against A/B and the
saved word inventory. It has one matched helper call, six symbolic stack writes
and two constant-address global writes. There are no import or missing-tail
references inside that finite normal path. Unseen computed/indirect accesses and
exceptional continuations remain UNKNOWN; the analysis stops at the old boundary.

The two unresolved macro intervals were partitioned at page/declaration boundaries
into 27 pieces: three section-tail pieces and 24 outer-extent pieces. Each retains
UNKNOWN initialization, denied admission and no claim of being unused. Dependency
rows distinguish instruction bytes, global writes, symbolic stack, TLS/PCR,
import slots, resources and absent memory.

| Full audit accounting | Bytes | Change |
| --- | ---: | ---: |
| DERIVED | 14,090,240 | 0 |
| Direct original-container FILE-BACKED | 0 | 0 |
| Independently proven EXPLICIT ZERO-FILL | 0 | 0 |
| UNBACKED section tail | 185,980 | 0 |
| UNKNOWN SizeOfImage-only remainder | 1,474,436 | 0 |
| Combined unresolved | 1,660,416 | 0 |

The finite references select three completely sourced pages (12,288 bytes), a
**subset** of DERIVED bytes, not newly proven content. This supports a nonexecutable
audit draft without pretending every SizeOfImage byte is needed. However, the
global-write page includes **392 bytes outside a declared PE section**. Known
source content and page backing do not grant logical write permissions to that
gap. The draft records exact section slices and denies whole-page permission
promotion; it installs no rights or mappings. Future scoped backing must preserve
byte bounds and refuse actual unproven accesses. The stack remains symbolic/unowned.

## T6 — draft and readiness decision

The full draft was supplemented with T4/T5 evidence; the new bounded draft is
explicitly DATA-only/nonexecutable. The unchanged actual N1 validator refuses the
267-region projection at its input budget and the three-page projection at its
unsupported origin/module/lifetime contract. Separate origin controls also refuse.
Source identity is true for A and false for B in both scopes. These identity checks
do not claim that later gates were reached. Original positive synthetic fixtures
separately exercise source, ownership, initialization and rollback refusals.
All 12,288 selected bytes are equal in A and B; the whole-source identity mismatch
does not make B corrupt or contradict that interval equivalence. The draft binds
the explicitly chosen pre-binding source identity, not whichever image passes.

There is no synthetic-provenance substitution, guessed TLS padding, unknown-source
initializer, code binding or executable title plan. Guest images constructed: **0**.
N1 requires an executable entry contract; it is not a general DATA-only title
mapping API. The audit drafts do not acquire executable permissions to satisfy it.

| Readiness obligation | Selected finite scope | Full title |
| --- | --- | --- |
| A: byte provenance | VERIFIED for selected source pages | PARTIAL |
| B: native backing | Not materialized/admitted; logical page gap unresolved | BLOCKED |
| C: initialization | Exact source-copy bytes known; runtime/context incomplete | BLOCKED |
| D: variable import rebinding | Not consumed by selected normal path | BLOCKED |
| E: TLS/PCR/stack/startup | PARTIAL mechanics / UNKNOWN title handoff | PARTIAL / UNKNOWN |
| F: code/CFI/stop/fault containment | BLOCKED; no admitted title binding | BLOCKED |
| G: title execution admission | NO-GO | NO-GO |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**. Native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED**. Synthetic hardware evidence remains separate.

## Regression evidence

All final selected children use external 30-second watchdogs. The three C++
profiles are Debug, Release and ASan+UBSan. Counts per profile:

| Fixture | Positive semantic checks | Expected refusals | Separate byte comparisons |
| --- | ---: | ---: | ---: |
| N1 plan, ownership and rollback | 26 | 49 | 16,384 |
| Original overhanging-section fixture | 28 | 17 | 327,680 |

The generic reader adds two source identity controls, four validator refusals and
four malformed-protocol refusals per profile. The private full/bounded A/B matrix
adds four identity controls and eight validator refusals per profile, with exact
reasons retained. No title code was compiled into those generic readers.

Metadata suites: 27 new initialization/PNG/coordinate methods, eight provenance,
22 bounded CFG and 20 residency methods: **77 methods**. Private instrumentation
records 66 assertion-method returns and 98 expected exception contexts. Those
counts are framework operations, not independent compatibility contracts. No bulk
byte comparisons are hidden in them; the PNG's six CRCs and bounded inflate are
separate private content checks.

Final selected 51 compile/test/audit children total 25.139 seconds of measured
child wall time. Peak RSS including compilation: 270,008,320 bytes; peak other-child
RSS: 72,155,136 bytes. Final unexpected exits, signal faults, timeouts and sanitizer
findings: **0**. Twelve nonzero malformed-parser exits are expected.

Two initial unsuccessful audit/harness runs remain preserved: the first private
audit asserted whole-page section coverage and stopped at the real 392-byte gap;
the revised draft records that limitation and grants no access. A private test-count
runner then lacked a test-directory import path; only the runner was corrected.
No runtime check or test expectation was weakened to hide either failure.

## Remaining evidence and single next action

Full title progress still needs an applicable conversion/initialization contract
for the retained declarations/missing tail, actual native variable-provider
consumer contracts, startup ownership and exception/continuation evidence. No
qualified new source resolves these full-title obligations; investigation stops
at that boundary rather than repeating exhausted loader experiments.

**Next action:** review an exact byte-bounded, nonexecutable partial-image contract
for the frozen normal slice, especially the source-backed page's undeclared gap,
owned stack/context, exact PC/stop binding and denied transitions. This is one
contract review, not authorization to execute the title or start another stage.
