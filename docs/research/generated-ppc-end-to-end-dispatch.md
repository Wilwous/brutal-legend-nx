# Phase 15 — generated PPC caller and end-to-end dispatch

Assessment: **2026-10-23**, baseline `v0.0.14-pointer-import-dispatch`
(`963a61362baca74758dfc19f76e7598e9366e1c9`). **VERIFIED** for the exact
original generated execution path below. Automatic XEX/codegen binding and full
Xbox conformance remain **PARTIAL**. Broader synthetic work: **CONDITIONAL GO**.
Private game-code execution: **NO-GO**. No proprietary instructions execute.

## Preservation and pinned inputs

The original `default.xex` SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
All 1,455 original executable/configuration/generated-output fingerprints retain
size, modification time and SHA-256; all 299 extracted-file digests match. The
installed SDK runtime is unchanged. No title update, analyzer rerun, private
regeneration, full game compilation or game execution occurs.

ReXGlue v0.10.0 is pinned to
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` with the unchanged eighteen-patch
[tls-import lock](../../tools/sdk/tls-import-lock.json). Read-only reconstruction
from pristine source plus the ordered patches matches all 31 modified SDK source
files; all 22 submodule pins, staged headers and three runtime hashes match.
The three CLI hashes match the preceding milestone's binaries. No SDK patch,
lock, globally installed library or private game-generated file is changed.

## Original program and actual generated execution

[Original encoder](../../tests/generated-dispatch/fixture.py): 236 bytes, eight
functions and 59 instructions. It writes raw PPC words, relative nm symbols,
assembly and original address metadata. Actual `recompile-tests` decodes/emits
those words; the emitted translation unit is compiled unchanged as a separate
source file against its matching patched SDK. No handwritten C++ substitutes for
the PPC entry or its scalar callee.

- Program SHA-256: `413aec6948ab77e4b4c03c507e211ec9930d8e2d852e9bc9e46ff1a9f86c09d8`.
- Generated translation-unit SHA-256, identical in all three profiles:
  `b0cb27072f3681e1927412ac758ac28d54f24aca75495d36837ea5ad94061e82`.
- Analysis entry: `caller_82010000`; generated scalar callee: `caller_820100B8`.
  The runner rejects instruction/function/call-graph or reviewed-body hash drift.

An independently constructed XEX2/PE fixture embeds these same bytes at its code
entry. Actual SDK UserModule loading checks and maps the image. A retained image
lease permits the host oracle to verify loaded bytes and entry metadata. The
thread's separate original XEX initializes actual XThread/PCR/TLS state.

The raw-test CLI uses a fixed analysis coordinate; explicit host metadata maps
its symbols to the loaded image's entry and imports. Two generated `blr` gate
identities are intercepted by [GeneratedImports](../../runtime/generated_imports.h)
before their bodies can execute. They are never registered as synthetic callable
functions. The indirect query gate has no synthetic host-function identity.
The admission layer rechecks actual XEX import membership, exact identity,
signature, module lease and bound XThread/PCR before using the existing adapters.
Unknown targets have no runtime resolver or raw-pointer fallback.

**PARTIAL integration boundary:** the XEX fixture, byte embedding, relocation
metadata, registration and initial argument/caller-area setup are host-authored.
This is not automatic production XEX-to-codegen discovery/linking or SDK title
entry scheduling. The generated caller, memory operations, direct/indirect call
instructions, argument preparation, scalar/vector arithmetic, SDK-result branch
and normal PPC epilogue really execute. SDK loading, TLS, imports and actual
export implementations are not replaced with success mocks.

One successful entry invocation proves the path with a copied scalar, doubled
vector bytes, thread-specific TLS value, wide arithmetic result, three SDK
returns and a generated success marker. An invalid query type returns the real
`0xc000000d`; generated PPC records it, branches around critical-section
initialization, omits the success marker and returns that status. Each
profile performs 83 main-entry invocations, including 32 across four concurrent
XThreads and eight load/unload cycles.

## Checked generated-memory census

The explicitly reachable main-entry scope is `entry` plus `echo`. Registered
import gates contain no executed PPC body; only the selected admitted SDK
contracts can be reached indirectly. The census reconciles PPC memory instruction
comments against macro sites, checks the exact call graph/body hash and verifies
the actual generated PCH's preprocessed macro definitions.

| Reachable generated guest-memory operation | Static sites |
| --- | ---: |
| U32 loads | 15 |
| U32 stores, including stack update | 9 |
| U64 loads | 2 |
| U64 stores | 2 |
| Full-vector loads / stores | 1 / 1 |
| Total / instrumented | **30 / 30** |
| Unsupported / unchecked in this reachable scope | **0 / 0** |

**VERIFIED: 100% within this scope.** Each normal successful invocation dynamically
completes all 30 sites: 18 reads and 12 writes. The SDK-error branch completes
28. Across all new groups per profile, counters report 1,321 reads + 911 writes
= 2,232 checked-copy attempts, 2,225 completed and seven deliberately refused
memory/MMIO transfers. Separate atomic/partial-vector roots reject before copy.
These counters are local to each active checked scope and do not count host
oracles or private SDK scratch access as generated memory operations.

**Excluded:** loader operations, SDK internals/scratch, host fixture control,
separate negative roots, unreachable generated functions and all private game
output. Register SIMD operations touch PPCContext storage, not guest memory.
SDK argument translation and arbitrary native C++ remain outside this boundary;
this is no complete sandbox or full guest-memory isolation claim. The older
private game-generated output remains uninstrumented and cannot inherit this
30-site result.

Separate emitted load/store roots test a four-byte span crossing a 4-KiB guest
boundary, BE bytes, read-only output, uncommitted pages, null/extent/stack bounds
and freed handles. All-or-nothing validation preserves a readable prefix when a
later page is refused. A 16-KiB host page cannot independently enforce every
4-KiB guest permission: explicit range/owner/page checks enforce this scoped
policy; global host protection is never bypassed.

## Pointer imports, ABI and error boundaries

| Contract reached from generated PPC | Confirmed behavior |
| --- | --- |
| RtlCompareMemoryUlong, direct | IN BE words across a guest-page boundary; real result is three matching words; input unchanged; reading RO data allowed |
| NtQueryVirtualMemory, indirect | OUT 28-byte result; same-owner query target; real region metadata and invalid-type status; generated failure branch |
| RtlInitializeCriticalSection, direct | INOUT 28-byte partial initialization; existing untouched fields preserved; guest argument retained by the adapter; initialized count verified |

Null/misaligned/oversized/zero/non-word/overflow/extent arguments fail explicitly.
RO and uncommitted OUT/INOUT spans reject without partial output; IN is first kept
on a committed page so each output contract is actually reached. Valid query and
critical-section outputs also cross a guest-page boundary. Freed/uncommitted
whole allocations are rejected at the first generated pointer call. The earlier
Phase 14 suite independently covers wider 64-bit argument rejection, stale token
reuse, foreign ownership, raw external release/protection mutation and scratch
allocation failure; it is rerun unchanged.

**VERIFIED subset:** generated 64-bit register argument/return arithmetic and
one explicitly encoded 32-bit ninth argument at `r1+0x54`, read by the generated
callee. Host setup reserves a caller linkage area; the PPC entry builds/releases
its 256-byte frame. r1/r2/r13, LR, r14/r15, f14 and v20 sentinel preservation are
checked. The supplied r2 sentinel proves preservation, not title TOC correctness.
PCR/r13 resolves actual SDK TLS, and per-thread generated writes/readback retain
distinct values. TLS/PCR addresses differ across simultaneous threads.

**PARTIAL ABI:** no inference about untested nonvolatile registers, CR fields,
FPR argument marshalling, typed vectors, mixed/variadic calls or complete Xbox
ABI. Historical wide-stack/nested callback checks remain separate SDK regressions.
Unverified, typed-vector argument/return, mixed-vector and variadic signatures
are rejected from the generated import path.

Observer errors at validated/scratch-ready/SDK-returned stages propagate out of
the generated entry without its success marker or partial output. The host
fixture restores the interrupted context for controlled teardown; this is
**PARTIAL callback/failure integration**, not guest exception delivery/unwind.
The real exception import RtlRaiseException is denied by admission. Atomics and
partial vectors fail before raw access; MMIO, unregistered/unaligned/zero targets
and instruction-fetch fallback are refused. Guest exception dispatch/unwind
remains unsupported; no speculative compatibility is added.

## Concurrency, ownership and cleanup

Four real XThreads each execute eight callers using distinct buffer capabilities,
PCR/TLS, generated state and thread-local dispatch/scope. Facade transactions
serialize each adapter's bounded copy; no race-freedom claim extends to arbitrary
raw SDK/host writes.

During the last SDK call of one generated entry, a controlled observer pauses
with a five-second condition-variable limit. External allocation release reports
host-policy busy; module unload returns the existing failure and closes new
admission while the image stays live. The already admitted call and generated
epilogue complete, then final unload succeeds. The outer execution image lease
and inner per-import lease both protect the image. The thread's TLS module is a
separate owner, so this does not confuse TLS lifetime with call-image lifetime.

Per profile the new suite balances **23 TLS/PCR lifetimes and thread destructors,
46 buffer allocations/releases and 164 SDK scratch allocations/releases**.
There are 163 actual SDK returns; one SDK-returned observer deliberately throws
before adapter completion. Dispatch reports 190 attempts and 162 normal returns.
Stack regions are already reclaimed after joining; TLS/PCR remain live until
legitimate handle/caller references are released, then become unreserved. Image
and ordinary heap free-page counts return to their initial values. Eight repeat
cycles do not use forced counter adjustment or manual object deletion.

**VERIFIED tested reclamation, PARTIAL general leak proof:** page counts,
capabilities, destructors and sanitizer results cover these fixtures. This is not
an exhaustive process-wide leak detector or proof about all SDK subsystems.
The separate debugger reference oracle still reports `[2,2,2,2,1]` and one actual
final destructor.

## Regression results

All Phase 5–14 core suites plus new generated dispatch run against the same
matching SDK profiles. Clients use O0, O2 and O2+ASan/UBSan; the unchanged
Sanitized SDK runtime uses O1+ASan/UBSan. No sanitizer recovery/suppression is added.

| Profile | Ordinary pass/fail | Expected fault/recovery pass/fail | Total pass | All core child seconds | Peak individual child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 124 / 0 | 23 / 0 | 147 | 83.584305 | 354,533,376 |
| Release | 124 / 0 | 23 / 0 | 147 | 76.577155 | 385,220,608 |
| Sanitized | 124 / 0 | 22 / 0 | 146 | 98.264584 | 431,980,544 |

**440 core passes, zero final failures, unexpected native signals, timeouts or
sanitizer findings.** Existing expected native deaths: 44 SIGBUS, three SIGSEGV
and nine SIGABRT, plus 12 controlled fault recoveries. The existing ASan-owned
plain default-signal case remains excluded only in Sanitized by the historical
runner policy; no new test is skipped or failure hidden. Negative typed-vector
compilation is expected refusal, not additional API success.

Each profile's 13 new groups includes 398 main assertions, 3,929 thread
observations and 58 expected C++ rejection events (host metadata and generated
failure paths). The rejection oracle accepts checked-memory/admission faults or
the explicitly authored callback exception; unrelated SDK or allocation
exceptions fail the group. No new group expects a native fault.

| New suite runtime children only | Summed seconds | Peak individual runtime-child RSS bytes |
| --- | ---: | ---: |
| Debug | 3.089378 | 29,638,656 |
| Release | 0.812198 | 23,609,344 |
| Sanitized | 1.705459 | 67,010,560 |

The first table includes generator, compiler, preprocessor and test children of
nine core suites; the second includes only the 13 new execution children. Sums
are monotonic elapsed child durations, not project calendar duration. RSS is
maximum wait4 individual-child usage, not concurrent-tree or game/Switch memory.
The separate debugger trace peaks at 1,070,972,928 bytes and is outside these core
tables. These numbers are **not** checked-vs-unchecked overhead benchmarks.
Performance cost under representative load remains **UNKNOWN**.

Separately passed: 27 historical component/source-control groups, three VMX runs
of 212,068 original invocations each, six watchdog units, six static-audit units,
six older encoder units, six new encoder units, 33-import source/signature audits
and the debugger reference oracle. Watchdog self-tests intentionally exercise
controlled timeout cleanup; core execution has no timeout.

Private development diagnostics are retained. Initial new-fixture build checks
caught missing generated-PCH inclusion and incorrect void owner-setter use.
Subsequent oracle failures exposed absent caller linkage space, the 16-KiB SDK
allocation-region report versus an 8-KiB requested capability, and a test that
incorrectly treated an earlier successful OUT call as partial output of a later
failed call. The caller area and exact per-call oracles were corrected using
source/observed contracts; runtime permissions and adapter failure behavior were
not weakened. Review also found an unintended indirect-import fallback to the
synthetic allowlist. It was removed: generated import calls now require the
active import scope and its exact indirect-only binding. Nine invalid target
classes (including the registered local entry/callee and direct-only import
gates) and absent import scope explicitly reject; final tests cover this fix.
The existing Apple FPCR assembly operand-width warning remains visible in build
diagnostics; no broader FPCR/CPU semantics claim is made. Final source-hashed
tests above supersede the development runs.

## Reproduction

All variables below name local private directories; do not place binaries,
generated output, original XEX images or diagnostics in this checkout.
Use a new workspace with the pinned lock if rebuilding is necessary:

```sh
python3 tools/sdk/build.py --lock tools/sdk/tls-import-lock.json --workspace "$SDK_WORK" --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$SDK_SOURCE_CACHE" --interposable-runtime
python3 tests/generated-dispatch/test_fixture.py
python3 tests/generated-dispatch/run.py --sdk "$SDK_DEBUG" --rexglue "$REXGLUE_DEBUG" --workdir "$PHASE15/generated-Debug"
python3 tests/generated-dispatch/run.py --sdk "$SDK_RELEASE" --rexglue "$REXGLUE_RELEASE" --workdir "$PHASE15/generated-Release"
python3 tests/generated-dispatch/run.py --sdk "$SDK_SANITIZED" --rexglue "$REXGLUE_SANITIZED" --workdir "$PHASE15/generated-Sanitized" --sanitize
```

This milestone reuses the already built, source-verified Phase 13 SDK rather than
changing installed files. For each profile set SDK to its staged directory,
REXGLUE to the matching source-build CLI (`rexglued` in Debug, otherwise
`rexglue`), SOURCE to the isolated pinned patched source, and SANITIZE to an
empty shell array or `(--sanitize)` in Bash:

```sh
python3 tests/pointer-import/run.py --sdk "$SDK" --workdir "$WORK/pointer" "${SANITIZE[@]}"
python3 tests/tls-import/run.py --sdk "$SDK" --source "$SOURCE" --workdir "$WORK/tls-import" "${SANITIZE[@]}"
python3 tests/image-runtime/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$WORK/image" "${SANITIZE[@]}"
python3 tests/synthetic/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$WORK/synthetic" "${SANITIZE[@]}"
python3 tests/instrumentation/run.py --sdk "$SDK" --source "$SOURCE" --workdir "$WORK/instrument" "${SANITIZE[@]}"
python3 tests/guest-memory/run.py --sdk "$SDK" --workdir "$WORK/guest" "${SANITIZE[@]}"
python3 tests/unified/run.py --sdk "$SDK" --source "$SOURCE" --pre-entry --workdir "$WORK/unified" "${SANITIZE[@]}"
python3 tests/pre-entry/run.py --sdk "$SDK" --workdir "$WORK/pre-entry" "${SANITIZE[@]}"
python3 tests/synthetic/reference-trace.py --sdk "$SDK_DEBUG" --workdir "$WORK/reference"
python3 tests/image-runtime/audit.py --source "$SOURCE" --output "$WORK/import-source-audit.json"
python3 tests/tls-import/audit.py --source "$SOURCE"
python3 tests/ppc/test_watchdog.py
python3 tests/guest-memory/test_audit.py
python3 tests/instrumentation/test_audit.py
python3 tests/synthetic/test_fixture.py
```

Historical leaf/source-control and VMX reproduction remains in the preceding
[Phase 14 report](pointer-import-synthetic-dispatch.md); these controls do not
substitute an old runtime for the unified SDK. Each potentially faulting child,
including new generator/compiler/test children, has an external monotonic
30-second process-group watchdog. Suite supervision is bounded to 300 seconds,
legacy suites to 180 seconds and debugger orchestration to 120 seconds. Raw
allocation-failure injection remains confined to existing dedicated children.

## Publication and Phase 16 decision

Publication admits only original runtime headers, fixture sources/tests, CI,
version/readme and English documentation. Original synthetic emitted outputs
also stay private for this run. Public-tree checks reject binary/symlink,
non-allowlisted, private-path and credential-shaped content. SDK patches and all
seven historical locks remain unchanged. Private logs, original/generated game
code and temporary review indices are excluded. The Linux CI checks public
contents, portable components, static audits, encoder and watchdog units; it does
not reproduce the local Mac SDK matrix.

**A — CONDITIONAL GO for broader original synthetic integration.** Exact next
milestone: Phase 16, validated image/import relocation binding and checked
image-data capabilities. Replace the explicit raw-test rebasing/gate metadata
with validated loader-derived binding for an original image; preserve admission,
add relocation/ownership failure cases and scoped checked-vs-unchecked performance
measurements. Continue ABI testing only for independently justified signatures.
Retain bounded execution and observable failures.

**B — NO-GO for private game-code execution.** All 24 denied entry-matrix imports
remain denied. Private generated-memory paths are uninstrumented; atomics,
partial vectors, MMIO, instruction fetch, typed-vector ABI, guest exception
handling, title TOC/cooperative stop and general DLL TLS indexing remain gates.
Performance overhead is unmeasured. The 33-import implementation matrix retains
27 PARTIAL / six BLOCKED full-conformance labels. There is no evidence of game
boot, game compatibility, full Xbox ABI or Nintendo Switch execution.
