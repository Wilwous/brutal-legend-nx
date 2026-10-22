# Phase 11 — synthetic execution module

Baseline: `v0.0.10-memory-instrumentation` (`48237fb`). Initial investigation: 2026-10-21; recovery/final verification: 2026-10-22. ReXGlue v0.10.0, pinned `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with the ordered sixteen-patch [synthetic lock](../../tools/sdk/synthetic-lock.json). All source, builds, diagnostics and generated output stay in a new private workspace. No global installation is modified. No proprietary code is compiled or executed in this phase.

## Generation format and scope

**VERIFIED:** the independently authored [encoder](../../tests/synthetic/fixture.py) produces 144 bytes / 36 big-endian PPC instructions, nine `test_` functions, a relative nm map and an assembly listing. The actual matching-profile CLI consumes these through:

```sh
"$REXGLUE" recompile-tests --bin-dir "$INPUT" --asm-dir "$INPUT" --output "$GENERATED"
```

This is upstream `TestModule` → `BinaryView` → `CodegenContext` → `AnalyzeTestBinary` → whole-function `FunctionNode::emitCpp` → upstream test templates. It uses the real decoder and dispatch to builders; no handwritten imitation of emitted functions, selected-builder loop or editing of the resulting translation unit is used. The complete generated function TU and PCH are compiled with `REX_CHECKED_ACCESS_HEADER="runtime/generated_access.h"` and linked to the matching patched SDK. Independent original oracles execute the resulting functions. The generated Catch2 case file is not the oracle and is not run.

**PARTIAL container integration:** the pinned normal `CodegenContext::Create(config, runtime)` calls `Runtime::LoadXexImage`; it is not a generic raw/ELF loader. The supported raw-test path supplies one executable analysis section, map-defined single-block functions and resolved direct call edges; it does not exercise production GapFill, PE/XEX loading, import/exception discovery or relocation. No synthetic signed/encrypted XEX is fabricated. The smallest supported fixture is used honestly: actual end-to-end **raw-test generation and synthetic execution**, with a separate original fixture loader. This does not establish end-to-end XEX loading or title startup.

The original sidecar describes a 48-KiB image with code, read-only data and writable data at host-page-separated offsets, plus one ABS32BE data-pointer relocation. Its section/relocation metadata is passed to the fixture loader through a generated metadata header. Analysis PCs are separate from runtime allocated guest addresses. Relative direct calls resolve to generated native functions; indirect targets are explicitly registered at loaded guest addresses. LR values emitted by the CLI retain analysis coordinates. No arbitrary rebasing, executable self-inspection or dynamic fetch is supported.

## Module loading

**VERIFIED in the original fixture loader:** validate sizes/alignment with subtraction/64-bit arithmetic, reject overlapping sections, excessive image bounds, W+code, oversize initialized data, invalid/duplicate relocations and invalid/duplicate function registrations before admission. Reserve the complete image, commit/zero only declared sections, populate original bytes and apply the supported relocation before setting code/RO to read-only and data to RW. Gaps remain reserved/uncommitted. Ownership is released on setup exceptions or unload; stale checked handles and retained dispatch scopes cannot access released backing.

Entry admission requires a registered aligned function within initialized code bytes, not merely any readable address. Null, unaligned, interior unregistered, RO/RW, image-end and unrelated entries are refused. Execution dispatch uses the host allowlist with no SDK resolver fallback. **PARTIAL execute protection:** `code` is trusted fixture-loader policy, not an SDK page-table execute bit. The SDK still refuses instruction fetch. Code bytes are guest data; only reviewed AOT function pointers execute on the host. Setup failure cleanup is implemented; allocator exhaustion and every host allocation exception are not exhaustively injected or proven.

## Real stack, TLS and the 64-KiB correction

**VERIFIED:** two real guest `XThread` instances use their SDK-created stack, PCR and default tool-mode TLS storage. Their freshly created allocations are tagged by the original derived host fixture before execution; explicit non-owning scope loans grant only the stack interval, PCR bytes and TLS interval. The SDK locked copy rechecks reservation owner, all intersecting pages, commitment, protections and MMIO before copying. Loans never return a backing pointer, silently adopt foreign title memory or transfer deallocation ownership.

The first thread child failed on `CheckedMemoryResult::unsupported_heap`: real stacks use the ordinary virtual **64-KiB** heap while the Phase 9 copy primitive accepted only 4-KiB virtual pages. The fixture's fail-stop handler produced SIGABRT. A separate bounded diagnostic child confirmed the same error before any unchecked store. [Patch 0015](../../patches/rexglue-sdk/0015-checked-virtual-stack-copy.patch) permits 4-KiB **or 64-KiB ordinary virtual** pages under the unchanged locked owner/extent/commit/protection checks. XEX and physical heaps remain refused. No global protection bypass, original SDK workspace change or install is made.

The harness reserves a 256-byte linkage/caller area below the real stack top before entry; this is explicit synthetic setup, not evidence that normal title launch prepares a correct ABI frame. Generated `stwu`, `std`/`ld`, saved LR and scalar loads/stores use this actual stack. Alignment, back-chain, lower/upper range failures, stack restoration and release after thread exit are tested. TLS is initially zero; generated `lwz` through actual r13/PCR reaches real slot 0, writes distinct values, and the actual `KeTlsGetValue` export observes each thread's value. Foreign thread PCR/TLS and crossing TLS/PCR bounds are refused. **Stack, PCR and TLS teardown pass after the recovery correction below.** Invalid PCR paths are checked. Earlier suites retain TLS slot allocation/free/exhaustion and failure tests.

**PARTIAL lifetime contract:** loans require trusted host metadata, a pinned allocation identity and no external Terminate/Reset/Restore/teardown or MMIO reconfiguration. All scopes end before owner tags are removed and SDK Exit frees the stack. This is not automatic ownership integration for arbitrary XThreads, title image/TLS layouts, APCs, fibers or concurrent termination. Tool mode's default TLS layout is not the game's layout.

A draft boundary oracle incorrectly assumed one-past-TLS must be inaccessible; it could be the start of the same thread's allowed PCR. The corrected oracle tests a transfer crossing the end of a single loan. **VERIFIED limitation:** integer pointers do not retain C/C++ object provenance; a numerical address valid in another explicitly granted region can be accessible. No complete guest memory isolation or per-object capability claim follows.

## Calls, ABI and checked accesses

**VERIFIED selected scope:** generated `bl` and `bctrl` reach only registered context/base functions. Omitted direct callees, unknown/unaligned indirect targets, unbound execution and invalid module entries fail explicitly. An original indirect callback throws through the generated call chain; the outer handler observes the exact error and the partially updated stack, with no invented guest unwind/rollback. The host restores its saved context before SDK thread exit.

The entry oracle checks restored 64-bit r1/r14, r2/r13 and saved LR; nonzero FPR/vector sentinels survive the scalar call fixture. Arguments use selected GPR values and big-endian guest data. The full-vector fixture loads bytes, performs `vaddubm`, stores bytes and compares independent byte expectations on the main and actual XThread contexts. No typed-vector ABI is used. Existing three compile-negative vector signature forms remain explicit refusals. Complete register/CR/FPSCR, typed vectors, variadics, arbitrary callbacks, TOC initialization and Xbox ABI remain **PARTIAL / BLOCKED**.

The whole generated fixture contains five U32 load, seven U32 store, one U64 load/store each, one full-vector load/store each, two raw-address references, one partial-vector guard, one direct and one indirect call site. The raw sites belong only to the refused atomic/partial-vector functions. Preprocessing of the actual generated PCH verifies all ten used access/dispatch seams select checked or refusal helpers. The generated TU digest is unchanged by compilation/execution and identical across profiles. Every function and all 36 emitted instruction comments are checked against the original manifest/count gate.

Tests cover BE bytes, valid cross-4-KiB access, RO writes, inaccessible/uncommitted crossings without partial writes, freed pages, invalid addresses, code writes, RO data, and full vectors. Additional loans cross two real 64-KiB pages, then verify RO/decommit/release/foreign-owner failures and stale-owner refusal after address reuse. Passing a null arena base to generated functions prevents ordinary arena-relative data fallback in this selected corpus; it is not a native C++ sandbox.

**Known bypasses:** SDK setup and `KeTlsGetValue` use SDK pointers outside the generated seam, by explicit reviewed host contract. Native functions, mapped imports, arbitrary hooks, normal title dispatch, default uninstrumented builds, raw helpers outside this corpus, setjmp/longjmp and exceptional control flow are not universally mediated. The original game-derived output is unchanged and remains uninstrumented. A source census is not a proof for all possible helpers or host code.

## XThread lifetime root cause and recovery

**VERIFIED on macOS ARM64:** the initial fifteen-patch run completed 60 generated-execution/stack/TLS observations, released stacks, joined threads and closed caller handles, but retained four PCR/TLS reservations for two threads. Derived-thread and automatic exit-marker destructor counts were both zero. The three final pre-recovery reclamation children returned exit 1; the saved result was 218 passing core checks and three critical failures, not an expected refusal.

A small original [reference probe](../../tests/synthetic/reference-probe.cpp) and [bounded read-only LLDB observer](../../tests/synthetic/reference-trace.py) inspect the actual SDK object's private atomic counter without a new SDK inspection API, layout reinterpretation or reference manipulation. Source tracing distinguishes object-pointer references from the object table's handle reference count:

| Checkpoint | Before fix: pointer refs | After fix: pointer refs | Legitimate ownership |
| --- | ---: | ---: | --- |
| After construction | 2 | 2 | Initial caller reference plus object-table reference |
| After Create | 2 | 2 | Same pointer refs; Create increments the existing handle count from 1 to 2 |
| Executing, before Exit | 2 | 2 | Caller plus table; execution handle keeps the table entry alive |
| After native Join | 3 | 2 | Old extra local Exit owner was stranded; corrected execution lease has ended |
| After caller closes handle | 2 | 1 | Table entry removed, only legitimate caller remains after correction |
| After final object_ref reset | 1, leaked | 0, destroyed | PCR/TLS/scratch/guest-object reclamation occurs in the SDK destructor |

`object_ref(new XThread(...))` adopts the constructor's initial reference; it does not add another retain. `ObjectTable::AddHandle` adds one pointer reference. `RetainHandle` increments a handle counter, not the object pointer counter; the table's one pointer reference remains until the last handle count reaches zero. `KernelState::RegisterThread` and current-thread TLS store raw pointers. `GetThreadByID` returns an explicitly retained temporary reference, which is independently tested as a legitimate owner. Wait/join is synchronization, not an implicit release of caller ownership; POSIX `post_execution` already performs `pthread_join`.

The old `XThread::Exit` adds `auto self = retain_object(this)` and reaches `Thread::Exit` → POSIX `Terminate` → `pthread_exit`. On this measured Mac, pthread exit does not destroy those C++ automatic objects. The live counter rises from 2 to 3 across Exit and stays there after join; the independent exit marker is not destroyed. This source/counter/destructor agreement confirms the stranded Exit reference, rather than assuming it from retained allocations alone. It prevents the final XThread destructor, which owns PCR/TLS reclamation. No registry reference or missing second join explains the observation.

[Patch 0016](../../patches/rexglue-sdk/0016-xthread-owned-exit.patch) corrects ownership at the SDK boundary:

- On macOS, `Exit` raises a private host completion request, caught only at the owned XThread entry boundary. `Execute` and callback scopes unwind first while stack/PCR/TLS backing remains live. Normal return follows the same finalization path.
- The boundary runs `FinishExit` after execution scopes have ended: guest exit status/process unlink/APC rundown and stack teardown complete before native termination. The completion request is host control flow, **not Xbox guest exception dispatch**.
- A pthread cleanup handler releases exactly the execution handle acquired by Create. It runs after native completion is signaled and before join returns. The code never forces deletion, guesses a residual count or decrements an unowned reference. If a caller/lookup/copy still owns the object, PCR/TLS remain live until that final owner releases it. With no caller owner, the execution lease protects the object through completion.
- Same-thread `Terminate` uses this owned Exit path. **Foreign asynchronous XThread termination is explicitly unsupported on macOS**, before guest status, stack, callback or handle state changes. It cannot safely preserve the callback/stack contract through the old forced-cancellation path. The refusal is a host error, not a fabricated Xbox status or a supported title-termination implementation.
- Native creation failure balances the execution handle, and process linkage is published only after a suspended native thread was successfully created. Failure also destroys unconsumed native start-data/callback storage. Outstanding Mac suspend semaphores close on destruction when wait cleanup has not already consumed them.

The source patch changes three SDK files. The supported macOS exit path is tested; Windows/Linux/Horizon exit semantics are **UNKNOWN**. Native cancellation bypassing XThread, fibers, APCs that themselves request exit during rundown, uncaught arbitrary host exceptions and unreviewed catch-all handlers are outside the verified scope. No general guest exception/unwind compatibility is claimed.

## Lifetime regression evidence

Fourteen original [lifetime groups](../../tests/synthetic/lifetime.cpp) add normal Exit, normal return, self-Terminate, real `ExTerminateThread` export, a handled original callback error, exit from an active callback, stack allocation failure, native creation failure, native callback-storage ownership, copied/lookup owners, caller release before execution completes, foreign-termination refusal, sixteen repeated cycles and four simultaneous threads. These are counted separately from the six original generated-module groups.

An automatic frame verifies that stack/PCR/TLS are still accessible during C++ unwinding and that callbacks have ended before destruction. Tests check guest exit status, initial/final process thread counts, registry removal, stack release at exit, PCR/TLS retention while a legitimate owner exists and reclamation after the last owner. A gate holds an active callback while another thread attempts unsupported termination; all guest status/backing state remains unchanged. A bounded wait times out without destroying backing. A caller may close its handle and drop its object reference before completion; the test then joins the independent pthread handle once and verifies final reclamation without dereferencing an expired SDK object.

The native-failure fixture loads an original [interposition library](../../tests/synthetic/create-failure.cpp) into **only its watchdog child** and arms it after runtime setup. Actual SDK `pthread_create` returns EAGAIN; the wrapper reports its existing NO_MEMORY status. The same original probes against the unchanged fifteen-patch SDK reproduced a process count increase from 0 to 1 despite creation failure, and a retained native callback capture (destructor count 0). Final probes observe unchanged process count, destroyed captures and one suspend-semaphore close per failed native factory. No global SDK/library/environment override is installed.

**VERIFIED:** the original critical assertions are unchanged. In every final profile, the generated-thread fixture reports two thread destructors, two automatic exit-marker destructors, zero retained PCR/TLS regions and all 60 thread observations complete. The debugger's final trace is `[2, 2, 2, 2, 1]`, followed by exactly one destructor when the last caller reference is reset.

## Explicit unsupported behavior

| Operation | Observation / classification |
| --- | --- |
| `lwarx` reservation/atomic | VERIFIED generated raw-address seam throws before returning a pointer; BLOCKED atomic semantics |
| `lvlx` partial vector | VERIFIED generated guard throws before body access; BLOCKED partial-vector semantics |
| MMIO-targeted generated load | VERIFIED aperture refusal; no emulated device operation |
| Guest instruction fetch | VERIFIED SDK `unsupported_fetch`, output unchanged; BLOCKED dynamic fetch/execute permission |
| Requested guest exceptions | VERIFIED original loader refuses the explicit requirement; generation gate excludes exception/nonlocal helpers; BLOCKED guest dispatch/unwind |
| XEX/physical heap loans | VERIFIED `unsupported_heap` remains; no title/physical memory support inferred |
| Typed-vector signatures | BLOCKED, expected compiler refusals retained in prior suites |

Only the two newly generated atomic/partial variants above are exercised here; Phase 10 retains four atomic and four partial-vector refusal variants. No claim is made that all exception-bearing instruction streams or all MMIO paths are recognized. No guest exception opcode is deliberately executed; existing isolated exception-export fail-stop regressions remain separate.

## Final regression evidence

**VERIFIED: 263 passing core checks, zero failures.** There are 195 ordinary passing groups (65 per profile) and 68 expected fault/recovery checks. Phase 5–10 retain their 203 passing core checks against the new SDK; Phase 11 supplies six original generated-module and fourteen additional lifetime groups per profile. The new suite has 546 assertions plus 60 generated-thread observations per profile. No assertion was weakened to clear the prior reclamation failure.

| Final profile | Ordinary groups pass / fail | Expected fault/recovery pass / fail | Execution-child seconds, summed | Peak execution-child RSS bytes |
| --- | ---: | ---: | ---: | ---: |
| Debug | 65 / 0 | 23 / 0 | 15.795129 | 34,717,696 |
| Release | 65 / 0 | 23 / 0 | 8.279902 | 22,265,856 |
| Sanitized | 65 / 0 | 22 / 0 | 12.764645 | 63,078,400 |

**VERIFIED:** zero final unexpected native faults, timeouts or emitted ASan/UBSan findings. Expected historical signal children are 44 SIGBUS, three SIGSEGV and nine SIGABRT (56 signals); the other twelve checks recover or use expected diagnostic exits. The ASan-owned plain default-signal case remains excluded only in Sanitized, as previously documented. Unsupported ABI/raw host-page slack observations remain blockers, not passing compatibility features.

Including compilation, generation, negative compiles, object sizing and preprocessing, summed child seconds / peak individual child RSS are:

- Debug: 57.360929 seconds / 352,567,296 bytes.
- Release: 49.494587 seconds / 372,932,608 bytes.
- Sanitized: 60.761899 seconds / 437,387,264 bytes.

RSS is wait4 per-child accounting, not concurrent process-tree or game memory. Execution timing includes generation/emitter tools but excludes compilation, negative compiles, preprocessing and size probes. These diagnostic measurements are not game performance estimates.

Separately: 27 historical component/source-control groups; three VMX characterization suites (212,068 original invocations each); six watchdog checks; six static-audit units; six encoding/sidecar units; and one read-only actual-reference debugger trace pass. Historical source controls are not an old runtime loaded beside the new library. VMX hardware equivalence and complete ABI remain unverified. Typed-vector compile-negative forms remain explicit refusals.

All sixteen source patches, submodule revisions and dependency pins were verified against pristine source in the new private workspace. SDK headers/runtime/codegen/CLI are staged per matching profile, and the complete Phase 5–11 matrix uses only the final matching outputs. The last source-verified SDK rebuild was incremental on that workspace's caches:

| Final verified incremental SDK rebuild | Monotonic seconds | Peak build-child RSS bytes |
| --- | ---: | ---: |
| Debug | 1.790445 | 340,262,912 |
| Release | 1.659105 | 222,101,504 |
| Sanitized | 2.472325 | 454,328,320 |

Cache-populating draft builds took 103.580727 seconds for Debug, 129.011818 for Release and 214.955639 for Sanitized; they are not represented as the final source verification. Final profile runtime/CLI hashes and all bounded records remain private. The actual original generated TU is identical across profiles and to the pre-recovery fixture: SHA-256 `7ccd9893b5e2e898e16080e2740a70ec8dc6a0b870cf95aed8641f2936f88d9e`. The new runner records hashes of eight original fixture/backend sources; all match the final working tree.


Every generation, compilation, preprocessing, debugger and execution child uses the external 30-second monotonic process-group watchdog with capped private output, explicit exit status, core suppression and wait4 timing/RSS. SDK acquisition/configuration/build commands have separate bounded supervision and one builder per workspace. Debug fixtures use O0, Release O2 and Sanitized O2 with fully supported SDK C-family ASan/UBSan O1 instrumentation and no recovery. System frameworks and assembly remain outside sanitizer coverage. ASan/UBSan do not prove absence of resource leaks; allocation and destructor oracles are independently required.

Historical pre-recovery failures and diagnostic attempts remain private: instruction-comment counting/include-order mistakes; two SIGABRT children confirming the old unsupported stack heap; the invalid adjacent TLS/PCR boundary assumption; the three genuine reclamation failures; and a redundant extra join after the SDK had already joined. Recovery also preserves the initial ineffective executable-local interposer attempt (SDK creation succeeded, so it was rejected as a fixture failure), then the real injected pre-fix native failures. Draft SDK builds are retained separately; final measurements use the last verified sixteen-patch outputs. No failed test is relabeled, discarded or suppressed to obtain the final result. The historical allocation SIGBUS is not rerun against an unfixed runtime.

## Preservation and publication

**VERIFIED:** the original `default.xex` SHA-256 remains `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`. All 1,455 original XEX/config/generated size/mtime/SHA fingerprints and all 299 extracted-file hashes match. The installed SDK runtime library is unchanged. The fifteen pre-recovery patch hashes and historical locks are retained; all new SDK changes are confined to the new private workspace. No title update is applied, no original file is modified and no game code is executed.

**VERIFIED publication review:** all 25 changed/new files were reviewed; a temporary index passes the allowlist/text/link/patch-lock checks for 123 public text files. The original nineteen-file work-in-progress snapshot and useful diagnostics are preserved privately. Original fixtures, backend and the first fifteen patch hashes are unchanged; the full sixteen-patch lock matches the final SDK build. Only intended original source/tests, narrow BSD SDK patches/lock, CI metadata and English documentation are included. No proprietary content, generated body, private absolute path, raw log, dump or binary is staged. Publication uses the requested completion commit and tag only after these checks; no GitHub Release is created.

Only the original encoder/fixtures/loader, bounded runner, checked-scope change, narrow BSD SDK patch/lock, CI encoding units and English documentation are intended for publication. No original/generated proprietary code, asset, absolute private path, log, dump, binary or temporary file belongs in the allowlist. The generated synthetic code also remains private by repository policy. Publication checks are bounded text/allowlist checks, not a universal copyright or secret-detection proof. No GitHub Release is created.

## Phase 12 gate

**NO-GO for even narrow proprietary execution.** The raw fixture and original loader do not verify XEX image ownership, production loader relocation, title stack/TLS/TOC initialization, executable authorization, the required import contracts or unresolved indirect/exception paths. Existing private output is not regenerated with the seam. Typed-vector ABI, atomic/partial-vector support and guest dispatch/unwind remain blocked; observed failures are host errors rather than Xbox exception semantics.

**Phase 11 status: PASS within the original synthetic macOS scope.** Its completion does not clear Phase 12. **Exact next milestone:** original synthetic XEX/image ownership, supported relocation/execute policy and required-import/ABI/TOC/TLS admission, including a safe cooperative title-stop contract, followed by a separate private-execution readiness review. Synthetic execution does not establish game compatibility, boot or Switch readiness. Do not execute private game code as an automatic next action.

[Exact recipes](../../tests/synthetic/README.md) · [Architecture](../architecture.md) · [Feasibility](../feasibility.md) · [Roadmap](../roadmap.md).
