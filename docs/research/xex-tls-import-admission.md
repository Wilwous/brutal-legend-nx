# XEX TLS and import signature admission — Phase 13

Baseline: `v0.0.12-image-runtime-contracts`, commit `6fe55df`.
Host assessment: 2026-10-22, macOS ARM64. **VERIFIED in the bounded original
synthetic scope; proprietary execution NO-GO.** No game code is executed.
The loader/thread/export integration is driven by original host fixtures; this
is **PARTIAL for guest entry/dispatch**, which these XEX fixtures never invoke.
The unchanged Phase 11 raw-test generation/execution regression remains separate.

## Inputs and toolchain

The baseline tree was clean. The private default.xex SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
Preservation checks cover 1,455 executable/manifest/generated-file fingerprints,
299 extracted files and the installed runtime library. No title update is applied,
no game output is regenerated, compiled or executed, and no global SDK is changed.

The isolated SDK is ReXGlue v0.10.0, revision
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with eighteen ordered patches in
[tls-import-lock.json](../../tools/sdk/tls-import-lock.json). Earlier lock files
and patches 0001–0017 remain unchanged. New patch 0018 changes five existing SDK
files; the original admission adapter and fixtures are separate GPL sources.
Headers, libraries and clients are rebuilt together. CMake 3.31.6, Ninja
1.11.1.git.kitware.jobserver-1, Apple Clang 17 (clang-1700.6.4.2), C++23,
macOS 15.7.7 / SDK 26.2 are the measured host tools.

## Header-driven TLS

**VERIFIED:** `XEX_HEADER_TLS_INFO` is key `0x00020104`; its low-byte length is
four 32-bit words. The optional-header entry points inside the bounded XEX header
at a 16-byte big-endian record:

| Offset | SDK field | Meaning used by the SDK |
| ---: | --- | --- |
| 0 | slot_count | dynamic 4-byte slot count |
| 4 | raw_data_address | guest address of initialized template bytes |
| 8 | data_size | static data prefix including zero-filled tail |
| 12 | raw_data_size | bytes copied from the image |

Original fixture bytes place the record at header offset `0x380`, a 16-byte
independent template in a read-only section, and a 32-byte static prefix followed
by 64 slots. No test-only runtime TLS header is substituted. XexModule Load and
LoadContinue parse the actual header; ReadTlsInfo returns a validated copy.
UserModule loads the original file through its actual VFS path. XThread Create
uses the executable module; CreateWithModule explicitly selects an independently
owned module without changing the global executable.

Before success, the loader checks a wide total size and rejects zero slots,
slots above 256, slot counts not divisible by four, misaligned template/static
size, raw size beyond static size, missing/nonzero-inconsistent template address,
foreign/out-of-image/wrapping template spans and totals above 1 MiB. These are
**host admission policies**, not asserted Xbox format limits. The four-slot
policy keeps the SDK's rounded process bitmap from advertising storage beyond
the thread block. Thirteen malformed fixtures roll back to the original heap
page count; explicit BSS-only metadata also loads. An absent TLS header retains
the historical 1,024-slot XThread default; complete no-header title semantics
are not certified.

XThread zeroes the complete block, copies only raw_data_size into the static
prefix, and places dynamic slots after data_size. PCR[0] points to the static
prefix, KTHREAD.tls_address points to dynamic slots, and r13 is the actual PCR.
The tests verify all 32 static bytes, zero tail, stack alignment, slot 63, invalid
64/UINT32_MAX, r1/r2/r13 preservation and independent per-thread writes.
Template contents must remain stable during creation; leases pin storage, not
concurrent data mutation. Fibers/APCs and arbitrary title TLS layouts remain UNKNOWN.

## Ownership, rollback and title process

**VERIFIED:** each successfully module-bound XThread owns an actual UserModule
ImageLease through final PCR/TLS reclamation. A close request refuses new calls
and leases while existing backing remains live. Exit/join free the native guest
stack; legitimate caller/table references retain PCR/TLS until final release.
Two independent modules and four XThreads retain distinct templates/blocks.
Eight repeated two-module cycles return system/XEX heaps to baseline. The original
Phase 11/12 teardown assertions and ownership patches remain intact.

