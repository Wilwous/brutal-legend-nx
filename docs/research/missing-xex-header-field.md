# T15 — absent XEX field semantics and provider readiness

2026-11-03; baseline `8bbb916835108f9c85d857e710e64c919fc74c4d`.
**Bounded investigation COMPLETE. Pinned SDK behavior VERIFIED; historical Xbox
conformance UNKNOWN; executable-module provider DRAFT/BLOCKED; game NO-GO.**
No title PPC was generated, compiled, linked or executed. No executable title
image, new loader, production provider or Switch application was constructed.

## Exact selected API contract

The pinned signature is `u32 RtlImageXexHeaderField_entry(ppc_ptr_t<xex2_header>, u32)`.
The SDK bridge takes the low 32-bit header argument in r3 and selector in r4;
r3 carries the result. These tests cover this fixed pointer/scalar signature,
not the complete Xbox calling convention or register preservation.
The helper compares the **entire** 32-bit key while traversing eight-byte BE
optional records after a 24-byte prefix. For selected key `0x20401` (kind1),
matching output is a guest address of the record's inline BE32 value.
The input must remain backed and readable throughout the scan and borrowed-result
use. The raw helper does not acquire an owner/lease or validate header magic,
size, record extents or pointer lifetime.

For a bounded catalog with no matching key, the SDK helper writes zero to its
output and returns `X_STATUS_NOT_FOUND`; the wrapper ignores that status and
returns zero. A present zero-valued field still returns a **nonzero address**.
This was measured using both the actual exported bridge and helper in existing
isolated SDK binaries, not a reimplementation of those methods.
[Helper source](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/system/user_module.cpp),
[export wrapper](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_rtl.cpp).
Actual staged library hashes and applied-source hashes are retained privately.

| Input case | Established result | Scope |
| --- | --- | --- |
| Structurally valid catalog, selected key absent | Helper not-found/output0; export0 | VERIFIED SDK implementation |
| Selected kind1 key present | Guest inline-word address; success | VERIFIED SDK implementation |
| Null/invalid header pointer | No safe null-return contract established | UNKNOWN; never passed to raw SDK in this stage |
| Corrupt/truncated/overflowing catalog | Strict offline parser refuses | Native policy only; SDK unsafe path not tested |
| Other requested representation | Strict parser refuses unsupported selector | No general SDK/Xbox error rule inferred |
| Uninitialized/stale/wrong module | No admission | Header-only API cannot establish module identity/lifetime |

A different low-byte key in the catalog is a different full selector, not a match
whose representation may be silently coerced. No other runtime export is added.
Duplicate selected keys are safely bounded test inputs: SDK takes the first;
the strict native draft rejects ambiguity. Historical duplicate semantics remain
UNKNOWN. The synthetic catalogs are lookup fixtures, not authenticated executable
containers or proof of successful Xbox module loading.

## Independent references and confidence

The source register is [references.json](../../tests/header-field/references.json).
All newly inspected sources are pinned, fingerprinted and kept privately; no
external implementation is copied into public Git.

| Source / exact revision | Observation | Authority / independence limit |
| --- | --- | --- |
| ReXGlue `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` | Actual helper/export agree with the original oracle | Baseline SDK, Xenia lineage |
| Xenia `95a5c3ee250f80c3b9d139658649d9ffb6db3eec` | Prior preserved helper agrees | Shared lineage; not counted as independent confirmation or rerun |
| XenonRecomp `ddd128bcca99fe8bfbb99bea583c972351fa6ace` | `XenonUtils::getOptHeaderPtr` returns null on absence and host inline-word address for kind1; actual header compiled unchanged | Separate host-parser API; independent historical derivation not established; not Xbox hardware |
| UnleashedRecomp `cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c` | Logging stub | No lookup semantics; void stub does not prove a zero return |
| XexUtils `01fecb17e4edc8b9b8774c3ae6aaa0fc21c2622f` | Export299 pointer-return declaration | Community prototype; no implementation or specified absent-field behavior |

