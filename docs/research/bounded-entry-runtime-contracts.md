# Phase 21 — bounded entry runtime contracts

2026-10-24 · baseline `v0.0.20-bounded-entry-cfg`, commit
`33a4c638780ad45272b4f571e7b1b3da27ab49ed`.

**VERIFIED PASS in the stated original-synthetic contract scope.** All required
original gates and the complete three-profile regressions pass. Private
instrumentation/startup readiness remains **BLOCKED/UNKNOWN**, and private game
execution remains **NO-GO**.
No proprietary code is executed, compiled, regenerated or modified.

## Frozen inputs and exact scope

The starting checkout is clean at Phase 20. The Phase [19](game-entry-readiness-audit.md)
and [20](bounded-entry-region-cfg.md) public/private inventories are reused, without
new title discovery or widening the region. The immutable private policy still
contains 19 admitted instructions, one nonexecuted pre-call boundary, two ranges,
four blocks, 19 normal edges, one native helper call/matched return, eight stores,
zero SDK imports and zero normal loops. Its namespace, decoded rows, section
associations and exceptional-transition inventory are independently fingerprinted.
Changing any of these requires a new audit; the synthetic manifest is a separate
namespace, never a substitute identity for the title.

Original XEX SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
Checks preserve 1,455 original XEX/config/generated fingerprints, 299 extracted
files, the installed runtime and nine reused private report/manifest/image
inputs. No title update or installed-SDK modification occurs. About 32 GiB was
available before this phase's temporary work; outputs and useful diagnostics
remain private.

The reused isolated SDK is ReXGlue v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with the unchanged nineteen-patch
[lock](../../tools/sdk/xex-binding-lock.json), SHA-256
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
The workflow reconstructs the patched source from pristine Git and rechecks
22 submodule pins, staged headers, CLI identities and runtime hashes. No new SDK
patch, permissive import or global protection bypass is introduced.

## Independent original module and real pipeline

[The original encoder](../../tests/entry-contracts/fixture.py) reuses only the
existing original XEX container writer and generic PPC encoders. It reads no game
file, instruction table or private manifest. Code ordering, registers, frame/save
offsets, constants and addresses differ from the private candidate. A private
read-only correspondence audit checks all eight width/update/stack-or-image and
signed-displacement classes, while confirming distinct encoded words and PCs.
Only aggregate classifications are published. Literal private stack/data
displacements are not public fixture constants; shape correspondence does not
certify an actual private address or its final launch stack window. Those exact
identities and checks remain required at private instrumentation/admission.

The XEX contains five native functions: a save helper, startup prefix, unadmitted
initializer and two scalar probes. The startup/helper region has 19 permitted
instructions plus one approved pre-call boundary. Two probe functions add four
sites: the total manifest has **24 before-instruction sites and 21 normal edges**.
All eight startup stores are reachable on the original normal scenario. Probes
supply adversarial address shapes without violating the startup stack contract.

The exercised path is:

```text
original PPC/XEX -> production codegen -> unchanged generated C++
-> actual SDK XEX loader -> exact image/code/manifest binding
-> original checked initial context -> generated startup/helper
-> checked stack/image stores -> approved exit or stop/refusal
-> native join -> legitimate final-owner teardown
```

PDATA drives production discovery; no manual function hints or hand-edited
emitted body is used. Forced regeneration and an independent second construction
must agree. All encoded words are independently decoded and reconciled to actual
function mappings, emitted comments, call/link/return and memory-width anchors.
Compiled PCH definitions must select the checked memory and bounded call seams.
A complete output census requires five U32 and five U64 store macros, zero load
macros, zero raw macros and no recognized direct-base/exception helper pattern.
Eight stores belong to startup; two belong to the scalar probes. A lexical census
is not exhaustive C++ alias analysis or a native sandbox.

Trusted host orchestration supplies a root and explicit original context values.
The actual SDK supplies XThread creation, native thread execution, stack/PCR/TLS,
module references and image leases. This is not normal SDK title startup.
No simulated loader or manually imitated generated execution is substituted.

## B02 — eight checked store contracts

