# Feasibility and technical blockers

Assessment date: **2026-10-19**. Verdict: **CONDITIONAL GO for binary intake and bounded experiments only**. There is insufficient evidence to start a complete port. There is also insufficient evidence for a categorical NO-GO.

GREEN means an existing solution is likely reusable, not proven for this game. YELLOW means substantial engineering. RED means an unresolved critical gate. UNKNOWN means missing evidence. Colors describe the present evidence, not a completion percentage.

| Subsystem | Status | Evidence and remaining gate |
| --- | --- | --- |
| Executable recompilation | UNKNOWN | No XEX; cannot validate loader, instruction coverage, function boundaries, jump tables or modules |
| Runtime/API compatibility | YELLOW | ReXGlue supplies a foundation; actual imports, TLS, exceptions and thread behavior unknown |
| Graphics | RED | No Brütal Legend shader/resource/command evidence or validated native-renderer integration |
| Audio | UNKNOWN | SDK has XMA support and Horizon reference adapters; game codecs, mixer and movies unidentified |
| Input | GREEN | libnx/controller adapters exist in references; game's API use and mapping remain untested |
| Filesystem | YELLOW | Local filesystem adapters exist; archive layout, path semantics and streaming unknown |
| Saves | UNKNOWN | Guest save behavior, format and platform services not inspected |
| Memory | RED | Required aliases, protection/fault behavior, coherence, real resident size and GPU budget not established |
| Performance | UNKNOWN | No game measurement; no FPS, resolution or playable prediction is justified |
| Game assets/middleware | UNKNOWN | No inventory, codec or middleware identification possible |
| Build pipeline | YELLOW | Reference host/cross separation exists; no selected, reproduced Brütal Legend build |
| Hardware deployment | UNKNOWN | NRO/full application mode is a candidate; no console test or deployment performed |

## Critical blocker register — before implementation

| ID | Blocker | Evidence required to clear or narrow it |
| --- | --- | --- |
| B01 | Original input absent | Local path to owner's ISO, or complete extracted disc with original XEX and provenance |
| B02 | Disc format and integrity unknown | Format signatures, game partition offset, directory bounds, complete successful extraction, unchanged source hash |
| B03 | Executable identity unknown | XEX SHA-256, execution version/base version, title/media IDs, entry/base, sections, compression, imports and dependent modules |
| B04 | ReXGlue acceptance unknown | Pinned loader result, function analysis and validation diagnostics, indirect-call/jump-table coverage, instruction gap report |
| B05 | Horizon guest memory/exception contract unproven | Required alias coherence, protection/invalidation behavior, controlled fault handling and register-state preservation on hardware |
| B06 | GPU/shader translation unproven | Game-specific render/shader census and one correct bounded rendering milestone, including relevant resolves/readbacks |
| B07 | Runtime, middleware and media dependencies unknown | Import inventory and traces; identification of audio/video/physics/other libraries only where evidenced |
| B08 | Memory and stock-clock performance unmeasured | Resident/heap/GPU/cache peaks and measured frame/CPU/GPU times in representative scenarios |
| B09 | Build/dependency configuration unvalidated | Reproducible host and target toolchain plus individually audited patch/license closure |
| B10 | Deployment and persistence untested | On-device launch/exit, controller/audio, SD reads, isolated saves and dock/handheld tests |

B05/B06 are critical unresolved engineering gates, not proof that Switch support is impossible. Resolving B01–B04 alone does not clear them.

## Evidence limits

Source/build inspection establishes that the reference architecture exists. No reference was built or executed in this investigation. README claims about playability and clocks are author reports, not independently verified results, and cannot be transferred to Brütal Legend. Some reference documentation describes earlier states that conflict with newer README claims; these conflicts are recorded in the comparison.

The current development host has GitHub CLI, Python, Apple Clang and an operational Docker engine. CMake, Ninja, ReXGlue, extract-xiso, xdvdfs and devkitA64 were not found on the checked PATH; the conventional devkitPro installation directory was absent. These observations do not rule out other installations or container toolchains. No dependencies were installed, no SDK was compiled and no hardware was connected.

## Decision policy

Continue intake and static inspection when the ISO becomes available. Review the exact binary and blocker report before code generation. Any later codegen runs in private, and successful analysis, generated C++, host linking, ARM64 linking, boot, rendering, audio, gameplay, saving and sustained performance are separate milestones.

A limited prototype becomes justified only when its specific binary/runtime prerequisites are supported by evidence. Stop or redirect if a required instruction, dynamic-code mechanism, mapping behavior or GPU feature lacks a correct, testable path. Evaluate existing tools before considering any new compiler.
