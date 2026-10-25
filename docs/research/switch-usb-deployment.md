# Original Switch probe: USB deployment and local package

**BUILD VERIFIED · PACKAGE VERIFIED · USB DEPLOY VERIFIED · HARDWARE EXECUTION
UNKNOWN.** ReXGlue integration remains BLOCKED; game execution remains NO-GO.
This is an extension of the independent Switch track, not Xbox Phase 22 recovery
or private-entry Phase 23. No proprietary code/assets are used.

## Application and build

The existing original 23-check probe is retained: 21 portable component checks
plus successful native wait/close. No arbitrary successes or SDK stubs were
added. The screen identifies **BRUTAL LEGEND NX PROBE**, says it is not the game,
shows PASS/FAIL and waits for **PLUS (+)**. A normal applet shutdown also exits.
The old two-second result timeout is removed. Yielding continues during the
result screen; native wait failure still aborts without freeing live worker
state. This UI wait intentionally has no automatic timeout. Startup/native
thread behavior on Switch is still unobserved.

The digest-pinned [toolchain](../../tests/switch-horizon/toolchain.json) remains
devkitA64 r29.2-1 / GCC 15.2.0, libnx 4.12.0-1 and switch-tools 1.13.1-1.
The NRO has an embedded NACP (`Brutal Legend NX Probe`, `Wilwous`, `0.1.0`) and
an original 256×256 NX/PROBE JPEG icon, authored geometrically without game
artwork or external fonts. Pillow 12.3.0 is used only by the host icon builder;
it is not a runtime dependency. No additional native installation is required
when using an existing isolated Python environment and the pinned container.

Two fresh builds have byte-identical ELF and NRO outputs. The JPEG is an explicit
shared build input; independent encoder/platform versions are not assumed to
produce identical JPEG bytes. The full source/input hashes and actual artifact
hashes are checked and recorded. Measured final properties:

| Property | Verified result |
| --- | --- |
| NRO bytes | 302,498 |
| ELF bytes, including debug information | 3,740,216 |
| NRO SHA-256 | `d8a8721fe9d38a9e66cc274b0b4b3af124abf05c9985686352bb108534f17c7e` |
| ELF SHA-256 | `bd8b54962ef4a842f2efbc4df9eeeae481a8939f559180eefabbd1f4ff88f6c1` |
| Embedded icon / NACP | 19,818 / 16,384 bytes |
| ELF/NRO audit | AArch64 PIE, bounded/nonoverlapping image and assets, RX/R/RW load segments, no RWX, no dynamic library or undefined strong symbol |

The prior SVC-wrapper census and privileged `0x52` observation remain unchanged;
no new permission or target-execution claim is made. The
[Horizon compatibility audit](switch-horizon-feasibility.md) retains its ABI,
mapping, TLS/PCR, exception and hardware evidence limitations.

The prebuilt toolchain's newlib contains two absolute diagnostic `__FILE__`
prefixes that consumer `-ffile-prefix-map` cannot change. A bounded
[post-link checker](../../tools/switch/sanitize_elf.py) replaces only those
known prefixes in read-only, non-executable ELF load bytes. It preserves file
length, every offset and relative filename suffix (including suffix-merging
coordinates). Executable bytes and all other bytes are checked unchanged.
Unexpected census/version/layout or remaining loaded private paths refuse.
The raw linked ELF and before/after hash receipt remain private. This changes
assertion filename presentation, not SDK functions, success/failure behavior
or global SDK installations. Eight independent synthetic ELF unit tests cover
the transformation and rejection paths. Final NRO/ZIP checks reject absolute
user/build paths; private ELF debug sections are not packaged.

## Build and package commands

From the repository root, use a fresh directory outside Git. Install the single
host image-encoding dependency into an isolated environment if needed:

```sh
python3 -m venv "$PRIVATE_WORK/probe-pack-env"
"$PRIVATE_WORK/probe-pack-env/bin/pip" install -r tools/switch/requirements.txt
docker pull --platform linux/arm64 \
  devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282
"$PRIVATE_WORK/probe-pack-env/bin/python3" tools/switch/package.py \
  --workdir "$PRIVATE_WORK/probe-package"
```

The package builder runs Debug/Release/ASan+UBSan host assertions, builds and
inspects the NRO twice, checks both hashes, and creates the ZIP twice to verify
identical bytes. Every native build/run/inspection child has the existing
30-second external monotonic process-group watchdog. Docker has no network,
devices or privileges and keeps output in a separate writable directory.
The cross-build cannot observe Horizon behavior.

