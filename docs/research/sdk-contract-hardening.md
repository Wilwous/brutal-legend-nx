# SDK contract audit and boundary hardening — T126–T145

2026-11-05. Baseline `c242881a5312df30503f2a134763e756a396d1dd`.
**Original title execution NO-GO.** This cycle corrects admission/diagnostic
behavior and the compatibility inventory; it does not add a duplicate compare
service or guess Xbox numerical semantics. [Contract matrix](compatibility-matrix.json)
and [qualified evidence](sdk-contract-evidence.json) separate implementation,
Windows reference, original synthetic observations and missing Xbox proof.

## Compare discovery and evidence decision

`RtlCompareMemoryUlong` was already registered in the synthetic XEX path at
xboxkrnl ordinal283/0x11b, bound by `runtime/horizon/xex.h`, and implemented by
`tests/horizon-xex/import.cpp`. Stage15 called the exact privately extracted,
fingerprinted ReXGlue SDK leaf through real generated PPC. The previous
T106–T125 service matrix omitted this legacy path; its five selected SdkServices
contracts were not the complete project inventory. This is now corrected.

The pinned signature is `u32(mapped_void source,u32 byte_length,u32 pattern)`.
Guest arguments use r3/r4/r5 low32 bits; the shim zero-extends its u32 result into
r3 only after validation. The C++ helper itself does not modify r3. `MappedPtr<void>`
returns `rex::be<U>*` from `as_array<U>` in the actual pinned SDK header, so the
source FIXME about assuming BE wrappers is not evidence of a current plain-host
endian bug. Input memory is copied atomically into retained bounded BE scratch;
no arbitrary guest pointer is passed to a host dereference.

| Case | Exact pinned SDK leaf | Existing native adapter policy |
| --- | --- | --- |
| Full16 matching bytes | Returns4 matching words | Same result, unchanged |
| Pattern / different / pattern / pattern | Returns3; comparison continues after mismatch | Same result, unchanged |
| Leading whole-word prefix in that example | Hypothetical prefix-byte oracle would return4 | This oracle is not substituted for the SDK |
| Misaligned Source or byte length | Returns0 before reading | Refuses before SDK under its stricter selected contract |
| Zero length | Returns0 without reading | Nonempty subset refuses |
| Unknown/unreadable/foreign/out-of-range memory | Does not establish safe guest failure semantics | Refuses before SDK; no host fault experiment needed |
| Valid native domain | Raw source implementation has no64-byte policy | Aligned nonempty IN≤64 in the retained synthetic RO image capability |