Return-zero and bad_alloc TLS/PCR injection, scratch allocation failure, oversized
stack and native-thread factory failure are isolated child tests. They assert
explicit NO_MEMORY, no published process linkage/native callback, zero exposed
PCR/TLS/stack addresses, released image lease and restored heap counts. Observed
partial TLS allocations are also checked as no longer live. XThread creation is
one-shot: retry on the same attempted object fails explicitly. It is not a
concurrent Create API. Late host allocation exceptions after a native thread
exists fail-stop rather than freeing its callback context; graceful cancellation
of that untested path remains BLOCKED.

A separate actual SetExecutableModule → default XThread Create test reads the
same XEX header into the title process and bitmap. It allocates all 64 indices,
checks exhaustion, writes/reads/frees them, rejects repeated and out-of-range
free, joins the guest thread and shuts down the SDK dispatch worker. Final module
destruction observes both guest and dispatch PCR/TLS allocations gone.

The initial test exposed **confirmed false success** in KernelState FreeTLS:
a slot outside process.tls_slot_size but below 256 could find a zero bitmap bit
and be reported freed. Patch 0018 rejects that index before bitmap mutation,
using the existing boolean failure contract. No Xbox status code is invented.
Initial failure logs remain private. Cross-module process bitmap allocation/free
is not generally admitted: explicit per-module binding does not reconfigure the
one global title-process bitmap. Simultaneous module template/value isolation is
verified; a general DLL TLS-index architecture remains PARTIAL.

## Import admission and the 33-entry matrix

The [matrix](../../tests/image-runtime/import-contracts.json) now records every
module/ordinal/symbol identity, pinned implementation declaration, argument and
return types, register/stack use, ownership, errors, outputs, context, status and
evidence. Source declarations are evidence of **this SDK**, not complete Xbox
contracts. In particular, a zero-argument UI stub does not establish the UI ABI.
The existing conformance classification remains **27 PARTIAL / six BLOCKED**.

[Admission](../../runtime/import_admission.h) permits only these six scoped
value-only signatures through actual linked SDK shims:

| Library | Ordinal | Export | Signature |
| --- | ---: | --- | --- |
| xboxkrnl.exe | 0x154 | KeTlsGetValue | u32(u32) |
| xboxkrnl.exe | 0x155 | KeTlsSetValue | u32(u32,u32) |
| xboxkrnl.exe | 0x135 | RtlNtStatusToDosError | u32(u32) |
| xboxkrnl.exe | 0x083 | KeQueryPerformanceFrequency | u32() |
| xam.xex | 0x3cb | XGetAVPack | u32() |
| xam.xex | 0x3cd | XGetLanguage | u32() |

Each call acquires a fresh module lease, checks an actual loaded import-table
record, exact caller library identity, ordinal, name and function type, and exact
signature. The SDK normalizes stored library names to `xboxkrnl` / `xam`; the
adapter compares only the explicit reviewed canonical names, avoiding the
resolver's permissive prefix matching. Direct calls use the declared identity;
indirect calls require the exact registered guest thunk address. Neither accepts
an arbitrary function pointer or unknown target. TLS calls additionally require
the matching bound XThread, Memory, current PPCContext and PCR/r13. Wrong context,
changed export identity/type, closing/unloaded image and unsupported signatures
throw explicit host errors before invocation; rejected-call contexts remain intact.

An original XEX containing all 33 function-import records proves all 27 denied
imports are refused under all three supported signature shapes (81 refusals).
Typed-vector argument, return, mixed and variadic signatures are also refused.
Name resolution alone conveys no permission. Denied pointer/handle/wait/fatal/UI
contracts cannot silently fall back to their raw shims in this adapter.

**PARTIAL integration:** the adapter is opt-in. Existing private generated import
calls are unchanged and do not automatically enter it. Trusted host code can still
call SDK symbols directly. The separate title-process index test deliberately
calls real KeTlsAlloc/Free outside admission to characterize that narrower path;
those imports remain denied by the production-facing adapter.

## ABI and errors

**VERIFIED selected subset:** new calls preserve observed r1/r2/r13/r31 sentinels,
use actual GPR32 returns and show TLS isolation under simultaneous native threads.
The existing unified suite reruns original full-width GPR64 arguments/results,
ninth 64-bit big-endian stack argument, FPR arguments/results, aligned nested
callbacks, r2/r13 preservation, host callback error/context restoration, raw vector
register operations and pointer translation. Generated original scalar/full-vector
fixtures also rerun. Those wider original host functions are a separate fixture
domain; they are not extra admitted title imports.

