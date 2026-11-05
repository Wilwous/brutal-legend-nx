# Xbox compatibility foundation — T106–T125

2026-11-05. Source baseline `44249bdffbc2284e9bcbc6b571bbcad2ac6c4e9c`.
**Original title execution NO-GO.** This cycle implements selected source-backed
native contracts, not general Xbox compatibility. The [machine-readable matrix](compatibility-matrix.json)
records input/output, errors, pointers, endianness, ownership, lifetime, threads,
implementation, tests and title relevance separately for 25 contracts/obligations.

## Baseline, evidence and priority

T85 and T105 are **USER-CONFIRMED HARDWARE VERIFIED**, following the user's
current task statement. This is evidence for the synthetic applications only.
No new photographs, exact hardware counters, FPS, GPU memory or latency measurements
were supplied/independently inspected in this cycle. The preserved T85 NRO SHA-256 is
`8e4268ba72d8f10ba3a94f55f17a4361ecde01fcfeac01f7355974eb4278ebba`;
T105 is `98b44442310c554afa360573e42444dce6d83a1d39e8a3b9d659498c14c3ac3d`.
Neither package was rebuilt or overwritten. New compatibility execution is host
synthetic evidence; new Switch hardware execution is UNKNOWN.

The private inventory joins the retained original 274 import records: 261 functions
and 13 variables, 82 XAM and 192 kernel records. Imported, bounded statically
observed and runtime-verified are distinct. Header lookup and executable-module
handle use have retained bounded static evidence. The five selected new exports
are imported; their original startup use is UNKNOWN. Title runtime invocations and
new original-title bindings both remain **zero**. The older SDK-source audit
(7 implemented, 184 partial, 61 stub, 9 unknown) is unchanged and is not a progress
percentage or a behavioral validation of those exports.

* P0: preserve/join the existing title inventory and module/header observations;
  reuse the already tested generic provider infrastructure.
* P1: audit five imported SDK contracts, explicit ABI signatures, TLS lifecycle,
  module/thread identity and transactional checked writes.
* P2: explicit Xenos endian conversions into the existing Scene3D buffer formats
  and a narrowly specified PCM conversion into the existing mixer.
* P3 deferred: guessed title providers, APC delivery, shader/tiling translation,
  Xbox exceptional continuation, unknown archive and transformed-bank decoding.

The SDK revision is `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`; actual source
file hashes, including previously patched files, are in the matrix. That revision
alone does not identify patched content. Pinned implementation behavior is **not
independent Xbox hardware authority**. No installed SDK, isolated SDK source or
patch lock was changed. No new SDK patch was needed.

## Implemented service subset and ABI

`SdkServices` requires a real admitted `Scope`, exact registered GuestThread,
Space and live module. The synthetic gate selects an ordinal through r11; actual
scalar arguments use r3–r5, results use r3. This is a native test bridge, **not XEX
import-loader admission**. Unknown ordinals and reserved/unsupported contracts
throw explicit native errors. Such errors do not pretend to be Xbox NTSTATUS.

| Export | Implemented native subset | Limits/refusals |
| --- | --- | --- |
| xboxkrnl294 RtlFillMemoryUlong | BE32 pattern, checked OUT owner, void with no invented r3 return | Nonempty aligned address/length, length ≤65536; no remainder or foreign/RO access |
| 338 KeTlsAlloc | First free cell; zero caller cell; UINT32_MAX on exhaustion | Native profile reserves startup slot0, allocates1–63 |
| 339 KeTlsFree | Zero all registered thread cells before releasing slot; bool1/0 | Invalid/unallocated returns0; permission failure leaves every cell and registration unchanged |
| 340 KeTlsGetValue | Current thread BE32 cell; out-of-capacity returns0 | Slot0 explicitly unsupported; in-capacity unallocated access follows pinned SDK behavior |
| 341 KeTlsSetValue | Set current thread BE32 cell, bool1/0 | Same capacity/reserved-slot rule; exact thread/PCR context required |

One serialized service owner represents one native process/module and at most
16 registered live threads. The thread captures/pins immutable TLS ownership;
unregister/close precedes GuestThread cleanup. New registrations and calls refuse
after module closure begins. The process has 64 cells, with slot0 reserved by the
existing startup fixture; this is a **supported subset**, not a full Xbox process
TLS implementation. A caller must not create multiple service owners for one
process or destroy registered GuestThread objects early. Tests cover two threads,
isolation, allocation reset of caller only, all-thread free, exhaustion, boundary
indices, wrong owners/PCR, closure and zero final resources.

