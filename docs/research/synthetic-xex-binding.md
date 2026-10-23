# Phase 16 — synthetic XEX binding and reproducible codegen

Assessment: **2026-10-23**. Baseline tag
`v0.0.15-generated-ppc-dispatch` points to `2d2399a8c68b1513fc599fdf11d7b41d8a8aaf65`;
main also contains the README follow-up `902def594116b58939cd45ff00b154c11fd4d393`.
The initial tree was clean. **VERIFIED** for the fixed synthetic scope below;
production title integration remains **PARTIAL**. No proprietary instructions run.

## Single-command workflow

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$PHASE16_TESTS"
```

Both variables name private directories outside the checkout. SDK_WORK contains
source, build results and matching staged Debug/Release/Sanitized profiles built
with `tools/sdk/xex-binding-lock.json`; PHASE16_TESTS must be new. To build the SDK
in the same workflow, add:

```sh
--build --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$PINNED_SOURCE_CACHE"
```

The optional source cache is only an object/source-reuse optimization. Without
it, the locked upstream sources are obtained by the builder. No global install
is performed. The existing Phase 13–15 SDK is retained unchanged; this milestone
uses a new isolated nineteen-patch SDK. Patch 0019 is the only SDK addition.

The workflow performs:

1. Verify the SDK revision, nineteen patch digests and reconstructed patched
   sources, 22 submodule revisions, staged headers and runtime hashes.
2. Construct original PPC/XEX fixtures at two explicit preferred image bases.
3. Run actual production `rexglue codegen` on each XEX manifest, including
   Register/Scan/Discover/GapFill/Merge/Validate/Write.
4. Regenerate with `--ignore-stamp`; require identical emitted C++/header digests.
5. Census every generated function, memory site and call edge; preprocess the
   actual PCH to verify checked memory/call macro selection.
6. Compile unchanged emitted C++, initialization and registration sources, link
   the matching real SDK, then run original isolated native XThread fixtures.
7. Run Phase 5–15 regressions, historical controls reconstructed from pristine
   pinned Git objects, source audits, portable units and the reference trace.

No hidden source snapshot or handwritten PPC caller is needed. Command records,
tool versions, manifests, image/object/executable/generated-file hashes, child
metrics and diagnostics are retained in private JSON/results directories.
Generated native binaries are not claimed bit-identical across build paths or
profiles; emitted C++/headers are identical across all three measured profiles.

## Original image and code identity

The original encoder constructs unencrypted XEX2/PE images containing 252 bytes
of PPC: three functions / 63 instructions. It reuses the independently authored
Phase 15 caller and scalar callee, applies source-construction BL relocations to
real XEX import thunks, and adds an original entry wrapper that reads image RO
data and writes image RW data. PDATA describes all three function extents. There
are **no manual function hints, raw-test addresses or runtime rebasing**.

| Field | Image A | Image B |
| --- | --- | --- |
| Preferred image base | `0x82000000` | `0x83000000` |
| Image size | `0x40000` | `0x40000` |
| Code base | `0x82010000` | `0x83010000` |
| Actual XEX/generated entry | `0x820100C4` | `0x830100C4` |
| Query import thunk | `0x8203000C` | `0x8303000C` |
| Native functions / imported SDK bindings | 3 / 3 | 3 / 3 |

Each image has four 64-KiB PE sections: `.header` at offset zero (R), `.text` at
`0x10000` (RX), `.rdata` at `0x20000` (R), `.data` at `0x30000` (RW). Security
page descriptors agree with this layout. The wrapper transfers BE word
`0x10203040` from image offset `0x20200` to `0x30400` through checked image copies.

- A XEX SHA-256: `3cc76ab106852cd1117e4fe0fe0665b5bffadf14da635b86cb31557bd3631e35`.
- B XEX SHA-256: `5bda95c456ef30e37d01fff8d079b98873a68e69cccdf61c49ebbc9b91373c3e`.
- A emitted body SHA-256: `ec539183f54e4f29ac28074df2e1b5dc879c27bc8b16f0c50f08bd0ce5792413`.
- B emitted body SHA-256: `e6fa5463136bbdebab44f538ff36c4e90829482eebd29aaa69cbf4dfffa59887`.

The fixture checks file SHA-256 before loading. Actual SDK UserModule loading
supplies sections, entry, imports and TLS. The original XexBinding validates
those against the compiled construction description and actual generated
PPCImageInfo/PPCFuncMappings: base/size/code range, entry, exact section layout,
loaded code bytes, TLS fields, all import identities and mappings, supported
codegen flags and three unique native function identities. Mismatches reject
before generated entry admission. No raw backing is returned to guest code.

The two generated products use compiler-only namespacing for global image-info,
map, entry/`__imp__` and registration symbols. All definitions and flags are
recorded; the generated files are never edited. The actual generated mapping
provides callable host functions at their **unchanged guest addresses**.

**PARTIAL relocation scope:** each product is constructed and regenerated for
its own fixed base. PPC BL displacements and absolute image references are
verified independently. The SDK is not instructed to relocate one compiled
product to another base; pairing A with B's generated metadata rejects. General
PE/DLL relocation, dynamic loading and title startup scheduling remain unverified.
The native fixture drives the admitted generated entry from XThread::Execute;
this does not establish normal title launch.

## Checked image memory and ownership

The existing checked virtual-copy API intentionally rejects XEX heaps. Patch
0019 adds a **separate** owner-checked image-copy path exposed by a retained
UserModule::ImageLease. It requires a loaded, completed, leased image and one
matching declared PE section. Byte bounds and R/W flags are checked before the
shared heap helper takes global/heap locks, checks owner identity, reservation,
commitment, page protection, bounded MMIO exclusion and then copies. Physical
aliases remain refused; instruction fetch remains unsupported. Ordinary
CopyOwnedMemory retains its original virtual-heap policy and still rejects XEX.

The checked generated Scope borrows a stable image lease for its lifetime.
Binding owns that lease and retains the module; each new admission/entry also
checks a fresh lease. Guest buffer capabilities, native stack/PCR/TLS loans and
image capabilities remain distinct. Import pointer adapters still accept only
their original opaque allocation tokens: this phase does **not** admit arbitrary
XEX-backed pointers as SDK import arguments.

Tests verify image RO refusal, fetch refusal, cross-section refusal, wrong owner,
ordinary-copy XEX refusal and a page-protection change that prevents an otherwise
RW PE-section write. Underlying host protection is never globally disabled.
PE execute flags authorize only known translated functions; no guest instruction
fetch or CPU execution of PPC bytes is implemented. A 16-KiB host page is not a
complete 4-KiB guest isolation boundary. The measured XEX sections use the SDK's
64-KiB XEX heap; ordinary guest buffers retain explicit 4-KiB range checks.

Reset/Restore and MMIO registration changes require quiescence. Arbitrary native
code/raw SDK mutation is outside this boundary. The SDK permits one active Memory
instance; multiple images and threads share that verified address space. Multiple
simultaneous Runtime/Memory instances are not claimed supported.

## TLS, import binding and results

Actual XEX TLS headers specify 64 slots, 32 bytes of static data, a 16-byte raw
template in `.rdata`, and a distinct address for each image. CreateWithModule
initializes real SDK XThreads from the selected image. Generated PCR/r13 accesses
write/read distinct thread values; simultaneous TLS/PCR addresses differ.
Stacks are reclaimed after exit/join; TLS/PCR remain until legitimate final
references are released. Image and ordinary heap page counts return to baseline.

The binder consumes actual XEX import records and emitted function mappings.
It verifies library, ordinal, name, function type, exact linked SDK shim and the
reviewed signature. It automatically installs direct compare/critical-section
and indirect query gates. Existing Phase 13–15 Admission still rechecks module
membership, fresh lease, context/PCR, signature and pointer capability per call.
Unknown targets have no resolver or synthetic-function fallback.

**VERIFIED scoped imports:** RtlCompareMemoryUlong (IN words), NtQueryVirtualMemory
(OUT 28-byte information) and RtlInitializeCriticalSection (INOUT 28 bytes).
Generated success/error paths execute real SDK implementations; invalid query
type returns `0xc000000d` and branches around the success marker. Null pointers
and invalid indirect targets are refused. Existing Phase 14–15 regressions retain
the broader permission/length/stale/ownership/callback/signature tests.

Automatic binding in this fixture covers these three contracts. Six existing
scalar signatures remain admitted by the separate unchanged Admission API;
24 matrix imports remain denied. No new Xbox ABI signature is inferred. Complete
ABI, typed-vector marshalling, guest exception dispatch/unwind, title TOC/stop
and general DLL/process TLS indexing remain **PARTIAL / BLOCKED**.

## Complete generated-memory scope

All three emitted functions per image are reachable and censused, including the
entry wrapper. The actual generated PCH selects checked memory and import seams.
The runner refuses raw/partial/atomic, trap, setjmp/longjmp, unresolved helper or
unexpected call paths before compilation/execution.

| Operation | Static sites per image |
| --- | ---: |
| U32 load / store | 17 / 12 |
| U64 load / store | 2 / 2 |
| Full-vector load / store | 1 / 1 |
| Total / instrumented | **35 / 35** |
| Unsupported / unchecked | **0 / 0** |

**VERIFIED 100% for this explicitly bounded scope**: 70 static sites across the
two separately generated images. A successful invocation completes 20 reads and
15 writes; the SDK-error branch completes 33 operations. Per profile, 256 entry
invocations produce 5,094 reads + 3,828 writes = **8,922 checked completions**.
The separate host copy microbenchmark is excluded from generated counters.

This result excludes SDK internals/scratch, host construction/oracles and all
private game-generated output. It is no complete native sandbox or private-game
coverage claim. Existing regressions keep atomics, partial vectors, MMIO/fetch,
typed-vector signatures and guest exception paths explicitly refused.

## Failure containment and development findings

Twelve new groups per profile cover successful two-image execution, mismatched
image/generated metadata and entry, six corrupt-image cases (code, entry,
overlapping sections, TLS, unsupported import and PE initialization), image
permissions, pointer/target errors, actual SDK status, eight reload cycles, four
concurrent XThreads, 200-entry batched execution, a copy control and expected
join fail-stop. All dangerous children have external monotonic 30-second
process-group watchdogs; suite budgets are 300 seconds, historical groups 180,
debugger orchestration 120. SDK configure/build commands also have finite budgets.

Private development failures are retained rather than counted as final passes:

- The initial loader-only image lacked PDATA; production codegen correctly
  rejected it with `Exception DataDirectory not found (size=0)`. Original PDATA
  and a separate non-executable header section correct the fixture. No forced
  generation or unsupported emitted code is used in the final products.
- Two-image linking initially collided on SDK-generated global registration and
  `__imp__xstart` names. Explicit compiler-only namespacing resolves this without
  changing guest addresses or rewriting emitted bodies.
- A negative test incorrectly required successful Unload for an object that
  never loaded. The existing SDK can return its existing unsuccessful status
  for that uninitialized state. The corrected oracle still requires load
  refusal, legitimate handle/final-reference release and baseline page counts.
- One development Debug benchmark died with SIGSEGV after about 5.28 seconds.
  The saved fault PC resolves to the fixture's std::string helper; its sample
  shows main waiting in the original five-second join while workers execute.
  A bounded debugger observation subsequently completed normally. **VERIFIED**
  harness defect: a failed join could unwind module/buffer/string/runtime owners
  while captured references remained in active workers. **PARTIAL attribution**
  of that saved native fault: timing and PC are consistent with this defect;
  an exact crashing debugger backtrace was not obtained.

The fixture now allows a finite 15-second normal join within the unchanged
30-second external budget and refuses teardown with live workers. A pending-thread
guard fail-stops before unsafe unwind. A deterministic child exercises the same
join failure path and must exit **70** with its explicit diagnostic. This is
expected process containment, not successful orderly TLS reclamation or guest
unwind. No forced thread deletion, foreign cancellation or global protection
bypass is introduced. Final stress/cleanup checks pass with no unexpected fault.
The old SIGSEGV diagnostic remains private evidence; broader asynchronous stop
and universal host-fault causality are not claimed solved.

## Final regressions and resources

| Profile | Regular/control groups | Expected fault/recovery checks | Core pass/fail |
| --- | ---: | ---: | ---: |
| Debug | 136 | 23 | 159 / 0 |
| Release | 136 | 23 | 159 / 0 |
| Sanitized | 136 | 22 | 158 / 0 |

**476 core passes, zero final failures, unexpected native signals, external
watchdog timeouts or sanitizer findings.** Expected historical native deaths:
44 SIGBUS, three SIGSEGV and nine SIGABRT; 12 controlled recoveries. Three new
expected join-failure children exit 70 and are included in regular/control
groups. They do not count as native faults or orderly resource reclamation.
The historical ASan-owned default-signal case remains excluded only in Sanitized;
no new failing test is skipped. Typed-vector negative compilation remains
expected refusal.

Each profile's new suite has 498 main assertions, 7,938 worker observations,
12 controlled rejection events, **30 balanced normal TLS/thread lifetimes,
60 buffer allocations/releases and 756 scratch allocations/releases**. All 756
SDK calls return through the real implementations. Those normal-lifetime counts
exclude the intentional process fail-stop. General leak/race freedom is not
proved by these selected counters and sanitizer checks.

Also passed: 27 historical component/leaf groups reconstructed from pinned Git,
three VMX runs of 212,068 invocations each, six watchdog units, six static-audit
units, six older encoder units, six Phase 15 encoder units, eight new XEX encoder
units, 33-import source/signature audits and the debugger reference-count oracle
`[2,2,2,2,1]` with one final destructor. Watchdog self-tests intentionally exercise
controlled timeout cleanup; core runtime children do not time out.

## Tool and performance measurements

ReXGlue v0.10.0 revision `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, CMake 3.31.6,
Ninja `1.11.1.git.kitware.jobserver-1`, Apple clang 17.0.0
(`clang-1700.6.4.2`), Git 2.50.1, macOS ARM64. SDK and clients are rebuilt/staged
privately. Clients use O0/O2/O2+ASan+UBSan; the Sanitized SDK uses O1+ASan+UBSan.
System frameworks, CommonCrypto and handwritten assembly are not fully sanitizer
instrumented. Existing FPCR operand-width and build-system warnings remain
visible; they do not establish hardware-equivalent CPU semantics.

