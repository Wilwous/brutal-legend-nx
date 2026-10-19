# Feasibility — Phase 3

Assessment: **2026-10-19 · v0.0.3-rexglue-analysis**. Verdict: **PASS for analysis/minimal generation with five function hints; broader port feasibility PARTIAL / unproven**.

Extraction, XEX loading, configured analysis and private C++ emission are verified. Two selected partitions compile on Mac. Full linking, instruction/runtime correctness, execution and Nintendo Switch compatibility are not established. [Phase 3 results and exact limitations](research/rexglue-analysis.md) supersede the historical [Phase 2 timeout](research/xbox360-analysis.md).

| Subsystem | Evidence state | Engineering assessment |
| --- | --- | --- |
| Input/extraction | VERIFIED | GOD/SVOD and DLC/TU readable; hash-verified extraction and source preservation passed |
| Executable parsing | VERIFIED | Main XEX fingerprinted; encrypted/LZX image loaded natively with ReXGlue 0.10.0 |
| Analysis / generation | VERIFIED with configuration | GapFill completes; five checked function hints clear default validation failures; 48,371 bodies emitted |
| CPU semantics | UNKNOWN | Zero generic unimplemented markers; 109 float16_4 warnings for mask 3 / shift 0 need independent semantic fixtures |
| Runtime symbols | VERIFIED | All 274 import ordinals recognized; all 261 function shims present; not semantic validation |
| Runtime semantics | BLOCKED / UNKNOWN | Imported unwind/exception and STFS APIs have explicit stubs; actual call paths untested |
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
| B05 memory/exceptions | Unresolved | Guest unwind/SEH imports include stubs; prove active requirements and Horizon host fault/register/alias semantics |
| B06 graphics | Unresolved | Inventory actual `.fxo` shaders and render features; validate PM4, formats/resolves, readbacks and invalidation before adapting a renderer |
| B07 runtime/media/assets | Unresolved | Audit execution paths through STFS/other stubs, bank encoding, FMOD/XMA/Bink timing, archive semantics, middleware and saves |
| B08 memory/performance | UNKNOWN | Measure guest/game/renderer memory and representative stock-clock hardware workloads after correctness milestones |
| B09 build/dependencies | Partly cleared | Native analysis/codegen and two sample object compiles pass; complete linking and Horizon SDK/Mesa/patch closure remain unvalidated |
| B10 deployment | UNKNOWN | NRO/application-mode permissions, launch/exit, controller/audio, persistence and dock transitions untested |
| B11 VMX128 semantics | Open validation gate | 109 warnings all use float16_4 mask 3 / shift 0. Xenia treats this permutation like mask 2; no proven opcode incompatibility, but conversion/lane semantics need original fixtures |
| B12 measurement supervision | Confirmed process defect | Second run reports 1,096.56 calendar seconds vs 77.033 monotonic seconds. The intended 300-second calendar bound was not enforced; fix sleep-inclusive and calendar deadlines before further analysis |

The PE declares 14 sections and SizeOfImage 15,750,656 bytes; the loaded XEX allocation is 14,090,240 bytes and the SDK's analysis view excludes the out-of-range discardable `.reloc` section. These differing header/allocation values require preserving their meanings, not using either as total game RAM or declaring the input corrupt without further evidence.

## Platform and resource constraints

The development host is macOS ARM64 with 8 GiB physical RAM and about 28 GiB free disk space at discovery. Python, Apple Clang and Docker were available; host CMake/Ninja, Rust/.NET and a Switch toolchain were not found on the checked PATH. No global packages/configuration were changed.

Peak tool RSS during Phase 3 was **670,859,264 bytes (639.78 MiB)**. This does not bound a full compile, linker or game runtime. Phase 3 retained about 190 MiB of generated output privately. The anomalous calendar duration prevents treating its monotonic timing as a wall-clock performance estimate.

Linux was **not required for the completed analysis**. A later Horizon baseline should use an isolated, pinned devkitPro/devkitA64 container with fixed SDK/Mesa commits and explicit resource limits. Select the image digest/tool versions when that experiment is scheduled; no reproducible target build is claimed now. The 8 GiB host and current free space must be rechecked before attempting large generated-code builds.

## Decision boundaries

No compiler redesign or migration is justified. The default discovery failures have a demonstrated configuration remedy. XenonRecomp has the same unexpected float16_4 guard and no demonstrated advantage for this case. Use original semantic fixtures before deciding whether the SDK needs a change; retain exception/STFS, graphics/media and hardware gates.

No silent force flags, automatic `live.dll` inclusion, title-update application, API stubbing or renderer optimization is justified by loader success. A complete generated C++ set would still require compile/link checks, behavioral tests and separate Switch hardware verification. There is no playable build and no Release.