| Original correspondence | Contract | Successful oracle | Negative oracle |
| --- | --- | --- | --- |
| C0 | First pre-frame U64 register save | Exact EA and eight big-endian bytes | Its generated store sees read-only backing; no byte changes |
| C1 | Second pre-frame U64 save | Independently seeded register value and EA | Same per-site refusal and unchanged-byte check |
| C2 | Third pre-frame U64 save | Independently seeded register value and EA | Same per-site refusal and unchanged-byte check |
| C3 | Fourth pre-frame U64 save | Independently seeded register value and EA | Same per-site refusal and unchanged-byte check |
| C4 | Pre-frame U32 saved linkage | Low 32 bits of supplied original LR at exact EA | Refusal before commit; destination unchanged |
| C5 | U32 frame/backchain update | Old SP stored at reserved frame base; new r1 checked | Refused update leaves destination and r1 unchanged |
| C6 | First image-backed U32 word | Signed base/displacement EA and known big-endian value | Read-only destination refused without a write |
| C7 | Second image-backed U32 word | Distinct signed displacement and expected value | Earlier stores remain accounted; this store commits no bytes |

The successful normal path requires exactly eight checked writes/completions and
no checked reads. Host verification uses separate owned/leased read copies,
never a raw pointer. Every negative store test requires the expected protection
result, the exact number of prior completed writes and unchanged destination
bytes. This is per-operation failure atomicity, **not rollback of the entire
startup sequence**: prior successful stores remain visible after later refusal.

Generated probes additionally verify successful and read-only-second-page
crossings for U32/U64, a successful unaligned U64 transfer, read-only image data,
uncommitted backing, a freed handle, unbound foreign allocation, 32-bit address
span overflow and escape across the image boundary. Expected protection,
uncommitted, ownership and invalid-range results are asserted separately.
Cross-page failure leaves the entire attempted span unchanged. The SDK validates
all pages before copying. Alignment success characterizes these host checked
copies; Xbox alignment exceptions, hardware atomicity and races are not proven.

The 4-KiB guest / 16-KiB host protection limitation remains. Explicit checked
spans provide the tested per-guest-page refusal. Raw native accesses and arbitrary
SDK-internal pointers are not isolated by this work. XThread's stack heap uses
64-KiB guest pages; tests use its actual granularity rather than assume 4 KiB.

## B03 — PE extent versus actual image ownership

The original normal fixture declares PE SizeOfImage larger than its actual XEX
security/loaded extent. Production generation and the actual SDK load it, while
the binder and image lease retain the smaller ownership. A larger PE declaration
does **not** create an allocation, extend guest ownership or authorize fetch.
A second metadata-only case increases the declaration further with the same
result. The normal original module executes only within validated loaded ranges.

One isolated child exercises seven metadata scenarios through the real loader. Three
out-of-owner/nonexistent section layouts reject at load. Four load successfully:
extra declared PE extent, overlapping data/code, moved code section and a section
ending shortly before the owner boundary. Only the otherwise unchanged larger-PE
case passes the strict original binder. The other three fail admission before
PPC execution. Ambiguous overlapping section reads and reads in the shortened
section's missing tail refuse. A span ending inside the shortened section reads
successfully. No negative metadata case executes PPC.

The result distinguishes loader acceptance from execution admission: current SDK
PE loading does not itself reject every overlap. The original fixed-layout binder
requires exact section names, ranges and flags, compiled mapping/code identity,
loaded extent and code bytes. Five independent altered code/entry/generated-size/
code-range descriptions reject. An actual loaded allocation retagged with a
foreign owner refuses image copy; its legitimate owner is restored before
teardown. The image boundary probe refuses an out-of-owner generated store.
These are bounded fixture policies, not a general arbitrary-title loader proof.

An active binding/thread lease makes unload return its existing SDK failure;
closing refuses new leases. Backing stays live through stop, native join and final
reference release. Repeated load/execute/unload restores image and virtual heap
page counts after every iteration. No metadata-driven ownership enlargement or
forced free of live resources is used. Relocation, all original image sections,
raw/native fetch and real-title binding remain unverified.

## B04 — initial state, stack and helper ABI

[The small original startup validator](../../runtime/entry_contract.h) requires
the actual current XThread/context, matching PCR/r13, a supplied aligned r1 with
the full conservative save/frame window inside the owned writable committed
stack, trusted supplied LR/r2/input/save-register values and exact owner identity.
It reads the window and checks page metadata without writing it. Tests reject
out-of-bounds, unaligned or mismatched SP, wrong LR/r2/r13/input, uncommitted stack
and foreign stack ownership **before generated admission**. The uncommitted page
is restored using the actual stack heap granularity before ordinary SDK teardown.

