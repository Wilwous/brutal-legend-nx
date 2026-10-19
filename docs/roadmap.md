# Research roadmap — Phase 3

## Completed

**R0 — Reference survey:** initial comparison of 28 supplied projects; four initial architectures examined; source/build facts separated from author claims. No reference port built or run.

**R1 — Private intake:** supplied input identified as GOD/SVOD plus three DLC packages and a separate title update. Full base hash-tree verification, safe bounds/path preflight and all package extractions passed. All 38 source game files retain SHA-256/size/mtime. Original XEX hashes, headers, resource inventory and alternate-header/update caveats recorded.

**R2a — Loader and host tooling:** official ReXGlue 0.10.0 runs natively on macOS ARM64; selected main XEX decrypts/decompresses and loads. All 274 imports recognized; all 261 function shims present. A small tool-mode helper inspected sections/imports without running guest code.

**R2b — Deep analysis / minimal generation:** completed with five checked function hints on the unmodified base. GapFill completed, configured Validate passed and private code generation exited 0. Generated output has 48,371 bodies plus 261 imported functions. Two selected partitions compile to Mac objects; no complete link or execution was attempted. The initial run's five branch-target failures and all 109 float16_4 warnings are recorded. The second run exceeded its intended calendar bound because monotonic supervision disagreed with the system timer; no third analyzer run was made. [Commands, coverage and timing caveat](research/rexglue-analysis.md).

## Next exact task: R2c — VMX semantics and runtime prerequisites

Keep `default.xex` SHA-256 `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456` as the unpatched baseline. No title update or extra module is authorized by the analysis result.

Create small original fixtures for float16_4 mask 2/3, shift 0: conversion boundaries, subnormals, NaNs/infinities, signs, source/destination aliasing and unchanged lanes. All 109 warning sites use mask 3 / shift 0; Xenia's source treats this permutation like mask 2. That supports testing an overly restrictive warning guard, not assuming a broken instruction or blindly suppressing diagnostics. Compare to an independent semantic reference; emulator agreement alone is not hardware proof. Do not compile the full game to test this operation.

**Acceptance:** reproducible, game-independent expected/results cases and a documented decision to retain the warning or propose a focused SDK change. Do not migrate toolchains without a demonstrated fix for a concrete deficiency. Before any separately budgeted analysis run, correct supervision to handle sleep-inclusive and calendar deadlines. The Phase 3 allowance of two analyzer runs is consumed.

## R2d — exception, runtime and variant/update decision

Privately inspect the 106 analyzer-noted SEH-related functions and audit required imported stubs, especially `RtlUnwind`, `__C_specific_handler`, `StfsCreateDevice` and `StfsControlDevice`. Identify relevant exception/function chunks and actual content/save/module paths; shim existence is not correctness. Freeze the base fingerprint. Separately validate `live.dll`'s changed headers and the update's source digest/patch relationship before selecting an updated baseline.

Inventory actual shader binaries from the known startup/archive family and classify the nonstandard sound-bank encoding without unpacking every asset. No replacement runtime, compiler or renderer should be written merely from filename/string evidence.

## R3 — independent Horizon probes (not started)

Use original minimal fixtures or appropriately licensed probes: guest address aliases and coherence, required protection/fault semantics, register preservation, threads/waits/APCs, Vulkan presentation/features, audio, pads and SD access. Record exact console/loader/Atmosphère/toolchain configuration. Begin at stock clocks in full application mode. A triangle or successful alias alone does not validate the game runtime.

## R4 — conditional smallest game milestone (not started)

Only after binary/runtime prerequisites are reviewed, choose one private observable milestone based on actual findings, such as entry plus a trace of early platform calls. Boot, rendering, movies/audio, gameplay, saves and sustained performance remain separate milestones. Only two selected generated partitions have been compiled as Mac objects. There is no complete linked game executable or execution result.

## R5 — broader port decision and distribution (not started)

Decide scope from validated shader/runtime behavior and measured physical-memory/stock-clock costs. Select reproducible host/target dependency versions and license closure before release planning. No estimate of FPS, completion date or playable compatibility is justified yet. No GitHub Release or automated binary pipeline is created.
