# Feasibility — Phase 4

Assessment: **2026-10-20 · v0.0.4-runtime-audit**. Verdict: **PARTIAL; runtime-correct game execution remains BLOCKED pending semantic and runtime contracts**. The Phase 3 configuration-assisted analysis/generation PASS stands.

Extraction, XEX loading, configured analysis and private C++ emission are verified. Two selected game partitions previously compiled on Mac. Phase 4 executed original isolated VMX fixtures at O0/O2 and with sanitizers, and classified all 261 imported functions by source behavior. Full linking, runtime correctness, execution and Nintendo Switch compatibility are not established. [Phase 3 results and exact limitations](research/rexglue-analysis.md) supersede the historical [Phase 2 timeout](research/xbox360-analysis.md).

| Subsystem | Evidence state | Engineering assessment |
| --- | --- | --- |
| Input/extraction | VERIFIED | GOD/SVOD and DLC/TU readable; hash-verified extraction and source preservation passed |
| Executable parsing | VERIFIED | Main XEX fingerprinted; encrypted/LZX image loaded natively with ReXGlue 0.10.0 |
| Analysis / generation | VERIFIED with configuration | GapFill completes; five checked function hints clear default validation failures; 48,371 bodies emitted |
| CPU semantics | PARTIAL / BLOCKED | Mask 2/3 diagnostic equivalence tested; finite in-range conversion agrees with independent truncate oracle. Overflow/infinity differ; aliased emission corrupts inputs. Hardware equivalence UNKNOWN |
| Runtime imports | VERIFIED source audit / untested conformance | All 261 mapped: 7 IMPLEMENTED, 184 PARTIAL, 61 STUB, 9 UNKNOWN; all 274 ordinals recognized including variables |
| Runtime semantics | BLOCKED / UNKNOWN | Context/unwind/handler stubs and STFS false-success outputs confirmed; RtlRaiseException has a possible static entry path, actual execution/boot necessity UNKNOWN |
| Graphics | UNKNOWN / major unresolved gate | D3D9/Xenos path and shader archive names identified; no translated shader or correct frame |
| Audio/middleware | UNKNOWN / substantial work | FMOD/Havok/Bink/Lua evidence, XMA imports; 157 banks lack standard initial FSB magic; codecs/active paths unproven |
| Input | INFERRED reusable foundation | XamInput and libnx reference adapters exist; no game input test |
| Filesystem/saves | UNKNOWN | File/content imports and archives identified; streaming/persistence behavior not tested |
| Memory | UNKNOWN / major unresolved gate | Header image/stack/TLS known; heap/GPU/streaming peaks, alias/protection and faults not tested on Switch |
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
| B05 memory/exceptions | Unresolved | CaptureContext/Unwind/handler only warn; static early RaiseException path exists. Separate debugger naming from guest unwinding and host fault recovery |
| B06 graphics | Unresolved | Inventory actual `.fxo` shaders and render features; validate PM4, formats/resolves, readbacks and invalidation before adapting a renderer |
| B07 runtime/media/assets | Source audit complete, execution unresolved | 61 stubs include STFS success with unwritten output, fabricated session/device results and missing I/O-manager operations. Required paths, banks, codecs and saves remain UNKNOWN |
| B08 memory/performance | UNKNOWN | Measure guest/game/renderer memory and representative stock-clock hardware workloads after correctness milestones |
| B09 build/dependencies | Partly cleared | Native analysis/codegen and two sample object compiles pass; complete linking and Horizon SDK/Mesa/patch closure remain unvalidated |
| B10 deployment | UNKNOWN | NRO/application-mode permissions, launch/exit, controller/audio, persistence and dock transitions untested |
| B11 VMX128 semantics | Confirmed SDK differences / hardware UNKNOWN | 212,068 synthetic invocations per build; finite in-range/mask/lane checks pass. Overflow/infinity become NaNs; same-register conversion corrupts source. No aliases among 237 static game sites; dynamic inputs unknown |
| B12 measurement supervision | Confirmed process defect | Second run reports 1,096.56 calendar seconds vs 77.033 monotonic seconds. The intended 300-second calendar bound was not enforced; fix sleep-inclusive and calendar deadlines before further analysis |

The PE declares 14 sections and SizeOfImage 15,750,656 bytes; the loaded XEX allocation is 14,090,240 bytes and the SDK's analysis view excludes the out-of-range discardable `.reloc` section. These differing header/allocation values require preserving their meanings, not using either as total game RAM or declaring the input corrupt without further evidence.

The complete [VMX methodology/results](research/vmx128-validation.md) and [261-function matrix, exception/STFS findings and reference comparison](research/runtime-audit.md) define what is confirmed. No SDK/runtime fix was applied; no title-specific patch or reference implementation was copied.

## Platform and resource constraints

The development host is macOS ARM64 with 8 GiB physical RAM and about 28 GiB free disk space at discovery. Python, Apple Clang and Docker were available; host CMake/Ninja, Rust/.NET and a Switch toolchain were not found on the checked PATH. No global packages/configuration were changed.

Peak tool RSS during Phase 3 was **670,859,264 bytes (639.78 MiB)**. This does not bound a full compile, linker or game runtime. Phase 3 retained about 190 MiB of generated output privately. The anomalous calendar duration prevents treating its monotonic timing as a wall-clock performance estimate.

Linux was **not required for the completed analysis**. A later Horizon baseline should use an isolated, pinned devkitPro/devkitA64 container with fixed SDK/Mesa commits and explicit resource limits. Select the image digest/tool versions when that experiment is scheduled; no reproducible target build is claimed now. The 8 GiB host and current free space must be rechecked before attempting large generated-code builds.

## Decision boundaries

No compiler redesign or migration is justified. The default discovery failures have a demonstrated configuration remedy. XenonRecomp has the same unexpected float16_4 guard and no demonstrated advantage for this case. Original fixtures now demonstrate specific numeric and alias concerns. Obtain independent Xbox special-value evidence and propose any narrowly scoped emitter change separately; retain exception/STFS, graphics/media and hardware gates.

No silent force flags, automatic `live.dll` inclusion, title-update application, API stubbing or renderer optimization is justified by loader success. A complete generated C++ set would still require compile/link checks, behavioral tests and separate Switch hardware verification. There is no playable build and no Release.
