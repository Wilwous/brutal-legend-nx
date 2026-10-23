# Pointer imports and original synthetic dispatch — Phase 14

Baseline: `v0.0.13-tls-import-admission`, commit `fc8e58a`.
Assessment: 2026-10-23, macOS ARM64. **VERIFIED for the explicit host-driven
synthetic import path; PARTIAL for general guest execution.** No proprietary code
is executed. The original XEX supplies actual image/TLS/import metadata; original
native XThread bodies drive PPCContext calls, not its guest entry instructions.
No listed loader/TLS/admission/adapter/implementation stage substitutes a mock.
The existing Phase 11 regression separately regenerates and executes its original
nine-function PPC module. These two scopes must not be conflated.

## Baseline and unchanged SDK

The working tree was clean and the Phase 13 tag resolved to the expected commit.
The private original XEX SHA-256 is unchanged:
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The preservation check covers 1,455 executable/manifest/generated-file fingerprints,
299 extracted files and the installed SDK runtime. No title update, game analysis,
private generated-code rewrite/compilation/execution or global SDK installation
occurs.

Phase 14 reuses the coherent Phase 13 SDK, **without an additional SDK patch**:
ReXGlue v0.10.0, revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, eighteen
ordered patches in [the existing lock](../../tools/sdk/tls-import-lock.json).
Read-only verification reconstructs 31 modified SDK files from pristine Git plus
those patches, verifies 22 submodule pins and compares patched staged headers and
all three runtime hashes to the existing build metadata. Original adapters/clients
are rebuilt against matching profiles. Historical SDK locks/patches remain intact.

The measured tools remain C++23, Apple Clang 17 (clang-1700.6.4.2), CMake 3.31.6,
Ninja 1.11.1.git.kitware.jobserver-1, macOS 15.7.7 / SDK 26.2. Interposable SDK
linkage remains confined to the private fixture SDK for child-local failures.

## Selected contracts

Selection follows the actual pinned implementation declarations and bodies,
not names. The [33-entry matrix](../../tests/image-runtime/import-contracts.json)
now includes explicit pointer contracts for these three exports:

| Import / ordinal in xboxkrnl.exe | Direction and extent | SDK behavior admitted |
| --- | --- | --- |
| RtlCompareMemoryUlong / 0x11b | IN; 4..65,536 bytes, length divisible by four | u32(ptr,u32 length,u32 pattern); read BE words; count every matching word, including matches after a mismatch |
| RtlInitializeCriticalSection / 0x12e | INOUT; exactly 28 bytes | void(ptr); initialize selected fields and preserve all untouched bytes |
| NtQueryVirtualMemory / 0xee | OUT; exactly 28 bytes; scalar query target inside the same allocation | u32(address,ptr,u32 type); type 0 success and type >=3 actual SDK INVALID_PARAMETER; unverified types 1/2 refused |

All buffer pointers are non-null, four-byte aligned and must carry a live opaque
SdkCheckedMemory allocation token from the matching manager/runtime. Addresses
and lengths are checked as uint64 before narrowing; the exact requested allocation
extent, not host padding, bounds access. Supported allocations are the facade's
ordinary virtual range `0x10000000..0x1fffffff`, with independent 4-KiB guest pages.
Every touched page must be reserved, committed, owner-matching and readable;
OUT/INOUT also require write permission. A token is not inferred from a raw integer.
XEX/physical/MMIO buffers and arbitrary external guest allocations are not adopted.

IN has snapshot semantics and no guest output. OUT scratch starts at zero; all
seven BE u32 information fields are checked on success. SDK INVALID_PARAMETER
leaves the original guest output unchanged. INOUT starts with the original bytes:
header type/absolute/signal and lock/recursion/owner fields are checked while
other header/list bytes retain their sentinel values. CS lock_count -1 has all-one
bytes in this SDK; no general native-int endianness claim follows. Initialization
does not certify later critical-section locking/waiter lifetime behavior.

The facade requires read permission even for OUT to perform bounded non-mutating
preflight. Null/zero-length/unaligned/mismatched contracts fail explicitly at the
host boundary, not with invented Xbox statuses. The only admitted query statuses
are actual SDK SUCCESS and INVALID_PARAMETER. Count returns must fit the number
of input words. Void initialization preserves the original PPCContext, including
r3; an ephemeral scratch address cannot become a return value.

**PARTIAL Xbox conformance:** SDK match-count semantics are not a verified Xbox
byte-count/first-mismatch contract. Query types/aliases/hardware policies and
critical-section use beyond initialization remain unverified. No shim is promoted
to full Xbox compatibility by passing these fixtures.

## Checked copy and ownership design