Generated execution performs the direct helper call, stores seeded nonvolatile
values and linkage, validates the matching LR return, reserves the frame and
writes the two image words. The approved exit is before the next initializer;
root return is intentionally not admitted for this prefix. Probe functions
exercise supported root return separately. A corrupted helper return address
refuses before returning to an unchecked continuation. Supplied r2/r13 remain
unchanged. Comparing all other context bytes after normalizing only the known
written registers verifies untouched state, including seeded FPR/vector state.
This is preservation evidence, not typed-vector marshalling support.

A host checkpoint is restored solely to finish the synthetic host-owned thread
cleanly. It never resumes the guest region or implements guest unwind. Native
join, stack release, final thread destructor, TLS/PCR release and final module
unload are required. Three successive original lifetimes exercise repeated load
and cleanup; historical regressions retain simultaneous-thread tests.

**Layer A — VERIFIED only under explicit original values:** mechanics and
failure contracts above. **Layer B — UNKNOWN:** actual Xbox title initial values,
loader/runtime initialization provenance, process state and other title-specific
ABI assumptions. No invented synthetic value is promoted to a known title input.
Full Xbox ABI, typed-vector signatures and arbitrary callback ABI stay unverified
or explicitly rejected.

## B06/B07 — hook agreement and exceptional refusal

The Phase 20 independent manifest builder and production before-instruction
hooks are reused unchanged. Canonical input/policy hashes define a separate
immutable namespace. Trusted host checks image and canonical manifest hashes
before loading; changed image or stale namespace bytes refuse. Supplied tables
and actual compiled mappings must agree. No untrusted runtime JSON policy or
silent native/import fallback is introduced.

The matrix requires **24/24 static/emitted/runtime-observed stop sites and
21/21 declared normal edges**. Startup/helper contributes 20 sites/19 edges;
probes contribute four sites/two edges. Checks include root entry, helper
call/return, all eight memory sites, stack setup, approved exit, incorrect entry,
unknown target, wrong return, denied continuation and an externally requested
stop at a generated memory boundary. This region has no loop; Phase 20/18 loop
regressions remain mandatory. All sites are stop points, so removing them leaves
no uncovered cycle; this does not bound time inside native SDK code.

Exceptional tests explicitly request an unadmitted exceptional target or an
invalid continuation identity at a generated control boundary. Both fail through
the same deny-by-default host gate before the helper/store or unverified handler
can run. Memory and invalid call/return failures remain observable host outcomes.
One stop occurs while thread/image resources are live and unload is pending;
ordinary owned cleanup must still complete. Expected fatal historical fail-stops
are recorded separately and have no graceful-cleanup claim.

**PARTIAL for actual guest exception integration:** these fixtures do not raise a
real Xbox exception or drive its dispatcher/handler tables. The private region's
known intersecting metadata is still immutable and non-admitted. An actual
private failure must reach a verified host abort before any guest handler or
unwind. Full guest dispatch/unwind is unsupported, not simulated as success.
The guest page table has no execute bit; native compiled identity/target admission
does not authorize dynamic guest instruction fetching.

## Required evidence matrix and four separate layers

| Blocker | Required contract and synthetic/negative proof | Relevant implementation path | Remaining private gap / blocks first experiment? |
| --- | --- | --- | --- |
| B02 | VERIFIED synthetic: C0–C7 values, EA/endian, eight individual protection refusals; cross-span/owner/lifecycle failures | Generated checked macros → Scope::copy → CopyOwnedMemory / leased CopyImageMemory | BLOCKED: eight actual private stores remain uninstrumented; yes |
| B03 | VERIFIED scoped synthetic: loaded bound wins, section/code mismatch refuses, active unload/owner tests | XexModule::AllocateImage/ReadPEHeaders/CopyImageMemory, UserModule::ImageLease, XexBinding | BLOCKED: exact private image/code/section identity and live leases not integrated; yes |
| B04 | VERIFIED original mechanics; UNKNOWN title startup inputs | XThread::CreateWithModule/Execute/Exit, current PPCContext, validate_entry_state, bounded shadow returns | BLOCKED for private admission: consumed values/provenance and ownership need independent evidence; yes |
| B06 | VERIFIED original 24-site/21-edge integration with hash/entry/stop negatives | Production midasm_hook, bounded manifest, BoundedCfg, GeneratedStop | BLOCKED: zero newly generated or executed private hooks; yes |
| B07 | VERIFIED host refusal and orderly cleanup in stated fixtures; PARTIAL real exception integration | BoundedCfg control gate, checked Fault, owned host stop/checkpoint, native join | BLOCKED: enforce actual private exceptional denial before dispatch/unwind; yes |

