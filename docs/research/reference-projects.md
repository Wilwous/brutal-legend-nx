# Reference-project comparison

Research snapshot: **2026-10-19**. All **28 user-supplied repositories were accessible** through GitHub metadata, a commit and a non-truncated tree. Their README files were retrieved and screened. No project was built, run or independently tested on a Switch.

Evidence labels: **SRC** = selected source/build/patch contents inspected; **DOC** = author documentation or README claim; **TREE** = file-layout evidence only. **VERIFIED** source presence is not verified runtime behavior. **INFERRED** relevance is our engineering judgment. **UNKNOWN** means not established. The [machine-readable register](reference-snapshots.json) records full commits, detected licenses and retrieved file paths; it contains no game content. Licenses below are GitHub root detection unless an explicit file/notice inspection is described.

## Three most relevant projects (INFERRED ranking)

1. **project8-sw:** clearest primary architecture baseline: ordered ReXGlue v0.10.0 patches, Horizon runtime, native PM4/Vulkan renderer, memory/exception audits and build/driver pins. Reuse patterns and audited platform pieces, not game addresses or timing fixes.
2. **the-simpsons-game-nx:** shows adaptation of that architecture to a different game and SDK baseline, including media-decoder integration, replacement shaders and an explicit private codegen workflow. Its older SDK base demonstrates why patches must be rebased deliberately.
3. **Mercenaries2-NX:** a further integration using a Horizon SDK snapshot plus native-renderer extensions and GPU profiling/resolves. Useful for testing what survived another game's workloads, not evidence of shared engine internals with Brütal Legend.

**UnleashedRecomp-NX** remains a required fourth architectural comparison: its XenonRecomp/XenosRecomp/plume path is different and is not a ReXGlue integration. It is a fallback/design reference, not the recommended initial runtime base.

## Priority 1: Xbox 360 comparison

All eight inputs are Xbox 360 versions (including the XBLA edition of Sonic Adventure 2). None is an original-Xbox x86 lifter. “Native” CPU execution here still uses substantial guest system and graphics compatibility machinery.

| Project | Actual approach | Compiler/build evidence | Graphics evidence | Reuse restrictions / depth |
| --- | --- | --- | --- | --- |
| project8-sw | ReXGlue PPC → C++ AOT + Xenia-derived runtime + native renderer | SRC: C++23/CMake; DOC: devkitA64, libnx, Ninja, Docker, host Clang; v0.10.0 SDK baseline | SRC: native PM4/register system and Vulkan provider; DOC: runtime SPIR-V translation, host-image resolves and NVK/NAK patches | GPL-3.0 root; BSD SDK/MIT driver patch boundaries documented; SRC + DOC |
| the-simpsons-game-nx | ReXGlue AOT + adapted Horizon/native-renderer stack | SRC build: CMake; DOC: SDK 0.8 from TheSimpsonsGameRecomp plus patches, devkitA64/Docker | SRC: native system; DOC: NVK/Vulkan, runtime translation plus replacement shaders, FFmpeg VP6 hook | GPL-3.0 root; separate SDK/Mesa/LGPL notices; SRC + DOC |
| Mercenaries2-NX | ReXGlue AOT + Horizon SDK snapshot + adapted native renderer | SRC app CMake; DOC: host ReXGlue 0.10.0, masseffect-nx SDK snapshot, Docker/devkitA64 | SRC: register/PM4 processing and Vulkan initialization; DOC: custom resolves/tessellation/profiling and NVK | GPL-3.0 root; native README GPL-3.0-or-later; SDK/driver distinct; SRC + DOC |
| UnleashedRecomp-NX | XenonRecomp CPU AOT + XenosRecomp shaders + custom runtime/renderer | SRC: .gitmodules and CMake select both tools; DOC: devkitA64, host tools, DXC, FFmpeg, NVK | SRC: plume/SDL2 linked; DOC: Vulkan NVK driver integration and specialized renderer | GPL-3.0 root, separate MIT tools; SRC + DOC |
| sa2-nx | ReXGlue CPU AOT including extra stage module; shader AOT uses XenosRecomp/DXC | SRC: app CMake, vendored Horizon audio/input code; DOC: host SDK and devkitA64 build | DOC: direct draw/state replacement, NVK/Vulkan, privately generated shader/resource packs | GPL-3.0 root, multiple per-component notices; selected SRC + DOC; no performance verification |
| skate3-switch | ReXGlue AOT integration through external engine tree | SRC: build script delegates to external ENGINE/CMake preset; DOC: generated code read from engine worktree | DOC: NXVK Vulkan; native mixer/guest waits and memory strategy described | No detected root license; notices describe component licenses and a GPL-2.0-or-later binary claim that needs separate audit; external dependency closure not reproduced |
| prototype2-nx | ReXGlue AOT + patched Horizon SDK + native renderer | SRC: project CMake; DOC: 0.10.0 host, masseffect SDK, Docker/devkitA64 and LTO | DOC/TREE: Vulkan native renderer and Mesa/NVK patches | GPL-3.0 root, mixed dependencies; selected build/notice inspection, runtime not audited |
| MarathonRecomp-NX | XenonRecomp/XenosRecomp AOT + game-specific runtime | SRC: .gitmodules/CMake; DOC: devkitA64, host tools, FFmpeg, NVK | DOC: plume/Vulkan and external driver build | GPL-3.0 root; exact fork/tool license closure not audited; selected SRC + DOC |