[SdkCheckedMemory](../../runtime/sdk_checked_memory.h) adds a private synchronous
transaction used only by the original [PointerBinding](../../runtime/pointer_imports.h).
It holds the facade mutex through preflight, actual invocation and final copy-out.
A nonblocking `try_release` returns the explicit host result busy during an active
transaction; ordinary facade reads/writes/releases serialize behind it. Separate
allocations in one manager are also serialized. Reentrant allocation/nested
transactions throw; reentrant read/write/release return host ownership refusal.
A thread-local transaction marker makes same-thread try_release return busy
**before touching std::mutex**. This avoids violating the nonrecursive mutex
[try_lock precondition](https://eel.is/c%2B%2Bdraft/thread.mutex.requirements.mutex.general).
Final review found this issue despite the Mac tests passing; the earlier draft
is not accepted as evidence of portable reentrancy correctness. Tokens retain their distinct
identity after release, preventing same-address reuse from reviving a stale token.

The transaction uses CopyOwnedMemory for a bounded snapshot, then non-mutating
page-permission checks for outputs. It allocates a private SDK scratch block,
tagged to its RAII owner, and fills it through the same checked copy API. Only the
scratch **guest address** is passed into the real SDK shim using a local PPCContext
copy. The selected synchronous shims neither retain the pointer nor return a
borrowed pointer. No guest backing host pointer is exposed by the adapter or its
diagnostic observer. Internal temporary spans cannot be requested through the
public import API.

A fresh module ImageLease covers admission and invocation. After the SDK returns,
bounded checked scratch readback obtains output bytes. CopyOwnedMemory validates
the complete original output span under SDK global-before-heap locks before a
single copy commits it. Invalidated commitment/protection/ownership therefore
fails without partial output or a reported scalar success. Only the declared
scalar result is copied back to the caller context. Exceptions restore no guessed
guest state: the original context/output was never committed. RAII reclaims
scratch and releases the facade/module guards on every tested path.

**Precise exclusion boundary:** this is not a new SDK heap lease. Facade operations
are serialized; arbitrary direct SDK release/protect/raw writes are outside that
exclusion. Controlled protection changes and direct heap release after actual SDK
execution are tested: copy-out refuses, context stays unchanged and scratch is
reclaimed. Input snapshot bytes stay valid independently of later guest storage.
No original guest pointer reaches the SDK implementation, so that implementation
cannot dereference freed original storage. Raw writes racing the snapshot/copy
outside the mediated API are not generally safe or race-free. SDK Reset/Restore,
runtime teardown and MMIO reconfiguration still require quiescence. There is no
global protection bypass or complete guest isolation claim.

## Admission and complete listed path

The [Phase 13 adapter](../../runtime/import_admission.h) now has explicit
PointerBinding overloads and three distinct pointer signatures. Its old scalar
API remains limited to six value-only signatures. Pointer calls require:

1. A fresh lease on the actual loaded UserModule.
2. Membership in its actual XEX import table.
3. Exact library, ordinal, export name, function type and signature.
4. The exact guest thunk address for indirect calls.
5. A matching current XThread, Memory, PPCContext and PCR/r13.
6. A valid same-manager allocation binding and the selected pointer contract.
7. Only the reviewed return/status behavior.

Missing capability, unknown targets/signatures, typed-vector/variadic requests,
wrong library prefixes, stale owners and unverified contracts fail before an
unsafe shim invocation. There is no pointer-to-scalar or raw-function fallback.
The matrix has **six scalar, three pointer-scoped, 24 denied** entries, while full
implementation/conformance labels remain **27 PARTIAL / six BLOCKED**. The updated
source/catalog/policy audit checks all 33. The historical Phase 13 refusal fixture
still rejects 27 imports under its scalar-only signature requests; the new fixture
rejects all 30 entries outside the three pointer contracts under every pointer
shape, including the six scalar exports. Those are different admission domains.

**VERIFIED listed integration:** independently authored XEX2/PE bytes → actual
SDK UserModule/XexModule/VFS loader → actual header-driven XThread TLS/PCR/stack →
actual import metadata/resolver → shared admission → checked adapter → actual
linked SDK export → validated return/error and copy-out → legitimate cleanup.
Both direct and indirect imports execute. The original host callback and
observer only drive tests/diagnostics or inject failure; neither substitutes an
SDK success result. There is no mocked runtime stage in this listed host-driven
path. **PARTIAL:** emitted guest callers and guest entry/fetch/exception dispatch
are not part of this new integration. Private generated calls remain unchanged
and do not automatically use the opt-in adapter.

## Lifetime, failures and ABI evidence

**VERIFIED:** four actual XThreads initiate concurrent calls; facade transactions
serialize them, with 64 repeated CS initializations and independent buffers. The
active-call unload fixture deliberately gives a thread TLS from module A while
calling module B. B has no thread-TLS lease: its fresh import-call lease alone
refuses unload during the gated call and permits cleanup after it returns.
Allocation try_release reports busy while the buffer transaction is active, also
when attempted from an original diagnostic callback. Thread join/final legitimate
release reclaims its stack/PCR/TLS, retaining Phase 11–13 guarantees. Eight module
reload cycles repeat the complete path and restore heap page counts.

Tests cover IN/OUT/INOUT, cross-page accesses, RO input versus refused RO output,
nulls, misalignment, zero/nonmultiple/overlarge lengths, 64-bit address overflow,
wrong allocation/manager, uncommitted/decommitted/freed/reused storage and output
sentinels. Scratch allocation returning zero or throwing bad_alloc is injected
only into two bounded children; each asserts one interception, no output/context
change or resource loss, then disarms and retries successfully. Observer exceptions
before allocation, after allocation and after actual SDK return propagate with
unchanged outputs/context and restored page counts. The controlled external
release test verifies final ownership refusal after a real SDK call.

The new original callback traverses the actual SDK GuestToHostFunction/REX_HOOK
bridge with a full-width uint64 tag and ninth uint64 stack argument carrying the
bounded guest address, performs admitted pointer dispatch and returns a checked
64-bit result. Success and callback error restore the saved PPCContext. This is
a separate original ABI fixture, not an extra title import. Existing GPR64/FPR,
stack, r1/r2/r13, raw vector, typed-vector negative compilation and exception
refusal regressions also rerun. **BLOCKED:** typed vectors, unverified mixed or
aggregate signatures, arbitrary callbacks and complete Xbox ABI/guest unwind.

Initial private diagnostics retain a Clang nested-aggregate initialization error
and a failed fixture expectation for the critical-section absolute-byte offset.
The latter was corrected against X_DISPATCH_HEADER (absolute is byte 1), without
weakening the checked fields/preserved-byte assertions. Original failed runs are
not counted as final passes. The saved earlier allocation SIGBUS is not repeated.

## Regression results

**VERIFIED: 401 core passes, zero failures.** Phase 5–13 contributes 362;
Phase 14 adds thirteen groups per profile. Each new run has **322 main-thread
assertions and 608 worker observations**, **23/23 TLS blocks, 26/26 guest buffers
and 118/118 private SDK scratch allocations/reclamations**. One guest buffer is
deliberately released through the raw SDK to test copy-out refusal; its release
is counted separately from facade success. There are **117 observed actual SDK
returns**; the extra scratch allocation is reclaimed by a callback error before
invocation. Scratch/PCR/TLS/page-count oracles independently check cleanup.
Repeated calls are not unique contract coverage.

| Profile | Ordinary pass/fail | Expected fault/recovery pass/fail | Execution-child seconds (sum) | Peak execution-child RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 111 / 0 | 23 / 0 | 20.637539 | 71,450,624 |
| Release | 111 / 0 | 23 / 0 | 12.461201 | 72,122,368 |
| Sanitized | 111 / 0 | 22 / 0 | 19.059065 | 72,925,184 |

Final core/new logs contain **zero unexpected signals, timeouts and emitted
ASan/UBSan findings**. Historical checks retain **44 expected SIGBUS, three
SIGSEGV and nine SIGABRT**, plus twelve controlled recoveries/diagnostic exits.
The historical ASan-owned plain default-signal case remains excluded only in
Sanitized; no new assertion or failure is suppressed. Typed-vector negative
compilation continues to be an expected refusal.

All recorded generation/compilation/preprocessing/negative-compile/size/execution
children, separately from total project elapsed time:

- Debug: 74.837445 summed child seconds; 355,893,248 peak individual-child RSS bytes.
- Release: 70.176733 summed child seconds; 378,191,872 peak individual-child RSS bytes.
- Sanitized: 88.104908 summed child seconds; 441,384,960 peak individual-child RSS bytes.

Execution sums include generator/emitter tools and exclude compilation, negative
compiles, preprocessing and size probes. RSS uses wait4 child accounting, not
total concurrent-tree or Switch/game memory. The unchanged SDK Sanitized profile
uses O1 ASan/UBSan/no recovery; clients use O0 / O2 / O2+ASan+UBSan. System
frameworks/assembly remain partly uninstrumented; sanitizers do not establish
race freedom, standards compliance or all guest resource reclamation.

Separately pass: 27 historical component/source-control groups; three VMX
characterization suites with 212,068 invocations each; six watchdog units, six
static-audit units, six encoding units; the 33-export source census and current
signature/catalog audit. Watchdog self-tests intentionally exercise controlled
timeout/group cleanup separately from the zero-timeout runtime matrix. A Debug
read-only XThread trace retains reference counts `[2,2,2,2,1]` and one destructor.
Hardware VMX equivalence remains UNKNOWN.

After the reentrancy review fix, all eight matching-profile core suites were
rerun in every configuration. Final recorded source hashes match the actual
fixture/adapter inputs; pre-fix successful-looking drafts remain private and are
not included in these totals. The original installed runtime and game input
fingerprints still match. No SDK source or staged runtime is rebuilt/modified
in this phase.

## Reproduction

Reuse matching SDK profiles from the unchanged Phase 13 isolated build, or build
the existing lock in a new private directory using [the SDK recipe](../../tools/sdk/README.md):

```sh
python3 tools/sdk/build.py --lock tools/sdk/tls-import-lock.json --workspace "$PHASE14/sdk-repro" --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$SDK_SOURCE_CACHE" --interposable-runtime
python3 tests/tls-import/audit.py --source "$SDK_SOURCE"
python3 tests/image-runtime/audit.py --source "$SDK_SOURCE" --output "$PHASE14/import-source-audit.json"
```

For PROFILE Debug, Release, Sanitized, SDK is the matching staged profile,
SDK_SOURCE the verified patched source and REXGLUE its matching executable
(`rexglued` in Debug, otherwise `rexglue`). SAN is empty or `--sanitize`.
Every output directory must be new and outside the public checkout.

```sh
python3 tests/pointer-import/run.py --sdk "$SDK" --workdir "$PHASE14/pointer-$PROFILE" $SAN
python3 tests/tls-import/run.py --sdk "$SDK" --source "$SDK_SOURCE" --workdir "$PHASE14/tls-$PROFILE" $SAN
python3 tests/image-runtime/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$PHASE14/image-$PROFILE" $SAN
python3 tests/synthetic/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$PHASE14/synthetic-$PROFILE" $SAN
python3 tests/instrumentation/run.py --sdk "$SDK" --source "$SDK_SOURCE" --workdir "$PHASE14/instrument-$PROFILE" $SAN
python3 tests/guest-memory/run.py --sdk "$SDK" --workdir "$PHASE14/guest-$PROFILE" $SAN
python3 tests/unified/run.py --sdk "$SDK" --source "$SDK_SOURCE" --pre-entry --workdir "$PHASE14/unified-$PROFILE" $SAN
python3 tests/pre-entry/run.py --sdk "$SDK" --workdir "$PHASE14/pre-entry-$PROFILE" $SAN
```

[Phase 13](xex-tls-import-admission.md) and the corresponding fixture READMEs retain
historical component/source-control, VMX, watchdog/unit and read-only reference
commands. Source controls are not an old runtime loaded beside the coherent SDK.
The new [test README](../../tests/pointer-import/README.md) describes the two
child-only injections. Every test/build child uses the external monotonic
30-second process-group watchdog with capped private diagnostics and explicit
exit codes. Public CI checks the allowlist, portable original components,
watchdog and matrix/catalog/policy coherence, not the macOS SDK matrix.

## Readiness and publication

**GO for broader original synthetic integration**, within the documented copying,
allocation ownership and context contracts. **NO-GO for a private game-code
experiment.** Twenty-four matrix entries remain denied; partial conformance of
all nine admitted SDK paths is not full title compatibility. Required pointer
outputs with borrowed lifetime, asynchronous APIs, waits, handles and global
process/DLL TLS-index policy need separate contracts.

Private generated game-memory/import calls remain uninstrumented. Atomics,
partial vectors, MMIO, instruction fetch authorization, typed-vector ABI,
guest exceptions/unwind, title TOC initialization and cooperative title stop
remain blocked or unverified. 4-KiB guest policy on 16-KiB host pages still needs
explicit range checks; unchecked aliases/raw writes are not isolated.

The design adds snapshot/scratch/readback copies, allocation and per-manager
serialization, plus existing bounded MMIO/page scans. This is a safety reference,
not a throughput/FPS or Switch-memory claim. Child timings include fixture setup;
no game performance inference is justified.

**Exact next milestone:** route an independently authored, actually emitted PPC
caller through this pointer admission boundary and checked module data accesses,
including denied dynamic targets and full call/access accounting. Establish
contracts for any additional reachable imports before expanding scope. No game
entry, full compilation, graphics/audio or Switch integration follows from this
synthetic result.

Publication is limited to original adapters/fixtures, contract records, allowlist
and English documentation. Existing SDK patch hashes remain unchanged. Private
inputs/configuration/generated game C++, raw logs, allocation addresses, absolute
local paths and temporary artifacts are excluded. A private temporary index
reviews all changed files and checks the public text/link/seven-lock allowlist.
No GitHub Release is created.
