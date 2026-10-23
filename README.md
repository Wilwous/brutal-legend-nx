# Brütal Legend NX

**Wilwous · v0.0.18-generated-loop-stop · Xbox 360 → Nintendo Switch / Atmosphère**

Evidence-first research toward a native ARM64 Nintendo Switch port of Brütal Legend using static PowerPC recompilation and a homebrew runtime.

**There is no playable port, game build, NRO, installer, or supported game version.** Successful loading or recompilation would not establish correct execution or Switch playability.

## Current status

Phase 18 generated PPC loop stop boundaries: **2026-10-23**, following `v0.0.17-image-pointer-shutdown`.

**VERIFIED original synthetic scope, 575 core checks passed across Debug/Release/ASan+UBSan:** production midasm hooks execute inside nine original generated functions and cover all six backward branches. Infinite-loop cancellation, SDK/pointer completion and concurrent lifetime tests use unchanged emitted C++ and the existing nineteen-patch SDK. Generic CFG policy and guest unwind remain PARTIAL. [Phase 18 evidence and reproduction](docs/research/generated-loop-stop-boundaries.md).

- **VERIFIED:** the supplied directory contains a LIVE Games on Demand/SVOD package with 33 data fragments, three DLC packages and one title-update package. It contains no standalone ISO.
- **VERIFIED:** full GOD hash-tree verification and private extraction succeeded. The base has 284 files totaling 5,544,498,100 bytes. All 38 source game files retained their SHA-256, size and modification time.
- **VERIFIED:** `default.xex` is an encrypted, LZX-compressed Xbox 360 PowerPC executable, Title ID `454108C5`, Media ID `7DD5E14B`. Its SHA-256 is `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
- **VERIFIED:** ReXGlue 0.10.0 runs natively on macOS ARM64, decrypts/decompresses and loads this XEX, and resolves all 274 imports to registry entries. Its library exports shims for all 261 imported functions. Some required APIs are stubs.
- **VERIFIED:** GapFill completes. Default analysis fails on five branch targets; five privately checked function hints allow Validate and C++ emission to finish with exit 0. There are 48,371 emitted bodies and 261 imported functions.
- **VERIFIED:** two selected generated partitions compile to macOS ARM64 objects. There are 109 float16_4 warnings requiring semantic checks; no generic unimplemented-instruction markers were found. No full game link or Switch build was attempted.
- **MEASUREMENT LIMIT:** the second run recorded 1,096.56 seconds calendar time versus 77.033 supervisor monotonic seconds. The intended 300-second calendar limit was not enforced; the cause of the clock discrepancy is UNKNOWN. Peak tool RSS was 639.78 MiB.
- **VERIFIED:** isolated original VMX fixtures ran at O0/O2 and with sanitizers: finite in-range conversion agrees with an independent truncate oracle; same-register corruption and overflow/infinity differences were reproduced. Xbox hardware equivalence remains UNKNOWN.
- **VERIFIED:** original checked memory/TLS components and diagnostic boundary pass standalone tests. A focused SDK patch fixes aliasing/TLS bounds and rejects unsupported STFS calls. Installed SDK and game output remain unchanged.
- **VERIFIED:** the native fault hang is an unclaimed-signal retry in the SDK. Bounded child/group diagnostics establish it; isolated fail-stop and controlled PC recovery pass.
- **VERIFIED limited contracts:** 13 real SDK exports called with PPCContext; selected register, memory, actual XThread/PCR/r13 and TLS tests pass. Phase 7 integrates these checks into a clean rebuilt SDK; complete Xbox ABI remains PARTIAL.
- **VERIFIED:** all 261 baseline runtime functions audited by source: 7 IMPLEMENTED, 184 PARTIAL, 61 STUB, 9 UNKNOWN. Baseline STFS false-success and selected exception stubs now have explicit failure paths; STFS functionality and guest exception semantics remain unresolved.
- **VERIFIED:** Phase 8 built the coherent isolated SDK with eleven ordered patches. Bounded XDBF decoding, checked allocation arithmetic and owner-checked XamFree pass 191 core checks plus six watchdog checks; the saved allocation SIGBUS is retained as a corrected failure history.
- **PARTIAL / BLOCKED:** selected XamAlloc and exception paths now fail explicitly at the host boundary. Xbox error values, typed vectors, complete guest exceptions and unchecked raw access to guest-page slack remain unresolved. No game entry ran.
- **VERIFIED limited scope:** Phase 9 verified original owner-checked copies on its 4-KiB virtual heap, with cross-page, endian, MMIO/host-page padding and selected concurrent fixtures. NtClose now rejects already closed handles using the existing status.
- **PARTIAL / BLOCKED:** none of the 791,353 lexical generated load/store sites is mediated. The direct entry closure has 33 imports and 21 unresolved indirect sites; fetch permissions, unsafe pointer imports, exceptions and typed-vector ABI prevent entry readiness.
- **VERIFIED selected instrumentation:** original PPC words use the fourteen-patch SDK decoder/builders/templates and an opt-in checked scalar/finite-FP/full-vector memory backend. Atomic/partial-vector operations and unsupported calls fail explicitly; the default mode is retained. No private generated output is changed or executed.
- **VERIFIED synthetic pipeline:** the actual pinned CLI generates an original nine-function PPC module; unchanged output executes through the checked memory layer and real patched SDK, including native XThread stack/PCR/TLS. A separate original fixture loader tests section/relocation/entry policy; normal title startup remains unverified.
- **UNKNOWN:** runtime correctness, shaders, decoded audio-bank format, game memory budget and stock-clock performance.

**Verdict: overall port feasibility PARTIAL; private game-code execution NO-GO.** Original synthetic callers exercise generated loop safe points, leased image pointers and cooperative lifetime contracts. Six scalar and three pointer signatures remain admitted; 24 imports remain denied. Host scheduling, full ABI, guest exceptions, general relocation, private memory mediation and Switch runtime integration remain unresolved. [Phase 18 evidence and limits](docs/research/generated-loop-stop-boundaries.md). [Feasibility and blockers](docs/feasibility.md).

## Technical approach

Proposed: privately validate and extract the owner's input, fingerprint the chosen XEX/update combination, validate pinned ReXGlue analysis and CPU semantics, then prove the required Horizon runtime and graphics behavior independently before integrating game code. Static CPU translation still requires guest memory/API compatibility and a correct graphics path.

The first architecture candidate is ReXGlue plus reviewed Horizon adaptations and a native Vulkan renderer through Mesa/NVK. Reference patches are not a universal drop-in. [Architecture](docs/architecture.md) · [Switch runtime](docs/research/switch-runtime.md)

## Research progress and roadmap

| Gate | Status |
| --- | --- |
| Reference survey | All 28 supplied repositories accessible; initial source/build comparison recorded |
| Private intake and extraction | Completed for the supplied GOD/base, DLC and title update; no update applied |
| XEX metadata and loader | Completed for the supplied `default.xex` |
| Function analysis / generation | Passed with five metadata hints; generated code remains private |
| Minimal compiler check | Two generated partitions compiled on Mac; complete link and execution untested |
| VMX / runtime source audit | Baseline imports classified; source alias/TLS/STFS corrections tested |
| Standalone memory/TLS foundation | Implemented/tested; selected real PPC export/PCR/TLS calls pass; coherent SDK integrated; full ABI and title launch remain unproven |
| Game runtime execution, shaders, audio and hardware | Not validated |
| Game build or distribution | Not started |

Next: Phase 19 CFG stop-policy and indexed site identity validation. Further synthetic integration is CONDITIONAL GO; private game execution remains NO-GO. [Roadmap](docs/roadmap.md).

## Game files and compatibility

Users must supply their own legally obtained game files. The supplied `live.dll` is a second XEX header variant with the same stored payload as `default.xex`; the separate title update is not applied. Neither variant nor any update is claimed playable. [Compatibility](docs/compatibility.md).

**Never upload** ISOs, XEX files, assets, music, shader binaries, keys, dumps or generated game code. Inputs, extracted files, local manifests, decoded images, full inventories/logs and build products must remain in a private workspace **outside public Git**. [Preparation policy](docs/research/game-file-preparation.md).

## Build and installation

No game build command or installation package exists. The repository contains research documentation, [original isolated VMX tests](tests/vmx128/README.md) and a publication check:

```sh
python3 tools/check_public_tree.py
```

[Single-command Phase 5–18 workflow](tests/generated-loop-stop/README.md) reproduces the current milestone with the nineteen-patch XEX-binding lock, actual codegen and three-profile regressions. [Isolated SDK build](tools/sdk/README.md) keeps dependencies, outputs and diagnostics private. [Phase 6 commands](tests/ppc/README.md) retain historical hybrid tests.

CI checks public contents/links, watchdog process-group cleanup, and builds/runs the original dependency-free memory/TLS components at O0/O2 and with sanitizers. It never fetches game data or the SDK, invokes codegen, builds the game or publishes releases. Overclocking is not a baseline requirement; no FPS or resolution is predicted.

## Contributing, credits and licensing

Research and project direction: **Wilwous**. See [CONTRIBUTING.md](CONTRIBUTING.md), [SECURITY.md](SECURITY.md), [reference comparison](docs/research/reference-projects.md) and [third-party boundaries](THIRD_PARTY_NOTICES.md). Local analysis used ReXGlue and xverter; their implementations and downloaded binaries are not vendored here.

Original foundation/tests/documentation are **GPL-3.0-only**; see [LICENSE](LICENSE). Focused SDK patch contributions follow **BSD-3-Clause** with [retained notices](patches/rexglue-sdk/README.md). Referenced components retain their own licenses. This license grants no rights to the game's code, assets, music or trademarks.

Unofficial research project with no affiliation with or endorsement by the game's rights holders, Microsoft, Nintendo or reference-port authors. No proprietary Nintendo SDK components are used or required by this repository.
