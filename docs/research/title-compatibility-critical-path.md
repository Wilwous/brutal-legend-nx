# Title compatibility and critical path, T146–T165

2026-11-06. Baseline `ec2d4616211e47896391df79d6e9363c605b61e2`.
Original execution remains **NO-GO**. The cycle fixes two native ownership
defects and validates a bounded BIKi metadata profile on the base resource
corpus. Both user-confirmed hardware baselines remain preserved.

## Evidence and priorities

The private import readiness ledger matches all 274 original import identities
against the pinned SDK export tables (261 functions and 13 variables). It
incorporates 33 earlier signature and behavior records and preserves unknown
fields explicitly. The retained bounded evidence distinguishes two selected
static import observations from simple import presence. Original title
invocations and admitted bindings remain zero.

There are nine implementation profiles for eight distinct export identities in
the selected Horizon fixtures. Fill ordinal 294 has two profiles. The four
legacy SDK bridges and five native adapters have separate scope and provenance.
Earlier host SDK probes are a separate body of evidence. An export table row,
an implementation tag, a synthetic bridge, and a title binding are four
different things. The [versioned matrix](compatibility-matrix.json) records
the profile IDs and exact limits.

Priorities were the retained module/header dependency, reproducible native
ownership defects, and a bounded reader for the identified BIKi format. The
dependency order was preservation and reproduction, implementation, independent
format checks and production synthetic replay, then cross build and publication.
The frozen startup order is reused from the existing evidence. [Startup footprint](title-startup-footprint.md) · [Writer and observer contract](title-writer-observer-contract.md).

## Two reproduced defects

**Field construction.** A native caller could directly construct a Field with
a valid retained Metadata reader, a valid offset, and an unrelated guest address.
The before-fix fixture returned the unrelated address while reading the expected
BE32 value from the real header. The constructor is now private to the checked
lookup. A compile-time regression rejects direct construction. Runtime coverage
checks address and value through provider teardown, moved borrow refusal, and
last-reader cleanup. Existing absence, duplicate selector, wrong generation,
sealed cell and revocation behavior remain covered.

**Incremental resource verification.** A trusted native caller with the exact
staging allocation identity could restore writable permission after the first
hash step, replace a previously verified byte, then restore read permission.
The before-fix fixture published the modified bytes after the hash completed.
The reproducer uses only original fixture bytes and native management authority.

The stream now freezes writes under its existing allocation lease from the end
of delivery until publication. Ordinary protect and commit refuse write access
during that interval. A competing pin cannot publish the freeze. Publication
checks live read permission and consumes the freeze under the Space mutex.
The existing read-only and explicit writable handoffs remain available. Read
revocation and irreversible seals remain authoritative. Failed cancellation
with a competing pin retains the job and its freeze. The tests cover the
original byte mutation, moved lease, revocation, seals, cancellation and balanced
cleanup. These are native ownership guarantees under the checked Space contract.

## BIKi metadata profile

The original reader accepts a standalone revision-i envelope whose declared
extent equals the actual source extent. It validates the 44-byte fixed header,
bounded audio descriptors, and frame index including the final extent word.
The index must start at the metadata boundary and describe increasing in-range
frames. Extents are checked before narrowing or allocation. The profile caps
frames at 16384 and tracks at 32. The reader makes at most two bounded metadata
reads and rejects short reads, unknown tags, malformed lengths, and unsupported
envelopes. Track flags and the auxiliary header word remain metadata.

