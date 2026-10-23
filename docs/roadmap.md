# Research roadmap — Phase 19

**Current Phase 19 gate (2026-10-23): VERIFIED static audit and bounded original regressions; entry readiness PARTIAL.** The lexical direct closure contains 108 generated functions, 33 imports, 21 unresolved indirect sites and 1,693 unmediated scalar accesses. Nine import contracts are scoped; 24 remain denied. The original stop adapter now uses one state snapshot to avoid a reproduced false rejection. Final Debug/Release/ASan+UBSan regressions pass 578 core checks; interrupted/failed attempts remain documented. No proprietary code executes. Phase 20 synthetic/static work is CONDITIONAL GO; private game entry is NO-GO. [Full audit, ranked blockers and acceptance criteria](research/game-entry-readiness-audit.md).

## Completed

**R0 — Reference survey:** initial comparison of 28 supplied projects; four initial architectures examined; source/build facts separated from author claims. No reference port built or run.

**R1 — Private intake:** supplied input identified as GOD/SVOD plus three DLC packages and a separate title update. Full base hash-tree verification, safe bounds/path preflight and all package extractions passed. All 38 source game files retain SHA-256/size/mtime. Original XEX hashes, headers, resource inventory and alternate-header/update caveats recorded.

**R2a — Loader and host tooling:** official ReXGlue 0.10.0 runs natively on macOS ARM64; selected main XEX decrypts/decompresses and loads. All 274 imports recognized; all 261 function shims present. A small tool-mode helper inspected sections/imports without running guest code.

**R2b — Deep analysis / minimal generation:** completed with five checked function hints on the unmodified base. GapFill completed, configured Validate passed and private code generation exited 0. Generated output has 48,371 bodies plus 261 imported functions. Two selected partitions compile to Mac objects; no complete link or execution was attempted. The initial run's five branch-target failures and all 109 float16_4 warnings are recorded. The second run exceeded its intended calendar bound because monotonic supervision disagreed with the system timer; no third analyzer run was made. [Commands, coverage and timing caveat](research/rexglue-analysis.md).

**R2c — Isolated VMX tests:** completed with an independent numeric oracle and the actual pinned emitter case. Each O0/O2/sanitized build executed 212,068 synthetic invocations. Finite in-range, distinct-lane and mask 2/3 checks passed. Overflow/infinity differ from IEEE round-toward-zero; same-register conversion corrupts source data. These are characterized gaps, not a hardware-equivalence PASS. No alias appears among the 237 prior game sites. [Methodology/results](research/vmx128-validation.md).

**R2d — Runtime source audit:** all 261 imported functions mapped/classified: 7 IMPLEMENTED, 184 PARTIAL, 61 STUB, 9 UNKNOWN. Exception/STFS behavior, a conservative static early-call closure, reference approaches and license boundaries recorded. No runtime API was implemented or game-specific patch copied. [Complete matrix](research/runtime-audit.md).

**R2e — Runtime component foundation:** original checked bounded memory, host-thread TLS and a diagnostic failure boundary implemented. Six original groups (including 4,000 seeded memory operations) and three SDK leaf groups pass at O0/O2 and with ASan/UBSan. A separate SDK patch fixes source aliasing and TLS bounds/wrap, and returns explicit STFS failure. Patched VMX regressions have zero alias differences; numeric semantics stay unchanged. Installed SDK/game output remain untouched. [Phase 5 contracts/results](research/runtime-foundation.md).

**Independent numeric evidence:** pinned Canary differs from Xenia/ReXGlue in rounding, denormals and upper range; a primary DirectXMath note distinguishes an Xbox variant. No hardware-verified special-value corpus exists here, so no numeric correction is applied.

**R2f — Native fault diagnostics and original PPC-context execution:** completed within the documented boundary. Two 30-second child/group watchdog reproductions establish SDK unclaimed-signal retry as the Phase 5 hang. Isolated fail-stop/PC-resume fixtures pass. Thirteen real SDK exports, register preservation, memory lifecycle, actual XThread/PCR/r13 and synthetic process TLS pass O0/O2 and limited ASan/UBSan checks. Source patches are tested against real runtime objects; no installed SDK is changed. [Exact contracts, 57 final checks and remaining limits](research/ppc-execution.md).

**R2g — Unified SDK and selected ABI/failure integration:** clean Debug/Release/ASan+UBSan builds of pinned ReXGlue with seven ordered patches completed. All 261 baseline shim symbols are present; one new library supplies Memory/MMIO/dispatcher/thread/export behavior. Original contracts/fault checks plus watchdog regressions total 137 passes. Selected GPR64/stack64/r2/nested callback and TLS-free failures are corrected. Debug private loader-only succeeds; sanitized loader aborts on XDBF alignment. Six additional runtime/vector/pointer blockers are recorded separately. [Exact reproduction and limitations](research/unified-sdk.md).

**R2h — Pre-entry safety:** bounded XDBF byte decoding and synthetic corpus; wide checked allocation arithmetic fixing the saved SIGBUS path; heap-maintained XAM ownership and foreign/stale/double-free rejection; explicit host failure for selected unsupported exception exports; controlled guest reservation/protection checks. Existing eleven-patch SDK sources and all original game/config/generated data are preserved. [Final build/test evidence and limits](research/pre-entry-safety.md).

