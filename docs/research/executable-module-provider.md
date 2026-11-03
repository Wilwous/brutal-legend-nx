# T14 — executable-module provider and selected XEX header contract

2026-11-03; baseline `77a482238c97a9ef590fc4f89ace42d31293e451`.
**Finite offline investigation COMPLETE; native provider DRAFT; title admission
BLOCKED / NO-GO.** No original PPC generation, compilation, linking, execution,
executable image or deployment. No runtime/loader/SDK source is changed. Exact
source bytes, title coordinates, field records and admission matrices stay private.

## Evidence categories and limits

| Category | Available evidence | What it establishes |
| --- | --- | --- |
| Real Xbox behavior | No qualified new observation | No hardware object, lifecycle or exception proof |
| Title static data | Original XEX fingerprint, independent directory scans, 53 previously bounded A/B instruction sites checked again | Exact finite consumer relationships and source-field presence/absence, not live mappings |
| Selected SDK | Revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, actual isolated patched-file hashes retained privately | Behavior of this implementation, not original Xbox conformance |
| Xenia reference | Revision `95a5c3ee250f80c3b9d139658649d9ffb6db3eec`, three source files downloaded and fingerprinted | Common implementation lineage, not an independent hardware oracle |
| Native draft | Original immutable synthetic bytes and untrusted snapshot declarations | Offline consistency/refusal, never actual ownership or execution admission |

The SDK exports `XexExecutableModuleHandle` (ordinal `0x193`) as a variable, allocates a four-byte
cell, and later writes the executable module's guest loader-object address into
it. Its own comments call the representation a compatibility fabrication.
[Xboxkrnl variable creation](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_module.cpp),
[publisher](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/system/kernel_state.cpp),
[Xenia ancestor comments](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xboxkrnl/xboxkrnl_module.cc).
Pinned upstream links describe source lineage; private hashes additionally identify
locally applied SDK patches. No prior Xenia/RXDK experiment was repeated.

## Exact bounded pointer contract

Use fresh symbolic guest coordinates, all ptr32 in big endian:

1. `BE32(import_slot) = P`, where `P` addresses the exported four-byte cell.
2. `BE32(P) = M`, where `M` identifies the intended executable module object.
3. On the nonnull-module route, `BE32(M + 0x58) = H`.
4. The conditional import receives `r3=H`, `r4=DEFAULT_HEAP_SIZE`.
5. If its result `Q` is nonzero, the consumer reads `BE32(Q)`.

The two loads before the module-null guard are mandatory in the finite prefix.
The header member is an additional guarded load; the result-word load occurs
only after the unproved import returns nonzero. The consumer does not write these
provider objects locally. Passing `H` to the import exposes header identity;
other calls can observe more state. No claim of complete pointer nonescape,
thread independence or object-layout completeness follows.

The consumed member agrees with the open-source
[loader prefix](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/include/rex/system/xmodule.h).
That agreement supports this read, not every field or every future observer.
The historical saved pointer, null substitution, a host address, a raw file offset
and an independently allocated but wrongly associated module all fail the contract.

## Applicable header query

Only the demonstrated public selector `XEX_HEADER_DEFAULT_HEAP_SIZE` (`0x20401`)
is investigated. For a matching optional directory key with low byte `1`, the
SDK guest-pointer overload, reached by `RtlImageXexHeaderField` (ordinal `0x12b`),
returns **the address of the inline four-byte value**,
not that value. The borrowed address is relative to the guest copy of the original
XEX header. It is not a pointer into the decompressed PE image or an address from
an on-disk offset alone. The source header has a 24-byte prefix and eight-byte
big-endian directory records. See
[selector and representation](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/include/rex/system/util/xex2_info.h),
[guest header copy and helper](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/system/user_module.cpp),
[wrapper](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_rtl.cpp).

