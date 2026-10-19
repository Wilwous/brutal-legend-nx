# Xbox 360 executable analysis

## Actual input result

**VERIFIED:** the user supplied a text brief. A search including hidden/ignored files in the task workspace and supplied attachment directory found no `.iso`, `.ISO`, `.xex`, or `.XEX`. This is a scoped search, not a claim that the user's computer contains no game image elsewhere.

No ISO was opened, mounted, rewritten or extracted. No XEX was read or hashed. No proprietary code or assets were generated. All game-specific fields below are **UNKNOWN**, not inferred from another edition or an internet database.

| Required field | Result / method when input exists |
| --- | --- |
| Image format/partition | UNKNOWN; inspect XDVDFS signatures and extents rather than trusting extension/size |
| Image readability/integrity | UNKNOWN; full sequential hash plus bounded directory traversal and complete extraction |
| Original XEX identity | UNKNOWN; exact byte count, SHA-256, `XEX2` magic |
| Game identity | UNKNOWN; execution info title ID, media ID, version/base version, disc fields; correlate with provenance |
| Header/loading | UNKNOWN; flags, optional headers, image base/entry, image size, encryption/compression modes |
| PE sections | UNKNOWN; names, guest VA/RVA, virtual/raw sizes, flags and file bounds |
| Imports/exports | UNKNOWN; library names/versions, ordinals, thunks/variables, exports and unresolved entries |
| Runtime state | UNKNOWN; TLS, stack/heap requirements, exception and `.pdata` metadata, required system flags |
| Dynamic code/modules | UNKNOWN; extra XEX/DLL modules, title-update patches, computed dispatch and executable generation |
| Resources | UNKNOWN; archive families, paths, sizes/counts, shaders, textures, music, video and localization |
| Middleware | UNKNOWN; only identify from explicit imports, strings and code/resource evidence |
| GPU/audio/input/saves | UNKNOWN; require binary inventory plus focused traces |
| Real memory/performance | UNKNOWN; header hints cannot establish runtime peaks |

## Verified tool capabilities (not a game result)

At [ReXGlue c94f5eb](https://github.com/rexglue/rexglue-sdk/tree/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518):

- [`XexModule`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/system/xex_module.cpp) implements XEX image loading and PE header/section reading, import handling and none/basic/normal compression branches. The normal path uses LZX decompression. Loader acceptance must be tested on the actual bytes; key material must never enter project reports.
- [`xex2_info.h`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/include/rex/system/util/xex2_info.h) defines execution/version, TLS, import and image fields. Header evidence alone does not identify an applied title update with certainty.
- [`Analyze`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/include/rex/codegen/analyze.h) declares registration, scanning, function discovery, vtable, gap-fill, merge and validation phases. Manual function/chunk, jump-table and exception hints exist in [`RecompilerConfig`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/include/rex/codegen/config.h). This is not proof that all functions in this game can be found.
- The inspected [`CLI`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/rexglue/main.cpp) registers codegen/init/recompiler-test commands. `codegen` performs analysis and generation together; no standalone public `rexglue analyze` or `rexglue info` command was established. Do not run a guessed command or use force to hide diagnostics.
- [`DecodedBinary`](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/src/codegen/decoded_binary.cpp) decodes executable sections in four-byte units using endian conversion. Host ARM64 compilation remains a separate step.
- The runtime contains guest context/dispatch, XMA decoding, memory and kernel services. Coverage of those components for Brütal Legend is untested.

## PPC → ARM64 assessment

The Xbox 360 input is expected to contain Xenon PowerPC code, but this file's machine type must still be verified. Generated C++ is an instruction-level translation with guest register/memory conventions, not recovered original source. Preserve big-endian loads/stores, VMX128 lane behavior, floating-point status/rounding, atomics/reservations, ABI register conventions, indirect dispatch, TLS and exception semantics. ARM64/SIMDe paths exist in related tools, yet do not imply correct coverage of every instruction or runtime behavior used here.

Exception support is tool/version dependent. XenonRecomp's inspected README states that guest exceptions are unsupported. ReXGlue's current config includes optional exception-handler generation, while Horizon reference overlays add their own host exception/SIMD restoration work. These facts are different and cannot be reduced to “exceptions supported.” Guest exception behavior and host fault recovery each need tests.

Relocations/import patching and guest addresses must be evaluated separately from native host linking. Enumerate actual relocations/base-address requirements and imported variables, then check that generated calls and runtime data reference the same mappings. A linked ARM64 binary is not a validation of those relationships.

## Existing alternatives

[XenonRecomp](https://github.com/hedge-dev/XenonRecomp/tree/ddd128bcca99fe8bfbb99bea583c972351fa6ace) is a credible fallback CPU translator, with its own function/jump-table limitations, ABI and runtime responsibilities. [XenosRecomp](https://github.com/hedge-dev/XenosRecomp/tree/990d03b28a27b50277ee5d8d942e1c5f873869d1) translates shader binaries to HLSL, then DXC can produce SPIR-V; its documented game-specific assumptions and missing features require adaptation. Neither supplies a generic complete Switch port.

Full emulation is outside the native-execution objective, but an emulator or original hardware can later provide a private behavioral comparison. A new compiler is not justified by current evidence.

## ReXGlue decision gate

Current answer to “can ReXGlue process this executable?”: **UNKNOWN — executable absent and tool not run**. Establish loader acceptance, metadata correctness, analysis coverage, instruction/exception support, all module requirements and imports before any implementation decision. Keep full diagnostics and generated output private; publish only a sanitized table of successes, failures and unresolved calls.
