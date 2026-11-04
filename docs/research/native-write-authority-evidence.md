# Title RW authority, initialization and observation boundary — T41–T52

2026-11-04. Baseline `f92781b688d211b5b27aebbe39620d3e2ffaa3f2`.
**Private eight-byte RW object DRAFT/BLOCKED; original entry NO-GO.**
This finite cycle adds source/metadata applicability evidence and a conditional
observation argument. It does not rebuild the already verified generic RW runtime.
No title PPC generation, compilation, linking, execution or hardware deployment.

## Additional qualified evidence

The private T40 two-store ledger, original XEX and A/B identities are reverified.
The eight consecutive bytes are inside the same PE data section and covered by an
original XEX data-page descriptor, independently checked against its raw header
word. This corroborates a data classification beyond the PE RW flag alone.
It is original metadata evidence, not a live page-table/owner/commitment snapshot.
The previously verified descriptor/content chain is not an RSA-signature or
hardware-state verification.

A narrow review of the pinned isolated SDK's `XexModule::AllocateImage`,
`LoadContinue`, `SetupLibraryImports`, `Unload` and `UserModule::LoadXexContinue`
confirms this implementation's sequence: owned fixed allocation/materialization,
import resolution, descriptor-based protection, TLS metadata/introspection and
module-header publication. DATA descriptors select RW; code/RO descriptors select
read. Owned images and image leases govern that implementation's release.
Actual working-source hashes are recorded privately because the SDK contains
existing patches; an upstream revision alone is not its complete identity.
These are implementation contracts, not authoritative Xbox loader observations.

All 535 raw original import-record slot addresses are independently decoded and
matched to the preserved 274-import inventory. Their four-byte spans do not
intersect either target. The selected SDK writes variable export addresses into
matching slots; the 13 variable slots are a subset of that conservative disjoint
set. Thus direct slot-fixup writes do not modify these eight bytes under this
implementation. This does not cover provider side effects, TLS/launch routines,
other initialization writes, aliases or original Xbox behavior.

The descriptor covers a larger page containing adjacent bytes. Applying its SDK
RW rule to that entire page would exceed the native interval policy. The 392-byte
gap remains inaccessible. Descriptor coverage is not permission to extend native
ownership beyond independently admitted logical intervals. No new sub-page
machinery or relaxed validator is needed.

Previously inspected native ports demonstrate that a native owner need not copy
historical allocator internals; they supply no new title-specific proof here.
No exhausted emulator, RXDK or hardware experiment is repeated. No new qualified
hardware interface or authoritative rule resolves the live-state obligations.
See [the existing native-contract source investigation](native-image-contract-investigation.md)
and [T29–T40 contract and test evidence](source-bound-writable-image.md).

## Initialization at the actual boundary

| Question | Finding |
|---|---|
| A/B source values | VERIFIED identical for the exact eight bytes; private values/digests retained |
| Direct import-slot mutation of targets | Excluded for the decoded slots and reviewed implementation |
| Values after all import-provider side effects | UNKNOWN |
| Values after earlier runtime/TLS/launch initialization | UNKNOWN |
| Read of target bytes in selected normal prefix | None in the preserved 19 normal sites |
| Full pre-entry writer/observer closure | Not established |
| Live target values at intended title entry | UNKNOWN |

A decompressed/saved value is not automatically the value at a later entry
boundary. The checked source-bound runtime can initialize a chosen native object
correctly from a descriptor; it cannot prove that the chosen descriptor captures
all original initialization. No guessed zero-fill, historical pointers, rebase,
title update or live-state substitution is introduced.

## Conditional native behavioral equivalence

Historical Xbox equivalence and bounded native behavioral equivalence are different
proof goals. A native allocation can select a different internal owner/backing
while preserving guest addresses, admitted operations, lifetime and observations.
It requires an explicit observational boundary and proof for the actual consumers.
Generic memory safety alone does not provide that proof.

For the two known constant full-width stores, substituting the store effects shows
that **after both complete**, every target byte is independent of its initial
symbol. This finite target-byte result requires that both writes are permitted,
addresses/payloads are correct and no earlier/intermediate observer or exceptional
exit is included in the claimed observation. It does not prove whole-context,
stack-address, call/return, concurrency or title-entry equivalence.

Four original symbolic examples validate the substitution and counterexamples:
complete overwrite removes all initial symbols; an early/intermediate observer
can distinguish initial states; a later alias write changes the result; leaving
one byte unwritten preserves an initial dependency. No original words/values or
PPC execution are used. These are equational examples, not a machine-checked proof
of the title CFG or a newly emulated runtime. Two equality assertions and four
counterexample-inequality assertions are kept separate from runtime test counters.

This weaker post-store observation result cannot satisfy an admission contract
that exposes bytes at entry unless initial-value correctness is established or
that stronger requirement is independently proved unnecessary for every admitted
observer. Existing generic guest APIs can read an admitted RW object before either
store. No title-specific observer closure is available to remove that obligation.

## Owner, lifetime and finite dependency boundary

