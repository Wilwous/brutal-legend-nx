# Feasibility — Phase 16

**Current Phase 16 gate (2026-10-23): VERIFIED for the exact original fixed-base synthetic XEX/codegen binding.** A single workflow constructs two XEX images, runs production ReXGlue codegen without function hints, compiles unchanged emitted C++, binds actual image/TLS/import metadata and executes through the real patched SDK. All 35 reachable memory sites per image are checked. All 476 core checks pass across Debug/Release/Sanitized with zero final unexpected signals, timeouts or sanitizer findings. General relocation, full ABI and title integration remain PARTIAL; 24 imports remain denied. Further synthetic work is CONDITIONAL GO; private game execution is NO-GO. [Phase 16 scope, failures, measurements and reproduction](research/synthetic-xex-binding.md).

Historical baseline assessment: **2026-10-22 · v0.0.11-synthetic-execution**. Phase 11 passed its original synthetic Mac scope; overall port feasibility remains PARTIAL and proprietary execution remains NO-GO. The Phase 3 configuration-assisted analysis/generation PASS stands.

Extraction, XEX loading, configured analysis and private C++ emission are verified. Two selected game partitions previously compiled on Mac. Phase 4 executed isolated VMX fixtures and classified all 261 imports. Phase 5 implemented original checked memory/TLS components and an SDK alias/TLS/STFS patch, with deterministic O0/O2/sanitized regressions. Phase 6 diagnoses the Phase 5 native read-only write timeout as an SDK unclaimed-signal retry. Original PPC-context calls to 13 exports, real XThread/PCR/TLS and isolated fail-stop/recovery checks pass. Phase 7 completes a clean coherent patched SDK build and 137 original fixture/fault/supervision checks; complete ABI conformance remains unresolved. That milestone left packed XDBF alignment and allocation false-success blockers. Phase 8 adds bounded byte parsing, checked allocator arithmetic, owner-checked XamFree and explicit unsupported exceptions; current results are in the pre-entry report. Full linking, runtime correctness, execution and Nintendo Switch compatibility are not established. [Phase 3 results and exact limitations](research/rexglue-analysis.md) supersede the historical [Phase 2 timeout](research/xbox360-analysis.md).

| Subsystem | Evidence state | Engineering assessment |
| --- | --- | --- |
| Input/extraction | VERIFIED | GOD/SVOD and DLC/TU readable; hash-verified extraction and source preservation passed |
| Executable parsing | VERIFIED bounded host parser | XDBF alignment defect corrected with byte decoding and synthetic adversarial regressions; no guest execution |
| Analysis / generation | VERIFIED with configuration | GapFill completes; five checked function hints clear default validation failures; 48,371 bodies emitted |
| CPU semantics | PARTIAL / BLOCKED | Alias-only SDK patch removes reproduced corruption; numeric behavior unchanged. Canary, Xenia/ReXGlue and library documentation do not establish one verified hardware oracle |
| Runtime imports | VERIFIED source audit / PARTIAL conformance | All 261 mapped: 7 IMPLEMENTED, 184 PARTIAL, 61 STUB, 9 UNKNOWN; all 274 ordinals recognized including variables |
| Pointer imports | VERIFIED selected SDK contracts / PARTIAL Xbox conformance | Three bounded synchronous adapters use actual SDK implementations, copy-only guest access, capability admission, output/error checks and cleanup. Six scalar and three pointer signatures are scoped; 24 entry-matrix imports remain denied. Original emitted callers now traverse these contracts; production title binding and full requirements remain open |
| Runtime semantics | BLOCKED / UNKNOWN | Selected context/unwind/handler exports now fail closed; full guest exceptions remain incomplete; integrated STFS rejection tested; RtlRaiseException has a possible static entry path, actual execution/boot necessity UNKNOWN |
| Graphics | UNKNOWN / major unresolved gate | D3D9/Xenos path and shader archive names identified; no translated shader or correct frame |
| Audio/middleware | UNKNOWN / substantial work | FMOD/Havok/Bink/Lua evidence, XMA imports; 157 banks lack standard initial FSB magic; codecs/active paths unproven |
| Input | INFERRED reusable foundation | XamInput and libnx reference adapters exist; no game input test |
| Filesystem/saves | UNKNOWN | File/content imports and archives identified; streaming/persistence behavior not tested |
| Memory | VERIFIED checked subset / PARTIAL coverage | The real-SDK opaque-handle copy layer passes selected page/ownership/lifetime/endian/concurrency tests; private generated game accesses remain unmediated. Real PPC memory-export lifecycle/errors and integrated MMIO/recovery/fail-stop pass on the rebuilt SDK. Whole unowned host pages now fault; accessible sub-host-page slack remains a critical raw-access limitation. Guest aliases, raw generated accesses and Horizon behavior remain open |
| TLS | VERIFIED selected header path / PARTIAL general semantics | Actual original XEX header drives templates, per-thread blocks, PCR/r13 and the default title-process 64-slot bitmap; lifetime/rollback and final reclamation pass. Explicit independent-module binding does not define a general DLL/process TLS-index architecture. Fibers/APCs remain unverified |
| Performance | UNKNOWN | Tool RSS/timing is not game performance; no stock-clock FPS/resolution prediction |
| Host toolchain | VERIFIED for limited checks | Pinned macOS ARM64 CLI completed emission; two partitions compile to objects; full compiler/link closure UNKNOWN |
| Target build/deployment | UNKNOWN | Private game C++ exists; no complete host game binary, ARM64 Horizon build, NRO or hardware test |

