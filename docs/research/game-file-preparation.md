# Private game-file preparation

This is the general ISO-intake procedure. **Phase 2 supplied a GOD/SVOD directory rather than an ISO; it was verified and extracted privately without conversion.** See [the actual binary report](xbox360-analysis.md) and [executed commands](phase2-commands.md). Use the procedure below for a future owner-supplied original ISO; it must never be rewritten, optimized, patched, repaired, or replaced.

## 1. Intake and isolation

Record local provenance, filename, size, modification time, free disk capacity and a full SHA-256 in a private intake record. Open the image read-only. Reading every byte for hashing tests sequential readability, not authenticity or filesystem correctness. A known trusted dump hash, if independently available, is additional evidence, not something to invent.

Create a new private working directory outside every public Git checkout with restrictive permissions. Use separate `extracted`, `reports`, `generated`, `build` and `tools` subdirectories there. Prevent accidental overwrites, output collisions and symlink traversal. Allow space for the source plus extracted files and future builds; do not guess this game's size.

## 2. Identify the actual disc format

The extension `.iso`, an ISO9660/UDF detector's output, or the image size alone cannot establish Xbox 360 game-partition layout. Distinguish raw/full XGD dumps, partition-only XISO/XDVDFS images, split/truncated images and unrelated containers. A GOD/STFS content package needs a different reader and must not be called an ISO.

In [extract-xiso at 3f5b62c](https://github.com/XboxDev/extract-xiso/blob/3f5b62cfe68f000b0e3c8a30104973f3a297948e/extract-xiso.c), `verify_xiso` probes `MICROSOFT*XBOX*MEDIA` at descriptor offset `0x10000` plus candidate partition bases `0`, `0x0FD90000`, `0x02080000`, and `0x18300000`. These are reader probe candidates, **not a definitive XGD generation classifier**. Do not infer the game's actual layout or XGD generation from this list.

Record observed signatures, both descriptor tags, candidate base, root-directory sector/length and image bounds. Validate directory/file extents before trusting names. If multiple candidates conflict, data is missing or the format is unsupported, stop and report it; do not alter the original to make it fit the reader.

## 3. Verify readability, then extract

Acquire/build the pinned local extraction tool separately and record its revision and executable hash. Review its applicable license if distributing the tool; no copy is bundled here. The inspected tool supports list and extract modes. **Never use rewrite/create/delete modes (`-r`, `-c`, `-D`) for the original.** Do not skip update folders during the first inventory.

The following are templates, not executed results. Set `GAME_ISO` to an existing absolute input file and `PRIVATE_WORK` to a fresh absolute directory outside the repository. Validate both paths before running commands. Keep their resolved values private.

```sh
# Both variables must already be set to validated absolute paths.
shasum -a 256 "$GAME_ISO" > "$PRIVATE_WORK/reports/iso-before.sha256"
extract-xiso -l "$GAME_ISO" > "$PRIVATE_WORK/reports/disc-listing.txt"
extract-xiso -x "$GAME_ISO" -d "$PRIVATE_WORK/extracted"
shasum -a 256 "$GAME_ISO" > "$PRIVATE_WORK/reports/iso-after.sha256"
cmp "$PRIVATE_WORK/reports/iso-before.sha256" "$PRIVATE_WORK/reports/iso-after.sha256"
```

Use an input filesystem exposed read-only to the extraction process where practical. If using a container, mount the ISO read-only and only the private output directory writable. Do not change source-file permissions or mount it read/write. Save stdout, stderr and exit codes privately. Any extraction read failure, invalid extent, traversal entry or incomplete output is a blocker. A successful directory listing alone does not read all file payloads. Reconcile extracted file counts/sizes and stream-hash every extracted file; when possible, compare each file to the corresponding image extent. A before/after match verifies unchanged source bytes, not that the input was a correct dump.

## 4. Locate and fingerprint executable(s)

Locate `default.xex` case-insensitively in the extracted game partition. Distinguish the root game executable from update-folder or additional module executables. Missing or multiple ambiguous candidates are blockers. Do not choose a file by size or silently apply an update. Check `XEX2` magic and bounds, then hash the **original extracted bytes** with SHA-256. Hash any decoded analysis copy separately and label it clearly.

Do not borrow a known hash/title ID from another game or another region. Record exactly which disc/update/module combination the report describes. Keep full file lists and paths private; only sanitized metadata is suitable for public documentation.

## 5. Version and executable metadata

Use the [ReXGlue XEX structures](https://github.com/rexglue/rexglue-sdk/blob/c94f5ebdcb3c9d1a460ca48e04f9758448f8d518/include/rex/system/util/xex2_info.h) and a bounded parser/loader appropriate to the pinned version. Record XEX flags, optional headers, title/media ID, execution version/base version (both raw and decoded), disc fields, image base/entry/size, compression/encryption type, TLS/stack data, allowed regions/system requirements and sections. Do not copy keys, signatures, payload bytes or decoded game code into a public report.

After decoding/loading privately if necessary, record PE machine/subsystem, section names and ranges, `.pdata`/exception data, import libraries/versions/ordinals/thunks/variables, exports, relocation information and extra executable modules. Record absent fields explicitly. Metadata version is evidence, not a complete proof of retail edition or update provenance.

## 6. Resource inventory

Produce a private manifest with relative path, byte count, SHA-256 and detected signature/type for every file; summarize counts and total bytes by folder/type and identify largest files. Keep unknown formats unknown rather than guessing by extension. Account separately for `$SystemUpdate`, movies, music/audio, localization, shader containers and additional executable modules.

Inspect archive headers/indexes read-only and establish whether they are bounded, compressed or encrypted before writing an unpacker. Identify codecs, texture/shader formats and middleware only from explicit evidence. A codec supported by the SDK is not proof the game uses it. Do not redistribute inventories containing proprietary data, payloads or personal paths.

## 7. Determine ReXGlue processability

Before generation, report input/loader blockers and select an exact SDK revision. First inspect its actual `--help` and available commands. The inspected upstream couples analysis with codegen, so do not invent a standalone analysis CLI or run it under the label “metadata inspection.” The Phase 2 bounded run is recorded separately; it did not complete analysis or emit game C++. Any further bounded run must set all manifest, generated-glue and output paths inside the private workspace; ReXGlue may update manifests and write generated support files relative to project roots.

Record loader status, compression handling, section/import resolution, function discovery, jump tables, computed calls, unsupported instructions, exception requirements and additional modules. Preserve all warnings; do not force generation past errors and call it success. Report each stage separately: **readable ISO → extraction → XEX metadata → loader → analysis → codegen → compilation → runtime**. None implies the next.

## Required private intake record

Use these fields; unknowns remain null with a reason: source bytes/hash, read result, format/signature/base evidence, extractor revision/hash/exit result, source before/after match, extraction counts/bytes, original XEX bytes/hash/magic, title/media ID, execution/base version, entry/base/sections/imports/modules, compression/encryption classification, resource summary, SDK revision and loader/analysis status, blockers, and next experiment. Never include key values. Do not commit the raw record automatically.
