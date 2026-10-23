# Phase 20 — bounded entry region and verified CFG manifest

2026-10-24 · baseline `v0.0.19-entry-readiness-audit`, commit
`a81dda589593db2daff24d0ef385a26d5348bdef`.

**VERIFIED PASS within static and original-synthetic scope.** A meaningful normal-flow
entry region is frozen and independently reconciled. B01 is closed **for this
region only**; B06 has a verified static placement foundation and separate original
synthetic proof. **CONDITIONAL GO for Phase 21 synthetic contracts. NO-GO for
private game-code execution.** No proprietary code is executed, compiled or
regenerated. This is not a whole-title CFG engine, title boot or Switch result.

## Preserved baseline and inputs

The initial checkout is clean at the Phase 19 baseline. The
[Phase 19 public audit](game-entry-readiness-audit.md), its existing private report,
entry inventory and preserved mappings are reused. Original XEX SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
Preservation checks cover 1,455 XEX/configuration/generated-file SHA-256, size and
mtime fingerprints, 299 extracted-file hashes and the installed runtime. The
reused historical loaded image and Phase 19 diagnostic inputs are separately
fingerprinted. No title update, private C++ rewrite, original-file overwrite or
new title-analysis run occurs. About 36 GiB was available before temporary work.

The unchanged isolated SDK is ReXGlue v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` with the existing nineteen-patch
[xex-binding lock](../../tools/sdk/xex-binding-lock.json), SHA-256
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
The workflow reconstructs patched source, checks 22 dependency pins and matching
staged headers/runtime hashes. No SDK patch or global installation is modified.

## Meaningful region and exact confidence boundary

The candidate completes a small startup stage: a native save-helper call,
register/linkage preservation, stack-frame preparation and initialization of two
writable image words. It stops **before** the next unverified initializer call.
This captures real startup effects rather than selecting an arbitrary first NOP.

| Private candidate fact | Verified static result |
| --- | ---: |
| Selected function/range identities | 2: partial entry owner + complete called helper suffix |
| Permitted instructions | 19 |
| Separately decoded pre-call exit word, never executed | 1 |
| Policy sites / basic blocks, including terminal boundary | 20 / 4 |
| Normal instruction-level edges | 19 |
| Admitted direct calls / matched returns | 1 / 1 |
| Conditional branches / loop backedges | 0 / 0 |
| Unresolved normal transfers / admitted SDK imports | 0 / 0 |
| Memory loads / stores | 0 / 8 |
| Four-byte / eight-byte stores | 4 / 4 |
| Fixed image data words / writable image sections referenced | 2 / 1 |
| Other shared helper-suffix entry aliases, not admitted | 3 |

Entry identity, owner and block extents, real PCs, raw words, exact data/stack
ranges, calls, return continuation and source/metadata fingerprints stay in a
private canonical manifest and dependency inventory. Only aggregate findings
are published. The root is the actual known title entry, not an arbitrary leaf.
The called helper has shared suffix aliases; they are neither additional roots
nor authorized targets. The selected ranges do not overlap. Normal execution
has a finite 19-instruction path to the pre-call boundary; failure can abort
sooner. This is not a host-time bound or a claim that memory accesses are safe.

**VERIFIED additional dependencies:** the entry owner has exception metadata,
including one scope conservatively intersecting the selected prefix. Exceptional
transitions are inventoried privately and explicitly **not admitted**, not
inferred unreachable. PE SizeOfImage exceeds the XEX security/loaded-image bound.
Selected code and referenced data are inside both, but whole-image equality,
relocation and title ownership cannot be assumed. B03/B07 remain execution gates. This reconfirms the historical layout caveat:
the analysis view excludes relocation data beyond the loaded image. A bounded
binder must use actual loaded ownership and validated section ranges.

## Two independently reconciled views

**A:** current original XEX header/hash and existing historical tool-mode loaded
image, its PE/PDATA metadata, and a separately authored PPC bitfield decoder.
The historical loader source records how the image was obtained without running
entry code. The dump is hashed and reused; this is not fresh decryption or a
new title load. Its provenance does not certify every relocation/import byte.

**B:** the existing preserved ReXGlue-generated instruction comments, actual
callee symbols, function mappings, explicit branch/link/return anchors and
memory-width macros. Entry mapping agrees with XEX and PE entry metadata. The
entry owner extent agrees with PDATA and the full emitted instruction count.
The helper's encoded BL target, mapping, decoded store sequence and terminating
return agree; the shared suffix is recorded explicitly rather than given a
fabricated independent PDATA record.

Twenty real addressed words, including the unexecuted boundary word, match the
second view. PCs come from image coordinates and independently validated ranges.
The decoder's addressed sequence is compared with emitted syntax and explicit
anchors; **PCs are never invented by adding four to C++ diagnostic ordinals**.
Disjoint/missing/shifted instruction streams, wrong links, callees, labels,
branches, widths or extents reject. This proves the stated normal CFG facts,
not all emitted C++ semantics, Xbox ABI or hardware instruction conformance.
No exception continuation is admitted by this normal graph.

The original [bounded decoder/manifest tool](../../tools/bounded_cfg.py) supports
only the reviewed scalar/control forms needed here and by the original fixtures.
Unknown instructions, computed tail transfers and unsupported conditional forms
fail closed. It accepts explicit bounded ranges, at most 32 functions and 256
sites; it performs no general discovery or speculative whole-game CFG recovery.

## Canonical manifest and site identities

The deterministic document binds XEX/image identity, exact code/view hashes,
CLI revision, SDK patch-lock hash, codegen configuration and function/range/PC
identity. The private manifest additionally binds the preserved generated view,
validator source, actual pinned CLI and exceptional-transition inventory. It
contains owner/range and basic-block extents, roots, exact edges, known import
identities, before-instruction sites, compatible event sets, explicit exits and
refusals, and default denials for callbacks, exceptions, computed tails, unknown
targets and unverified continuations.

Site roles include function/block entry, branch, join, backedge, call/return,
memory, long path, pre/post-import, stop, exit and refusal. Compatible events at
the same owner/PC/phase coalesce. Incompatible duplicate ownership/phase/action
or targets reject. Sorted records receive dense indices. A policy SHA-256 and
namespace SHA-256 bind the complete canonical policy and its input bindings;
indices have meaning **only within that immutable namespace**.

Production configuration has one hook per PC. Post-import observation uses the
verified next instruction's before-hook, after the synchronous call returns;
independent duplicate before/after events are not silently written to the same
map key. Changed code/configuration/revision/policy requires a new namespace and
validation. Shared aliases are not silently interchangeable entry identities.

Validation compares a supplied document with independently rebuilt authority,
not just its self-reported hash. Rehashing an omitted-site/forged-edge document
still rejects. Before synthetic SDK loading, the host checks actual image and
canonical manifest hashes against compiled expected values. Compiled immutable
site tables also validate index/PC, owners, targets, continuations and graph
completeness. There is no untrusted JSON runtime interpreter or performance
optimization replacing the existing stop lookup.

## Deny-by-default synthetic control policy

[The opt-in original policy](../../runtime/bounded_cfg.h) admits only explicit
roots and exact context/base, r1 bounds/alignment and r2/r13 preservation. Every
manifested instruction has a before-hook. Conditional edges use the reviewed CR
condition; native direct/indirect calls require the exact site, target and actual
compiled mapping. Calls push bounded shadow continuations. Returns must match
the actual current caller, owning function and LR; a union of possible static
return targets is insufficient. Root return must match the trusted initial LR.

The exact original native target and the exact import path are separate explicit
choices. There is **no import-to-native fallback**, resolver fallback, raw guest
pointer or unlisted callback authorization. The existing general direct-call API keeps its native-call contract. A new
strict import-only API is used for classified imports; it refuses null and
native-only symbols before invocation. Existing import admission is unchanged. New fixture call macros select this scope only through an explicit
checked-header opt-in. Unsupported typed signatures remain denied.

Approved pre-call exit, unknown target, boundary escape, wrong return and a
requested exceptional callback have distinct controlled host outcomes. Expected
refusals are not successful guest execution. The exceptional callback test
requests an unadmitted original target through the control gate before a store;
it executes neither the target nor that store. It does **not** implement/test
Xbox exception dispatch or wire the real title's handler into the SDK. Future
integration must enforce the declared denial before exceptional transfer; B07
is still open. Host checkpoint restoration supports cleanup only, never guest
resume/unwind or fabricated Xbox success.

## Independently authored production XEX analogue

The public original [encoder](../../tests/bounded-cfg/fixture.py) uses only the
pre-existing original XEX container writer and generic PPC encoders. It reads
no game file or private instruction data. Its code/registers/values differ from
the private candidate. The full image contains ten functions / 82 encoded words;
nine selected ranges manifest 74 before-instruction sites and 19 blocks. Some
sites stop/refuse before their underlying instruction executes.

PDATA drives real production `codegen`; there are no manual function hints.
Emitted C++ is forced-regenerated, hashed, independently reconciled and compiled
unchanged. A second independent construction must preserve the image, manifest
and configuration. All profile/repeat emitted hashes and the canonical namespace
agree. Native binary bit reproducibility is not claimed.

The analogue adds forward branches, both sides of conditional joins, repeated
paths, direct calls/returns, a known native indirect callee, denied unknown and
callback targets, a pre-initializer exit, denied range escape, a long scalar path,
an infinite loop and a real admitted critical-section import. Actual SDK XEX
loading, code/section/TLS/import binding, native XThread, stack/PCR/TLS, image
leases, pointer adapter and SDK implementation are used. The native harness
selects explicit roots; this is not normal SDK title launch.

Fourteen admitted scalar guest-memory sites (eight U32 stores, three U64 stores,
two U32 loads and one U64 load) reconcile with checked PCH macros. A separate
whole-output lexical census finds 15 scalar sites, including one unadmitted
suffix load, and no raw/base-pointer/exception-helper bypass pattern. This is
not exhaustive C++ alias analysis or a native sandbox. All transfer attempts
use the existing checked layer; a deliberate invalid address is refused.

The guest page table still has no execute bit. Native compiled admission depends
on exact image/code identity and target policy; it supplies no dynamic guest
instruction-fetch authorization and does not equate readable pages with code.

## Stop-boundary proof and containment

**VERIFIED for the original analogue:** all **74/74 static sites are emitted
and runtime-observed as stop polls**; all **72/72 declared normal edges** are
observed, with zero unexpected edges. Both direct loop backedges are independently
decoded, hooked and observed. Removing all stop vertices leaves an acyclic graph;
missing required entry/backedge/long-path/import/exit/refusal coverage rejects.

Every admitted instruction has a point, covering entries, conditional joins,
straight paths and controlled exits. Pre/post-import points are checked separately:
a request at the post-import boundary occurs after actual synchronous SDK return,
commit and scratch cleanup. No interruption inside SDK instructions is claimed.
The original infinite loop stops from inside generated execution, without
returning naturally to a host driver.

Per profile: 273 policy-site visits and 271 actual stop polls. Two deliberately
injected callback refusals happen in the trusted observer **before** the poll;
all 74 distinct stop sites are nevertheless observed by the complete matrix.
Counts are observations, not unique instruction execution or a timing guarantee.

External monotonic/process-group watchdogs cap every potentially faulting child
at 30 seconds, with a 300-second suite ceiling and shorter 15-second native join /
10-second observer limits. The nineteen-patch SDK's owned exit and legitimate
references preserve backing until join/final release. Active unload refuses and
closes new admission while leases remain. No global protection bypass, forced
live-thread deletion, unsafe continuation or guest unwind is introduced. Existing
failed-join exit-70 containment regressions remain mandatory.

**Private scope limitation:** its 20 planned sites have independently verified
identities and no normal cycle, but **zero new private hooks are emitted or
executed**. Synthetic coverage must not be generalized to private instrumentation.
The 4-KiB guest / 16-KiB host limitation remains: explicit checked spans are
mandatory, and raw memory is not isolated by this work.

## Region dependencies and Phase 19 blocker ledger

The complete address/register/stack/section/exception inventory stays private.
Exclusions below derive from every decoded normal instruction and explicit
pre-transfer boundary, not one lexical path. Exceptional paths are denied by
policy, not proven impossible.

| ID / dependency | Status after Phase 20 | Minimum remaining evidence |
| --- | --- | --- |
| B01 region/control | VERIFIED closed for the frozen 19-instruction normal-flow region | Apply/validate its actual execution admission in a later authorized integration; the wider 108-function closure stays incomplete |
| B06 stop/site foundation | VERIFIED static identities + complete original synthetic proof; PARTIAL title integration | Generate and independently verify actual private hooks later; no current game stop claim |
| B02 memory | BLOCKED for execution: eight private stores still unmediated | Validate exact pre-frame/stack and image-word extents, all pages/owners/permissions/endian/failure paths, then reproducible private instrumentation |
| B03 image/code binding | BLOCKED for execution: historical view and PE/XEX extent discrepancy | Original mismatched-extent fixtures; conservative actual loaded-owner bound, exact code/range identity and leases; never enlarge ownership from PE alone |
| B04 initial ABI/thread | PARTIAL: consumed state identified; launch values UNKNOWN | Original twin for the exact register/linkage/stack contracts and live XThread/PCR/image ownership; preservation is not initial-value proof |
| B05 SDK imports | EXCLUDED WITH EVIDENCE from permitted normal instructions | Preserve pre-initializer exit; zero required SDK imports inside the region. All 24 existing denials remain; full title requirements are not excluded |
| B07 guest exceptions | BLOCKED / explicitly non-admitted; one intersecting owner scope | Validate observable host abort and owner cleanup before any exceptional guest continuation; do not implement guessed guest unwind |
| B08 unsupported operations/ABI | EXCLUDED WITH EVIDENCE for explicit atomic/vector/trap operations; no dynamic dispatch/fetch path is admitted | Dynamic stack address/MMIO safety still needs checked refusal. Full ISA/typed-vector ABI remains unsupported/unverified |
| B09 dynamic loading/callback/TLS policy | EXCLUDED WITH EVIDENCE as normal instruction dependencies; host startup PARTIAL | No admitted DLL/callback/TLS API path; still validate quiescent thread creation and context/lifetime admission. General process/DLL TLS policy is deferred |

The region has no TOC/TLS/PCR instruction dereference, but the selected host
execution architecture still needs correct bound thread/PCR and module lifetime.
The internal helper uses the context/base call contract and exact link/return;
no new imported ABI signature is inferred. Full Xbox ABI and guest unwind remain
unverified. Graphics, audio, streaming and Horizon are later gates.

## Regression evidence

**VERIFIED selected final matrix: 629 passing core groups/checks, zero final
failures, unexpected native signals, timeouts or emitted sanitizer findings.**
The unchanged Phase 5–19 core contributes 578; the new suite contributes 17
per profile. No assertion or prior acceptance criterion is weakened.

| Profile | Ordinary groups | Fault/capture/recovery | Total | All core child seconds | Peak individual core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 187 | 23 | 210 | 140.664820 | 369,377,280 |
| Release | 187 | 23 | 210 | 128.387812 | 392,036,352 |
| ASan/UBSan | 187 | 22 | 209 | 161.888147 | 433,324,032 |

New per-profile outcomes are kept separate: **four natural returns, one approved
region exit, five refused transfers, four owned generated stops, one checked
memory failure, six invalid-table refusals, two strict import-class refusals and one manifest-hash refusal**.
The policy/hash groups contain validation subcases, not guest invocations. Seventeen passing test
groups do not mean seventeen successful guest executions.

Each new profile records 371 main assertions / 78 worker observations; 38 checked
memory attempts, 37 completed and one deliberate refusal; two real SDK import
returns and two reclaimed scratch allocations. Sixteen actual XThread/TLS/PCR
lifetimes and sixteen image unloads finish with legitimate destructor/page-count
oracles. The hash-only group never loads a runtime. No orderly cleanup is claimed
for the separate old expected join fail-stops.

| New suite only | Generate + regenerate seconds | Compile seconds | Runtime child seconds | Runtime peak RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 0.279357 | 7.021730 | 1.675995 | 29,409,280 |
| Release | 0.087350 | 7.917084 | 0.651390 | 23,166,976 |
| ASan/UBSan | 0.364661 | 9.688422 | 1.770646 | 65,323,008 |

Durations sum independently supervised children, not calendar project time. RSS
is maximum wait4 usage of an individual child, not process-tree/game/Switch memory.
Clients use O0/O2/O2+ASan+UBSan; the unchanged Sanitized SDK uses O1/no recovery.
System frameworks/CommonCrypto/assembly are not fully instrumented. ASan/UBSan
are not race detectors or complete resource/isolation proofs. New policy scopes
are single-invocation fixtures; exhaustive reentrancy/concurrency is not claimed.

Existing expected fault tests retain 44 SIGBUS, three SIGSEGV and nine SIGABRT
exits, plus twelve capture/recovery results and nine separate expected join
fail-stop exits 70. The historical ASan-owned plain default-signal case remains
excluded only in Sanitized. Separately pass 27 component/source-control groups,
69 Python units (22 new), three VMX characterizations of 212,068 invocations each,
33-import source/signature audits and the Debug reference trace
`[2,2,2,2,1]` followed by one destructor. Hardware VMX equivalence remains UNKNOWN.

**Retained development diagnostics:** the first original fixture exposed one
unreachable word omitted by production discovery; the range/count check correctly
rejected it. The authored escape fixture was changed to a contiguous boundary
shape, with checks retained. A second census incorrectly treated extern hook
declarations as calls; the call matcher was anchored and a negative/extern unit
added. A later diagnostic aggregator indexed the intentionally empty policy-only
stop vector; all runtime children had passed, but aggregation failed. It now
requires the full vector for every executed case and explicitly permits empty
vectors only for the two nonexecution validation cases. No failure is counted as
a final pass. An initial private PE/XEX size-equality assumption also rejected;
that real metadata discrepancy is now an explicit B03 blocker, not suppressed.

Publication review also found that the new facade delegated classified imports
through a general helper that permits native calls. It now uses a strict
import-only API, with explicit null refusal and a negative case for a native
symbol already known to the lower checked scope. This closes a classification
fallback without removing the old general native-call contract. No failed final
runtime group is relabeled as successful.

After the policy, observations and strict import gate were finalized, the entire
Phase 5–20 workflow was rerun against all three profiles. The selected final
results come from this one source-hash-verified complete workflow; all previous
attempts and useful diagnostics remain private.

## Reproduction and publication boundary

```sh
# Variables name new private output directories and matching staged SDK/CLI.
python3 tests/bounded-cfg/test_manifest.py
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$ALL_TESTS"
python3 tests/bounded-cfg/run.py --sdk "$SDK_DEBUG" --rexglue "$CLI_DEBUG" --workdir "$CFG_DEBUG"
python3 tests/bounded-cfg/run.py --sdk "$SDK_RELEASE" --rexglue "$CLI_RELEASE" --workdir "$CFG_RELEASE"
python3 tests/bounded-cfg/run.py --sdk "$SDK_SANITIZED" --rexglue "$CLI_SANITIZED" --workdir "$CFG_SANITIZED" --sanitize
python3 tools/check_public_tree.py
```

The existing [workflow](../../tests/xex-binding/workflow.py) now includes this
suite and its 22 independent manifest/decoder/hook units. Tests cover ordering,
repeat construction, hash/namespace drift and rehashed forgery, compatible versus
incompatible duplicates, missing sites, bad edges/unknown targets/exits, index
mismatch, unsupported events/instructions, shared/invalid ranges, branch escape,
uncovered cycles, misplaced/missing hooks, extern declarations and link/callee/
label mismatches. [The fixture recipe](../../tests/bounded-cfg/README.md) documents
budgets and scoped behavior. No command takes a proprietary executable to codegen.

Detailed private reproduction uses the preserved loaded image, XEX/header/PDATA,
Phase 19 inventory and selected original ReXGlue body/mapping files through the
read-only bounded tool. The private selector, exact manifest/dependency inventory,
exception facts, hashes and useful logs remain outside Git. The updated architecture page also removes
a historical exact-entry coordinate; its original text is retained privately and
Git history is preserved. Public Phase 20 content is
limited to original source/tests and sanitized English aggregates. Review checks
all changed files, explicit allowlist, text/path/link and eight patch-lock rules;
these are not universal copyright/secret detection. CI checks portable public
units/components, not the local SDK-linked Mac matrix. No GitHub Release is made.

## Phase 21 and game-execution decisions

**A — CONDITIONAL GO for Phase 21 synthetic runtime-contract validation.** Build
an original twin for the exact required save/frame/store widths and lifetimes,
initial consumed registers, root/link/return, loaded-owner versus PE section
bounds, exception-refusal/host-checkpoint cleanup, and pre-initializer exit.
Require complete checked mediation, code/manifest binding, stop/negative cases
and final reclamation under all three profiles. No need is established for all
33 imports, general DLL TLS indexing, graphics or full guest unwind in this
bounded region; do not expand those speculatively.

**B — NO-GO for private game-code execution.** B02/B03/B04 and actual B06/B07
integration remain open. The frozen manifest narrows dependencies but does not
instrument old private output, provide verified launch state or enforce its
policy on the game. A later separately bounded generation/integration task and
independent readiness review are required. No private execution is authorized
by this milestone; synthetic success proves neither boot nor game/target
compatibility.