The exact original directory has **no matching selector**, confirmed with
independent record unpacking and literal full-key byte comparisons. Detailed
records and contents are private. In this SDK, missing-field output is zero and
the wrapper ignores the helper's not-found status. That source behavior would
leave the bounded consumer's prior default intact. It does not prove Microsoft's
original implementation or justify a null module. The source helper does not
provide the required malformed-header bounds or lifetime admission by itself.
The Xenia [same-family helper](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/user_module.cc)
agrees; shared ancestry is not counted as independent conformance evidence.

## DRAFT native construction and transaction

A future trusted registry must associate the intended executable source digest,
module identity and monotonically fresh generation with all storage. Reserve and
commit fresh nonoverlapping guest ranges; initialize in a private transaction;
copy the exact bounded original header; validate the single permitted query;
write the object member and cell with checked BE32 operations; seal minimal guest
read permissions; then publish the import binding and admit consumers. Alignment
of four bytes is a proposed minimum for the observed word loads, not a complete
Xbox alignment guarantee. No guest executable permission is needed for metadata.

A lease must pin cell, module prefix, header and returned field for the complete
consumer scope. Enforce current rights and module/thread association at admission;
the association is a conservative native policy, not a newly proved Xbox TLS rule.
Publication/rebinding needs synchronization with readers, withdrawal before module
destruction, lease draining and generation checks. Pointer-cell atomicity alone
cannot make the multi-object transaction safe. Roll back every unpublished
allocation on failure. Do not expose partial success, invent an Xbox status code,
or free borrowed header storage while a consumer can still use it.

The reviewed SDK creates a module record, separately copies header bytes, and
publishes a cell. Its plain BE32 publication and the early return on clearing the
executable module do not themselves establish safe concurrent withdrawal or
stale-cell prevention. This is a source limitation, not a reproduced hardware
fault. A full synchronized owned provider is **not implemented** in T14.

## Nonexecuting prototype and validation

[xex_header_contract.py](../../tools/xex_header_contract.py) contains only a strict
selected-field parser and immutable snapshot audit. It never allocates guest
memory, publishes an import, calls PPC, implements Xbox return codes or grants
admission. Snapshot metadata is caller-supplied and cannot prove live ownership,
commitment, permissions, lease validity, thread identity or allocator cleanup.
Consistent snapshots still return `execution_admitted=false` and
`runtime_provider_installed=false`.

The parser refuses malformed bounds, duplicate/missing selected records and any
unsupported selector. Missing-field refusal is **draft policy**, distinct from
the SDK's zero return. It returns a source offset and scalar separately; the draft
only computes a checked guest coordinate after validating its explicit backing.
This is not a full authenticated XEX/security/loader parser.

[Original tests](../../tests/module-provider/test_contract.py) use independent
literal records, a separate `struct` oracle and asymmetric synthetic coordinates.
They cover pointer depth/endian mistakes, module mismatch, stale generations,
wrong owner/rights, missing backing, overlap/wrap/alignment, malformed field
bounds, changed provenance, missing leases, thread mismatch and destroyed-record
refusal. Repeated creation/destruction is represented only by immutable snapshot
generations. Refusal leaves snapshots unchanged. **No actual allocation lifecycle,
rollback, lease release or before-generated-entry runtime check is claimed.**
Those stay blocked pending a justified runtime provider.

Reproduce without any title input:

```sh
python3 tests/module-provider/test_contract.py
python3 tests/callee-observations/test_static.py
python3 tests/startup-dependencies/test_static.py
python3 tests/bounded-cfg/test_manifest.py
python3 tests/context-equivalence/test_model.py
python3 tests/stack-provenance/test_model.py
python3 tests/stack-geometry/test_fixture.py
python3 tests/native-provenance/test_intervals.py
python3 tests/native-provenance/test_initialization.py
python3 tests/native-image-plan/run.py --workdir "$TMPDIR/t14-plan-new"
python3 tests/native-provenance/run_data.py --workdir "$TMPDIR/t14-data-new"
python3 tests/pre-entry-stack/run.py --workdir "$TMPDIR/t14-stack-new"
python3 tests/native-provenance/run_reader.py --workdir "$TMPDIR/t14-reader-new"
python3 tests/horizon-foundation/core.py --workdir "$TMPDIR/t14-core-new"
```