Microsoft's Windows documentation defines byte units, ULONG alignment, byte
length divisible by four, and zero for invalid alignment/length. It is authoritative
for Windows, not Xbox360. [Microsoft contract](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlcomparememoryulong).
ReactOS's own implementation stops at the first different ULONG and returns a
byte distance; this is another Windows implementation, not Xbox evidence.
[ReactOS pinned source](https://github.com/reactos/reactos/blob/c76421c2308c33df6d6e90346102a5a4a0bcb00e/sdk/lib/rtl/mem.c).
Xenia retains an all-matching-word count with the same source structure. Its
lineage is shared with the SDK, so agreement is not an independent Xbox oracle.
[Xenia pinned source](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xboxkrnl/xboxkrnl_rtl.cc).

No qualified original Xbox observation/contract resolving prefix, return units or
invalid-memory semantics was obtained. **T127 remains PARTIAL; exact Xbox numerical
correction is BLOCKED.** The SDK leaf, registration, ordinal and all existing SDK
locks remain unchanged. No Windows behavior is silently imposed on Xbox.

## Confirmed defects and minimal corrections

First, the unchanged Stage15 host runner initially produced passed422/failed1,
result197650, SDK entered64/completed61 and zero remaining allocations/leases.
Its structured ABI-refusal expectation failed. Inspection found no `abi_rejected`
emission in the current entry/context validator, although invalid MSR/context was
correctly refused. `GuestThread::Execution` now emits that structured event before
the existing invalid-context error. No permissive admission or fabricated success
was introduced. The original initial failure logs remain private and unchanged.

Second, an original regression reproduced a legacy native helper gap: after a
valid Scope had been created, changing r13 allowed `ImportFrame::compare` to enter
the SDK. The diagnostic child exited10, explicitly marking reproduction. This was
a direct native helper admission gap; normal generated dispatch already had its
own entry check. Do not describe this as original game execution or a demonstrated
exploit of a complete generated game path.

All four legacy memory service paths now revalidate context at their native entry:
compare, critical-section initializer, query and fill. Frame lookups also recheck
critical context, not just the PPCContext object's address. Initializer/query/fill
recheck after their trusted admission observer, before SDK execution. A new
production-generated regression deliberately corrupts PCR in each observer and
checks that the callback actually ran, SDK entry did not advance, and intended
output stayed unchanged. This avoids counting an unrelated earlier refusal as a
passing observer test.

A first attempted recheck used fresh admission after the observer. That broke
existing accepted-query/fill stop ordering: a stop callback marks the module
closing, but an already admitted operation must retain its lease, complete, and
stop at the next generated boundary. Those four failing assertions are retained.
The final implementation distinguishes:

* `validate_context`: new admission; closing module refuses through a fresh lease.
* `validate_inflight_context`: recheck the same owned stack/PCR/TLS/ABI invariants
  under the existing Execution lease; accepted operation may finish while closing.

The latter is used only after admission within existing leased operations. It
creates no new entry, import binding, raw memory mapping or loader fallback.
Reclaimed/decommitted ownership cannot appear while those leases remain live;
current permissions still govern publication. No module closing policy, loader,
binder or image admission proof was weakened.

## Coherence, lifetime and ABI limits

| Service | Pointer/effects | Return and errors | Remaining limits |
| --- | --- | --- | --- |
| Compare283 | Retained aligned RO IN≤64, BE snapshot | SDK matching-word count; native invalid domain refuses | Exact original Xbox prefix/units UNKNOWN |
| Fill294 | Existing legacy OUT≤64 and separate SdkServices OUT≤65536 profiles | Void; r3 unchanged; nonempty aligned subset | No remainder/zero broadening |
| Initialize302 | Retained aligned INOUT28, copied transactional scratch | Void; unchanged bytes on refusal | No Enter/Leave/contention/APC equivalence |
| Query238 | Selector0, retained uniform whole-page metadata; checked OUT28 | Success publishes; supported invalid selector failure does not publish | No executable/tail residency inference or arbitrary query domain |

No new PPC instruction family or numeric ABI change was justified. Existing
production corpus tests arithmetic, update stores, BE memory, CA32, CR/XER,
LR/CTR/direct/indirect/return edges, selected FPR arithmetic and VMX block operations
against original independent oracles. Low32 pointer extraction remains consistent
with the pinned SDK ABI; this is deliberate coercion, not a new host pointer.
Typed-vector signatures, OE forms, reservation atomics, raw partial-vector paths,
unknown computed branches and exceptional continuation stay denied/unverified.

New thread tests distinguish process/module instance from identical public module
numbers, stale registration from a live new GuestThread, slot allocation lifetime,
thread withdrawal, module closing and pinned cleanup. Internal `Slots::Token`
generations reject stale tokens. Raw Xbox u32 TLS indices **contain no generation**:
a reused numeric slot denotes the current cell; stale-index rejection must not be
invented. The existing capacity64/startupslot0-reserved native subset remains;
there is no new Xbox KTHREAD/APC delivery model.

`Space::write_words` numerical/publication implementation is unchanged. New tests
cover a committed first region plus uncommitted second region, RO/foreign second
region, logical adjacent4-byte permission boundaries, stale handles after address
reuse, stale views after decommit/recommit, pinned concurrent teardown refusal and
caller-level stale-module refusal. Deterministic synchronization replaces timing
races. Space owns allocations, not modules: a module admission lease is the caller's
obligation, and the test does not claim Space invents that ownership relationship.
This transaction guarantees all-or-nothing admission/publication of its own words;
it does not roll back guest instructions that already executed.

## Graphics, audio and resources

No translation/backend rewrite was justified. Existing four endian modes and
float5/u16-or-u32-index conversion still pass independent lane/index oracles and
actual Scene3D snapshot consumption. New bit-exact edge tests preserve signed zero
and finite float denormals; unsupported formats/shaders/tiling remain denied.

The PCM adapter remains explicit front-only FL/FR from six BEfloat planes,48kHz,
256 samples. Fixed bit-pattern/int16 expected pairs cover ±1, ±0.5, signed zero,
subnormals, one-quantum samples and nearest representable values below full scale.
NaN, signaling-NaN representation, infinities, beyond-unit samples and any nonzero
surround contribution refuse; negative zero is zero. There is no implicit clipping
of out-of-domain input: only valid +1 maps to32767 at output quantization.
The asymmetric truncation of the negative value just above-1 is tested. Output
byte order is explicit s16LE, independent of native integer representation.
Existing queue/play/stop/cancellation/lifetime tests pass; XMA remains unsupported.

Resource review reuses the preserved title metadata and existing immutable
manifest-bound streams. Explicit size/provenance, bounded partial delivery,
cancellation and stale handle behavior remain tested. Compression declarations
and dfpf/FSB-like payload transforms are still insufficient for a title decoder.
No original archives/assets were extracted for these tests or published.

## Production integration, tests and build

No generated C++ was edited. The actual production Stage15 generator runs twice,
plus comparison with its production-default instruction output. Independent census:
**21 functions,203 instructions,97 hooks,49 scalar/vector memory sites,4 backedges,
17 calls and20 returns**. It uses the existing four real SDK bridges; the new driver
adds three observer-corruption cases through generated initialize/query/fill
callers. The guest prologue executes before these in-flight refusals, so zero guest
entry is claimed only for the separate invalid pre-entry case, not those callbacks.

| New suite, each Debug / Release / ASan+UBSan | Positive semantics | Expected refusals | Byte comparisons |
| --- | ---: | ---: | ---: |
| Compare/source oracles + generated boundaries | 332 | 16 | 6528 |
| Word transactions and deterministic teardown | 11 | 9 | 0 |
| TLS/module lifetime | 15 | 10 | 0 |
| Format edge cases | 4 | 10 | 1084 |
| Total per profile | 362 | 45 | 7612 |

These totals are assertions, not new supported imports or a compatibility percentage.
The compare oracle covers152 length/mismatch combinations; it independently reads
bytes and distinguishes total-word count from a hypothetical whole-word prefix
byte count. It is not an independent measurement of an Xbox kernel.

Stage15 normal/controller/no-trace runs now pass423/0 per profile, result197650,
SDK64/61,72 expected refusals, zero allocations/leases. Stress passes759/0 with
93 expected refusals per profile. Historical hardware427 is not substituted for
the current source's actual423 host assertions. Seven unsupported-operation/typed
signature compile refusals are independently checked. Earlier development and
regression failures remain saved; no failure counter was suppressed.

Additional three-profile regressions: NativeImagePlan75/49 with16384 byte
comparisons; native-module97/21; variables120/84; header115/66 plus reader10/5;
stack window5/16; context differential486/34,24 runs,12 equivalent pairs and10
counterexample pairs, externally verified393216 bytes. The previous compatibility
production fixture remains23/5 with1040 comparisons, result0x89abcdef, SDK5/5;
T105 remains42/11,340 ticks, result340, zero final resources (its reported host
underflow1 is retained). Existing SDK service/format and V2 graphics/audio/resource/
timing components also pass. Heterogeneous old suite counts are not relabeled as
new semantics. The window-only runner does not claim its optional old leaf replay.

All new compile/execution children use an external monotonic30-second process-group
watchdog. Debug=-O0, Release=-O2; actual ASan/UBSan=-O1 with no recovery. Final runs
have no unexpected fault, timeout, sanitizer finding or unreleased owned resources.
The final receipt contains167 bounded children including7 expected negative
compilations,217.40 seconds of summed child wall time and437108736-byte peak
host child RSS. Docker client RSS is not the container compiler or Switch RSS.
The initial baseline diagnostic failure, initial overly strict closure recheck and
initial duplicate-link error in the new harness remain in private diagnostics.

The pinned offline devkitA64/libnx container successfully compiles/links a new
**AArch64 ELF of this original SDK audit**, including the actual SDK bridges and
production-generated original corpus; undefined symbols0. No NRO/package/deployment
was created. This changes defensive CPU contract validation, not the GPU/audio/USB
backend. A new hardware candidate is not warranted; new adapter hardware execution
remains UNKNOWN. T85/T105 packages and user-confirmed hardware scope are preserved.

```sh
python3 tests/sdk-contract-audit/test_evidence.py
python3 tests/sdk-contract-audit/run.py --workdir "$OUT/portable"
python3 tests/horizon-acceptance/run.py --workdir "$OUT/acceptance" --toolchain "$TC" --sdk-source "$SDK" --stage 15
python3 tests/sdk-contract-audit/run.py --workdir "$OUT/contracts" --toolchain "$TC" --acceptance-output "$OUT/acceptance"
python3 tests/sdk-contract-audit/build.py --workdir "$OUT/aarch64" --toolchain "$TC" --acceptance-output "$OUT/acceptance"
```

Outputs must be fresh external directories. TC/SDK refer to existing pinned private
open-source toolchain locations; no title input is accepted. CI runs SDK-free
evidence/transactions/format edges; actual SDK/codegen/AArch64 cases are explicitly
local verification. No fake CI SDK implementation is substituted.

## Before / after and readiness

| Area | Before | After | Applicable title proof |
| --- | --- | --- | --- |
| Compare | Already real legacy SDK service; omitted from latest small matrix | Existing behavior characterized, matrix corrected, same numerical leaf | Imported; exact Xbox semantics UNKNOWN |
| Other SDK | Legacy init/query/fill + newer bounded TLS/fill | Fresh entry context and retained in-flight context rechecks | SYNTHETIC VERIFIED; no original binding |
| ABI diagnostics | Invalid context rejected; structured event missing | ABI refusal includes structured event | GENERIC VERIFIED; original startup UNKNOWN |
| Transactions | Bounded atomic word publication | New cross-region/stale/concurrent teardown proof cases | GENERIC VERIFIED; title owner evidence unchanged |
| TLS | Bounded native cells/registered threads | Generation/reuse/withdrawal/closing distinctions tested | PARTIAL; full Xbox threads/APC BLOCKED |
| Graphics/audio | Explicit native buffer/PCM subsets | Numeric edge fidelity/refusals added | SYNTHETIC VERIFIED; original formats UNKNOWN |
| Resources | Safe original resource streams; unknown archives | Regression/audit retained; no speculative decoder | PARTIAL/BLOCKED |
| Title admission | 13 variable providers denied; no original execution | Same zero admitted title providers/invocations | BLOCKED |

| Stages | Result |
| --- | --- |
| T126 | Completed baseline and existing-registration audit |
| T127–T128 | PARTIAL: source/native contract characterized; exact Xbox correction BLOCKED; no duplicate/numerical change |
| T129–T132 | Completed bounded independent oracles, coherence/inventory and proven boundary hardening; full ABI unverified |
| T133–T134 | Completed native lifecycle/transaction tests; raw-index and full Xbox thread limitations explicit |
| T135–T136 | Existing independent instruction differentials replayed; diagnostic and fail-stop checks fixed; exception continuation BLOCKED |
| T137–T139 | Existing subset and numeric/resource-edge audit completed; shaders/XMA/unknown archives remain BLOCKED |
| T140–T143 | Scoped matrix, generated-boundary integration, negative and three-profile regression completed |
| T144 | AArch64 ELF verified; public-only audit/CI; no NRO/deployment |
| T145 | Evidence-scoped decision completed; original execution NO-GO |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**. Native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED**. No gate changed. New generic correctness improves
safe synthetic support of imported identities, not original image applicability.
Live image ownership/permissions, initial values and observer/APC order, virtual
tails, provider lifetime and exception continuation still require qualified external
evidence. No title PPC was generated, compiled, linked or executed; no new original
call-site observation or title provider admission occurred.

Highest-value next action: acquire a qualified independent Xbox360 compare-contract
observation distinguishing a four-word mismatch case from full-match byte/word
units, with documented alignment/zero behavior, before changing the legacy SDK leaf.
A synthetic test on a qualified legal hardware observation interface could supply
that evidence; none is currently available locally. Do not repeat the same source
lineage or substitute Windows as Xbox proof. Stop at T145; T146 not started.