## Phase 11 synthetic execution review

**VERIFIED supported fixture:** actual pinned CLI generation of nine original functions / 36 instructions, compilation of unchanged emitted code and real patched runtime execution. An original fixture loader validates code/RO/RW sections, one relocation type, lifetime and entry/call policy. Two real XThreads exercise mediated native-stack/PCR/TLS accesses and scalar/vector operations. Patch 0015 corrects the checked-copy heap restriction for ordinary virtual 64-KiB stack pages while retaining physical/XEX refusal and unsupported fetch.

**VERIFIED recovery:** the Mac native-exit path stranded a local retained object reference. Patch 0016 unwinds to the owned thread boundary before native completion, then releases its matched execution handle. The original teardown assertions pass unchanged in Debug/Release/Sanitized: PCR/TLS reclaim only after the last legitimate owner. Native-creation failure rollback/callback ownership, explicit cooperative exit, actual ExTerminateThread, early caller release, repeated cycles and simultaneous threads pass. All 263 core checks pass, with no unexpected native faults/timeouts or emitted ASan/UBSan findings. Foreign asynchronous termination is explicitly refused before state changes; other platforms and guest exception behavior remain unverified.

**PARTIAL / BLOCKED:** the raw-test API does not test XEX loading, production analysis or arbitrary relocation. Host loans require explicit trusted lifetime control. Full ABI/TOC, title TLS/image ownership, required imports, unknown dynamic targets, atomics/partial vectors and guest exception dispatch/unwind remain unsupported or unverified. **Phase 12 NO-GO for proprietary execution.** [Results, root cause, recovery and exact next milestone](research/synthetic-execution.md).

## Historical Phase 10 instrumentation review

**VERIFIED selected prototype:** actual SDK decoding/emission/template rendering routes original scalar/finite-FP/full-vector memory fixtures through the checked owner/page/lifetime layer. Unsupported atomics, partial vectors, borrowed strings and unknown calls are refused. Three typed-vector signature forms are rejected at compile time. Default/checked execution and selected template compatibility are independently checked.

**PARTIAL / BLOCKED:** no private game output is instrumented or executed. Scalar macro eligibility is a static estimate; old raw vectors/direct calls require regeneration. Image/stack/TLS capabilities, instruction-fetch authorization, imported pointer/ABI contracts, dynamic targets and guest exceptions remain gates. The conservative backend has substantial measured overhead. **Phase 11 NO-GO for proprietary execution.** [Coverage, failure history, measurements and exact next milestone](research/generated-memory-instrumentation.md).