`Space::write_words` adds bounded atomic preflight/publication for 1–64 disjoint
aligned owned BE32 cells. Under the existing Space mutex it verifies all owners,
ranges, commitment and current write rights before any effect. It allocates no
publication storage and exports no raw pointer. A refusal on the second thread's
TLS cell cannot partially clear the first. This is host serialization, **not PPC
lwarx/stwcx reservation or general guest atomic semantics**. GuestThread slot
identity and Scope context validation are small additive queries into existing
checks; no loader, binder or protection policy was relaxed.

No new PPC instruction implementation or numeric ABI correction was justified.
Production codegen tests exercise scalar r3–r5/r11, u32 result zero extension,
BE32 memory, 96-byte stack frame, direct linked calls and LR return. Exact selected
nonvolatile r20/r31, f14 and v20 state remains preserved. This preservation check
is not typed-vector or floating-parameter marshalling. r2/r13/PCR/TLS ownership
continues to use the existing verified synthetic contract. Typed-vector signatures,
untested register/argument classes, reservation semantics, unknown indirect calls
and Xbox unwind/exception continuation remain denied/unverified.
`RtlCompareMemoryUlong` retains ambiguous source semantics/endian comments and is
not newly admitted; unrelated character exports were not implemented.

## Graphics, audio and resource boundary

`format_translation.h` consumes explicit owned checked copies, never guessed
formats or snapshot host pointers. Xenos enum values and word transforms are
source-backed: none, byte-swap within16, byte-swap within32, swap16-bit halves.
All four apply to u32/float32. The u16 subset admits none and byte-swap within16.
An independent byte-lane permutation oracle tests each mode.

Finite float5/stride20 vertices (≤4096) and bounded u16/u32 triangle indices
(≤12288) convert to existing Scene3D BEfloat5/BEu16. Wider indices narrow only
when in range. A nonvisual Scene3D draw snapshot consumes the converted triangle.
This verifies a concrete buffer translation, **not Xbox device initialization,
GPU command processing or original shaders**. Texture pitch/channel conversion,
tiling, swizzle, compressed formats, shader translation, render-target aliasing
and Xbox synchronization remain unsupported. The existing native OpenGL renderer
was not modified.

The pinned SDL driver input is 48kHz, six BEfloat32 planes of256 samples, 6144 bytes.
The new converter accepts front-left/front-right only; the other four planes
must be zero. All samples must be finite and in [-1,1]. Output is native stereo
s16LE with explicitly authored truncate(value×32768), clamp[-32768,32767]. This is
an adapter policy, **not a claim of Xbox bit-exact surround mixing**. Original
oracle PCM reaches the actual existing Mixer/HostAudioBackend. Wrong rate, size,
surround contribution, NaN, range, stale/foreign or unreadable buffers refuse.
No XAudioSubmitRenderDriverFrame shim is registered: client identity, semaphores,
callbacks and completion contracts are not proven. XMA remains unsupported.
Existing native queue/play/stop/volume/teardown tests were rerun.

Resource research reuses the retained metadata without new archive extraction.
BIKi identifies the existing movie containers; dfpf header/payload pairs expose
resource names but not a complete proven payload transformation. `.fsb` extensions
do not imply ordinary FSB: retained banks lack ordinary FSB magic at zero.
FMOD/Havok/Bink/Scaleform/Lua string evidence is not middleware integration.
No independently justified new title archive reader was identified; speculative
T122 decoding is deferred. Existing immutable manifest/source identity, bounds,
partial-delivery budget, cancellation and cleanup regressions remain the generic
resource path. No original texture, music, archive or title instruction entered tests.

## Production-generated integration and independent checks

The original synthetic XEX fixture contains **3 functions, 59 instructions,
59 instruction hooks and 22 static checked scalar memory sites**. SHA-256:
`f4dd9d62d3dc8e2551dd735282d9ba8acfd526f94d55bff6a046406efccde0fc`.
It uses the production ReXGlue `codegen` path, twice with identical output;
generated C++ is never manually edited. An independent decoder compares all59
instruction annotations; independent opcode counting verifies22 memory sites,
7 direct calls and3 returns. Public tests pin the original container identity.

