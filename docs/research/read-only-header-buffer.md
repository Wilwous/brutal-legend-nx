# T16 — standalone read-only guest XEX header buffer

2026-11-03; baseline `d6256a6419cd0d72d39529f0b0ab2cd738abcaff`.
**COMPLETE within the isolated logical guest-memory contract.** Original header
provenance, inert materialization and checked read-only admission are VERIFIED
for this single metadata object on the host. Executable-module provider, full
title image and game execution remain BLOCKED / NO-GO. No title instructions
were generated, compiled, linked or executed; no title imports or entry were bound.

## Source and allocation contract

[header_buffer.h](../../runtime/horizon/header_buffer.h) uses existing `Space`,
`Memory` and `ModuleLife` ownership/lease APIs. A trusted descriptor binds the
entire source digest, exact interval digest, offset/length, explicitly selected
guest coordinate and the actual native module identity/instance. A canonical
`HEADER01` digest is checked against an independently supplied trusted digest.
Hashes compare supplied evidence; they do not authenticate an arbitrary caller's
provenance claim. The source span must remain valid and immutable during copying.

Before reservation, the mechanism checks nonzero identities, matching module
instance, a bounded source extent, checked ptr32 arithmetic, guest alignment,
header magic, declared byte length and optional catalog bounds. Source size is
capped at64MiB and header size at1MiB. This is a bounded metadata validator, not
an authenticated XEX loader or a complete validator of every optional payload.

The buffer reserves its **exact logical length** in a supplied guest arena,
commits RW for initialization, copies only the verified interval, seals it read
only, acquires an allocation lease, verifies the checked copy and then exposes
its reference. No host pointer escapes. Page-rounded backing does not widen the
logical region; bytes beyond its exact length remain inaccessible. Collisions
refuse, including when a different owner already occupies the requested range.
No section, stack, PCR/TLS, SDK object or historical PE address is inferred.

A registry allows one live header per `ModuleLife` instance. Separate module
instances may have the same numeric module label but distinct generations and
buffers. Source identity binds the newly authored native owner; it does not
claim original Xbox loader-object identity. No pointer from preserved image B
is imported.

## One permission authority

[checked_memory.h](../../runtime/checked_memory.h) now provides an opt-in
irreversible read-only seal **inside the existing permission authority**.
`Space` only delegates it. After sealing, `protect` and `commit` cannot grant write
or read/write, even after decommit/recommit. Revocation to none and restoration
to read remain permitted. Every real access rechecks current rights and epoch.
A published header therefore cannot become writable through these APIs.

An isolated pre-change probe also confirmed that an unknown `Access` value could
create a view while permissions were none. No bytes were read and no code ran;
subsequent typed reads/writes had separate access-mode checks. The central check
now refuses unsupported operations explicitly. New regressions cover this path.
The correction applies to ordinary allocations too; existing supported behavior
is regression-tested.

There is no execute permission or execution capability in this checked backend.
An attempted execute-like unsupported access is refused centrally; unsupported
permission encodings also refuse. This proves logical admission refusal, **not**
host CPU execute faults, `mprotect`, complete guest isolation or Switch hardware
behavior. Host-page granularity limitations are not bypassed or solved here.

## Lifetime and concurrency

A published buffer holds both an allocation lease and a `ModuleLife` lease.
Release/decommit are refused while the allocation is pinned. Module close closes
new admission but cannot finish while the owned header remains live. A reader
holds shared state, not a raw buffer pointer. Closing stops new readers; active
readers can finish; final reader release performs cleanup. The buffer wrapper
may be destroyed after a reader was acquired without reclaiming its memory early.

All state/ref/read/close operations serialize on the object mutex. Lock order is
state → Space → Memory. The registry lock is never held while acquiring state;
module admission finishes its internal lock before entering state. Actual read
and permission changes serialize through Space. Cleanup atomically consumes the final allocation pin and releases its exact token
under the Space lock, then unregisters the header and drops the module pin.
Construction failures use the same cleanup path before publication. A source
review found an unpin/release window in the first draft; atomic retirement fixes
it. The full selected campaign was rerun after that correction.

`Space` and `ModuleLife` are borrowed native objects and must outlive all retained
state. Their existing destructors fail-stop on outstanding leases. Callers must
also retain the C++ wrapper during its API calls. This is not protection against
arbitrary native memory corruption or unsynchronized deletion of C++ objects.

Raw `Space` pins are trusted native management rather than header `Reader`s.
They block atomic retirement without changing the header's pin or rights. Keep
the wrapper until these external pins drain, then retry `close`; tests cover a
last Reader finishing while an external pin remains. Destroying native owners
while unmanaged pins remain is a fail-stop precondition violation. Automatic
wrapper-independent last-reader cleanup applies to header Readers; it does not
extend arbitrary external C++ objects' lifetimes.

Read capabilities carry exact buffer/module-instance identity. Old references
and views refuse after cleanup/reallocation, even at the same selected logical
address. **A bare ptr32 alone does not distinguish address reuse.** Such a raw
address must not replace the retained capability/lifetime contract in a future
provider. The current bounded arena quarantines backing addresses and is not a
general recycling heap. No unbounded create/destroy endurance claim is made.

## Private original-header exercise

The original XEX SHA-256 matches the preserved baseline. Only the original
container's offset0, declared **12,288-byte header** is selected. Independent raw
record traversal, bounds and source/header hashing agree with T14/T15; the
requested selector remains absent. Neither reconciled PE view is used as header
backing. Detailed catalog contents, interval digests, module values and actual
guest coordinates remain private.