## R2i / Phase 9 — checked guest accesses and entry-contract gates

Completed the bounded original-fixture implementation and static readiness audit. A real-SDK owner-checked copy layer validates 4-KiB pages and full spans; unsupported fetch and MMIO/host-page padding fail closed. Synthetic import tests expose and correct NtClose accepting closed handles. The literal entry closure has 108 generated functions, 33 imports and 21 unresolved indirect sites. No generated access is mediated and no game code is executed. [Contracts, final regressions and NO-GO justification](research/guest-memory-entry-readiness.md).

## R2j / Phase 10 — original emitted-access mediation and fail-closed dispatch

Original decoder→builder→template fixtures validate an opt-in checked-memory seam on the real SDK. Selected scalar/finite-FP/full-vector transfers pass; atomics, partial vectors, raw strings and unsupported calls are refused. Typed-vector signatures remain unsupported. The prior private game output is statically assessed but never rewritten/compiled/executed. [Exact coverage, regressions and limitations](research/generated-memory-instrumentation.md).

## R2k / Phase 11 — original synthetic execution module (completed in measured scope)

The actual pinned raw-test CLI generates nine original functions / 36 instructions; unchanged output executes through the checked seam and real patched runtime. An original fixture loader validates section/relocation/entry policy. Two real guest XThreads exercise their native stack, PCR/r13 and TLS with explicit scope loans. A narrow SDK correction permits checked ordinary virtual 64-KiB stack copies; physical/XEX/fetch remain refused. Unsupported operations and call targets fail explicitly. **Recovery completed:** a read-only debugger trace confirms the stranded local Exit reference on Mac. The owned SDK exit boundary now unwinds scopes before native lease release; all original teardown assertions and fourteen additional lifetime groups pass unchanged in meaning across Debug/Release/Sanitized. Native factory failures roll back process/lease/callback resources, and foreign asynchronous termination explicitly refuses unsafe work. All 263 core checks pass; historical failures remain private evidence. This is not production XEX loading or title startup. [Reproduction, regressions and limitations](research/synthetic-execution.md).

## R2l / Phase 12 — transactional image ownership and runtime contracts (completed in measured scope)

Preserved the fifteen-file initial work and unchanged original failure assertions. Patch 0017 replaces whole-heap Reset with exclusive tagged reservation/commit/rollback, reclaims failed image/metadata stages and gives UserModule a single header/processor owner. Explicit retained image leases refuse unsafe unload and close new admission; no live object is forcibly deleted. The actual SDK passes independent-image/collision, ten rollback stages, two allocator injections, reload, copied-reference metadata, active lease and concurrent-image fixtures. All 323 core checks pass on the coherent Debug/Release/Sanitized SDK. Existing import/ABI/TLS/unsupported-operation limits remain; no proprietary code is executed. [Full Phase 12 report](research/image-ownership-runtime-contracts.md).

## R2m / Phase 13 — original XEX TLS and import admission

Actual XEX-header templates initialize real independent-module and default title-process XThreads. Bounded metadata validation, retained image lifetime, rollback and process-slot bounds are covered by original fixtures. Six value-only import signatures have explicit identity/membership/context admission; the 27 remaining imports are refused. GPR64/FPR/stack/callback/raw-vector regression evidence remains a selected ABI subset. The matrix retains 27 PARTIAL / six BLOCKED implementation contracts. No proprietary execution occurs. [Full scope, measurements and gate](research/xex-tls-import-admission.md).

## R2n / Phase 14 — checked pointer imports and host-driven synthetic dispatch

Three capability-bound IN/OUT/INOUT adapters execute the listed real XEX/TLS/import/SDK path with bounded snapshots, private scratch, output validation and retained ownership. Active module unload and allocation-release attempts, cross-page failures, callback reentrancy/errors and repeated/concurrent XThreads are covered. SDK patches/locks remain unchanged. The shared matrix now admits six scalar and three pointer-scoped signatures while denying 24; full Xbox conformance remains partial. No proprietary code or guest XEX entry executes. [Results, reproduction and limits](research/pointer-import-synthetic-dispatch.md).

## R2o / Phase 15 — original generated caller and access accounting

Completed in the explicit synthetic scope: actual pinned generation of eight original functions / 59 instructions, unchanged generated caller execution, 30/30 checked reachable memory sites, three pointer contracts and real SDK-result branching. Four concurrent XThreads, active unload/release refusal and repeated image/thread/buffer reclamation pass. All 440 core checks pass across three profiles. Host-authored raw-test rebasing and binding remain PARTIAL; private game output remains uninstrumented. [Complete evidence and limitations](research/generated-ppc-end-to-end-dispatch.md).

## R2p / Phase 16 — synthetic XEX binding and reproducible codegen