**BLOCKED:** typed-vector argument/return/mixed ABI still fails the existing
negative compilation tests. Full Xbox ABI, TOC initialization, variadic/mixed or
aggregate signatures, arbitrary callbacks and guest exception dispatch/unwind
remain unverified. A host exception is not guest unwind support.

For admitted operations, no guest pointer output or ownership transfer occurs.
Tests cover exact status translations (success, INVALID_HANDLE, unknown fallback),
constant AV/language policy, positive frequency, invalid slot results, repeated
calls and per-thread concurrent calls. This does not establish clock/hardware/user
policy equivalence. Existing Phase 9/12 fixtures rerun invalid/closed handles,
missing module with zeroed output, XConfig output initialization/short buffers,
allocation failures, owner release, concurrency and STFS NOT_IMPLEMENTED.
Unverified pointer outputs remain denied. XamShowMessageBoxUIEx still contains
an unimplemented false-success stub; its actual ABI/completion ownership is UNKNOWN,
so no speculative replacement is added and it is always denied. The new source
census is not an API conformance certificate.

## Final regression evidence

**VERIFIED: 362 core passes, zero failures.** The existing Phase 5–12 suites
contribute 323; the new suite adds thirteen groups per profile. Each new run has
**769 assertions, 1,836 thread observations, 41 TLS allocations and 41 confirmed
reclamations**. Of those allocations, 36 are independent-module thread blocks,
two belong to the title/default dispatch path, and three are reclaimed during
partial PCR/native-creation failure. Repeated checks are not unique coverage.

| Profile | Ordinary pass/fail | Expected fault/recovery pass/fail | Execution-child seconds (sum) | Peak execution-child RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 98 / 0 | 23 / 0 | 21.924324 | 70,795,264 |
| Release | 98 / 0 | 23 / 0 | 12.012918 | 72,417,280 |
| Sanitized | 98 / 0 | 22 / 0 | 19.564938 | 73,940,992 |

Final core/new logs contain zero unexpected signals, timeouts or emitted sanitizer
findings. Historical fault checks retain **44 expected SIGBUS, three SIGSEGV and
nine SIGABRT**, plus twelve controlled recovery/diagnostic results. These are
expected fault tests, not hidden failures. The historical plain default-signal
case remains excluded only from Sanitized because ASan owns that signal path.
Typed-vector negative compilation remains an expected refusal. No assertion is
weakened.

Recorded generation/compilation/preprocessing/negative-compile/size/execution
children (sum of child durations, not project elapsed time):

- Debug: 71.499554 seconds; 363,380,736 peak individual-child RSS bytes.
- Release: 64.797602 seconds; 385,515,520 peak individual-child RSS bytes.
- Sanitized: 84.370909 seconds; 433,242,112 peak individual-child RSS bytes.

Execution sums include generation/emitter tools and exclude compilation,
preprocessing, negative compiles and size probes. RSS is wait4 child accounting,
not total simultaneous-tree or Switch memory. SDK Sanitized uses O1 ASan/UBSan
without recovery; fixtures use O0 / O2 / O2+ASan+UBSan. System frameworks/assembly
remain partly uninstrumented; sanitizers do not prove race freedom or all resource
reclamation. Explicit guest-page/destructor oracles are independently required.

Separately pass: 27 historical component/source-control groups; three VMX
characterization suites with 212,068 invocations each; six watchdog units, six
static-audit units and six encoder units; the 33-export source census and new
signature/catalog audit. A separate Debug read-only XThread trace retains pointer
references `[2,2,2,2,1]` followed by one destructor. Hardware VMX equivalence remains
UNKNOWN. Portable CI adds matrix/catalog/policy coherence, not a local SDK run.

The final source-verified SDK rebuild took 18.806128 / 21.361841 / 35.237637 seconds
for Debug / Release / Sanitized, with peak build-child RSS 462,290,944 / 440,532,992 /
577,093,632 bytes. These are incremental rebuilds in a new isolated workspace
after its initial cache-populating build. All clients use matching final headers
and runtime fingerprints.

