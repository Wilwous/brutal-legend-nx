# Feasibility — Phase 6

Assessment: **2026-10-20 · v0.0.6-ppc-execution**. Verdict: **PARTIAL; runtime-correct game execution remains BLOCKED pending semantic and runtime contracts**. The Phase 3 configuration-assisted analysis/generation PASS stands.

Extraction, XEX loading, configured analysis and private C++ emission are verified. Two selected game partitions previously compiled on Mac. Phase 4 executed isolated VMX fixtures and classified all 261 imports. Phase 5 implemented original checked memory/TLS components and an SDK alias/TLS/STFS patch, with deterministic O0/O2/sanitized regressions. Phase 6 diagnoses the Phase 5 native read-only write timeout as an SDK unclaimed-signal retry. Original PPC-context calls to 13 exports, real XThread/PCR/TLS and isolated fail-stop/recovery checks pass; coherent rebuilt-runtime and complete ABI conformance remain unresolved. Full linking, runtime correctness, execution and Nintendo Switch compatibility are not established. [Phase 3 results and exact limitations](research/rexglue-analysis.md) supersede the historical [Phase 2 timeout](research/xbox360-analysis.md).

| Subsystem | Evidence state | Engineering assessment |
| --- | --- | --- |
| Input/extraction | VERIFIED | GOD/SVOD and DLC/TU readable; hash-verified extraction and source preservation passed |
| Executable parsing | VERIFIED | Main XEX fingerprinted; encrypted/LZX image loaded natively with ReXGlue 0.10.0 |
| Analysis / generation | VERIFIED with configuration | GapFill completes; five checked function hints clear default validation failures; 48,371 bodies emitted |
| CPU semantics | PARTIAL / BLOCKED | Alias-only SDK patch removes reproduced corruption; numeric behavior unchanged. Canary, Xenia/ReXGlue and library documentation do not establish one verified hardware oracle |
| Runtime imports | VERIFIED source audit / untested conformance | All 261 mapped: 7 IMPLEMENTED, 184 PARTIAL, 61 STUB, 9 UNKNOWN; all 274 ordinals recognized including variables |
| Runtime semantics | BLOCKED / UNKNOWN | Context/unwind/handler stubs confirmed; isolated STFS rejection tested; RtlRaiseException has a possible static entry path, actual execution/boot necessity UNKNOWN |
| Graphics | UNKNOWN / major unresolved gate | D3D9/Xenos path and shader archive names identified; no translated shader or correct frame |
| Audio/middleware | UNKNOWN / substantial work | FMOD/Havok/Bink/Lua evidence, XMA imports; 157 banks lack standard initial FSB magic; codecs/active paths unproven |
| Input | INFERRED reusable foundation | XamInput and libnx reference adapters exist; no game input test |
| Filesystem/saves | UNKNOWN | File/content imports and archives identified; streaming/persistence behavior not tested |
| Memory | VERIFIED limited components / UNKNOWN integration | Checked bounded arena passes bounds/ownership/epoch/protection/endian tests. Real PPC memory-export lifecycle/errors pass; protected writes fault, but unpatched dispatcher retries indefinitely until watchdog kills its group. Isolated fail-stop is tested. Guest aliases, raw generated accesses and Horizon behavior remain open |
| TLS | PARTIAL | Real XThread/PCR/r13, selected PPC export calls, 64-slot process lifecycle/exhaustion and isolation pass; patched real-class bounds pass. Synthetic metadata only; game TLS, fibers/APCs and whole-runtime integration untested |
| Performance | UNKNOWN | Tool RSS/timing is not game performance; no stock-clock FPS/resolution prediction |
| Host toolchain | VERIFIED for limited checks | Pinned macOS ARM64 CLI completed emission; two partitions compile to objects; full compiler/link closure UNKNOWN |
| Target build/deployment | UNKNOWN | Private game C++ exists; no complete host game binary, ARM64 Horizon build, NRO or hardware test |

## Current blocker register

