# Phase 2 commands and results

Assessment: **2026-10-19**. This is an execution record, not a game build guide. Paths below use neutral placeholders; actual paths, full logs, original Python/C++ analysis helpers and manifests remain in the private working directory. Do not paste a real personal path into an issue or public document.

`SOURCE_ROOT` denotes the supplied directory; `GOD_HEADER` its base LIVE header; `PRIVATE_WORK` the private Phase 2 directory outside Git; `TOOLS` its tool directory; `SDK` the extracted `mac-arm64` SDK directory; `PRIVATE_POC` the fresh private probe directory. These are **descriptions**, not automatic environment-variable assignments. Helpers referenced below are original local research drivers, not upstream commands or files delivered by this public repository.

## Tool coordinates

| Tool | Exact version / fingerprint | Acquisition or result |
| --- | --- | --- |
| xverter | `75c196aefe28692bd02dbdf8fb552485759733cd` | Shallow local clone; GOD/XDVDFS/STFS modules loaded directly with Python importlib; stdlib path only |
| ReXGlue SDK | Tag v0.10.0, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` | Official `rexglue-sdk-0.10.0-mac-arm64.zip`, 69,059,624 bytes |
| SDK ZIP SHA-256 | `1192638b51a6963aa6a4f24b77ebcb90c16f7fd0b91b311e11a90100f5274326` | Locally calculated; not claimed to be independently signed/authenticated |
| Native CLI SHA-256 | `78c1ff17254180e936b701bd97a17ea53b6626a111236dd1cf1aa4a3b1615eea` | Locally calculated; Mach-O ARM64 |
| Python / compiler | System Python 3.9; Apple Clang 17.0.0 | Existing tools; no global install/configuration changes |

Sources: [xverter GOD reader](https://github.com/blank-query/xverter/blob/75c196aefe28692bd02dbdf8fb552485759733cd/xverter/formats/god.py), [XDVDFS reader](https://github.com/blank-query/xverter/blob/75c196aefe28692bd02dbdf8fb552485759733cd/xverter/formats/xdvdfs.py), [STFS reader](https://github.com/blank-query/xverter/blob/75c196aefe28692bd02dbdf8fb552485759733cd/xverter/formats/stfs.py), [official SDK release](https://github.com/rexglue/rexglue-sdk/releases/tag/v0.10.0).

An initial inspection also examined raburton/god2iso and rrockru/god2xex; neither was executed on the game or bundled. The selected xverter reader allowed direct, verified read-only streaming without a temporary ISO.

## Discovery and preservation

Python `Path`/`os.walk` inspected directory/file types, relative names, lengths and symlink state. Header reads established LIVE/content-type fields. Python `hashlib.sha256` streamed each of 38 source game files in 8 MiB chunks, before and after the operations. Actual results: 5,724,270,592 bytes read in each full pass, all hashes/sizes/mtime unchanged. No repair, conversion, source-write or update-application command ran.

## Verification/extraction

The executed local driver, with path prefix normalized:

```sh
python3 "$PRIVATE_WORK/extract_private.py"
```

It loaded the pinned `god.py`, `xdvdfs.py` and `stfs.py` modules directly with `importlib.util.spec_from_file_location`. Its actual library operations were:

```python
# god, xd and stfs are the pinned modules; paths resolve only in private storage.
god.convert(str(header), None, verify_only=True)
with god.GodStream(str(header)) as stream:
    # Before extraction: validate signatures, bounds, names/collisions,
    # directory cycles/count limits and file-extent overlaps.
    extent = xd.validate_image(stream)
    result = xd.extract(stream, str(private_base_output), quiet=True)