`--from-build "$VERIFIED_BUILD"` can repackage an unchanged previously verified
build without recompiling. It rejects mismatching source hashes, failed child
records, missing icon/inspection evidence or changed ELF/NRO bytes. A final
package can therefore identify the published clean source commit without
repeating completed native tests. Changed build inputs require a new build.

ZIP layout (six regular files; no absolute paths, symlinks or parent traversal):

```text
switch/brutal-legend-nx/brutal-legend-nx.nro
switch/brutal-legend-nx/build-info.json
switch/brutal-legend-nx/README.txt
switch/brutal-legend-nx/SHA256SUMS
switch/brutal-legend-nx/LICENSE.txt
switch/brutal-legend-nx/THIRD_PARTY.txt
```

NRO/ZIP checksums and exact local paths are in `package-result.json` outside
Git. The manifest records public source revision, dirty status, build and
packaging source hashes, pinned toolchain, ELF/NRO hashes and test counts.
It contains no private paths. This is local integrity/provenance verification,
not a signed attestation or proof against a forged build receipt. No SDK
implementation, generated PPC C++, game input or private diagnostic is packaged.

## Verified macOS USB workflow

The user selected Hekate → Tools → USB Tools → SD Card. Exactly one mounted
volume qualified: external, removable USB, writable FAT, with an actual
`switch/` directory, regular `bootloader/hekate_ipl.ini` and
`atmosphere/package3`. Selection used diskutil plist metadata, mounted-device
identity and these structural markers, not the volume name. The exact mount,
device identifier and checksums remain in the private deployment receipt.

```sh
python3 tools/switch/deploy.py --list
python3 tools/switch/deploy.py --transport usb \
  --package "$PRIVATE_WORK/probe-package/package"
```

The no-argument package default is the current user's private
`.local/share/brutal-legend-nx-switch-usb/ready/package` directory. For ambiguous
discovery the command refuses to write; inspect candidates and explicitly add
`--volume "$SWITCH_SD"`. A selected path still must pass every check. There is
no `--force`, formatting, partitioning, raw-disk writing, recursive SD copy,
firmware update, reboot, application launch or automatic unmount operation.

The deployer:

1. Validates the local NRO hash, current public build/packaging inputs and passing
   source-bound build receipt before volume mutation.
2. Rechecks USB/filesystem/UUID/device identity and rejects the host filesystem,
   missing structures, ambiguous candidates and changed mountpoints.
3. Uses directory-relative descriptors and `O_NOFOLLOW` for fixed application
   paths. It creates only the named app directory and its own app files/lock.
   Symlink/linked/nonregular targets, stale locks and orphan metadata refuse.
4. Writes an exclusive temporary file in that directory, flushes it, and verifies
   the complete copied bytes. On macOS, `F_FULLFSYNC` is requested after fsync;
   unsupported full flush is reported explicitly.
5. Copies any previous NRO to a unique, verified recoverable backup before
   replacement. Older backups are never pruned. The observed first install had
   no previous NRO, so no backup was needed; backup behavior is mock-tested.
6. Uses macOS `renameatx_np(RENAME_EXCL)` for an initial no-overwrite install, or
   same-directory replacement after checking the previous file's identity.
   Atomic rename is not a multi-file/power-loss transaction guarantee.
7. Verifies final bytes, syncs, closes its lock, runs macOS `sync`, rechecks the
   volume and final bytes, and reports that safe ejection is still required.

Concurrent invocations of this tool are excluded by its lock. Descriptor-based
checks and initial exclusive rename prevent tested path/link/race mistakes;
this is not a security boundary against arbitrary hostile concurrent writers
with full access to the mounted volume. A stale lock is retained for inspection,
not automatically bypassed. Failure may leave a valid backup or a completed copy
whose final sync failed; a nonzero result must not be called a successful deploy.

The actual initial copy returned **COPIED_VERIFIED**; repeating the command
returned **ALREADY_CURRENT_VERIFIED** with the same hash. macOS sync completed
and final bytes were rechecked. Hekate/Atmosphère marker hashes and all existing
`switch/` directory entries were preserved. No save or unrelated application
was overwritten. The volume was deliberately left mounted for user-controlled
safe ejection.

The initial trial NRO is retained as evidence. The final package removes the
toolchain diagnostic prefixes; installing that updated NRO uses the same
verified backup-before-replace path. Exact final SD backup and artifact hashes
are recorded in the private handoff receipt rather than publishing volume paths.