Completed in the fixed synthetic scope: two original XEX products drive actual production codegen with PDATA and no manual function hints. Automatic binding validates image/code/section/TLS/import identity; unchanged emitted functions run at their actual guest addresses. Patch 0019 adds leased checked XEX memory copying without broadening ordinary virtual copies or fetch. All 35 reachable memory sites per image are checked; 476 core checks and the separate controls pass. Historical development failures and harness fail-stop limits remain documented. [Single-command workflow and evidence](research/synthetic-xex-binding.md).

## R2q / Phase 17 — image-backed pointer contracts and cooperative shutdown

Original generated XEX callers use all three existing pointer contracts with leased image capabilities, RO/RW permissions, full-span bounds and output rollback. Active call, capability, TLS/PCR, XThread and image ownership remain pinned through cooperative completion, actual join and final reclamation. Concurrent callers, eight repeated cycles, callback errors and fail-stop join behavior are tested on the unchanged nineteen-patch SDK. Scheduling and safe-boundary placement remain host-authored. [Evidence, exact scope and regressions](research/image-backed-pointer-shutdown.md).

## R2r / Phase 18 — generated loop stop boundaries

The original nine-function XEX uses actual production midasm hooks at all six backward branches and explicit whole-instruction safe points. Finite/infinite/nested/memory/import loops, concurrent requests, active unload and checkpoint/lease cleanup execute through unchanged generated C++ and the real SDK. The finite unhooked benchmark retains checked memory. General guest cancellation and arbitrary-title instrumentation remain unverified. [Coverage, overhead and regressions](research/generated-loop-stop-boundaries.md).

## R2s / Phase 19 — readiness audit (completed in static/synthetic scope)

Reconciled the preserved entry/memory census; all 33 imports are classified A9/B18/E6, with initialization necessity UNKNOWN for every row. Private detailed identifiers stay outside Git. No game generation/execution occurs. A reproduced original stop-state race is corrected by one atomic snapshot and a 256-cycle regression per profile. All 578 selected final core checks pass; one initial false rejection, storage interruption and two calendar-watchdog terminations remain recorded. [Evidence and ranked B01–B10 matrix](research/game-entry-readiness-audit.md).

## Next: Phase 20 — bounded region and identity evidence

**CONDITIONAL GO for original synthetic/static work.** Address B01/B06 first: privately identify the smallest meaningful entry region ending before unsupported effects; independently validate actual PCs, forward joins, cycles, transfers and stop placement. Use an original synthetic analogue and conflict/omission tests. Unknown indirect targets must reject before transfer. Do not construct a speculative whole-title CFG engine or execute game code. Exit with a defensible region manifest, or a documented finding that the candidate cannot meet the contract.

## Phase 21 — only the selected runtime dependencies

After Phase 20, address B03/B04/B05 and any demonstrated B07/B08/B09 dependency. Validate exact image/code binding, consumed initial registers/stack/TLS/PCR and only the necessary import/pointer/error contracts using original generated callers. Preserve all other denials. Full guest unwind or all 24 denied imports need not be implemented if independently excluded or stopped before use. Require all three profiles, complete access/call/stop accounting and legitimate final-owner cleanup.

## Phase 22 — private instrumentation and independent reassessment

After the region/contract evidence exists, use a separately bounded generation-only task to produce new private output with pinned production configuration and complete checked seams (B02). Preserve historical game outputs and configuration. Validate every access/transfer/site against independent metadata, exact compiled image binding and negative original regressions. Any renewed title analysis needs its own finite allowance. Only then reassess a later separately authorized private experiment; this roadmap grants no execution permission.

**Private game entry remains NO-GO.** The mandatory gate is complete checked memory, supported-or-rejected operations, exact import ABI/pointer/error admission, image/code ownership, consumed initialization, stop and indirect coverage, observable failure and external process containment for the actual frozen region. Typed-vector ABI, full Xbox ABI and guest exception dispatch/unwind remain unsupported or unverified. Indexed lookup performance, graphics/audio, broad DLL loading and Horizon work follow demonstrated dependencies, not a generic emulator checklist.

Keep XEX/assets/configuration/old generated output unchanged and private. No full game compilation, boot or Switch deployment follows from synthetic success. The saved allocation SIGBUS and all useful failed/interrupted diagnostics remain private evidence.

## R3 — independent Horizon probes (not started)

Use original minimal fixtures or appropriately licensed probes: guest address aliases and coherence, required protection/fault semantics, register preservation, threads/waits/APCs, Vulkan presentation/features, audio, pads and SD access. Record exact console/loader/Atmosphère/toolchain configuration. Begin at stock clocks in full application mode. A triangle or successful alias alone does not validate the game runtime.

## R4 — conditional smallest game milestone (not started)

Only after binary/runtime prerequisites are reviewed, choose one private observable milestone based on actual findings, such as entry plus a trace of early platform calls. Boot, rendering, movies/audio, gameplay, saves and sustained performance remain separate milestones. Only two selected generated partitions have been compiled as Mac objects. There is no complete linked game executable or execution result.

## R5 — broader port decision and distribution (not started)

Decide scope from validated shader/runtime behavior and measured physical-memory/stock-clock costs. Select reproducible host/target dependency versions and license closure before release planning. No estimate of FPS, completion date or playable compatibility is justified yet. No GitHub Release or automated binary pipeline is created.