## Four initial references: subsystem evidence

| Area | project8-sw | the-simpsons-game-nx | Mercenaries2-NX | UnleashedRecomp-NX |
| --- | --- | --- | --- | --- |
| Host vs target | Separate native rexglue and Horizon libraries in build/source | Native codegen before cross build in build guide | Windows host codegen followed by Docker cross build in guide | Host Xenon/Xenos tools explicitly configured in CMake |
| Kernel/runtime | Horizon memory/threads/fibers/exceptions in inspected SDK patch | Port overlay over game's vendored SDK; details depend on baseline | masseffect-nx SDK + patch documented; actual external SDK not built/audited | Bespoke kernel/runtime; selected Switch memory implementation inspected |
| Memory | Alias/protection/write-watch limitations documented; later fault/SIMD patches exist | Adapted platform stack; no independent memory result | Same-family platform stack; no independent memory result | Source uses process-code alias and process-memory mapping SVCs; permissions/success are hardware-dependent |
| Audio | Inspected overlay implements libnx audout; SDK XMA/FFmpeg support | DOC: SDK audio and native FFmpeg VP6 cutscenes | Notices/build describe SDK XMA/FFmpeg; no game audio execution verified | Source XAudio callback/frame bridge; CMake links SDL2_mixer; full mixer not audited |
| Input | libnx driver in inspected Horizon overlay | DOC: adapted Horizon controller layer, letter/position mapping | DOC: Horizon controller layer through external SDK | Source SDL event-based input patches; Switch HID implementation exists in tree, not audited |
| Files/saves | SDK filesystem and writable user directories; title-specific semantics remain | DOC: external game root and separate saves/cache | DOC: separate game_root and logs | Custom runtime/integration; no save implementation audit |
| Renderer | PM4 thread, register state and native Vulkan initialization in source | Same-family native system in source | Adapted PM4/native Vulkan system in source | Custom game-specific renderer; plume dependency and NVK build path |
| Hardware evidence in this research | None | None | None | None |

## Exact source evidence and portability limits

### project8-sw

- [Build and pinned dependencies](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/docs/building.md): SDK `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` (tag v0.10.0), ordered overlay, and Mesa base `d4a00ea0ab3f59afb967cc5d779e4263d237bd77` (shortened in that guide).
- [Horizon runtime patch](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/patches/rexglue-sdk/0004-switch-horizon-runtime.patch): actual audio/controller, memory, exception/fiber and build adaptations. [FP/SIMD follow-up](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/patches/rexglue-sdk/0006-switch-exception-fp-state.patch) illustrates correctness obligations beyond ARM64 compilation.
- [Native system source](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/src/game/src/native/thps_native_system.cpp) and [renderer design](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/docs/native-renderer.md): ring/register/interrupt handling, resource conversion and host-image resolve decisions. Correctness assumptions must be retested per game.
- [Memory audit](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/docs/SWITCH_MEMORY_REPROTECTION.md): a working alias map is insufficient to prove reprotection/write-watch semantics. [Vulkan capability report](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/docs/SWITCH_VULKAN_CAPABILITIES.md) is reference-specific, not a universal driver feature contract.

