# Phase 17 — image-backed pointers and cooperative shutdown

Baseline: `v0.0.16-synthetic-xex-binding`. Verification date: 2026-10-23.

**VERIFIED in the explicit synthetic scope:** all 518 core checks pass across
Debug, Release and Sanitized. No final unexpected signals, timeouts or sanitizer
findings. Host scheduling/general cancellation remains PARTIAL. No proprietary
code is loaded or executed.

## Reproducible inputs and boundary

The isolated SDK remains ReXGlue v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with the same nineteen patches in
[the XEX-binding lock](../../tools/sdk/xex-binding-lock.json). Its SHA-256 is
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
No SDK source, dependency, staged header, library or global installation is
changed in this phase. The workflow reconstructs patched source from pinned Git
objects, verifies dependency revisions and staged headers, and verifies each
linked library's hash. It reuses the isolated Phase 16 build.

The preserved original XEX SHA-256 is
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
All 1,455 saved XEX/manifest/generated fingerprints and 299 extracted-file hashes
are checked privately. The title update is not applied. Game files, previous
proprietary generated output, manifests and private diagnostics stay outside the
public tree. These preservation checks do not execute game code.

The [original Phase 16 encoder](../../tests/xex-binding/fixture.py) produces the
same two fixed-base XEX products. Production `codegen`, forced regeneration,
unchanged C++ compilation, actual SDK loading, XEX-header TLS, generated PPC
entry and the real three import shims are used. No SDK shim is replaced.
The new suite executes image A; the retained Phase 16 suite executes both A and
B. Each has three emitted functions, 63 instructions and 35 mediated guest
memory sites: 17 U32 loads, 12 U32 stores, two U64 loads, two U64 stores and one
full-vector load/store each. The compiler-only linkage namespaces do not edit
emitted files or relocate the guest image.

| Artifact | SHA-256 |
| --- | --- |
| Original XEX A | `3cc76ab106852cd1117e4fe0fe0665b5bffadf14da635b86cb31557bd3631e35` |
| Original XEX B | `5bda95c456ef30e37d01fff8d079b98873a68e69cccdf61c49ebbc9b91373c3e` |
| Generated body A | `ec539183f54e4f29ac28074df2e1b5dc879c27bc8b16f0c50f08bd0ce5792413` |
| Generated body B | `e6fa5463136bbdebab44f538ff36c4e90829482eebd29aaa69cbf4dfffa59887` |

These are independently authored synthetic products, not game-derived code.
Products and emitted C++ remain private to keep publication boundaries simple.

## Image pointer contracts

[PointerBinding](../../runtime/pointer_imports.h) has an explicit `Image` tag.
The existing opaque heap-allocation constructor and its contracts remain intact.
Admission also requires the binding's exact UserModule identity to match the
import owner. An image capability retains its module and a birth image lease.
A separate fresh per-call lease refuses closing admission. A capability created
while closed has no birth lease and cannot revive on a later load. A live birth
lease prevents unloading/replacing its image incarnation.

| Contract | Validated input | Output policy |
| --- | --- | --- |
| `RtlCompareMemoryUlong` | Nonzero, four-byte-aligned IN; nonzero length divisible by four, at most 64 KiB; U32 pattern | Actual SDK matching-word count, bounded by length/4; guest bytes unchanged |
| `NtQueryVirtualMemory` | Readable same-image target and writable aligned 28-byte OUT; U32 region type; types 1/2 refused | Zeroed scratch; commit only on SDK success; existing `STATUS_INVALID_PARAMETER` propagates without output commit |
| `RtlInitializeCriticalSection` | Readable/writable aligned 28-byte INOUT | Preserve input in scratch, commit selected SDK writes only after success; original PPCContext remains intact |

Wide guest addresses/lengths are validated before narrowing. Every full span must
fit exactly one readable PE section and one exclusively owned committed image
allocation. OUT/INOUT additionally require the PE write flag and page write
permission before invoking the SDK. Cross-section, wrapping, empty, oversized,
misaligned, foreign-image and out-of-image arguments are refused. RO IN and an
exact section-end word are accepted. RO OUT/INOUT are refused. Unloading blocks
new entries and new imports while outstanding leased copies may finish.