[Generic data reader](../../tests/header-buffer/data.cpp) was first verified on
original synthetic data with a separately encoded Python descriptor/hash oracle.
The same generic reader binaries then consumed the original file as DATA only.
A fresh isolated arena held exactly the header region. Checked byte comparison,
source digest, RO seal, write/unsupported-execute/bounds refusal and cleanup pass
in all three profiles. No unrelated image memory, unknown tail or permission-gap
bytes were allocated or exposed. The full source file was read only to verify
its fingerprint; its executable payload was never mapped into guest memory.

This is **actual logical guest admission of that isolated header**, beyond T7's
inert host copy. It is not original Xbox residency evidence, a Horizon hardware
run, an executable image, a module object or an imported variable binding.

## Reproducible original tests and exact evidence

[Lifecycle tests](../../tests/header-buffer/contract.cpp) cover source and interval
hash mismatch, malformed sizes/catalog, overflow/alignment, collision, wrong owner
and module, missing commitment, irreversible write refusal, unsupported execution,
revocation/restoration, stale/moved reader, old views after reuse, duplicate
publication, closed module, repeated cleanup and six initialization-failure points.
Two module instances and32create/publish/close cycles are tested. A latch-controlled
reader remains live across module close, buffer close and wrapper destruction;
release/decommit refuse and last-reader cleanup is checked. This is one declared
deterministic interleaving plus a lock/lifetime argument, not exhaustive scheduling.

```sh
python3 tests/header-buffer/run.py --workdir "$T16_OUTPUT"
python3 tests/native-image-plan/run.py --workdir "$T16_PLAN_OUTPUT"
python3 tests/native-image-contract/run.py --workdir "$T16_OWNER_OUTPUT"
python3 tests/module-provider/test_contract.py
python3 tests/header-field/test_contract.py
python3 tests/native-provenance/run_data.py --workdir "$T16_DATA_OUTPUT"
python3 tests/pre-entry-stack/run.py --workdir "$T16_STACK_OUTPUT"
python3 tests/horizon-foundation/core.py --workdir "$T16_CORE_OUTPUT"
```

Use new external output directories. Local T7 data/context tests additionally used
preserved pinned SDK headers. T8 entry and T10/T12 regressions used their existing
original production-generated fixture pipeline and independently checked results;
all private commands/inputs/outputs are retained for reproduction. No title input
was provided to those generators. SDK sources, headers and patch locks were not
modified. CI runs the SDK-independent new suite and existing component checks.

| Measure per profile | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| New lifecycle semantic assertions / expected refusals | 115 / 66 | 115 / 66 | 115 / 66 |
| New lifecycle byte comparisons | 1,216 | 1,216 | 1,216 |
| Synthetic reader assertions / refusals / bytes | 10 / 5 / 32 | 10 / 5 / 32 | 10 / 5 / 32 |
| Private header reader assertions / refusals / bytes | 10 / 5 / 12,288 | 10 / 5 / 12,288 | 10 / 5 / 12,288 |
| NativeImagePlan semantic / refusal / bytes | 75 / 49 / 16,384 | 75 / 49 / 16,384 | 75 / 49 / 16,384 |
| Image ownership passing / refusal / bytes | 45 / 17 / 327,680 | 45 / 17 / 327,680 | 45 / 17 / 327,680 |
| T7 data positive / refusal; context positive / refusal | 29 / 38; 26 / 23 | 29 / 38; 26 / 23 | 29 / 38; 26 / 23 |
| T8 window positive / refusal; generated entry positive / refusal | 5 / 16; 39 / 28 | 5 / 16; 39 / 28 | 5 / 16; 39 / 28 |
| T8 original foundation passing / failing | 108 / 0 | 108 / 0 | 108 / 0 |
| T10 positive / refusal / bytes | 87 / 72 / 180,224 | 87 / 72 / 180,224 | 87 / 72 / 180,224 |
| T12 positive / refusal; oracle / differential bytes | 905 / 40; 638,976 / 589,824 | 905 / 40; 638,976 / 589,824 | 905 / 40; 638,976 / 589,824 |
| Horizon core passing / failing; checked-memory foundation cases / failing | 20 / 0; 6 / 0 | 20 / 0; 6 / 0 | 20 / 0; 6 / 0 |

Five affected Python suites pass95methods, including unchanged T14/T15 parsers and
context models. Byte counts and refusals are separate measures, never added into
an inflated test total. All final header/fixture allocation, lease and registration
counts are zero. Header tests invoke no PPC body; older original generated
regressions execute their own synthetic PPC only.

The selected110 externally supervised children take a summed81.538690 seconds;
peak RSS342,097,920 bytes including compilation,39,731,200 bytes excluding it.
Every child has a30-second monotonic process-group watchdog. There are9expected
nonzero data-refusal exits,0expected signals,0unexpected failures/faults/timeouts
and0sanitizer findings. Initial successful development suites and the pre-change
probe remain privately preserved and are excluded from these final totals.

## Separate readiness decisions

| Obligation | Decision |
| --- | --- |
| A original synthetic buffer mechanism | VERIFIED within checked native contract |
| B original header source provenance | PROVEN offline |
| C private header data-only materialization | VERIFIED |
| D isolated header checked RO guest admission | VERIFIED on host logical backend only |
| E executable-module provider | BLOCKED |
| F full title NativeImagePlan | BLOCKED |
| G original title execution | NO-GO |

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**; native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED**. This isolated object does not close complete N-G4.
The392-byte permission gap, unknown virtual tail/remainder, variable providers,
startup context, opaque callees, historical API equivalence, full ABI and guest
exception continuation remain.

The single smallest next proof is a **nonexecuting, owned native metadata prefix
with a retained reference to this header**, qualifying module/source-instance
association and coupled lifetime. It must not publish the exported value cell,
bind imports or claim the complete Xbox module layout. This is a recommendation;
no next stage, provider, game entry or deployment is started here.