**DOC conflict:** the top README reports gameplay, while the SDK-patches README and historical audits still describe earlier nonplayable or unverified stages. Treat those documents as reports from different development stages, not independent current hardware validation. Performance statements involving altered clocks do not establish Brütal Legend feasibility.

### the-simpsons-game-nx

[Build guide](https://github.com/R-drg/the-simpsons-game-nx/blob/3cb2cdf7fa1665a1f2d653aa7502fe4baa03680f/docs/building.md) pins TheSimpsonsGameRecomp `285bea3259ce194c28f9e9ef01ff6897e4893734` and documents its modified 0.8 SDK. [Native system](https://github.com/R-drg/the-simpsons-game-nx/blob/3cb2cdf7fa1665a1f2d653aa7502fe4baa03680f/src/game/src/native/native_system.cpp) establishes that an implementation exists. [Notices](https://github.com/R-drg/the-simpsons-game-nx/blob/3cb2cdf7fa1665a1f2d653aa7502fe4baa03680f/THIRD_PARTY_NOTICES.md) explain the Project 8/native-renderer ancestry and component boundaries. This is not the same patch base as upstream 0.10.0. Its VP6 work is evidence for that game's needs, not this game's movie format.

### Mercenaries2-NX

[Build guide](https://github.com/penettrator/Mercenaries2-NX/blob/65167852496f94b278a82ffea465c51703575f01/docs/building.md) pins the masseffect-nx snapshot `a85b0b419ec3e332165ae1617c980bd399856a44`, host ReXGlue `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, and Mesa `d4a00ea0ab3f59afb967cc5d779e4263d237bd77`. [Native source](https://github.com/penettrator/Mercenaries2-NX/blob/65167852496f94b278a82ffea465c51703575f01/app/src/native/mercs2_native_system.cpp) shows Vulkan setup and register processing. [Native README](https://github.com/penettrator/Mercenaries2-NX/blob/65167852496f94b278a82ffea465c51703575f01/app/src/native/README.md) explicitly credits copied/adapted Project 8 code and its GPL terms.

**Configuration distinction:** the source cvar default for `mercs2_renderer` is `xenos`; the native README describes an earlier default state, while the main README describes the packaged native path. Do not infer the effective release configuration from the cvar default alone. No release was downloaded or tested. Forwarder/overclock instructions in the reference are not baseline requirements adopted here.

### UnleashedRecomp-NX

[Submodules](https://github.com/ChanseyIsTheBest/UnleashedRecomp-NX/blob/b346e37e02987229e57c7f8b6557dc9211257c21/.gitmodules) identify XenonRecomp, XenosRecomp and plume. [App CMake](https://github.com/ChanseyIsTheBest/UnleashedRecomp-NX/blob/b346e37e02987229e57c7f8b6557dc9211257c21/UnleashedRecomp/CMakeLists.txt) separates host code generators and target runtime, links SDL2/SDL2_mixer/plume, and includes Switch-specific paths. [Memory source](https://github.com/ChanseyIsTheBest/UnleashedRecomp-NX/blob/b346e37e02987229e57c7f8b6557dc9211257c21/UnleashedRecomp/kernel/memory.cpp) demonstrates its separate alias strategy; [Mesa integration](https://github.com/ChanseyIsTheBest/UnleashedRecomp-NX/blob/b346e37e02987229e57c7f8b6557dc9211257c21/docs/SWITCH-MESA.md) documents a different driver/renderer tuning stack. The largely upstream PC README is insufficient to establish Switch behavior on its own.

## Lower-priority screening (DOC/TREE, not implementation audits)

These projects were checked for existence, README claims, tree and detected root license. Detailed audio/memory/input/compiler correctness and full reuse-license closure are **UNKNOWN unless stated**. Their different ISAs and platform APIs make them secondary patterns rather than Xbox 360 runtime candidates. No dynamic translator or source-port classification is inferred merely from the word “native.”

| Repository (pinned README) | Actual source / approach described | Graphics/build pattern documented | Detected root license / relevance |
| --- | --- | --- | --- |
| [antoxa2584x/nfsu1-sw](https://github.com/antoxa2584x/nfsu1-sw/blob/d597e08f88e3f415eb44366b22e75054f2dfd15d/README.md) | Original Xbox x86/XBE → C static lifting + API/runtime replacements | Vulkan/NVK and OpenGL; devkitA64/Linux | UNKNOWN; packaging/runtime patterns, no PPC reuse |
| [antoxa2584x/nfsu2-sw](https://github.com/antoxa2584x/nfsu2-sw/blob/d3bbc3094f8e1ad3b34593eabd431f7d251b14cb/README.md) | Original Xbox x86 static lifting + API/runtime replacements | Vulkan/NVK and OpenGL; devkitA64/Linux | UNKNOWN; different CPU and NV2A graphics |
| [antoxa2584x/nfsc-sw](https://github.com/antoxa2584x/nfsc-sw/blob/655ec76ccec0756a00ee017b099f89461f496f82/README.md) | Original Xbox version, x86 static lifting (not Xbox 360 Carbon) | Vulkan/NVK and OpenGL | UNKNOWN; different ABI/GPU |
| [antoxa2584x/burnout3-sw](https://github.com/antoxa2584x/burnout3-sw/blob/2b379b3885c83e5dae9f2b16218a0dcfce896265/README.md) | Original Xbox x86 static lifting | Vulkan/NVK and OpenGL | MIT root; audit components before reuse |
| [antoxa2584x/flatout2-sw](https://github.com/antoxa2584x/flatout2-sw/blob/a89f73d64ab57c2c97ccbb4d31b5aa5b64bd7f21/README.md) | Original Xbox x86 static lifting | Vulkan/NVK, D3D8 runtime family | MIT root; general integration patterns only |
| [R-drg/defjam-sw](https://github.com/R-drg/defjam-sw/blob/5381916d7075c285491e87d30bca1d5e72d2bd82/README.md) | Original Xbox x86 → C lifting with xboxrecomp and kernel/NV2A/audio compatibility | Switch/Linux runtime integration | NOASSERTION; permission scope unresolved |
| [thelinkin3000/halo-ce-universal](https://github.com/thelinkin3000/halo-ce-universal/blob/c69b144634e2feee3b4561dca9ca7362ffc01e26/README.md) | Xbox decompilation → native source port with platform replacements | Switch deko3d; other platforms Vulkan/OpenGL | CC0-1.0 root; underlying decomp/dependency boundaries separate |
| [fildicio/jak-project-switch](https://github.com/fildicio/jak-project-switch/blob/fea3f4577c3f112c47204482f8c75e753ec930ec/README.md) | PS2 Jak trilogy via OpenGOAL decompile/recompile workflow | ARM64 GOAL tooling, asset extraction | ISC root; language/engine pipeline is not Xenon PPC |
| [NaGaa95/ProjectCane](https://github.com/NaGaa95/ProjectCane/blob/13dcb79ad6f80dc72e330cf18f31f69594419864/README.md) | PS2 Sly Cooper reverse-engineered native PC reconstruction | Windows/Linux CMake, C++20 and FFmpeg; no Switch claim established from this README | UNKNOWN; not evidence of a Switch ReXGlue port |
| [artslay/NearChuckle_nx](https://github.com/artslay/NearChuckle_nx/blob/4c6df5b2e19f354ae2a7dba899ef0573bf184eb4/README.md) | Far Cry Android/Linux NearChuckle port through VNX Translation Core; translation type not established | Mesa, SDL3, devkitA64/Make | UNKNOWN; CPU translation mode and original-binary lineage need audit |
| [cutarev/sh2-nx](https://github.com/cutarev/sh2-nx/blob/c3c9c0afba8c76b0ff2645ac3c1d00f985a9479e/README.md) | PC x86 executable → C AOT + Windows API replacements | SDL2/OpenGL/FFmpeg; Linux debugging path | UNKNOWN; not a PS2-to-Switch emulator/port |
| [cutarev/sh3-nx](https://github.com/cutarev/sh3-nx/blob/a1f99dc97a81c62f224f1ee7eca71c0392789c8c/README.md) | PC x86 executable → C AOT + Windows API replacements | SDL2/OpenGL | UNKNOWN; different guest APIs |
| [72ak/sh4-nx](https://github.com/72ak/sh4-nx/blob/f58c3f9e64612ab193920c1d12c85305ef231d9e/README.md) | PC SILENT HILL 4.exe → C via patched xboxrecomp | cutarev SDL2/OpenGL runtime family | UNKNOWN; mentions PS2 motivation but input is PC x86 |
| [r4dius/outrun2-decomp](https://github.com/r4dius/outrun2-decomp/blob/e1e1c592b802a6f3f7020a676abd3ba1be479e62/README.md) | PC executable reconstruction in C++, with remaining x86-translated portions | Per-platform native folders, including Switch | UNKNOWN; hybrid reconstruction/lifting, not complete source recovery |
| [hazevauks/openPREYswitch](https://github.com/hazevauks/openPREYswitch/blob/d4e942490f7bf516f55a256fb09894fc922a10ec/README.md) | Prey PC id Tech 4 engine source port plus game-code work | SDL2/OpenGL via Mesa, devkitPro | GPL-3.0 root; source-port strategy requires available engine source |
| [centollOS/SwitchWaker](https://github.com/centollOS/SwitchWaker/blob/9e71996e948d88b01523ea591c61f6131e9d7c51/README.md) | GameCube Wind Waker decompilation compiled natively with platform replacements | Aurora/RecompCore lineage | MIT root; GameCube SDK/graphics differ from Xbox 360 |
| [givethesourceplox/dusklight-NX](https://github.com/givethesourceplox/dusklight-NX/blob/111682f21eea5378594fa34d58e399f0a25eec26/README.md) | Twilight Princess reverse-engineered reimplementation/decompilation lineage | Aurora; D3D12/Vulkan/Metal family documented | CC0-1.0 root; game/dependency rights separate |
| [chasem-dev/sms-switch](https://github.com/chasem-dev/sms-switch/blob/b16ef264ce19cc0a9db29dc0a83a011ce3b4fb76/README.md) | GameCube Super Mario Sunshine decompilation → ARM64 native port | OpenGL/SDL through Switch platform layer | MIT root; README explicitly says early/emulator-tested, not hardware proof |
| [centollOS/SwitchWakerHD](https://github.com/centollOS/SwitchWakerHD/blob/093c96cf590d23b07472e3cf2aa66f9a7a32cd4e/README.md) | Wii U PPC/RPX → C AOT + Cafe OS replacements | GX2 → deko3d | MPL-2.0 root; PPC similarity does not supply Xbox kernel/Xenos support |
| [fsalamancar/VCSNX](https://github.com/fsalamancar/VCSNX/blob/694cd0a2d18529879fda96f9a0639b75d4db5b8a/README.md) | PSP game code → C++ AOT, separate game module/runtime | Vulkan/Mesa NVK, SDL2, local module build | NOASSERTION; MIPS/PSP runtime not Xenon/ReXGlue |

## Transferable patterns and rejected assumptions

**INFERRED reusable patterns:** exact executable fingerprint gates, separate host/cross builds, pinned SDK/driver patches, private generation, isolated saves/caches, explicit controller layout, on-device diagnostics, renderer feature census and small platform probes.

**Do not transfer without evidence:** game addresses/function maps, shader replacements, archive loaders, busy-wait patches, native mixer substitutions, clock presets, discarded rendering work, save formats or “playable” labels. Similar publishers, release periods or game genres establish none of those relationships.

**License gate:** no implementation was copied. The top ReXGlue ports have GPL application/native-renderer code and separately licensed SDK/driver changes. Other inputs include missing or nonstandard detected licenses. [Third-party boundaries](../../THIRD_PARTY_NOTICES.md) govern any later reuse. Some reference build layouts keep generated output under ignored repository directories; this project's stricter policy requires it outside the public checkout.

## Verification method and limits

GitHub API metadata, exact default-branch commit, recursive tree and raw README were checked for every supplied repository. Selected build files, runtime patches, native-system sources and notices were inspected for the primary set. Public source links are pinned to those commits. An initial anonymous API rate limit was resolved using the existing authenticated GitHub CLI; it was not recorded as repository absence. No inferred URL replacement was substituted for an unavailable repository.

A speculative `XboxDev/xdvdfs` candidate was unavailable at that path and was not selected or counted among the 28 supplied references. The verified extraction candidate is XboxDev/extract-xiso. Secondary screening is intentionally shallower than the four initial architectural investigations. No claims are made about full compilation success, tested releases, uninspected runtime subsystems or performance of any reference on our hardware.
