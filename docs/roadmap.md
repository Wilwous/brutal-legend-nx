# Research roadmap — Phase 2

## Completed

**R0 — Reference survey:** initial comparison of 28 supplied projects; four initial architectures examined; source/build facts separated from author claims. No reference port built or run.

**R1 — Private intake:** supplied input identified as GOD/SVOD plus three DLC packages and a separate title update. Full base hash-tree verification, safe bounds/path preflight and all package extractions passed. All 38 source game files retain SHA-256/size/mtime. Original XEX hashes, headers, resource inventory and alternate-header/update caveats recorded.

**R2a — Loader and host tooling:** official ReXGlue 0.10.0 runs natively on macOS ARM64; selected main XEX decrypts/decompresses and loads. All 274 imports recognized; all 261 function shims present. A small tool-mode helper inspected sections/imports without running guest code.

## Next exact task: R2b — investigate incomplete function analysis

Use only `default.xex` SHA-256 `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`, unpatched and without `live.dll`. The first 45-second run reached GapFill, then was terminated at the bound. No game C++ was emitted.

Profile the pinned SDK's GapFill progress with compact diagnostics and a fixed further budget (for example, no more than 120 seconds wall time and 2 GiB resident memory, checked before starting). Determine whether the stage is progressing or repeatedly examining the same regions. Do not infer an infinite loop from the first timeout. Avoid full debug-volume logging, force flags, broad function guesses or speculative patches.

**Acceptance:** either complete Merge/Validate and record function/jump-table/indirect-call/instruction diagnostics, or produce a reproducible, bounded stage-specific blocker with enough sanitized evidence to investigate the analyzer. Preserve all raw/code-derived output privately. End the task before full game compilation or port work. If analysis requires a new SDK revision/patch, compare the same input and record the exact difference.

## R2c — semantic prerequisites and variant/update decision

Audit required imported stubs, especially `RtlUnwind`, `__C_specific_handler`, `StfsCreateDevice` and `StfsControlDevice`. Identify relevant exception/function chunks and actual content/save/module paths; shim existence is not correctness. Freeze the base fingerprint. Separately validate `live.dll`'s changed headers and the update's source digest/patch relationship before selecting an updated baseline.

Inventory actual shader binaries from the known startup/archive family and classify the nonstandard sound-bank encoding without unpacking every asset. No replacement runtime, compiler or renderer should be written merely from filename/string evidence.

## R3 — independent Horizon probes (not started)

Use original minimal fixtures or appropriately licensed probes: guest address aliases and coherence, required protection/fault semantics, register preservation, threads/waits/APCs, Vulkan presentation/features, audio, pads and SD access. Record exact console/loader/Atmosphère/toolchain configuration. Begin at stock clocks in full application mode. A triangle or successful alias alone does not validate the game runtime.

## R4 — conditional smallest game milestone (not started)

Only after binary/runtime prerequisites are reviewed, choose one private observable milestone based on actual findings, such as entry plus a trace of early platform calls. Boot, rendering, movies/audio, gameplay, saves and sustained performance remain separate milestones. No game compilation or executable build exists now.

## R5 — broader port decision and distribution (not started)

Decide scope from validated shader/runtime behavior and measured physical-memory/stock-clock costs. Select reproducible host/target dependency versions and license closure before release planning. No estimate of FPS, completion date or playable compatibility is justified yet. No GitHub Release or automated binary pipeline is created.