The implementation uses a host snapshot, an owner-tagged SDK scratch allocation,
the actual SDK import, a checked scratch readback and a final full-span checked
image copy. Guest big-endian bytes pass through unchanged; scalar results are
explicit U32 values. No raw guest backing pointer reaches the observer or API.
No scratch address escapes through registers or committed pointer fields for
these three reviewed implementations. Their synchronous, nonretaining behavior
is the evidence boundary; this is not a generic pointer-marshalling facility.

Exceptions, callback failures, invalid SDK status and failed final protection
checks unwind scratch and call leases without committing that output. Rollback
is per import, not a transaction over all earlier imports or a generated entry.
An earlier successful query remains committed if the later critical-section
call fails. The final copy independently validates all pages under SDK locks
before writing any byte.

Image adapters serialize across bindings using an image-transaction mutex and
the checked-memory facade mutex. Nested pointer transactions and callback
reentry fail before taking a conflicting lock. Same-thread nonblocking facade
release reports the existing host `busy` result. Arbitrary raw SDK writes,
Reset/Restore, MMIO reconfiguration and destruction still require trusted
quiescence; this is not a native C++ sandbox or a global linearizability claim.
These fixed XEX sections and heap pages are 64 KiB, on a 16-KiB host. Iterating
4-KiB metadata boundaries does not create independent 4-KiB host protections.
The earlier 4-KiB guest / 16-KiB host raw-access limitation remains; complete
guest memory isolation is not established.

## Cooperative lifecycle and ownership

[CooperativeStop](../../runtime/cooperative_stop.h) exposes explicit atomic
states. It does not cancel a running SDK import or throw through guest frames.
A deterministic host observer requests stop while a generated caller is paused
inside scratch processing or immediately after an actual SDK return. Releasing
the observer permits import readback/commit and caller completion. The bounded
synthetic body then observes its safe boundary and returns. No later entry is
admitted by that body after the boundary. The `stop-import` group pauses at
`sdk_returned`, while the admitted import, scratch and leases are still active.
It verifies that call's completion/lifetime; timing inside the SDK function's
instructions is not independently controlled or established.

| Stage | Observable invariant |
| --- | --- |
| Running / requested | Thread, image, TLS/PCR, active import, birth lease, call lease and scratch remain live |
| Safe boundary | Current generated body/import scopes have returned or reached a supported host error boundary; duplicate request/boundary is refused |
| Exited marker | Original body and checked scopes have unwound; marker immediately precedes actual SDK `Exit` |
| Joined | Actual native wait/join succeeds before host releases XThread owners |
| Reclaimed | Object-table handle and final caller reference released; TLS/PCR gone; stack already gone after native completion |
| Unloaded | All image capabilities/bindings released; actual SDK unload succeeds; image pages absent |

Callback/import completion precedes observing the safe boundary when a request
arrives inside an import. This ordering preserves synchronous import ownership.
The exited marker alone does not establish native completion; join does.
Scheduling and safe-boundary placement are host-authored for bounded original
bodies. **PARTIAL:** no in-instruction cancellation, arbitrary generated loop
polling, guest exception dispatch/unwind or title scheduler is supplied.

| Resource | Owner and final release |
| --- | --- |
| Image section reservations | XexModule tagged allocations; failed load rolls back; unload refuses outstanding leases |
| Guest XEX header copy / processor module | UserModule, freed by final legitimate destruction; an image unload need not destroy a retained UserModule |
| Import admission | Retained UserModule plus fresh lease for each call; actual import membership/signature/context checked again |
| Binding and pointer capabilities | XexBinding lease and explicit pointer birth lease pin the image; release only after body use ends |
| Scratch / per-call lease | RAII through synchronous SDK return, callback and commit; balanced on error paths |
| Stack | Native XThread execution ownership; gone after successful wait/join |
| TLS/PCR | XThread final ownership, retained through join; module TLS lease lasts through final TLS/PCR free |
| Call references | Nested scopes/Admission hold legitimate owners until unwound; no forced counter reset or live-thread deletion |

