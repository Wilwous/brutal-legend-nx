# Brütal Legend NX

**Wilwous · v0.0.1-research · Xbox 360 → Nintendo Switch / Atmosphère**

Research toward a native Nintendo Switch port of Brütal Legend using static PowerPC recompilation and a homebrew runtime. The intended target is ARM64 Horizon with devkitPro and libnx.

**There is no playable port, game build, NRO, installer, or supported game version.** This repository currently contains original research documentation and a publication check. Successful recompilation would establish neither correct execution nor Switch playability.

## Current status

Research snapshot: **2026-10-19**.

- **VERIFIED:** all 28 supplied reference repositories were accessible; the four initial references received source/build inspection. [Comparison and evidence](docs/research/reference-projects.md).
- **VERIFIED:** no ISO or XEX was found in the supplied attachment directory or the task workspace. The attachment contained the project brief only. No game data was extracted or modified.
- **VERIFIED:** ReXGlue ports require a host code generator, a separately built target runtime, and substantial platform/renderer work. UnleashedRecomp-NX uses XenonRecomp/XenosRecomp rather than ReXGlue.
- **INFERRED:** a ReXGlue-based prototype is a reasonable first experiment, subject to the exact executable and the gates below.
- **UNKNOWN:** Brütal Legend's binary compatibility, imports, assets, middleware, graphics requirements, memory footprint, and achievable performance.

**Verdict: CONDITIONAL GO for bounded investigation; no authorization implied for a full port implementation.** Missing binary evidence and unproven graphics/memory behavior prevent a game-port implementation decision. [Feasibility and blockers](docs/feasibility.md).

## Technical approach

Proposed, not implemented:

1. Identify and read the owner's original Xbox 360 ISO without modifying it.
2. Extract to a private directory outside this checkout, locate `default.xex`, and record SHA-256, execution metadata, imports, sections, and resource inventory.
3. Evaluate the exact executable with a pinned ReXGlue host build. Preserve diagnostics and any derived output privately.
4. Prove the required memory, threading, audio, input, and graphics behavior on Switch before integrating the game.
5. If justified, compile translated C++ to ARM64 and connect a narrowly adapted graphics path to Vulkan through Mesa/NVK.

This is static recompilation plus runtime/API compatibility and graphics translation, not an original-source port. The native CPU objective does not remove the need to reproduce guest system and GPU semantics.

[Architecture](docs/architecture.md) · [Xbox 360 analysis](docs/research/xbox360-analysis.md) · [Switch runtime](docs/research/switch-runtime.md)

## Game files

Users must supply their own legally obtained Xbox 360 copy. **Do not upload an ISO, XEX, resource archive, music, shader binary, keys, memory dump, or generated game code to this repository or an issue.**

[Game-file preparation](docs/research/game-file-preparation.md) defines the read-only identification, extraction, hashing, inventory, and ReXGlue evaluation procedure. A filename ending in `.iso` is not proof of disc format. No game executable hash has been established for this project.

Keep the original ISO, extracted files, local manifests, generated C++, translated shaders, caches, and build products in a private workspace **outside the public Git repository**. Ignored directories inside the checkout do not satisfy this project policy.

## Roadmap

| Gate | Required evidence | Status |
| --- | --- | --- |
| R0: reference research | Source/build comparison, dependency and license boundaries | Initial survey complete; lower-priority projects are README/tree screening only |
| R1: game intake | Readable ISO, executable hash/metadata, resource inventory | Blocked: input absent |
| R2: recompiler assessment | Loader/analysis diagnostics, function/import coverage, instruction gaps | Not attempted |
| R3: platform probes | Guest aliases, synchronization, faults, audio/input and Vulkan on hardware | Not attempted |
| R4: limited prototype | One bounded observable milestone with explicit success/failure criteria | Not started |
| R5: port development | Evidence-based subsystem plan and feasibility review | No decision |

See [roadmap](docs/roadmap.md). Overclocking is not a baseline requirement. No frame-rate or resolution target is claimed.

## Build and installation

There is no game build command or installation package at this stage. Do not expect this checkout to produce an NRO. The research CI validates only public repository contents and documentation links:

```sh
python3 tools/check_public_tree.py
```

It does not fetch game data, run a recompiler, build a game, or publish releases. [Compatibility](docs/compatibility.md) records all untested states.

## Contributing

Contribute reproducible findings, sanitized diagnostics, and links to exact source revisions. Read [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), and [third-party boundaries](THIRD_PARTY_NOTICES.md) first.

## Credits and licensing

Research and project direction: **Wilwous**. Technical references include ReXGlue/Xenia, XenonRecomp/XenosRecomp, project8-sw, the-simpsons-game-nx, Mercenaries2-NX, UnleashedRecomp-NX, devkitPro, libnx, and Mesa. [Reference register](docs/research/reference-projects.md) credits every supplied project.

Original project material is licensed under **GPL-3.0-only**; see [LICENSE](LICENSE). No third-party implementation is vendored. Referenced code retains its own license; this license grants no rights to game code, assets, music, trademarks, or console SDK materials.

Unofficial research project; no affiliation with or endorsement by the game's rights holders, Microsoft, Nintendo, or the authors of the reference ports. All game names and trademarks belong to their respective owners. No proprietary Nintendo SDK components are used or required by this research repository.
