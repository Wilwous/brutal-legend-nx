# Targeted investigation — native recompilation image contracts

2026-10-30; baseline `3a3996219f9a65e10ce18cc36e086e04873d0bfa`.
**G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED; original game execution NO-GO.**
This is an explicitly requested source investigation and original host-only
experiment after the freeze, not Stage24, console deployment or game admission.
No title load, proprietary instruction execution, SDK change or NRO rebuild.

## Decision: distinguish two proof goals

**PROVEN BY SOURCE:** native ports need not reproduce a console loader exactly.
UnleashedRecomp creates its own flat host-backed guest aperture, materializes
selected XEX data and invokes native translated functions. A separate runtime
also explicitly maps PE-sized sections and commits zeroed backing. These are
concrete alternatives, not evidence that our title's tail is Xbox-owned BSS.

**HOST VERIFIED:** an original, finite native-image model can own a virtual tail
outside the synthetic XEX descriptor extent without changing the strict loader.
Every accepted byte is backed by an explicit native owner. **HYPOTHESIS:** the
same separation could unblock a future Brütal Legend native layout after its
initialization and consumed startup data are independently justified.

Goal A (historical Xbox mapping) and Goal B (safe and functionally correct native
execution environment) are different specifications. Goal B's memory safety can
be tested without completing A. Its title-specific functional correctness cannot
be inferred from safety, generic PE rules or another game running. We previously
coupled native owner construction too tightly to Xbox residency authority. That
is worth separating architecturally; it does not retrospectively satisfy G4.

## Exact source inventory and scope

All retrieved sources were read only. No reference project was built or run.
Exact file SHA256 receipts and source copies are retained privately. Links pin
revisions rather than moving branches. Listed projects are not assumed working,
independent, or hardware-equivalent merely because a README says so.