The fixtures verify distinct TLS/PCR for four concurrent callers, eight complete
start/stop/reload cycles, a retained pointer capability preventing unload after
join, and stale/closed admission before and after unloading. Image and ordinary
virtual page counts return to baseline only after all legitimate module/header
references leave scope. Adapter counters measure capability and call-lease
acquisitions/reclaims; SDK internal lease counts are not directly exported.
Successful SDK unload independently requires zero active image leases.

An inner 15-second join bound and outer 30-second process-group watchdog prevent
unsafe stack unwinding across live workers. A deliberately failed 10-ms join
uses expected host exit 70, preserving the Phase 16 fail-stop contract. It proves
process containment, **not** orderly in-process TLS/image reclamation. No foreign
termination, live XThread deletion or protection bypass is used. The observer
rendezvous is separately bounded at 10 seconds.

## Regression evidence

All **518 core groups/checks** pass: 450 ordinary groups (including six expected
join fail-stops) and 68 native fault/capture/recovery checks. The latter include
44 expected SIGBUS, three SIGSEGV and nine SIGABRT exits, plus 12 controlled
capture/recovery outcomes. There are **zero final failures, unexpected signals,
timeouts or sanitizer findings**. Dangerous children use independent monotonic
process-group supervision, with no indefinite waits or sanitizer recovery.

| Profile | Ordinary groups | Fault/capture/recovery | New groups | Final failures |
| --- | ---: | ---: | ---: | ---: |
| Debug | 150 | 23 | 14 | 0 |
| Release | 150 | 23 | 14 | 0 |
| ASan/UBSan | 150 | 22 | 14 | 0 |

Per profile, the new suite's 13 ordinary lifecycle children report exactly:

- 672 main assertions and 1,778 worker observations.
- 58 generated entry invocations and 63 controlled rejections.
- 1,083 checked reads + 841 writes = 1,924 completed generated accesses.
- 22 TLS/PCR lifetimes, 22 descriptor buffers and 22 cooperative stops.
- 149 scratch allocations/reclaims and 149 actual SDK returns.
- 162 per-call image leases/reclaims and 40 birth capabilities/reclaims.
- 19 successful module unloads, with image/ordinary-heap pages back at baseline.

All measured allocation, TLS, scratch, call-lease and capability balances are
zero outstanding at the normal child endpoints. The fourteenth child is the
expected join exit 70 and deliberately has no orderly-reclamation counters.
The existing Phase 16 suite supplies three more expected join fail-stops across
the profiles. No failed-join resource lifetime is mislabeled as normal cleanup.
SDK-internal admission/Scope/TLS image leases are additionally checked by final
Unload success; their individual internal counters are not exported.

| Profile | Generate + regenerate A/B (s) | Compile generated + harness (s) | New runtime children (s) | Runtime peak RSS (bytes) |
| --- | ---: | ---: | ---: | ---: |
| Debug | 0.471701 | 10.241210 | 2.512551 | 29,835,264 |
| Release | 0.125625 | 10.968021 | 0.796374 | 24,248,320 |
| Sanitized | 0.472878 | 12.937724 | 1.804487 | 68,075,520 |

These durations sum independently supervised child wall times, not a game or
console benchmark. New-suite compiler-inclusive peak RSS is 367,443,968 /
385,417,216 / 458,031,104 bytes. All core inner children total 108.501570 /
97.833450 / 125.410531 seconds. All supervised workflow stages, including
separate controls, total 375.395557 seconds; their largest measured peak RSS is
995,672,064 bytes in the debugger reference control, not guest execution.

Separate passing controls are not added to the 518 core count: 27 original
foundation/SDK-leaf groups, 32 Python unit tests, 33 import-source audit records,
the 33-entry signature matrix and the debugger reference trace
`[2, 2, 2, 2, 1]` with one destructor. Each of three VMX profiles characterizes
212,068 scalar/vector cases. Recorded infinity/NaN/overflow differences remain
expected characterization results; Xbox hardware equivalence is UNKNOWN.