Each output directory must be new and outside the checkout. Optional pinned SDK
headers were supplied for the local data-context regression; commands without
those headers explicitly skip that case. Generated-entry integration was not
rerun; unchanged T10–T12 execution checkpoints remain prior evidence. Local
selected results follow; these baseline runs do not implement the draft provider.

## Selected regression evidence

Nine Python suites pass **171 methods**, including **38** new provider/parser
methods. An independent test observer records **46 successful semantic assertion
calls**, **107 expected refusal contexts**, and **one four-byte literal byte
comparison** in the new suite. These measures are not additive test counts.
Forty truncation cases and 32 repeated refusal cases are included in the 107
contexts. Provider success results remain explicitly non-admitting.

Unchanged C++ component regressions were also rerun:

| Suite / measure | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| NativeImagePlan semantic / refusal | 75 / 49 | 75 / 49 | 75 / 49 |
| Data contract positive / refusal | 29 / 38 | 29 / 38 | 29 / 38 |
| Data reader guest / rollback / input refusal | 3 / 1 / 3 | 3 / 1 / 3 | 3 / 1 / 3 |
| Pinned-header synthetic context positive / refusal | 26 / 23 | 26 / 23 | 26 / 23 |
| Stack window positive / refusal | 5 / 16 | 5 / 16 | 5 / 16 |
| DATA-only reader identity / native refusal / protocol refusal | 2 / 4 / 4 | 2 / 4 / 4 | 2 / 4 / 4 |
| Horizon core passing / failing | 20 / 0 | 20 / 0 | 20 / 0 |

Per profile, byte comparison counters are 16,384 for NativeImagePlan and 32 each
for data and synthetic context; another 32 copied data bytes have an independently
computed digest oracle. These units stay separate from semantic checks. All final
allocation/buffer/lease counters reported by these fixtures are zero. No new
generated-entry invocation is claimed by this campaign.

The selected **75 externally supervised compile/test children** take a summed
32.309139 seconds, with peak RSS 274,513,920 bytes including compilation and
23,920,640 bytes excluding compilation. Each child has a 30-second monotonic
process-group watchdog. There are **21 expected nonzero refusal exits, zero
expected signals, zero unexpected failures/faults/timeouts and zero sanitizer
findings**. A preliminary driver print mislabeled three expected protocol exit-2
refusals; saved per-case expected codes resolve the summary. No case was suppressed
or rerun to alter the result. The initial direct run and superseded 37-method provider run are retained
privately and excluded from selected counts. The final 38-method corpus adds
refusal of truthy non-boolean metadata declarations. New Python code has no C++ optimization/sanitizer
profile; the table is separate unchanged C++ baseline evidence.

## Readiness and stopping point

| Obligation | Status |
| --- | --- |
| Variable identity and finite consumer pointer chain | PROVEN static/source identity |
| Original selected-field presence/absence | PROVEN offline |
| Selected pointer/missing-field semantics | CONDITIONALLY PROVEN for pinned SDK, Xbox conformance UNKNOWN |
| Native executable-module object/publication | DRAFT / BLOCKED for title |
| Live owner, current rights, stable lifetime, synchronized withdrawal | BLOCKED |
| Generic parser/snapshot mechanism | VERIFIED within original offline tests |
| Original-title provider binding and executable image | BLOCKED / NO-GO |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED** and native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED** remain unchanged. The 392-byte permission gap,
unresolved virtual tail and SizeOfImage remainder, thirteen variable-provider
obligations, other opaque callees, startup context, full ABI and exception
continuation remain. No convenient header/provider result discharges them.

The smallest next title-specific proof is to qualify the selected absent-field
query's native compatibility contract against an independent authoritative
reference or observation. The exact source header and finite consumer are now
known; full Xbox object semantics and safe owned publication still require proof.
T15 is not started. No new NRO, tag, Release or game execution is authorized.
