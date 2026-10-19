# Research roadmap

## R0 — reference architecture

Initial survey completed: all supplied URLs checked, primary Xbox 360 methods distinguished, four initial references source/build inspected, tools and licensing boundaries recorded. Lower-priority references remain screening-level research; no runtime claim is independently tested. No external code imported.

## R1 — private game intake (next task)

Follow [game-file preparation](research/game-file-preparation.md). Accept a local ISO path, never an upload to GitHub. Record source SHA-256 before and after, format/partition evidence, extractor revision and diagnostics, original XEX hash/metadata, imports/sections, executable modules and inventory. Retain originals and all extracted data outside Git. Publish only a sanitized findings summary.

**Acceptance:** complete readable extraction, unchanged ISO, unambiguous original `default.xex`, bounded metadata/inventory report, and an explicit list of malformed/missing/unsupported elements. Stop with a blocker report if any prerequisite fails.

## R2 — bounded recompiler assessment (after R1 review)

Select and build a pinned host SDK. Read its actual CLI help; do not invent an `analyze` or `info` command. Examine loader and function-analysis diagnostics, exports/imports, exception metadata, modules, indirect branches and unsupported instructions. If analysis is coupled to C++ emission, treat it as private codegen and obtain the implementation-phase decision first. No full game recompilation in the current phase.

**Acceptance:** distinguish input loading, analysis coverage and emitted-code correctness. Unknown function targets or silently forced errors do not count as success.

## R3 — platform proof of concept

Use independently authored minimal fixtures or appropriately licensed probes that contain no game-derived code. Prove the specific guest address/alias pattern, threading/waits, safe faults and register preservation, Vulkan presentation, audio output, pad mapping and SD access. Test at stock clocks in full application mode. A triangle alone does not validate guest GPU semantics or memory write watches.

**Acceptance:** reproducible on-device logs, real committed memory numbers and clean exit; unresolved correctness requirements remain blockers.

## R4 — smallest game milestone (conditional)

After R1–R3 evidence review, use private translated code to reach one clearly defined milestone, such as entry plus a trace of the first platform calls. Choose the actual milestone from binary findings rather than assuming the title screen is immediately reachable. Keep graphical, audio, persistence and performance milestones separate. Do not claim playability from boot.

## R5 — evidence-based port plan

Only after a bounded milestone succeeds, estimate subsystem work from observed behavior and decide whether broader development is justified. No invented timeline, FPS, memory target or release date. Add reproducible builds and distribution checks when there is an actual artifact worth assessing.