The workload allocates/sets/gets/frees TLS, invokes void fill and calls two
explicit native graphics/audio translations. Normal execution yields u32
`0x89abcdef`, SDK entered/completed5/5, native translations2, dispatches9,
checked reads9/writes13 and hook polls74 (including existing entry polls).
It verifies normalized exact registers, BE fill, TLS reset, a triangle snapshot,
1024 independently expected queued PCM bytes, completion and cleanup. Bad PCR
and prior stop refuse before a generated entry; unknown ordinal, RO publication
and invalid index fail observably. The gate is original authored fixture code,
not a proprietary import thunk. Variable providers are separately regression-tested,
not claimed to be exercised by this new no-function-import image's gate.

## Tests and reproducibility

Use fresh **external** output directories, a previously prepared pinned toolchain
and no title input:

```sh
python3 tests/xbox-compatibility/test_fixture.py
python3 tests/xbox-compatibility/components.py --workdir "$OUT/components" --sdk-include "$TC/include"
python3 tests/xbox-compatibility/run.py --workdir "$OUT/production" --toolchain "$TC"
python3 tests/xbox-compatibility/build.py --workdir "$OUT/aarch64" --host-output "$OUT/production" --toolchain "$TC"
python3 tests/context-equivalence/run.py --workdir "$OUT/context" --toolchain "$TC"
python3 tests/native-image-plan/run.py --workdir "$OUT/plan"
python3 tests/native-module/run.py --workdir "$OUT/module"
python3 tests/variable-imports/run.py --workdir "$OUT/variables"
python3 tests/header-buffer/run.py --workdir "$OUT/header"
python3 tests/pre-entry-stack/run.py --workdir "$OUT/stack"
python3 tests/platform-services/run.py --workdir "$OUT/services"
python3 tests/platform-v2/components.py --workdir "$OUT/v2-components"
python3 tests/platform-v2/run.py --workdir "$OUT/v2" --toolchain "$TC"
```

Every new compile/execution child has an external monotonic30-second process-group
watchdog. Debug=-O0, Release=-O2, Sanitized=-O1 with ASan/UBSan and no recovery.
The original portable runtime harness also ran in these three profiles under that
watchdog: six cases, including4000-operation independent memory model per profile.
No deliberately faulting OS memory test or proprietary code was executed.

| Suite, each of Debug / Release / ASan+UBSan | Positive semantics | Expected refusals | Separate byte comparisons |
| --- | ---: | ---: | ---: |
| New transactional words | 69 | 10 | 0 |
| New SDK/context services | 164 | 18 | 16 |
| New graphics/audio formats | 23 | 17 | 2312 |
| New generated integration | 23 | 5 | 1040 |
| Existing NativeImagePlan | 75 | 49 | 16384 |
| Existing native module | 97 | 21 | 4 |
| Existing variable imports | 120 | 84 | 16 |
| Existing header buffer + reader | 125 | 71 | 1248 |
| Existing stack window | 5 | 16 | 0 |
| Existing generated context differential | 486 | 34 | Oracle425984; differential393216 |
| Existing T105 production integration | 42 | 11 | Not reported separately |

New suites total **279 positive semantics, 50 expected refusals and3368 byte
comparisons per profile**; repeated profiles are not new compatibility contracts.
Existing context differential runs24 cases,12 equivalent pairs and10 counterexample
pairs per profile; its external Python verifier checks393216 bytes independently.
Additional platform regressions retain their original heterogeneous counts:
T85 lifecycle31/7 (one injected cleanup error observed), graphics10/26, input14/16,
audio58 checks, resources36/48 with5036 copied bytes; V2 graphics20/41/16 bytes,
audio55 semantics/26 refusals/10 bytewise checks, resources225 checks/40 refusals/
78995 comparisons and timing42 checks/17 refusals. These are not summed into an
inflated SDK/import coverage count. T105's one reported host underflow is preserved,
not suppressed or reclassified as device evidence.