FFmpeg supplies an independent demuxer reference for the little-endian header,
track layout, and frame offsets. The stricter extent and index requirements are
this project's profile policy. FFprobe 9.0.2 was also run with stream probing
disabled on the independently authored fixture. It agreed on dimensions, frame
rate, sample rates and channel counts. A separate Python oracle checks the
canonical parsed index and descriptor digest. No FFmpeg code is vendored or
linked. [Pinned FFmpeg demuxer](https://github.com/FFmpeg/FFmpeg/blob/8105ae2afe2c4182a4ad44b1b9c45a3146f0c0b1/libavformat/bink.c).

The private read-only check reconciled all 74 base BIKi files against their
retained extraction records and file sizes. It read 245100 bytes of header and
index metadata in the independent oracle. The sanitizer-built inspector
matched the complete canonical parsed metadata for **67 exact standalone
envelopes**. Seven physical extents exceed the declared envelope and were
refused. Their first declared indexes are structurally consistent in the
independent audit. The extra intervals remain unclassified and were not treated
as padding. No payload was read by the private audit or decoded. Old extraction
hashes are provenance references. The audit hashes the new metadata prefixes
without rehashing the full movie payloads.

This closes a structural resource proof for 67 files. Codec decoding,
playback, original consumer binding, transformed FSB-like banks and dfpf archive
payload semantics remain open. BIKi track flags supply metadata only. They do
not establish an Xbox PCM, XMA or voice callback contract.

## Before and after

| Area | Before | After | Title relevance and limits |
| --- | --- | --- | --- |
| Module/header | Checked lookup plus a public unchecked Field constructor | GENERIC VERIFIED checked construction and retained lifetime | Related dependency is TITLE STATICALLY OBSERVED. Original provider BLOCKED |
| Resource publication | Writable rights could be restored during incremental SHA verification | GENERIC VERIFIED lease-controlled write freeze and explicit transfer | Reusable resource integrity. Original archive runtime still BLOCKED |
| BIKi | Signature inventory | GENERIC VERIFIED strict header and index reader. 67 base envelopes validated | TITLE APPLICABILITY PARTIAL. Seven suffixes and codecs remain unresolved |
| SDK / ABI | Selected scalar, FPR and VMX fixtures and four legacy memory services | Same numerical behavior, versioned profiles and current regression evidence | No new Xbox hardware contract. Compare prefix/units UNKNOWN |
| Threads / TLS | Owned native stack, PCR, registered TLS and closure checks | Same bounds with passing current production and lifetime regressions | APC, startup values, KTHREAD and reservations BLOCKED or UNKNOWN |
| Graphics / audio | Xenos endian and explicit vertex/index subset, front PCM subset | Existing Scene3D and mixer regressions pass with the frozen stream path | Shaders, texture tiling and compressed audio remain unsupported |
| Title execution | G4/G5/G8 and native gates retained | Zero new title admissions | External proof still required |

## Verification

All host profiles ran with the existing bounded process-group watchdog.
Debug, Release and ASan/UBSan passed. The table separates new semantic
assertions, expected refusals, and bytes where the runners report them.

| Suite per profile | Assertions | Expected refusals | Byte evidence |
| --- | ---: | ---: | --- |
| BIKi original contract | 11 | 33 | 84 metadata bytes read, zero payload. Separate canonical digest oracle |
| Variables / header lifetime | 108 non-byte assertions | 86 | 16 individual byte checks |
| V2 resources | 194 successful non-refusal assertions | 50 | 79000 byte comparisons |
| NativeImagePlan | 75 | 49 | 16384 byte comparisons |
| Generated module startup | 27 | 12 | Nine pre-entry cases with zero new PPC entries |
| Existing generated SDK compatibility | 23 | 5 | 1040 byte comparisons |
| Existing T105 integration | 42 | 11 | 39240 original resource bytes delivered |

Production ReXGlue module startup uses five functions, 33 instructions,
33 hooks and 12 checked memory sites. It reports result31, two native header
queries and one controlled stop. The existing T105 generated consumer delivers
streamed assets through the corrected path with 340 ticks, result340 and
1353/1353 completed services. Its original host underflow counter remains1.
The five-call SDK compatibility workload returns `0x89abcdef`. Stage15 reports
423 passing assertions including 72 refusals in normal/controller/no-trace
modes and 759 passing assertions including 93 refusals in stress mode. Its
seven expected negative compilations remain checked. Existing TLS, format,
transaction, graphics, PCM and ABI edge fixtures also pass.

Final host tests report zero unexpected failures, sanitizer findings and
remaining allocations/leases in the owned cleanup cases. The seven private
envelope refusals are expected and recorded separately. The original two
defect witnesses and intermediate AArch64 compilation diagnostics are retained.

The pinned offline devkitA64/libnx build checks the metadata, module startup
and T105 ELF targets. The hardware scope stays with T85/T105. No new NRO
or device acceptance is justified for the CPU and ownership changes.

```sh
python3 tests/bink-metadata/run.py --workdir "$OUT/bink" --ffprobe "$FFPROBE"
python3 tests/variable-imports/run.py --workdir "$OUT/variables"
python3 tests/platform-v2/components.py --workdir "$OUT/v2-components"
python3 tests/module-startup/run.py --workdir "$OUT/module" --toolchain "$TC"
python3 tests/platform-v2/run.py --workdir "$OUT/v2" --toolchain "$TC"
python3 tests/bink-metadata/build.py --workdir "$OUT/a64" --toolchain "$TC" --module-output "$OUT/module" --v2-output "$OUT/v2"
```

Output directories must be fresh and external. The optional FFprobe check
receives only the original fixture. CI runs the portable parser and ownership
regressions. Private resource audits, SDK-assisted production and cross build
remain local receipts.

## Stage disposition and critical path

| Stages | Outcome |
| --- | --- |
| T146–T147 | Reconciled selected service inventory and reused the finite startup dependency order |
| T148–T153 | Selected ABI, SDK, synchronization and TLS reviewed with regression evidence. Broader Xbox semantics remain PARTIAL/BLOCKED |
| T154 | Checked native field construction fixed and verified. Original provider BLOCKED |
| T155–T158 | Existing graphics and PCM subsets verified. New GPU or voice translations require actual format and consumer evidence |
| T159–T160 | Bounded BIKi metadata profile complete. Title applicability PARTIAL |
| T161–T163 | Production module/stream replays, metadata oracle and affected three-profile tests complete |
| T164 | AArch64 ELF checkpoint and public-only publication audit |
| T165 | Critical path assessed. Original execution NO-GO |

1. Establish live owner, reservation, commitment, RW permission and lifetime
   for the two fixed image-backed store destinations. Establish their initial
   bytes and the first observer ordering across provider initialization and
   early APC delivery. This is the smallest open evidence task.
2. Qualify the actual entry thread, registers, owned stack, PCR and TLS and
   resolve the required image initialization ranges without guessing virtual
   tail contents. Prove the bounded accepted call and stopping sequence.
3. Resolve applicable exceptional transitions, unknown continuations, and
   provider initialization/lifetime before admitting the selected fragment.
   Only then review the complete evidence against strict NativeImagePlan.

The preserved 19-site normal prefix has no ordinary SDK imports. The later
module/header consumer and Compare semantics are separate obligations. Neither
the BIKi results nor the two generic fixes establish the missing original
startup observations. **EXTERNAL EVIDENCE REQUIRED** under the retained T65
acquisition decision. Historical **G4 BLOCKED, G5 UNKNOWN, G8 BLOCKED**.
Native **N-G4 BLOCKED, N-G5 PARTIAL, N-G8 BLOCKED**.
The cycle stops at T165. [External tracing decision](xbox-startup-tracing-feasibility.md).