All three codegen runs and forced regenerations preserve emitted file hashes.
Source, object, executable, image, manifest and SDK-library hashes are checked
again against final artifacts. The 70 compiled memory sites across A/B are
mediated; the new integration executes A's 35, while the Phase 16 regressions
execute both. This is not coverage of private game-generated code.

Host: macOS 15.7.7 ARM64; Apple clang 17.0.0 (clang-1700.6.4.2), CMake 3.31.6,
Ninja 1.11.1.git.kitware.jobserver-1 and Git 2.50.1. Clients use O0 Debug / O2
Release and O2 ASan+UBSan; the reused Sanitized SDK uses O1 with no recovery.
System frameworks, CommonCrypto, debugger and handwritten assembly are outside
full sanitizer instrumentation. No ThreadSanitizer or Xbox hardware claim is
made. Compiler FPCR operand-width warnings are retained; no new SDK fix is
inferred from them.

| CLI profile | SHA-256 |
| --- | --- |
| Debug | `88ec5114eaebed3f7a89e52fb661e3452a73a66917b7126bfa8236cc74fb779f` |
| Release | `c8e1e6056a1793377b750745279af4dbe1f5ef1c02fdb4ec26642eea8baf97e3` |
| Sanitized | `a2cd6771a92cfdd8a4889ef83194891b8ba383ef58ce6a59db33046a8c15be8a` |

## Development failures retained privately

An initial compile rejected assignment to the SDK's deliberately nonassignable
move-only ImageLease; construction now uses `emplace`. Early fixture oracles
checked page reclamation before remaining module references died, and expected
different `NtQueryVirtualMemory` region metadata. Corrected assertions use the
actual audited SDK contract and final-owner lifetime. No ownership assertion is
removed from final global reclamation checks.

An exploratory protection test protected the entire 64-KiB data section, causing
the original generated wrapper's own checked write to fail before the intended
pointer import. The first test driver then attempted an invalid stop transition
and aborted. Two such exploratory SIGABRT exits and failed development results
remain private. Neither was a timeout or a final regression result.
The final protection preflight probe invokes the actual admitted SDK path
without first writing through the protected wrapper; RO section cases still use
the real generated caller. Unexpected worker errors are counted as failures and
unwind to a supported host boundary before join. Final results are reported
separately; historical failures are not counted as final passing tests.

## Readiness and remaining gates

**A — CONDITIONAL GO for broader original synthetic integration.** Extend explicit
safe-boundary placement to bounded generated loops and independently justified
call contracts. Preserve fail-closed signatures, lease pinning and final-owner
reclamation. No safe copy-batching or title cancellation claim is made here.

**B — NO-GO for private game-code execution.** The entry matrix still denies 24
imports. Private generated memory paths remain uninstrumented. Atomics, partial
vectors, MMIO, instruction fetch and typed-vector ABI remain unsupported.
Guest exceptions/unwind, general relocation, title TOC/TLS/import/stop contracts
and Nintendo Switch/Horizon runtime integration remain unresolved. These new
fixtures justify no proprietary execution experiment or game compatibility
claim. Overall port feasibility remains PARTIAL; Switch behavior UNKNOWN.

## Commands and publication

[The new suite recipe](../../tests/image-pointer-shutdown/README.md) and
[the full workflow](../../tests/xex-binding/workflow.py) use private output roots:

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$TESTS"
python3 tests/image-pointer-shutdown/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# Add --sanitize only for the Sanitized SDK.
python3 tools/check_public_tree.py
```

The full workflow validates all nineteen patches and historical pristine
controls, then reruns Phase 5–16 and the new Phase 17 suite. C++ generation is
repeated and source hashes compared; SDK/tool/compiler versions, artifact/source
hashes, exit codes, output, RSS and timing stay in private results. All generated
C++ is unchanged. Binary object bit reproducibility is not claimed.

Publication contains only reviewed original source/tests and English findings.
Public CI checks the allowlist, documentation links, encoders, import policy,
watchdog and dependency-free foundation O0/O2/Sanitized tests. It does not fetch
an SDK or reproduce the local SDK-linked integration suites. No GitHub Release
or game build is created.