| Profile | SDK build seconds / peak RSS bytes | New codegen seconds | New build seconds | New runtime seconds / peak runtime RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 106.567693 / 557,744,128 | 0.513890 | 10.110230 | 6.820833 / 29,655,040 |
| Release | 130.231199 / 550,125,568 | 0.124086 | 10.390736 | 1.132105 / 23,937,024 |
| Sanitized | 215.779903 / 935,460,864 | 0.380119 | 11.959267 | 2.862974 / 67,616,768 |

| Profile | All core child seconds / peak individual-child RSS bytes | 200-entry batch seconds | Raw / checked 1,024-copy seconds |
| --- | ---: | ---: | ---: |
| Debug | 96.007311 / 362,823,680 | 3.622004 | 0.000001000 / 0.259956791 |
| Release | 84.751508 / 389,513,216 | 0.384682 | 0.000001166 / 0.027900917 |
| Sanitized | 108.056832 / 447,102,976 | 0.997237 | 0.000000500 / 0.070653167 |

Times are sums of monotonic child durations, not project calendar time. RSS is
peak individual-child wait4 usage, not concurrent-tree or game/Switch memory.
Generation includes first and forced-repeat production codegen for both images;
fixture builds include the emitted objects and host fixture link; runtime includes
all twelve new isolated children. Full core includes older suites and compilation.
No SDK build, debugger or historical-control RSS is silently presented as game
memory.

