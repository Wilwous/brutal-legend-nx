# Xbox 360 binary analysis — Phase 2

Assessment: **2026-10-19 · v0.0.2-binary-analysis**. This supersedes Phase 1's missing-input status. Facts below apply only to the supplied bytes, not every retail/digital edition.

**VERIFIED** = direct file/tool/source observation. **INFERRED** = interpretation requiring further testing. **UNKNOWN** = not established. **BLOCKED** = a specific current obstacle or bounded operation that did not complete. Source/header hashes are metadata, not an authenticity certificate.

## Discovery, preservation and extraction

The supplied path is a directory. It contains a `454108C5` title directory with:

| Content | Format evidence | Source files / bytes | Extraction result |
| --- | --- | --- | --- |
| Base game | `LIVE`, content type `0x00007000`, SVOD data fragments; XSF marker and embedded XDVDFS descriptor | One 45,056-byte header + 33 fragments; 5,572,784,128 bytes total | 284 files, 36 directories, 5,544,498,100 bytes |
| DLC | Three `LIVE` packages, content type `0x00000002` | 149,217,280; 57,344; 61,440 bytes | 8, 1 and 2 files, separately extracted |
| Title update | `LIVE`, content type `0x000B0000` | 2,150,400 bytes | Four files, separately extracted; not applied |

No standalone ISO or unpacked XEX existed at the supplied location. An unrelated desktop metadata file was excluded from the game-file manifest. All **38 source game files (5,724,270,592 bytes)** were opened read-only and SHA-256 hashed before and after analysis; hashes, sizes and modification times match. No original was rewritten. Extracted data, decoded images, logs and manifests remain outside Git.