The native design already binds source digest, destination, module instance,
allocation generation, reservation/commitment, checked rights, leases and rollback.
It is mechanically VERIFIED in its original synthetic scope. A title-appropriate
native owner must additionally connect this object to the actual module/thread,
initialization sequence, import-visible aliases, persistent consumers and teardown
boundary. Those connections remain incomplete. No full historical owner layout
is required merely to choose native backing, but guest-visible identity and
lifetime relationships must still be preserved.

The finite review stops at the preserved normal prefix and previously unproved
callee boundary. The six symbolic stack stores depend on original r1 and may not
be presumed disjoint from arbitrary image ranges without its separate contract.
No target read is observed in this prefix; no successful summary for later callees,
computed consumers, persistent observers or exception transitions is inferred.
There is no recursively expanded title reachability analysis in this cycle.

The required candidate matrix remains:

| Obligation | Private title finding |
|---|---|
| Source identity / destination / fixed-store widths and endian | VERIFIED finite scope |
| Initial values at live boundary | UNKNOWN |
| Native reservation and commitment mechanism | VERIFIED generic; title instance not admitted |
| Applicable RW authority | CONDITIONALLY PROVEN only under explicit observational premises; title premises unresolved |
| Module owner / thread relationship | PARTIAL design, applicability UNKNOWN |
| Lifetime / alias / observer closure | UNKNOWN |
| Adjacent gap isolation | VERIFIED generic checked path; original gap permissions UNKNOWN |
| Bounded nonexecuting RW data object | DRAFT/BLOCKED |
| Full NativeImagePlan / executable title image | BLOCKED |
| Original entry | NO-GO |

Because mandatory premises are still missing, T48 does not repeat the T40 private
RW attempt or create another mapping. Its strict provenance-only refusal remains
in force. No production C++ defect or newly justified runtime contract was found;
T49 makes no runtime change or speculative provider.

## Focused validation and preservation

```sh
python3 tests/image-write-authority/test_contract.py
python3 tests/writable-startup/test_fixture.py
python3 tests/native-image-binding/test_fixture.py
python3 tests/startup-dependencies/test_static.py
python3 tests/module-provider/test_contract.py
```

Five externally bounded 30-second checks pass: 69 unittest cases plus the unchanged
original fixture-identity script. Four of the cases are new observation examples;
there are no new guest runtime refusals or executed PPC tests in this cycle.
Byte/tuple comparisons and source hashes are not promoted to independent semantic
compatibility facts. Zero unexpected failures/timeouts occur. No sanitizer run is
newly claimed: all runtime headers, prior C++ tests and generated-fixture sources
match the verified T40 fingerprints. Its Debug/Release/ASan+UBSan memory, mixed
RO/RW, gap, NativeImagePlan, owner/generation/lease, header/module, stack/pre-entry,
rollback and integrated execution results are preserved and reused as historical
evidence, not added to new test totals. A full local rebuild was unnecessary.
Hosted CI still runs the existing portable C++ and sanitizer jobs for publication.

Original XEX/A/B, all old checkpoints, SDK locks and hardware-verified NRO/ZIP
artifacts remain unchanged. Detailed addresses, values, raw words and manifests
stay private. An incremental private ledger stores exact source fingerprints,
applicability, missing premises, targeted commands, publication/CI receipts and a
tested source recovery bundle. No redundant proprietary image copy is made.

## T41–T52 decisions and stop

T41 source ledger and T46 finite dependency review are VERIFIED in their exact
static scope. T42 supplies metadata/implementation corroboration, but historical
allocation/section authority is UNKNOWN. T43 live initialization, T44 title owner/
lifetime and T45 unconditional native equivalence remain PARTIAL/UNKNOWN.
T47 preserves gap exclusion; T48 remains BLOCKED. T49 requires no C++ change;
T50 focused checks and T51/T52 admission audit complete the finite cycle.

T52 A live initial values: UNKNOWN; B applicable owner: PARTIAL/UNKNOWN;
C title RW authority: UNKNOWN (native-policy argument conditional only); D title commitment/lifetime: UNKNOWN;
E native gap exclusion: VERIFIED checked-path scope;
F bounded title RW object: BLOCKED; G full image: BLOCKED; H entry: NO-GO.
Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED**.
Native **N-G4 BLOCKED / N-G5 PARTIAL / N-G8 BLOCKED**.
All 13 providers, original r1/r28–r31/LR, complete PCR/TLS/thread state, unknown
callees, six exceptional sites, virtual tail and SizeOfImage remainder remain
separate blockers. No gate is promoted and no T53 is started.

**Single next proof:** establish a finite writer/observer closure for the exact
eight bytes from a clearly defined post-materialization boundary to first
admitted observation. Direct IAT slots are now excluded; provider/TLS/launch
side effects and intermediate observers remain the smallest unresolved part.
The promising independent route is a bounded audit of that initialization path,
corroborated by a qualified pre-entry snapshot if such an interface becomes
available. None is currently available; saved A/B images cannot replace it.
Do not expand into unrelated full-game reverse engineering or repeat exhausted
hardware/emulator loops without new evidence.
