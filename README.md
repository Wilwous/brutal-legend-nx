# Brütal Legend NX

**Wilwous · v0.0.2-binary-analysis · Xbox 360 → Nintendo Switch / Atmosphère**

Evidence-first research toward a native ARM64 Nintendo Switch port of Brütal Legend using static PowerPC recompilation and a homebrew runtime.

**There is no playable port, game build, NRO, installer, or supported game version.** Successful loading or recompilation would not establish correct execution or Switch playability.

## Current status

Phase 2 snapshot: **2026-10-19**, following `v0.0.1-research` (`ef2b882`).

- **VERIFIED:** the supplied directory contains a LIVE Games on Demand/SVOD package with 33 data fragments, three DLC packages and one title-update package. It contains no standalone ISO.
- **VERIFIED:** full GOD hash-tree verification and private extraction succeeded. The base has 284 files totaling 5,544,498,100 bytes. All 38 source game files retained their SHA-256, size and modification time.
- **VERIFIED:** `default.xex` is an encrypted, LZX-compressed Xbox 360 PowerPC executable, Title ID `454108C5`, Media ID `7DD5E14B`. Its SHA-256 is `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
- **VERIFIED:** ReXGlue 0.10.0 runs natively on macOS ARM64, decrypts/decompresses and loads this XEX, and resolves all 274 imports to registry entries. Its library exports shims for all 261 imported functions. Some required APIs are stubs.
- **BLOCKED:** the bounded 45-second analysis stopped during `GapFill`. Analysis validation and game C++ generation were not reached. No host game or Switch build was attempted.
- **UNKNOWN:** complete instruction/function coverage, runtime correctness, shaders, decoded audio-bank format, memory budget and stock-clock performance.

**Verdict: CONDITIONAL GO for further bounded analysis; full implementation remains premature.** [Binary findings](docs/research/xbox360-analysis.md) · [Feasibility and blockers](docs/feasibility.md) · [Commands and exact limits](docs/research/phase2-commands.md)

## Technical approach

Proposed: privately validate and extract the owner's input, fingerprint the chosen XEX/update combination, complete pinned ReXGlue analysis, then prove the required Horizon runtime and graphics behavior independently before integrating game code. Static CPU translation still requires guest memory/API compatibility and a correct graphics path.

The first architecture candidate is ReXGlue plus reviewed Horizon adaptations and a native Vulkan renderer through Mesa/NVK. Reference patches are not a universal drop-in. [Architecture](docs/architecture.md) · [Switch runtime](docs/research/switch-runtime.md)

## Research progress and roadmap

| Gate | Status |
| --- | --- |
| Reference survey | All 28 supplied repositories accessible; initial source/build comparison recorded |
| Private intake and extraction | Completed for the supplied GOD/base, DLC and title update; no update applied |
| XEX metadata and loader | Completed for the supplied `default.xex` |
| Function analysis / generation | Incomplete: 45-second bound reached in GapFill; no game C++ emitted |
| Runtime, shaders, audio and hardware | Not validated |
| Game build or distribution | Not started |

Next: investigate the incomplete function-analysis stage on the exact fingerprinted base executable, without forcing errors or starting a port. [Roadmap](docs/roadmap.md).

## Game files and compatibility

Users must supply their own legally obtained game files. The supplied `live.dll` is a second XEX header variant with the same stored payload as `default.xex`; the separate title update is not applied. Neither variant nor any update is claimed playable. [Compatibility](docs/compatibility.md).

**Never upload** ISOs, XEX files, assets, music, shader binaries, keys, dumps or generated game code. Inputs, extracted files, local manifests, decoded images, full inventories/logs and build products must remain in a private workspace **outside public Git**. [Preparation policy](docs/research/game-file-preparation.md).

## Build and installation

No game build command or installation package exists. The repository contains research documentation and an original publication check:

```sh
python3 tools/check_public_tree.py
```

CI checks public contents and local documentation links only. It never fetches game data, invokes codegen, builds the game or publishes releases. Overclocking is not a baseline requirement; no FPS or resolution is predicted.

## Contributing, credits and licensing

Research and project direction: **Wilwous**. See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), [reference comparison](docs/research/reference-projects.md) and [third-party boundaries](THIRD_PARTY_NOTICES.md). Local analysis used ReXGlue and xverter; their implementations and downloaded binaries are not vendored here.

Original project material is **GPL-3.0-only**; see [LICENSE](LICENSE). Referenced components retain their own licenses. This license grants no rights to the game's code, assets, music or trademarks.

Unofficial research project with no affiliation with or endorsement by the game's rights holders, Microsoft, Nintendo or reference-port authors. No proprietary Nintendo SDK components are used or required by this repository.