**Observed macOS filesystem metadata:** FAT also acquires AppleDouble
`._brutal-legend-nx` and `._brutal-legend-nx.nro` companions for the new app
directory/file. They are associated with this app, not additional homebrew.
An initial footprint assertion detected them. A narrowly scoped cleanup trial
of the new objects' provenance attribute was temporary: macOS recreated it on
access. That ineffective cleanup was removed from the final deployer. Existing
quarantine/security attributes are not changed and no global metadata-cleaning
command is used. Final footprint accounting explicitly includes the app's own
OS-generated companions; existing directory entries and boot-file hashes still
must match. The ZIP itself contains no AppleDouble files.

## Verification and retained diagnostics

| Final suite | PASS | FAIL |
| --- | ---: | ---: |
| Original portable assertions, Debug / Release / ASan+UBSan | 21 / 21 / 21 | 0 / 0 / 0 |
| New result-loop assertions, same three profiles | 6 / 6 / 6 | 0 / 0 / 0 |
| Mock filesystem/deployment/package units | 45 | 0 |
| Synthetic ELF diagnostic-path units | 8 | 0 |
| Fresh NRO builds and inspections | 2 + 2 | 0 |
| ZIP structure/content/determinism | Verified, six exact members | 0 |
| Actual USB copy + repeat identity/hash/sync | Verified | 0 |

The mock suite checks wrong/read-only/internal/missing volumes, ambiguity and
traversal, missing markers, parent/app/target symlinks, linked/nonregular targets,
space/write/flush/hash/rename failures, concurrent target creation/change,
mount change, locks, metadata preservation, exact backups and no pruning,
corrupt/source-mismatched packages and safe ZIP membership. Each mock run uses
only temporary host folders and has an external 30-second watchdog. **No real
SD negative/destructive test was performed.** There were no unexpected native
faults, timeouts or sanitizer findings.

An initial unit import failed because the previous `inspect.py` fixture shadowed
Python's standard `inspect` module. It is renamed `inspect_nro.py`, and both the
unit suite and affected build/provenance checks now pass. The initial import
error, two overly narrow post-copy footprint assertions, metadata diagnostics,
earlier successful builds and final corrected receipts are retained privately;
they are not silently discarded or counted as passing tests.

## Separate future nxlink workflow

The installed container's actual `nxlink --help` reports `--address/-a`,
`--retries/-r`, `--path/-p`, `--args` and `--server/-s`. A native macOS nxlink was
not installed. Its separate prepared command is:

```sh
python3 tools/switch/deploy.py --transport nxlink \
  --package "$PACKAGE_DIR" --nxlink "$NATIVE_NXLINK" --address "$SWITCH_IPV4"
```

This checks native CLI support and requires an explicit IPv4 address. It refuses
while a matching USB SD volume remains mounted. No network transfer was attempted.
Upload argument handling is mock-tested; actual network/runtime behavior is
UNKNOWN. No `-s`, application stdio redirection or runtime log capture is
implemented or claimed.

Hekate UMS exposes a disk while Horizon/hbmenu is not running; it cannot execute
the NRO or stream its runtime output. NetLoader is a separate hbmenu facility
used after normal Horizon boot. See the primary
[Hekate UMS source](https://github.com/CTCaer/hekate/blob/master/nyx/nyx_gui/frontend/gui_tools.c)
and [hbmenu NetLoader instructions](https://github.com/switchbrew/nx-hbmenu#netloader).
USB deployment requires no Wi-Fi.

## User handoff

1. Safely eject the identified microSD volume in macOS Finder. If it is busy,
   close files using it and retry a normal eject; do not force-unmount it.
2. After macOS confirms ejection, stop USB mass-storage mode in Hekate.
3. Boot your existing Atmosphère configuration normally.
4. Open hbmenu using your existing method; record applet/application launch mode.
5. Launch **Brutal Legend NX Probe** at
   `sdmc:/switch/brutal-legend-nx/brutal-legend-nx.nro`.
6. Record the complete screen/error and environment. Expected success is
   `PASS=23 FAIL=0`. Press **PLUS (+)** to exit normally.

Copied bytes, NRO metadata and host tests do not establish Switch execution.
Hardware result remains UNKNOWN until actual device output is observed.
Complete Xbox ABI, guest exceptions, ReXGlue/Horizon integration and game
compatibility remain unverified/blocked. No Phase 22 completion tag, main merge
or GitHub Release is created by this task.