| Repository / revision | Relevant functions and observed model |
| --- | --- |
| [hedge-dev/UnleashedRecomp](https://github.com/hedge-dev/UnleashedRecomp/tree/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c) | `Memory::Memory`, `Translate`, `FindFunction`, `Heap::Init`, `LdrLoadModule`, `GuestThreadContext`, `GuestThread::Start`. Independent flat native guest layout, runtime image copying, selected game-specific heap hooks: **B + E**, not exact A. Working-port status is upstream-reported, not independently revalidated here. |
| [hedge-dev/XenonRecomp](https://github.com/hedge-dev/XenonRecomp/tree/ddd128bcca99fe8bfbb99bea583c972351fa6ace) | `Xex2LoadImage`: analysis/materialization into a host-owned byte array, then PE-section metadata. Tool, not an independently working runtime. Its decompressor/image metadata cannot prove runtime reservation or guest lifetime. |
| [XDanfr/RaymanOriginsRecomp](https://github.com/XDanfr/RaymanOriginsRecomp/tree/c1bf7b90e95d476e63aa4a0b077905ff6dbde9bd) | `GuestMemory::Init`, `LoadXexImage`, `RuntimeHeap::{Init,Alloc,Free}`, `GuestMain`, `GuestThreadMain`, `InitMainThread`: flat native mapping plus Xenon image copy, native threads and custom runtime: **B**. Thread source explicitly follows Unleashed/Xenia; not independent corroboration. Runtime results remain upstream claims. |
| BelmanteGu/RaymanOriginsRecomp | GitHub repository API returned HTTP404 on this date. Revision, current contents and relationship **UNKNOWN**; not treated as a second working implementation. The XDanfr README's old clone URL does not establish availability or independence. |
| [nimauria/Xenon-Recomp](https://github.com/nimauria/Xenon-Recomp/tree/0b0bfb7b34ad0cd11488e2624860715a83179d0e) | `map_xex_image`, `load_xex`, `AddressSpace`, `ExceptionDispatcher::{dispatch_memory_fault,dispatch_exception}`. Explicit section-plan allocation, zero commit and final permissions: **B**, with compatibility policy. Source implementation, not a verified working static title port or Xbox oracle. |
| [rexglue/rexglue-sdk](https://github.com/rexglue/rexglue-sdk/tree/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518) | `XexModule::{ReadImage,ReadImageUncompressed,ReadImageBasicCompressed,ReadImageCompressed,ReadPEHeaders,LoadContinue,Unload}`, `XThread::{AllocateStack,Create,Execute,Exit}`. Xenia-derived service model **D**, reproducing selected loader mechanisms rather than proving A. Upstream inspected; project remains on its unchanged pinned patched SDK. |
| [abduznik/awesome-x360-recomps](https://github.com/abduznik/awesome-x360-recomps/tree/d502cb9fe86cad01b9076a930a006c2bb3957265) | Discovery catalog only. No loader or independent test evidence. Used to locate the following concrete ReXGlue implementation, not to certify playability. |
| [FluffyQuack/rexglue-MetalSlugXX](https://github.com/FluffyQuack/rexglue-MetalSlugXX/tree/bb11ea63595c9af62cb97feb1ef330ba05d32c92) | `metalslugxx/src/main.cpp` registers `MetalslugxxApp::Create`; its SDK `XexModule` implements the same descriptor/payload-oriented loading family, **D**. Concrete title application plus loader source, not an independent tail solution. No hardware execution reproduced. |

GPL-3.0 licenses are present for Unleashed/Rayman; XenonRecomp and nimauria
identify MIT. ReXGlue loader files carry BSD notices; the full SDK is mixed
licensed. Catalog API metadata gives no unambiguous SPDX license. No third-party
implementation or game-derived code is copied into our source tree; public
additions are an original test and this paraphrased source-linked assessment.

## What actually initializes the image

[Unleashed `LdrLoadModule`, lines122–169](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/main.cpp#L122)
copies NONE data using security imageSize, or BASIC block data and explicit
zeroSize runs, directly to `g_memory.Translate(loadAddress)`. It does not walk PE
sections to grow a separately owned image. [Memory construction](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/kernel/memory.cpp)
allocates the `PPC_MEMORY_SIZE` aperture RW, protects page zero and inserts
function mappings. Translation is base+offset with assertions, not per-object
commit/permission/lease mediation. Host anonymous mapping semantics supply fresh
backing; this is not a justified title-specific virtual-tail initialization rule.
`preload_executable.cpp` maps/locks the **native host executable** on Windows for
preloading. It is not the XEX loader or a statically embedded guest image.

[Heap initialization/hooks](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/kernel/heap.cpp)
partition fixed ranges for native allocators and replace selected title routines.
[GuestThreadContext/Start](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/cpu/guest_thread.cpp#L16)
allocate/zero a combined PCR/TLS/TEB/stack block, set selected fields and r1/r13,
then call the lookup result with r3. Thread context destruction frees that block.
This is selected native startup, not evidence for our title's TLS template, r2 or
all initial values. No transactional image-unload contract is established by
these functions. The source's failure/assertion assumptions are unsuitable as our
safety contract; for example, its POSIX fallback failure check tests nullptr after
mmap, and assertions are not complete runtime bounds checks.

[Rayman `LoadXexImage`](https://github.com/XDanfr/RaymanOriginsRecomp/blob/c1bf7b90e95d476e63aa4a0b077905ff6dbde9bd/runtime/loader.cpp)
uses `Image::ParseImage` and copies image.size bytes into the native aperture.
[GuestMemory](https://github.com/XDanfr/RaymanOriginsRecomp/blob/c1bf7b90e95d476e63aa4a0b077905ff6dbde9bd/runtime/memory.cpp)
protects the null page; `Translate` itself is unchecked base+guest. It does not
provide per-image ownership from a metadata label. Its own title-specific parser
fork is not assumed identical to the separately pinned upstream XenonRecomp.

[XenonRecomp `Xex2LoadImage`](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/XenonUtils/xex.cpp#L127)
uses security imageSize for NONE/NORMAL buffer sizing; BASIC sums data+zero runs
and zeroes those runs. It subsequently records PE VirtualSize ranges from the
materialized array. Recording that virtual range does not allocate extra backing
or demonstrate a loaded PE tail. Raw/virtual/security/descriptor sizes are not
interchangeable authorities.

The most relevant **explicit** native mapping example is
[nimauria `map_xex_image`](https://github.com/nimauria/Xenon-Recomp/blob/0b0bfb7b34ad0cd11488e2624860715a83179d0e/src/xbox/xex/loading/image_mapping.cpp):
section spans use max(virtual_size,raw_size,bytes.size), wide arithmetic and native
page alignment; allocation plans merge page ranges, reserve/commit zeroed backing,
copy section bytes and apply final permissions. Failures call rollback_mappings.
Descriptor permissions are preferred; absent descriptor coverage falls back to PE
permissions. This **PROVEN BY SOURCE** policy deliberately creates native owners
and zeroed space; the comment that virtual tails must be zero is implementation
policy, not independent Xbox evidence. Merged padding/permission unions may admit
more bytes than our restricted experiment; do not adopt that policy wholesale.
The complete runtime's concurrency, lifetime and title correctness were not tested.

[Current ReXGlue materialization](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/system/xex_module.cpp#L497)
remains payload-sized for NONE and descriptor-sized for BASIC/NORMAL, followed by
PE metadata/protection. ReadImage resets a heap; Unload releases the base. Our
owned transactional patches deliberately avoid substituting that broad Reset.
[Metal Slug's loader](https://github.com/FluffyQuack/rexglue-MetalSlugXX/blob/bb11ea63595c9af62cb97feb1ef330ba05d32c92/src/system/xex_module.cpp#L497)
shares this family. Neither inspected source proves a solution to this title's
14-section virtual-tail mismatch. No relocation implementation from these slices
is claimed correct; the proposed first model keeps fixed guest addresses and
refuses relocations rather than inventing a rebase.

## Direct comparison and independently testable path

| Contract | Current strict project loader | Proposed separate native image model |
| --- | --- | --- |
| Extent authority | Validated materialized XEX extent; section outside owned backing refuses, observed status0xC0000001 | Explicit independently justified image plan; native owners newly allocated for each approved span |
| Initialization | Checked original image materialization within descriptor/payload bounds | Exact source-byte ranges plus explicit justified zero runs; PE VirtualSize alone cannot authorize zero |
| Ownership/lifetime | Tagged SDK image allocations, module generation, leases, transactional rollback | Retain the same principles, separate namespace and manifest identity; never label new native backing as Xbox-observed ownership |
| Protection | Checked spans/commit/protection; rejects invalid binding | Keep mediation, deny unknown bytes/padding, no W+X; preserve owner/epoch checks, active-reference unload refusal |
| Startup | Consumed original title values lack provenance | Declare and validate stack/PCR/TLS/r2/r13/import needs for the bounded scope; do not borrow another game's constants |
| Faults | No unverified handler/return continuation admitted | Explicit terminal refusal policy for a restricted experiment, with observable context and bounded process containment |

Required architecture: an opt-in `NativeImagePlan` consumer separate from
`XexModule`, with versioned identity, checked extent arithmetic, source/hash
binding, exact initialization coverage, zero provenance, permission granularity,
no-overlap checks against every guest object, staged construction/final admission,
leases and rollback. The existing strict path must keep refusing its unsupported
original image. Do not automatically retry its failure using the alternative.

Migration path: original synthetic producer contract → independent plan verifier
and negative corpus → checked mapper transaction → generated synthetic caller
with exact image binding and startup contracts → independent admission review.
This task reaches only the host-side mapping model. A future title plan still
needs evidence that omitted bytes have no required meaning, zero initialization
is semantically correct, actual reachable loads/stores are covered, and required
imports/startup/exception behavior are available. Static access inventories do
not prove every dynamic address or initialization requirement.

## Original synthetic proof of concept

[Runner](../../tests/native-image-contract/run.py),
[fixture producer](../../tests/native-image-contract/fixture.py),
[original contract test](../../tests/native-image-contract/contract.cpp).
Reuses the independently authored NONE-XEX encoder: four sections/descriptor
extent0x40000; final section's VirtualSize extends to native extent0x50000 while
its raw size remains0x10000. The producer explicitly authorizes the final0x10000
bytes as zeros. No proprietary bytes, instructions or addresses are used.

The model uses existing `Space` checked backing, five owned64-KiB spans and a
separate16-KiB stack. It copies the four initialized spans, explicitly zeroes the
authored tail, then protects the first three spans read-only. Exhaustive comparison
checks all327680 accepted bytes against producer bytes/zeros and checks committed
owner queries. Tests cover absent zero authority, overlapping plan ranges, foreign
neighbor collision/preservation, five partial-construction failures, backing
exhaustion, RO writes, final-byte/cross-end bounds, null/unknown pointers, aligned
stack/bounds/teardown, active lease unload refusal, stale views after reload and
normal cleanup.

**HOST VERIFIED:** Debug/Release/ASan+UBSan each327725 passed,0 failed. Most counts
are byte comparisons, not distinct integration scenarios. All six compile/run
children exit0;30-second external process-group watchdog per child. No unexpected
faults, timeout or sanitizer finding. Metrics/raw receipts stay private.

This is a finite fixture model, **not a general XEX parser or cryptographically
bound production plan**, no PPC generation/entry, no actual PCR/TLS initialization,
no new Horizon proof and no title-functional-correctness proof. The author flag
is a trusted test input, not acceptable provenance for an arbitrary original XEX.
Read/write permissions here are logical; code is never host-executable. Same-page
padding, shared protection pages, arbitrary relocations, concurrent loader failures
and full failure-time context need further independent tests.

Reproduce only in a fresh directory outside the checkout:

```sh
python3 tests/native-image-contract/run.py --workdir "$NEW_SYNTHETIC_OUTPUT"
```

## Exceptions: can G8 be narrower?

[Unleashed native dispatch](https://github.com/hedge-dev/UnleashedRecomp/blob/cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c/UnleashedRecomp/cpu/guest_thread.cpp#L142)
directly calls the lookup result. The inspected memory/dispatch functions do not
prove unknown-target, unsupported instruction or general fault recovery safety.
Other handlers not inspected remain UNKNOWN, not assumed absent.
[Rayman entry](https://github.com/XDanfr/RaymanOriginsRecomp/blob/c1bf7b90e95d476e63aa4a0b077905ff6dbde9bd/runtime/main.cpp)
checks main-entry lookup, installs a crash handler, catches all C++ exceptions
around GuestMain, and uses process exit in the UI shutdown path. Those mechanisms
are not proof of Xbox unwind, correct exception classification, worker cleanup
or safe continuation. Unsupported instruction policy also requires generator and
call-site audits beyond these loader slices.
[nimauria ExceptionDispatcher](https://github.com/nimauria/Xenon-Recomp/blob/0b0bfb7b34ad0cd11488e2624860715a83179d0e/src/kernel/exception/exception.cpp)
snapshots thread/process host handlers and maps memory faults into records. Its
comments claim semantics; source structure alone does not establish Xbox kernel
handler search/unwind compatibility. ReXGlue POSIX signal handling likewise
cannot establish guest continuation semantics.

**HYPOTHESIS:** for a precisely restricted experiment that permits no guest
exception continuation, G8-native can require fail-stop rather than complete
Xbox dispatch. Every reachable fault/unsupported operation must be observed with
required context, must leave no admitted guest PC, and must terminate within an
external bound. C++ scoped errors can support controlled rollback; native SIGBUS/
SIGSEGV cannot be treated as catchable C++ errors or promised safe cleanup.
Contain native faults in a disposable child process; never retry a faulting PC.
Guest continuation becomes required only if the proposed functionality depends on
it. Whether the original bounded title scope does so remains UNKNOWN. Thus this
is a narrower *new specification*, not a declaration that historical G8 passed.

## Outcome and exact next milestone

A better architectural direction is defensible: **separate native image ownership
from historical Xbox ownership, preserve strict refusals, prove initialization
and safety explicitly**. The working-port evidence motivates this direction but
does not provide the title-specific bytes/zero/startup proof.

Next recommended milestone is a separately approved **synthetic NativeImagePlan
validator and generated-caller integration**, with immutable producer identity,
partial-page denials, TLS/PCR fixtures, full failure matrix and terminal-refusal
observability. No title execution is justified. Keep historical G4/G8 BLOCKED,
G5 UNKNOWN, game NO-GO. This report does not reopen the frozen Xbox branch or
silently replace its proof goals with weaker names.