## Historical Phase 9 readiness review

**VERIFIED limited integration:** an opaque lifetime/byte-bound adapter copies through a new locked primitive on the real SDK virtual heap. Synthetic cross-page, owner/free, endian, MMIO and selected concurrency contracts are tested; an import regression fixes NtClose false-success after close. Thirteen patches are pinned; historical locks are unchanged.

**PARTIAL / BLOCKED:** no guest execute permission exists, so fetch is refused. None of the 791,353 lexical generated load/store sites is mediated. The direct entry closure has 33 imports and 21 indirect calls; unchecked pointer imports, UI false-success, exception dispatch/unwind, typed vectors and title TLS/TOC remain gates. Checked fixture success cannot establish a safe game-entry scope. [Exact contracts, import matrix and final measurements](research/guest-memory-entry-readiness.md).

## Current blocker register

| ID | Status | Blocker and required evidence |
| --- | --- | --- |
| B01 input | Cleared for supplied data | GOD rather than original ISO; 38 source game files hash-preserved |
| B02 extraction | Cleared for supplied data | Internal hash checks/bounds/extraction passed; authenticity/signature provenance remains UNKNOWN |
| B03 identity/modules/update | Partly cleared | Main XEX metadata known; `live.dll` has changed headers with identical stored payload; source authenticity not established; TU not applied |
| B04 function analysis | Cleared with five metadata hints | Default Validate fails on five branch targets; explicit bounded spans permit Validate and emission. Exhaustive control-flow coverage remains unproved |
| B05 memory/exceptions | Unresolved | Diagnostic boundary rejects unsupported exception operations; installed SDK void stubs remain. The 45 s timeout is diagnosed as unclaimed-signal retry; isolated dispatcher fail-stop/PC-resume pass under group watchdogs. Selected coherent MMIO/dispatcher integration passes; selected explicit logical pointer checks pass; new bounded owner-checked copies cover original fixtures only; existing raw-access integration, guest unwind and full signal safety remain unresolved |
| B06 graphics | Unresolved | Inventory actual `.fxo` shaders and render features; validate PM4, formats/resolves, readbacks and invalidation before adapting a renderer |
| B07 runtime/media/assets | Source correction / integration unresolved | Integrated real PPC exports return STFS NOT_IMPLEMENTED with unchanged output sentinel; installed SDK remains unchanged. Other false-success stubs, required paths, banks, codecs and saves remain UNKNOWN |
| B08 memory/performance | UNKNOWN | Measure guest/game/renderer memory and representative stock-clock hardware workloads after correctness milestones |
| B09 build/dependencies | Partly cleared | Native analysis/codegen, two sample object compiles and clean patched SDK builds pass; checked allocation/ownership regressions are added in Phase 8; complete game linking and Horizon SDK/Mesa closure remain unvalidated |
| B10 deployment | UNKNOWN | NRO/application-mode permissions, launch/exit, controller/audio, persistence and dock transitions untested |
| B11 VMX128 semantics | Alias source fix tested / numeric hardware UNKNOWN | 212,068 invocations per patched build: zero alias differences. Numeric comparison results retained; Canary uses different rounding/denormal/range behavior. No verified special-value hardware corpus |
| B12 measurement supervision | Confirmed process defect | Second run reports 1,096.56 calendar seconds vs 77.033 monotonic seconds. The intended 300-second calendar bound was not enforced; fix sleep-inclusive and calendar deadlines before further analysis |

The PE declares 14 sections and SizeOfImage 15,750,656 bytes; the loaded XEX allocation is 14,090,240 bytes and the SDK's analysis view excludes the out-of-range discardable `.reloc` section. These differing header/allocation values require preserving their meanings, not using either as total game RAM or declaring the input corrupt without further evidence.