The locally used open-source [xverter revision](https://github.com/blank-query/xverter/tree/75c196aefe28692bd02dbdf8fb552485759733cd) is MIT-licensed. Its GOD reader checked the header self-hash, master/subtable hash chain and all data blocks; full verification passed. This verifies internal consistency, not Microsoft signatures, legal provenance or equivalence to an untouched retail dump.

The logical stream is **5,545,418,752 bytes**, with `MICROSOFT*XBOX*MEDIA` at logical `0x10000`; its largest allocated extent is **5,545,416,704**, within the stream. A read-only preflight found 320 entries, no unsafe/colliding paths or overlapping file extents, and valid bounds. Direct stream extraction avoided creating an intermediate ISO. Base, DLC and update extraction all passed the reader's hash checks. Total extracted: **299 files / 5,694,838,936 bytes**.

## Main executable identity

| Field | VERIFIED value |
| --- | --- |
| Main candidate | Base game root `default.xex` |
| File size | 5,677,056 bytes |
| SHA-256 | `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456` |
| Magic / flags | XEX2 / module flags `0x00000001` |
| XEX header | 12,288 bytes; 15 optional headers; security record offset `0x90` |
| Title ID / Media ID | `454108C5` / `7DD5E14B` |
| Execution version / base version | Both raw `0x00000002`, decoded with SDK layout as `0.0.0.2` |
| Disc | 1 of 1 |
| Guest machine | PE `0x01F2` (`POWERPCBE`), PE32 optional magic `0x010B`, Xbox subsystem 14 |
| Image base / entry | `0x82000000` / `0x827F1498` |
| Encryption / compression | XEX normal encryption (1); normal/LZX compression (2); window 32,768 bytes |
| XEX image size / loaded allocation | 14,090,240 bytes (`0xD70000`) |
| PE SizeOfImage | 15,750,656 bytes; a different header field, not measured resident RAM |
| Default stack | 262,144 bytes (`0x40000`) |
| Default heap optional header | Absent; no heap budget inferred |
| TLS | 64 slots; raw address `0x82CB0E00`; data/raw size 28 bytes |
| System flags / region mask | `0x600` / `0xFFFFFFFF` |
| Allowed-media mask | `0x4` |

The version is the XEX bitfield value, **not a claimed marketing version or title-update number**. Region/media metadata does not authenticate the copy. No encryption keys, payload bytes, signatures or proprietary instructions are published.

## Additional executable and update

`live.dll` is also XEX2, **not an ordinary host DLL**. Its module flags, image base, entry point and file length match the main candidate. SHA-256: `0fc273f7abc9935b1ad615a5a7dfa3bbf6f5eb8236fffdad636ea35420345027`.

Its Media ID is `6DD37C21`, execution/base version raw `0x8` (`0.0.0.8`), system flags `0x604`, media mask `0x1000`, and import library version/minimum `2.0.16537.0`. Exactly **367 stored bytes differ**, all before payload offset `0x3000`; the entire stored encrypted payload is byte-identical. **INFERRED:** these are header variants rather than evidence of two independently compiled code bodies. Which represents an untouched source edition is **UNKNOWN**. Identical stored payloads do not independently prove identical decoded images under every header/key interpretation.

The analysis selected `default.xex` only. `live.dll` was neither registered as a runtime module nor automatically scanned into a manifest. Their identical preferred base is an additional reason not to assume it is an independent DLL. Presence and imported `XexLoadImage` alone do not establish that the game needs to load it.

The separate update contains `default.xexp` (1,374,208 bytes), a patch archive pair and an audio bank. The XEXP has XEX2 patch flags `0x50`, encryption 1 and delta compression 3. Its descriptor names source version raw `0x2` and target `0x102` (`0.0.1.2`); that matches the base's version field but is not a complete patch-compatibility proof. Its SHA-256 is `b2cfb6f6e0fb51cc7198ac054e0ad378c80c82db15415caa69e7e96eb5c4afb2`. It was **not applied**, and no updated executable was generated.

## Sections and memory-layout evidence

A metadata-only native helper used ReXGlue's tool-mode loader, without calling game entry code, to inspect the loaded image. The PE declares 14 sections:

| Section | Guest VA | Virtual bytes | Characteristics |
| --- | --- | --- | --- |
| `.rdata` | `0x82000600` | 865,068 | `0x40000040` |
| `.pdata` | `0x820D3A00` | 263,288 | `0x40000040` |
| `BINKCONS` | `0x82114000` | 10,528 | `0x40000040` |
| `.text` | `0x82120000` | 11,101,064 | `0x60000020` |
| `BINK` | `0x82BB6400` | 65,320 | `0x60000020` |
| `BINKBSS` | `0x82BD0000` | 17,568 | `0xc0000080` |
| `.data` | `0x82BD4600` | 886,904 | `0xc0000040` |
| `BINKDATA` | `0x82CAD000` | 15,732 | `0xc0000040` |
| `.tls` | `0x82CB0E00` | 29 | `0xc0000040` |
| `.XBMOVIE` | `0x82CB1000` | 8 | `0xc0000040` |
| `.edata` | `0x82CC0000` | 7,490 | `0x40000040` |
| `.idata` | `0x82CD0000` | 1,214 | `0xc0000040` |
| `.XBLD` | `0x82CE0000` | 272 | `0x42000040` |
| `.reloc` | `0x82CE0200` | 775,292 | `0x42000040` |

The SDK analysis view includes **13** sections. It skips `.reloc` because the declared section extends past the loaded XEX image end. That is an observed loader/analysis choice, not by itself proof of corruption or support for arbitrary rebasing. The relocation directory is RVA `0xCE0200`, size 775,292; preserve this distinction when evaluating relocations.

The PE exception directory is RVA `0xD3A00`, size **263,288** (`.pdata`). Guest exception handling is relevant: imports include `RtlRaiseException`, `RtlUnwind`, `__C_specific_handler` and context routines. Neither unwind correctness nor exceptional control-flow coverage has been demonstrated. Header stack/image/TLS sizes do not establish heap, decoder, GPU, cache or total memory peaks.

## Imports and runtime requirements

The clear XEX import records contain **164 records for xam.xex** and **371 for xboxkrnl.exe**. These are record counts, not distinct functions: function imports have value/thunk records. Both request library version `2.0.8276.0`, minimum `2.0.7371.0`.

The loader reports **274 logical imports: 82 xam + 192 xboxkrnl**, comprising **261 function imports + 13 variable imports**. A registry lookup found no unknown ordinal, and `nm -gU` on the downloaded SDK runtime found an exported `__imp__` shim for every imported function. Variable mappings are present in the loader result. This establishes **symbol availability**, not full implementation or runtime correctness.

SDK export tags are mostly unset for these function entries even when their executable shims exist; treating missing `kImplemented` tags as 261 missing implementations would be incorrect. Representative source review finds real implementations and explicit stubs:

| Required area | Static/import evidence | Current conclusion |
| --- | --- | --- |
| Threads and synchronization | ExCreateThread, affinity/priority, events, semaphores, waits, APCs and critical sections | Foundation exists; actual thread count, scheduling and Horizon semantics untested |
| Memory | NtAllocate/Free/QueryVirtualMemory, physical allocation/mapping and protection queries | Guest mapping and coherence must be proved separately on Switch |
| Files and content/saves | NtCreate/Read/WriteFile, async/scatter reads and XamContent functions | SDK paths exist; game's save/streaming/container semantics not exercised |
| Audio | XAudio client/frame callbacks and XMA context/buffer APIs; XAUDIO2/XAUDLTCG static-library records | Existing SDK foundation; actual bank decode/mixer/movie synchronization untested |
| Input | XamInputGetState/SetState/GetCapabilities | Host adapters exist; no gameplay input test |
| Graphics | D3D9LTCG/D3DX9/XGRAPHC static records, Vd ring/engine/swap APIs | Xbox 360 graphics path present; no shader or renderer correctness proof |
| Exceptions | RtlRaiseException, RtlUnwind and __C_specific_handler | **BLOCKER:** latter two are explicit stubs in the inspected SDK |
| STFS | StfsCreateDevice and StfsControlDevice | **BLOCKER:** source stubs return success without the complete required behavior; whether those paths execute is unknown |
| Modules, networking/profile/voice | XexLoadImage, NetDll, XamUser/Session/Voice families | Some paths may be optional, but none may be assumed irrelevant without traces |

Sources at the exact SDK commit: [exception stubs](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_rtl.cpp#L562), [STFS stubs](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_io.cpp#L749), [module loading](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/kernel/xboxkrnl/xboxkrnl_modules.cpp#L86). This is a representative semantic audit, not a claim to have audited all 261 implementations.

Seventeen static-library header records identify SDK libraries at build `8276`, including XAPILIB, LIBCMT, XBOXKRNL, XMP, XGRAPHC, XONLINE, D3DX9, XAUDIO2, XAPOBA, XMIC, XPARTY, XHV2, XMCOREL, XUIRUNL, XUIRNDRL, D3D9LTCG and XAUDLTCG. Static-library records are not 17 dynamically imported modules.

## Resource inventory (base game)

| Family | Count | Total bytes | Evidence / interpretation |
| --- | --- | --- | --- |
| Archive headers/indexes (`.~h`) | 19 | 4,334,788 | All begin `dfpf`; bounded string inspection identifies named resource categories |
| Archive payloads (`.~p`) | 19 | 1,152,825,344 | Paired filenames; layouts/compression not comprehensively parsed |
| Sound-bank-named files (`.fsb`) | 157 | 2,131,829,952 | No ordinary FSB magic at byte zero; precise encoding/transform remains UNKNOWN |
| Movies (`.bik`) | 74 | 2,242,446,728 | All 74 begin `BIKi`, consistent with Bink video; no decoding performed |
| Configuration/scripts (`.lua`) | 5 | 229,178 | Named Lua files; contents remain private |
| Configuration (`.cfg`) | 2 | 16,076 | Buddha configuration filenames |
| Fonts (`.ttf`) | 2 | 395,484 | Two files with TrueType signatures |
| Main executable (`.xex`) | 1 | 5,677,056 | Fingerprint above |
| XEX header variant (`.dll`) | 1 | 5,677,056 | Fingerprint and caveat above |

The remaining four files comprise two extensionless files, one install manifest and one preload file (1,066,438 bytes total). The table plus those four reconcile to 284 base files / 5,544,498,100 bytes. Full per-file hashes and paths remain private.

Index strings identify texture, mesh-related and animation resource names, and startup paths under `bin/xenon/shaders/` ending in `.fxo`. They establish where future asset/shader investigation should start, not a verified archive parser or a complete shader census. Separate localization archive pairs are named for `enUS`, `deDE`, `esES`, `frFR` and `itIT`; an English voice-bank path exists. Actual language completeness and runtime selection remain untested.

Loaded-image strings and RTTI provide evidence for **FMOD**, **Havok** physics/animation, **Bink** and **Lua 5.1.2**. A single Scaleform marker is weak evidence of an additional UI dependency; its exact integration/version is **UNKNOWN**. Middleware strings can be retained unused code, so their presence does not establish every active runtime dependency. No proprietary middleware SDK or replacement implementation was imported.

The 157 `.fsb` filenames and FMOD strings make an FMOD bank path plausible, but their nonstandard leading bytes prohibit claiming verified FSB headers or codecs. Do not start a decoder from extension alone. The large banks and movies call for streaming/buffering analysis; storage size is not a resident-memory requirement.

## Bounded ReXGlue 0.10.0 result

The official **macOS ARM64 SDK release** ran natively. `rexglue --version` returned `0.10.0`; its `codegen --help` was inspected. The tag resolves to `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. No SDK dependency build, package-manager install or global configuration change was needed.

A fresh private manifest selected only `default.xex`, with no update, DLL scan, game-specific overrides, error forcing or optimization settings. The actual tool command was `rexglue --log-level debug --log-file "$PRIVATE_POC/rexglue.log" codegen "$PRIVATE_POC/brutal_legend_manifest.toml"` with all resolved paths private.

Limits: **45 seconds wall time, 60 seconds CPU, 2 GiB sampled RSS, 32 MiB logs, 256 MiB generated files, core dumps disabled**. The supervisor stopped the process with SIGTERM at **45.309 seconds**, exit **-15**, because the wall-time limit was reached. Peak sampled RSS was **144,637,952 bytes**; this is a tool measurement, not a game memory estimate.

**VERIFIED:** XEX decryption/decompression and loading succeeded; section inspection and import discovery succeeded; analysis reached Register → Scan → Discover → **GapFill**. Vtable discovery found game/middleware RTTI. **BLOCKED:** Merge/Validate and C++ emission were not reached within the bound. The only generated file was SDK CMake glue. **No translated game C++**, shader translation, game compilation or game execution occurred. A timeout is not proof of an infinite loop, unsupported instructions or an impossible recompile.

A separate original metadata-only helper linked to the same prebuilt runtime in seconds, using `tool_mode=true` and no entry-point call. It returned zero and produced the section/import/header observations above. Initial compilation needed the SDK's external-fmt define; the successful compile retained one SDK FPCR inline-assembly operand-width warning. That helper is not a port, compiler or proof of FP instruction semantics.

## Outcome and next task

**VERIFIED:** supplied GOD is readable and extractable, the selected encrypted XEX can be processed through ReXGlue's loader, and expected kernel symbols exist. **UNKNOWN:** complete function/instruction coverage, exception correctness, image/update provenance, active stub paths, bank transforms, shader features and Switch memory/performance. **BLOCKED:** function analysis did not complete within the deliberately limited run; known required API stubs remain unaudited in execution.

**CONDITIONAL GO:** continue bounded host analysis. Do not implement the full port or speculate a playable build. The next task is to profile the GapFill stage on the same `default.xex` hash with a fixed further budget and compact diagnostics, establish whether it progresses, and produce a validated function/unresolved-target/instruction report before any full code generation. Keep `live.dll` and the update out of that baseline until their header/update relationship is separately validated.

[Commands and reproducibility limits](phase2-commands.md) · [Blocker register](../feasibility.md) · [Architecture](../architecture.md)