Private history preserves the initial library-name mismatch, the failing title
TLS free oracle and superseded test runs. Final source hashes are checked against
every recorded fixture result; all thirteen new groups are rerun after final
admission identity/type checks and source-census changes. The saved Phase 8 SIGBUS
and game-code operations are not repeated.

## Reproduction

Use a new private workspace outside the checkout; CMAKE/NINJA identify the pinned
host tools described in [the builder recipe](../../tools/sdk/README.md).

```sh
python3 tools/sdk/build.py --lock tools/sdk/tls-import-lock.json --workspace "$PHASE13/repro" --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$SDK_SOURCE_CACHE" --interposable-runtime
python3 tests/tls-import/audit.py --source "$PHASE13/repro/sdk"
python3 tests/image-runtime/audit.py --source "$PHASE13/repro/sdk" --output "$PHASE13/import-source-audit.json"
```

For each PROFILE in Debug, Release, Sanitized, set SDK to
`$PHASE13/repro/staged/$PROFILE`, SDK_SOURCE to `$PHASE13/repro/sdk`, and REXGLUE to
`$SDK_SOURCE/out/mac-arm64/$PROFILE/rexglue` (`rexglued` in Debug). SAN is empty,
or `--sanitize` for Sanitized. Every output directory must be new.

```sh
python3 tests/tls-import/run.py --sdk "$SDK" --source "$SDK_SOURCE" --workdir "$PHASE13/tls-$PROFILE" $SAN
python3 tests/image-runtime/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$PHASE13/image-$PROFILE" $SAN
python3 tests/synthetic/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$PHASE13/synthetic-$PROFILE" $SAN
python3 tests/instrumentation/run.py --sdk "$SDK" --source "$SDK_SOURCE" --workdir "$PHASE13/instrument-$PROFILE" $SAN
python3 tests/guest-memory/run.py --sdk "$SDK" --workdir "$PHASE13/guest-$PROFILE" $SAN
python3 tests/unified/run.py --sdk "$SDK" --source "$SDK_SOURCE" --pre-entry --workdir "$PHASE13/unified-$PROFILE" $SAN
python3 tests/pre-entry/run.py --sdk "$SDK" --workdir "$PHASE13/pre-entry-$PROFILE" $SAN
```

[Phase 11](synthetic-execution.md) and the [runtime](../../tests/runtime/README.md),
[VMX](../../tests/vmx128/README.md) and [watchdog](../../tests/ppc/README.md) recipes
supply the historical component/source-control, VMX, unit and read-only reference
checks. These source controls do not load an older runtime beside the unified SDK.
Every execution/build child is supervised externally for 30 seconds with process
group termination and bounded diagnostics. SDK acquisition/build budgets remain
finite. Child-only interposition requires private `--interposable-runtime`; no
ordinary child has a DYLD override. Assertions are not weakened.

## Readiness and publication

**GO for further original synthetic integration. NO-GO for private game-code
execution.** Header-driven TLS and six admitted signatures remove specific gates;
27 import contracts remain denied. Existing private generated accesses are
unmediated, and no proposed title path has complete checked memory/import closure.
Atomic/partial-vector operations, MMIO, instruction fetch authorization, typed
vector ABI, guest exceptions, arbitrary callbacks and title TOC/stop remain blocked.
The 4-KiB guest / 16-KiB host protection limitation still requires explicit checked
ranges; no full guest isolation or native C++ sandbox is claimed.

**Exact next milestone:** original checked pointer-import adapters with retained
buffer owners, output/error contracts and concurrent release tests; then a complete
synthetic dispatch path that enforces admission on every import and accounts for
every memory access. Reassess a specific private scope only after those checks.
No game entry, full game build, graphics/audio or Switch integration is authorized
by this result. Synthetic success does not prove game compatibility.

Publication is restricted to intended original source/tests, SDK patch/lock,
contract records and English documentation. A private temporary index audits all
changed files, local links, seven patch locks and private path/credential patterns.
Private game inputs/configuration/generated code, failure logs, allocation addresses
and build artifacts remain outside Git. CI checks the portable public components,
watchdog and catalog/policy coherence; it does not rerun the local macOS SDK matrix.
No GitHub Release is created.