The complete [VMX methodology/results](research/vmx128-validation.md) and [261-function matrix, exception/STFS findings and reference comparison](research/runtime-audit.md) define what is confirmed. No fix was applied to the installed SDK/runtime; isolated source corrections are described below. No title-specific patch or reference implementation was copied.

The [Phase 6 report](research/ppc-execution.md) records 39 PPC groups, 12 final fault checks and 6 supervision checks with zero final failures, plus two diagnostic timeouts. It distinguishes real export/object behavior from whole-runtime integration and left full ABI, unchecked pointers and invalid TLS-free success unresolved at that milestone. Phase 7 corrects invalid-free propagation and selected ABI cases; unchecked pointers remain a blocker.

The [Phase 5 report](research/runtime-foundation.md) separates original standalone contracts, source-only SDK corrections and the unchanged native SDK probe. No full game or Switch execution result follows from these checks.

## Phase 8 safety decision

**VERIFIED within tested host contracts:** bounded XDBF byte parsing; checked allocation-size/alignment arithmetic; capacity and returned-region checks before zeroing; heap-maintained XAM ownership; selected foreign/stale/double-free rejection; explicit fail-closed exception exports and integrated memory denials. All builds/tests use the new coherent SDK. No global sanitizer suppression or installed SDK modification is made.

**PARTIAL / UNKNOWN:** host-page protection does not isolate individual 4 KiB guest pages on this 16 KiB host. Logical guest-range validation is not an allocation lifetime pin and is absent from raw generated accesses. Typed vectors, full Xbox ABI/error codes, same-owner pointer ABA, save-state owner serialization and complete guest exception dispatch/unwind remain unvalidated or unsupported. Successful codegen/loader/fixture behaviour cannot establish entry readiness.

**NO-GO for proprietary execution. Exact next task: original synthetic XEX/image ownership and required-import admission, with supported ABI/TOC/TLS, executable-target and cooperative title-stop policies, before a separate private-execution readiness review.** [Phase 11 evidence and gate](research/synthetic-execution.md).

## Platform and resource constraints

The development host is macOS ARM64 with 8 GiB physical RAM and about 28 GiB free disk space at discovery. Python, Apple Clang and Docker were available; host CMake/Ninja, Rust/.NET and a Switch toolchain were not found on the checked PATH. Those were initial PATH findings. Phase 7 uses a private pinned CMake/Ninja environment and native Xcode SDK build; no global SDK or build-tool install was replaced.

Peak tool RSS during Phase 3 was **670,859,264 bytes (639.78 MiB)**. This does not bound a full compile, linker or game runtime. Phase 3 retained about 190 MiB of generated output privately. The anomalous calendar duration prevents treating its monotonic timing as a wall-clock performance estimate.

Linux was **not required for the completed analysis**. A later Horizon baseline should use an isolated, pinned devkitPro/devkitA64 container with fixed SDK/Mesa commits and explicit resource limits. Select the image digest/tool versions when that experiment is scheduled; no reproducible target build is claimed now. The 8 GiB host and current free space must be rechecked before attempting large generated-code builds.

## Decision boundaries

No compiler redesign or migration is justified. The default discovery failures have a demonstrated configuration remedy. XenonRecomp has the same unexpected float16_4 guard and no demonstrated advantage for this case. Original fixtures now validate an alias-only source correction; numeric changes remain unjustified by conflicting software evidence. The host fault cause and tiny original PPC/export bridge now have bounded evidence. Phase 7 supplies one coherent patched SDK and selected callback/r2, 64-bit argument and failure checks. Phase 8 fixes the characterized XDBF alignment and allocation overflow paths. Next integrate checked guest accesses and establish fail-closed critical entry/import contracts before any private entry trace; retain guest exception, STFS functionality, graphics/media and hardware gates.

No silent force flags, automatic `live.dll` inclusion, title-update application, API stubbing or renderer optimization is justified by loader success. A complete generated C++ set would still require compile/link checks, behavioral tests and separate Switch hardware verification. There is no playable build and no Release.