The four layers are deliberately separate: **(1)** original synthetic contract
correctness; **(2)** private generated-code instrumentation readiness, still
BLOCKED; **(3)** actual game startup-state knowledge, still UNKNOWN; **(4)** game
execution admission, **NO-GO**. B01's frozen normal graph is unchanged. No normal
SDK import is required; all 24 previous import denials and unsupported signatures
remain. No broader emulation, gameplay, graphics/audio or Horizon work is added.

## Regression measurements and retained diagnostics

**VERIFIED: 752 passing core groups/checks, zero final failures, unexpected native
signals, timeouts or emitted sanitizer findings.** The unchanged Phase 5–20 core
contributes 629; the new 41 groups per profile contribute 123. Negative refusal
groups are acceptance tests, not successful guest invocations.

| Profile | Ordinary | Fault/capture/recovery | Total | Core child seconds | Peak individual core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 228 | 23 | 251 | 145.177258 | 376,242,176 |
| Release | 228 | 23 | 251 | 126.570385 | 412,254,208 |
| Sanitized | 228 | 22 | 250 | 169.497661 | 495,091,712 |

| New suite only | Generate + regenerate seconds | Compile seconds | Runtime child seconds | Runtime peak RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 0.252750 | 6.739976 | 3.906713 | 29,900,800 |
| Release | 0.114803 | 7.348851 | 1.739478 | 23,691,264 |
| Sanitized | 0.887840 | 9.141563 | 4.264158 | 69,173,248 |

New per profile: **842 main assertions, 310 worker observations; four approved
startup exits, three natural probe returns, five control refusals, one owned
generated stop, nine pre-entry context refusals and sixteen checked-memory
failures**. The memory failures are eleven protection, one uncommitted, three
ownership and one invalid-range refusal. Image/namespace mismatches, seven
metadata scenarios, five altered binding descriptions and the actual allocation
failure are separate host validation subcases; none is counted as guest success.

Each C0–C7 has four successful byte/value observations (one ordinary plus three
repeat lifetimes) and one individually injected protection refusal, per profile.
There are 185 policy visits, 182 actual stop polls and full 24-site/21-edge
coverage. Three observer-injected transfer refusals occur before polling; they
do not reduce the distinct-site coverage from the complete matrix. The checked
layer observes 90 writes, 74 completed and sixteen refused; host verifier reads
are outside these generated-access counters.

All 38 XThread/stack/TLS/PCR lifetimes and 38 thread-backed image unloads reclaim
legitimately. The facade records 76 allocations and 76 releases. Actual SDK image
lease counters are not exposed as a new API: pending unload refuses, new leases
close, and final unload succeeds only after all legitimate references release.
Existing pointer capability/lease/scratch counters must balance. Five additional
host-only successful metadata/binding image loads and failed-load/allocator
rollback cases also restore image/virtual page counts. These figures are scoped
observations, not a complete SDK-allocation census or a concurrency proof.

Historical expected outcomes remain 44 SIGBUS, three SIGSEGV and nine SIGABRT
exits, twelve capture/recovery results and nine separate expected join fail-stop
exits 70. No orderly cleanup is claimed for those fatal exits. The old plain
default-signal case remains excluded only under ASan because ASan owns that
signal path; this unchanged documented exclusion explains the one-count profile
difference. Watchdog self-tests deliberately induce timeouts as separate tests
of containment, not unexpected suite timeouts.

Separately pass 27 component/source-control groups, 79 Python units (ten new),
33-import source/signature audits, three VMX characterizations of 212,068
invocations each and the Debug reference trace `[2,2,2,2,1]` followed by one
destructor. Known VMX special-value/hardware limitations remain unchanged.