| ID | Status | Blocker and required evidence |
| --- | --- | --- |
| B01 input | Cleared for supplied data | GOD rather than original ISO; 38 source game files hash-preserved |
| B02 extraction | Cleared for supplied data | Internal hash checks/bounds/extraction passed; authenticity/signature provenance remains UNKNOWN |
| B03 identity/modules/update | Partly cleared | Main XEX metadata known; `live.dll` has changed headers with identical stored payload; source authenticity not established; TU not applied |
| B04 function analysis | Cleared with five metadata hints | Default Validate fails on five branch targets; explicit bounded spans permit Validate and emission. Exhaustive control-flow coverage remains unproved |
| B05 memory/exceptions | Unresolved | Diagnostic boundary rejects unsupported exception operations; installed SDK void stubs remain. The 45 s timeout is diagnosed as unclaimed-signal retry; isolated dispatcher fail-stop/PC-resume pass under group watchdogs. Coherent MMIO/dispatcher integration and guest unwind remain unvalidated |
| B06 graphics | Unresolved | Inventory actual `.fxo` shaders and render features; validate PM4, formats/resolves, readbacks and invalidation before adapting a renderer |
| B07 runtime/media/assets | Source correction / integration unresolved | Isolated real PPC exports return STFS NOT_IMPLEMENTED with unchanged output sentinel; installed SDK remains unchanged. Other false-success stubs, required paths, banks, codecs and saves remain UNKNOWN |
| B08 memory/performance | UNKNOWN | Measure guest/game/renderer memory and representative stock-clock hardware workloads after correctness milestones |
| B09 build/dependencies | Partly cleared | Native analysis/codegen and two sample object compiles pass; complete linking and Horizon SDK/Mesa/patch closure remain unvalidated |
| B10 deployment | UNKNOWN | NRO/application-mode permissions, launch/exit, controller/audio, persistence and dock transitions untested |
| B11 VMX128 semantics | Alias source fix tested / numeric hardware UNKNOWN | 212,068 invocations per patched build: zero alias differences. Numeric comparison results retained; Canary uses different rounding/denormal/range behavior. No verified special-value hardware corpus |
| B12 measurement supervision | Confirmed process defect | Second run reports 1,096.56 calendar seconds vs 77.033 monotonic seconds. The intended 300-second calendar bound was not enforced; fix sleep-inclusive and calendar deadlines before further analysis |

The PE declares 14 sections and SizeOfImage 15,750,656 bytes; the loaded XEX allocation is 14,090,240 bytes and the SDK's analysis view excludes the out-of-range discardable `.reloc` section. These differing header/allocation values require preserving their meanings, not using either as total game RAM or declaring the input corrupt without further evidence.

The complete [VMX methodology/results](research/vmx128-validation.md) and [261-function matrix, exception/STFS findings and reference comparison](research/runtime-audit.md) define what is confirmed. No fix was applied to the installed SDK/runtime; isolated source corrections are described below. No title-specific patch or reference implementation was copied.

The [Phase 6 report](research/ppc-execution.md) records 39 PPC groups, 12 final fault checks and 6 supervision checks with zero final failures, plus two diagnostic timeouts. It distinguishes real export/object behavior from whole-runtime integration and leaves full ABI, unchecked pointers and invalid TLS-free success unresolved.

The [Phase 5 report](research/runtime-foundation.md) separates original standalone contracts, source-only SDK corrections and the unchanged native SDK probe. No full game or Switch execution result follows from these checks.

## Platform and resource constraints

The development host is macOS ARM64 with 8 GiB physical RAM and about 28 GiB free disk space at discovery. Python, Apple Clang and Docker were available; host CMake/Ninja, Rust/.NET and a Switch toolchain were not found on the checked PATH. No global packages/configuration were changed.

Peak tool RSS during Phase 3 was **670,859,264 bytes (639.78 MiB)**. This does not bound a full compile, linker or game runtime. Phase 3 retained about 190 MiB of generated output privately. The anomalous calendar duration prevents treating its monotonic timing as a wall-clock performance estimate.

Linux was **not required for the completed analysis**. A later Horizon baseline should use an isolated, pinned devkitPro/devkitA64 container with fixed SDK/Mesa commits and explicit resource limits. Select the image digest/tool versions when that experiment is scheduled; no reproducible target build is claimed now. The 8 GiB host and current free space must be rechecked before attempting large generated-code builds.

## Decision boundaries

No compiler redesign or migration is justified. The default discovery failures have a demonstrated configuration remedy. XenonRecomp has the same unexpected float16_4 guard and no demonstrated advantage for this case. Original fixtures now validate an alias-only source correction; numeric changes remain unjustified by conflicting software evidence. The host fault cause and tiny original PPC/export bridge now have bounded evidence. Next build one coherent patched SDK and validate callback/r2, 64-bit argument and failure contracts before any private entry trace; retain guest exception, STFS functionality, graphics/media and hardware gates.

No silent force flags, automatic `live.dll` inclusion, title-update application, API stubbing or renderer optimization is justified by loader success. A complete generated C++ set would still require compile/link checks, behavioral tests and separate Switch hardware verification. There is no playable build and no Release.