The 200-entry batch includes fixture setup/oracles/thread teardown, so it is not
pure PPC or SDK latency. A separate hot four-byte RO host-read control compares
1,024 direct volatile reads with the checked Scope/image-copy path while holding
a valid image lease. This raw control is original host test code, outside the
generated execution path and without imports. It never disables protection or
accesses invalid memory. The raw duration is close to clock resolution; ratios
would be misleading. The measured copy cost includes bounded MMIO scanning,
locks, PE/owner/page checks and copying. Representative throughput, batching
benefits and Horizon costs remain **UNKNOWN**; no game performance is inferred.

## Preservation, publication and next milestone

Original XEX SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
All 1,455 original XEX/config/generated fingerprints and 299 extracted-file
digests match. No title update is applied. The previous eighteen-patch SDK and
globally installed runtime are unchanged; nineteen-patch source/header/runtime
identity is independently verified. No game analyzer, private regeneration,
full game compilation or proprietary execution occurs.

The publication allowlist admits only original runtime/tests/workflow, the
BSD SDK patch and lock, CI and English documentation. Images, emitted C++, native
products, logs, private paths and original/generated game-derived content remain
outside Git. CI checks portable encoding/audits/components and the public tree;
it does not reproduce the Mac SDK matrix or access game files. No GitHub Release
is created.

**A — CONDITIONAL GO for further synthetic integration.** Exact Phase 17 task:
original image-backed pointer contracts and cooperative-stop/failure lifetimes.
Establish checked image pointer views for a separately justified synchronous SDK
contract, add initialization/stop fault injection and improve measured copy
batching without weakening denial/ownership. General relocation, broader
imports and title scheduling require independent evidence.

**B — NO-GO for a private game-code experiment.** Remaining gates include all
24 denied imports, uninstrumented private generated-memory paths, atomics,
partial vectors, MMIO/instruction fetch, typed-vector ABI, guest exceptions,
title TOC/cooperative stop and general DLL TLS indexing. Full conformance labels
remain 27 PARTIAL / six BLOCKED. Nintendo Switch allocation/address aliases,
threading/fault delivery, target toolchain, graphics/audio and physical hardware
execution are unverified. Synthetic success does not prove game boot, game
compatibility, full Xbox ABI or Nintendo Switch compatibility.