Final bounded semantic runs: no unexpected fault, timeout, sanitizer finding or
remaining owned allocation/lease; source/SDK diagnostics and per-child timing/RSS
are preserved privately. The final checkpoint records150 bounded child commands
(including compilation/generation/inspection),145.55 seconds of summed child
wall times, and483459072-byte peak host child RSS. New integration execution
peaked at61030400 bytes across profiles. These are not Switch memory/performance
measurements; container records measure the Docker client, not container compiler
RSS (container limit1GiB). Earlier development attempts remain saved: two optional
stack-runner input preflight errors (wrong argument, absent old summary), a Docker
compiler PATH lookup failure and a GCC misleading-indentation error in the new
harness. The runner uses the actual compiler path and explicit braces; warnings
were not disabled. Those attempts are not counted as passing regressions.
The optional old generated leaf replay was not run by the window-only command;
new production integration, T105 and context differential cover actual generated
entry checks separately, without fabricating a mock replacement.

Portable CI runs the fixture/matrix and words/formats suites without an SDK.
SDK-context adapters, production generation and cross linking are explicitly local
checks, not misrepresented as GitHub execution of the private SDK pipeline.

## Before / after and stage decisions

| Area | Before | After | Title applicability |
| --- | --- | --- | --- |
| ABI | Existing selected scalar/vector execution and typed-signature denials | New real scalar SDK-call workload and context revalidation; no speculative numeric change | PARTIAL; complete ABI unverified |
| SDK services | Existing module/header and synthetic native services | Five source-backed native export subsets tested through generated PPC | Imported identities proven; original invocation/admission absent |
| Variables | Generic owned lifecycle;13 original providers denied | Same13 denied; retained evidence indexed per provider | BLOCKED |
| Threads/TLS | Owned stack/PCR/static TLS/startupslot0 | Registered process cells1–63, isolation, atomic all-thread clear, closure checks | PARTIAL; APC/KTHREAD/startup unknown |
| Memory | Checked ranges/current rights/leases | Atomic bounded disjoint word publication | GENERIC VERIFIED; original image permissions unchanged |
| Graphics | Native Scene3D/OpenGL | Explicit endian vertex/index conversion into real Scene3D consumer | SYNTHETIC VERIFIED; exact title format UNKNOWN |
| Audio | Native PCM mixer | Front-only six-plane BEfloat adapter into existing mixer | SYNTHETIC VERIFIED; XAudio/XMA BLOCKED |
| Resources | Bounded original manifest streams | Regression/research retained, no guessed archive decoder | PARTIAL/BLOCKED |
| Startup/exceptions | Missing owner/observer/startup evidence; fail-stop | No new external evidence or exceptional continuation | BLOCKED |

| Stages | Decision |
| --- | --- |
| T106–T108 | Completed baseline lock, retained private import inventory and scoped machine matrix |
| T109–T110 | Selected ABI audit/hardening completed; full ABI remains PARTIAL; no new instruction implementation justified |
| T111–T112 | Five source-backed supported subsets completed; no claim of exact hardware SDK equivalence |
| T113–T114 | PARTIAL; generic provider regression and new native TLS verified, all13 original providers/APC/startup still BLOCKED |
| T115 | Bounded memory transaction and refusal checks completed; guest exceptions remain BLOCKED |
| T116–T117 | PARTIAL Xbox mapping; explicit buffer subset completed; original textures/device behavior UNKNOWN |
| T118 | Xbox shader/draw-state translation BLOCKED; concrete existing Scene3D consumer tested |
| T119–T120 | PARTIAL Xbox audio; explicit front-only converter completed; engine/callback/XMA BLOCKED |
| T121–T122 | Retained metadata review PARTIAL; unknown archive translation deferred without new evidence |
| T123–T124 | Original production workload, targeted integrations, three host profiles and AArch64 compile/link checkpoint |
| T125 | Evidence-scoped readiness audit completed; original execution remains NO-GO |

No third visual demo, new NRO or deployment is warranted: no native GPU/audio/USB
backend behavior changed. An isolated pinned devkitA64/libnx AArch64 ELF compile/
link checks the new adapters with the production-generated original module;
this does not execute the ELF on Switch. T85/T105 hardware baselines remain intact.

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**. Native **N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED**. Remaining external premises include applicable live
image ownership/RW authority and observer order, virtual-tail/SizeOfImage contents,
startup register/thread/PCR/TLS values and lifetime,13 variable providers and
exceptional continuation. No source/software-only synthetic success resolves them.
The highest-value next engineering action is to obtain an independent expected
contract for the imported `RtlCompareMemoryUlong` (return units, guest endianness
and alignment), then test a bounded adapter only if that ambiguity is resolved.
This is separate from the external startup/owner/observer evidence needed for
original-title admission. No T126 is started.