Results come from one complete final source-hash-verified workflow. Generated
source/header hashes, canonical namespace and manifest hashes agree across all
three profiles and repeat construction; native binary bit reproducibility is
not claimed. Host: macOS ARM64 / Apple Clang 17; clients O0/O2/O2+ASan+UBSan,
unchanged Sanitized SDK O1/no recovery. System libraries/assembly are not fully
instrumented. ASan/UBSan do not establish race freedom. Durations sum supervised
children rather than calendar project time; RSS is the maximum individual wait4
child usage, not process-tree/game/Switch memory.

One development Debug group failed because the test tried to recommit an
unaligned 4-KiB slice of the actual 64-KiB XThread heap. The native child returned
an explicit failure, with no unexpected signal or timeout. Its diagnostics are
retained privately. The test now decommits/recommits its own actual heap page;
no SDK change or global permission relaxation was needed. Subsequent Debug
validation passed all 41 groups. Additional result-specific refusal and untouched
register oracles were added before the final full workflow.

## Reproduction and publication

```sh
python3 tests/entry-contracts/test_fixture.py
python3 tests/entry-contracts/run.py --sdk "$SDK_DEBUG" --rexglue "$CLI_DEBUG" --workdir "$ENTRY_DEBUG"
python3 tests/entry-contracts/run.py --sdk "$SDK_RELEASE" --rexglue "$CLI_RELEASE" --workdir "$ENTRY_RELEASE"
python3 tests/entry-contracts/run.py --sdk "$SDK_SANITIZED" --rexglue "$CLI_SANITIZED" --workdir "$ENTRY_SANITIZED" --sanitize
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$ALL_TESTS"
python3 tools/check_public_tree.py
```

Variables denote matching staged SDK/CLI and new private output directories.
Every generator/compiler/runtime child has a 30-second external monotonic
process-group watchdog. The encompassing workflow caps each suite at 300 seconds;
thread join/observer gates are independently limited to 15/10 seconds. Original
allocator injection is a child-only existing macOS interposer, armed after
runtime setup; a real SDK allocation failure must propagate, never become success.
The public CI adds ten portable encoding/binding units and retains previous
public/components checks. It does not rerun the local SDK-linked Mac matrix.

Only original source/tests and sanitized aggregate documentation are intended
for publication. Exact private addresses/operands, preserved game output, detailed
correspondence, logs and native/generated artifacts remain outside Git. Review
uses an explicit file allowlist, staged text/path/link checks and eight patch-lock
checks. The reviewed snapshot contains 15 intended changed files and 203 allowed
text files, with no selected private address hits. These checks are not universal
copyright or secret detection. No Release.

## Phase 22 and execution decisions

**Synthetic contract reuse: GO in the verified scope. Phase 22: CONDITIONAL GO**
for instrumentation and independent audit only. Phase 22
must be a separately bounded instrumentation-only task and final admission audit:

1. Preserve this frozen private region/namespace and the historical XEX/config/
   generated files. Define a new derived instrumentation configuration/output;
   do not overwrite or silently reuse the old identity.
2. Reproducibly generate only under an explicit finite analysis/generation budget
   into a new private directory, with the pinned CLI/SDK and complete checked
   scalar/call/stop seams. No game compilation/execution follows automatically.
3. Independently reconcile all eight actual store EAs/widths, 20 hook identities,
   19 normal edges, exact helper/link/return and pre-initializer boundary; inspect
   the emitted/PCH memory paths and reject every missing/raw/unsupported transfer.
4. Bind exact input/configuration/code/manifest hashes to the actual loaded-owner
   bound and validated code/data sections. A larger PE declaration grants no
   ownership. Require current module/thread leases, PCR and exact stack window.
5. Obtain independent evidence for all consumed initial title inputs and startup
   provenance. Synthetic supplied values are insufficient; UNKNOWN stays a gate.
6. Audit every potential exceptional/error continuation and enforce observable
   host abort before guest dispatch/unwind. No guessed handler or compatibility
   continuation may be added.
7. Repeat independent negative/stop/ownership/reclamation checks and review
   complete actual private mediation, initialization, immutable identity and
   external containment before proposing any separately authorized experiment.

**Private game-code execution: NO-GO.** No current result authorizes proprietary
entry. An experiment would require all actual instrumentation, bindings, initial
state/provenance, exceptional refusal and bounded-stop evidence, an independent
admission review and a separate explicit scope. Synthetic success proves neither
game boot nor Nintendo Switch compatibility.