The separately compiled [XenonUtils parser](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/XenonUtils/xex.h)
is not a replacement guest runtime: for kind0 it returns a host pointer, while the
SDK guest helper returns an immediate. Comparisons use only selected-key lookup
and null/relative-pointer observations. The
[Unleashed stub](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/kernel/imports.cpp)
and [XexUtils declaration](https://github.com/ClementDreptin/XexUtils/blob/01fecb17e4edc8b9b8774c3ae6aaa0fc21c2622f/src/Kernel.h)
do not resolve the missing proof.

Two bounded search rounds and the source candidates above yielded no qualified
Microsoft contract or authorized hardware observation for this case. This is an
accessibility/evidence limit, not proof that documentation cannot exist. Research
stops here; no old Xenia/RXDK execution experiment was repeated. Evidence supports
**implementation consistency**, not HISTORICAL PROVEN or full provider readiness.

## Original title applicability

The exact original fingerprint and T14 checkpoint are preserved. The requested
key, full-key absence and kind1 representation were independently checked again.
Fifteen relevant existing A/B words were checked using literal fields, without
new title sites or recursive analysis. The consumer explicitly tests the result
for zero and skips the returned-word read on that branch. Under the verified SDK
semantics it retains its earlier default, which reaches another unproved callee
as an argument. That continuation may observe it; actual reachability, external
call results, original Xbox behavior and normal return remain UNKNOWN.
Detailed words, addresses, catalog records and dataflow stay private.

Zero for a proven absent field does not create the preceding cell/module/header,
prove a valid header argument, or permit a convenient null module. No historical
pointer is reused and no original import is bound.

## Generic offline improvement and tests

[lookup_selected](../../tools/xex_header_contract.py) now distinguishes `PRESENT`
and `ABSENT` from malformed input without returning a guest pointer or Xbox status.
An absent result has no source offset or scalar value. Existing `selected_field`
and the T14 provider snapshot policy still refuse absence and duplicate records;
existing successful result schema is retained. No runtime/loader code is changed.

[Original unit fixtures](../../tests/header-field/test_contract.py) use a separately
written record oracle. [Eight differential cases](../../tests/header-field/fixture.py)
cover empty/missing catalogs, first/last records, present zero payload, different
full-key representation, duplicate first-match behavior and wrong-endian records.
The strict parser also refuses wrong magic/endian prefix, truncation, declared-size
and catalog overflow. Stale module, wrong owner, rights and lifetime snapshot
refusals are retained in the unchanged T14 suite; those are not actual SDK
module-lifecycle guarantees.

[SDK fixture](../../tests/header-field/sdk.cpp) links existing isolated binaries
and calls the actual helper and exported PPC-context bridge with owned original
metadata. No generated PPC function body is invoked. Entire backing bytes are
checked unchanged and every fixture allocation is released. The optional
[reference fixture](../../tests/header-field/reference.cpp) compiles the supplied
pinned XenonUtils headers unchanged. Dangerous malformed/null/stale inputs are
refused offline rather than supplied to either raw parser.

Reproduce parser/regressions without SDK or title files:

```sh
python3 tests/header-field/test_contract.py
python3 tests/module-provider/test_contract.py
python3 tests/callee-observations/test_static.py
python3 tests/startup-dependencies/test_static.py
python3 tests/bounded-cfg/test_manifest.py
python3 tests/native-image-plan/run.py --workdir "$TMPDIR/t15-plan-new"
```

Optional actual SDK comparison, using an already prepared isolated staged root
with Debug/Release/Sanitized subdirectories and three pinned reference headers:

```sh
python3 tests/header-field/run_sdk.py --sdk-root "$SDK_STAGE_ROOT" \
  --reference "$REFERENCE_HEADERS" --workdir "$T15_OUTPUT"
```

The runner verifies revision/library and reference fingerprints; output must be
new and outside the checkout. It downloads/builds no SDK and modifies no SDK files.
Omit `--reference` to explicitly skip that comparison. CI runs the original
Python fixtures and existing component profiles, not this SDK-dependent option.

## Selected results

The five Python suites pass **119 methods**: new12, T14 prototype38, callee21,
metadata26 and CFG22. The new suite has **31 successful semantic assertion calls**
and **40 expected refusal contexts**. These are separate measures, not additive
coverage. Invalid module and stale snapshot cases remain in the T14 suite.

| Selected component measure | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| Actual SDK fixture assertions / failures | 50 / 0 | 50 / 0 | 50 / 0 |
| SDK helper/export agreement with separate oracle | 8 / 8 | 8 / 8 | 8 / 8 |
| Separate host-parser agreement | 8 / 8 | 8 / 8 | 8 / 8 |
| SDK unchanged backing bytes compared | 272 | 272 | 272 |
| Outstanding fixture allocations / PPC bodies | 0 / 0 | 0 / 0 | 0 / 0 |
| NativeImagePlan semantics / expected refusals | 75 / 49 | 75 / 49 | 75 / 49 |
| NativeImagePlan byte comparison counter | 16,384 | 16,384 | 16,384 |

There are **eight unique differential cases**, replayed in three profiles; they
are not 24 independent cases or eight hardware observations. There are zero
SDK/host-parser disagreements in the selected scope. The strict duplicate policy
intentionally differs in one case; that is a refusal policy, not a discovered
Xbox incompatibility or a hidden failure.

The selected23 compile/test children use external30-second monotonic process-group
watchdogs: summed child time13.353950 seconds, peak RSS327,057,408 bytes including
compilation. Final unexpected failures, signal faults, timeouts and sanitizer
findings are all0. Three existing SDK FP-control inline-assembly operand-width
warnings are retained without suppression. No production C++ changed or SDK was
rebuilt; the new C++ fixture and separate reference were nevertheless tested in
all three profiles. Broader unchanged T10–T12 runtime campaigns were not repeated.

Development diagnostics are retained privately: an initial failed source edit
left the new unit test unable to import its function; the applied patch fixed
that. One initial SDK-fixture compilation failed because of an incorrect type
namespace; it was corrected before runtime tests. Earlier successful SDK-only and
uncounted unit runs are also retained and excluded from selected totals. None of
these development events is relabeled as an expected fault or hardware evidence.

## Provider obligations and stop decision

| Obligation | Status after T15 |
| --- | --- |
| A selected-field lookup | VERIFIED SDK; separate host-parser consistency; historical UNKNOWN |
| B original header-byte provenance | PROVEN offline fingerprint/bounds; no live mapping |
| C consumed module-object prefix | PARTIAL static agreement; full/current identity unqualified |
| D native guest binding | BLOCKED; prototype only, no original binding |
| E runtime-owned provider storage | BLOCKED |
| F reservation/commitment | BLOCKED for this title provider |
| G current guest read permissions | BLOCKED |
| H retained header/object lifetime | BLOCKED |
| I synchronized publication/withdrawal | BLOCKED |
| J cleanup/rollback of an admitted provider | BLOCKED |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED** and native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED** remain. No full obligation is discharged by a lookup
result. Image permission gaps, unknown tails/remainder, other variable providers,
startup context, opaque callees, ABI and exceptions remain as previously recorded.

The next smallest actionable **N-G4 memory proof** is a nonexecuting,
source-digest/generation-bound guest header buffer: trusted ownership, explicit
reservation/commitment, current read-only access, retained lease, and destruction
only after the last reader. This can be investigated independently of historical
API semantics, without publishing module/import bindings or permitting entry.
It is a recommendation, not work begun here. T16 is not started; game NO-GO.