# Repeated separately for each DLC and title-update package:
entries = stfs.list_entries(str(package))
count = stfs.extract(str(package), str(private_package_output), verify=True)
```

The driver refused a pre-existing base output and never selected writer/build modes. Results:

- Full GOD verification: pass; all data hashes, header self-hash and 32 inter-part MHT chain links checked; 11.4 seconds reported by the tool.
- Preflight: 320 entries; allocated extent 5,545,416,704 within a 5,545,418,752-byte logical stream.
- Base extraction: 284 files, 36 directories, 5,544,498,100 bytes.
- DLC extractions: 8 + 1 + 2 files. Title update: 4 files, kept separate and unapplied.
- Full extracted SHA-256 manifest: 299 files / 5,694,838,936 bytes. Verification, extraction and output hashing completed in 34.4 seconds in this run.

No integrity check here establishes original retail authenticity or verifies the platform RSA signature.

## Clear XEX headers

```sh
python3 "$PRIVATE_WORK/analyze_headers.py"
```

The original bounded Python parser read XEX2 optional headers, execution/file-format/TLS/static-library/import metadata using SDK structures as reference. It checked file offsets before reads, excluded key/signature/payload material from its reports, and hashed the original extracted bytes. It did not implement a compiler or decompress the encrypted image itself. Results and the alternate-header/update caveats are in [the binary report](xbox360-analysis.md).

## Native tool check and bounded analysis

```sh
file "$SDK/bin/rexglue"
otool -L "$SDK/bin/rexglue"
"$SDK/bin/rexglue" --version
"$SDK/bin/rexglue" codegen --help
"$SDK/bin/rexglue" init --help
```

Results: Mach-O ARM64; bundled runtime and macOS framework dependencies resolved; version `0.10.0`; commands execute successfully. `codegen` combines analysis and emission. No invented standalone `analyze`/`info` command was used.

Private manifest as created (relative paths are inside the private Phase 2 workspace):

```toml
[project]
name = "brutal_legend_probe"
game_root = "../extracted/base"

[entrypoint]
file_path = "../extracted/base/default.xex"
out_directory_path = "generated/default"
```

Executed through the original local supervisor:

```sh
python3 "$PRIVATE_WORK/run_bounded_poc.py"
```

Its exact subprocess argument order, with only absolute prefixes replaced by placeholders:

```sh
"$SDK/bin/rexglue" --log-level debug \
  --log-file "$PRIVATE_POC/rexglue.log" \
  codegen "$PRIVATE_POC/brutal_legend_manifest.toml"
```

The supervisor set a 60-second CPU limit and disabled core dumps, checked process RSS, logs and generated size every 0.5 seconds, and enforced 45-second wall / 2 GiB RSS / 32 MiB logs / 256 MiB generated-output limits. RSS is sampled, not an OS-enforced address-space limit. The process working directory was private. It used no force option, game-specific function hints, DLL scan, title-update application or guest entry execution.

**Result:** load success; 274 logical imports; 13 sections in the analysis view; Register, Scan, Discover and GapFill reached. SIGTERM at 45.309 seconds, subprocess exit `-15`, reason **wall-time limit**. Peak sampled RSS 144,637,952 bytes. Only generated CMake glue exists; no translated game C++ was emitted. Merge/Validate, instruction-emission coverage and code compilation remain untested. The timeout does not establish a specific analyzer defect.

## Metadata-only helper

A small original C++ helper linked against the downloaded SDK, called `Runtime::Setup` with kernel initialization and `tool_mode=true`, then `LoadXexImage`, and inspected section/import/export metadata. It never called the guest entry point. A private loaded-image copy enabled PE-field and middleware-marker checks; it includes runtime-patched import data and is **not** the original XEX or an independently authenticated PE file.

```sh
clang++ -DSPDLOG_FMT_EXTERNAL -std=c++23 -O0 \
  -I"$SDK/include" "$TOOLS/metadata_probe.cpp" \
  -L"$SDK/lib" -lrexruntime -Wl,-rpath,"$SDK/lib" \
  -o "$TOOLS/metadata_probe"
"$TOOLS/metadata_probe" "$PRIVATE_WORK/extracted/base" \
  "$PRIVATE_WORK/metadata-default" "$PRIVATE_WORK/decoded/default-loaded.bin"
```

Initial compile without `-DSPDLOG_FMT_EXTERNAL` failed because the SDK expects external fmt; that definition is declared in its exported spdlog CMake configuration. One corrected compile succeeded in 1.8 seconds, with one SDK FPCR assembly operand-width warning. The helper returned exit 0 within a 15-second bound. No dependencies were compiled. Sections, exception/relocation directories and 274 recognized ordinals were inspected.

```sh
nm -gU "$SDK/lib/librexruntime.dylib"
```

A local Python comparison matched all 261 required function names to exported `__imp__` shims. Thirteen remaining imports are variables. This checks symbols only. Representative source inspection found imported exception/STFS stubs; registry tag absence alone was not used to classify implementation coverage.

## Reproduction limits

This record gives executed tool arguments with sanitized path prefixes and names the retained local drivers and their operations. Those helpers are private research artifacts, not a public installer/API contract. Reproduction elsewhere requires independently supplying the game and equivalent safe local drivers; no proprietary file or code-derived diagnostic is included here. Only sanitized metadata/aggregate observations enter Git. Native game, Switch cross-build, GPU/audio decoding and hardware tests were not attempted.
