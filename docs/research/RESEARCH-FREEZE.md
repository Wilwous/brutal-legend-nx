# Brütal Legend NX — final research freeze

2026-10-29 · research baseline `31d4dfa0128cc10998a2bb0ec61a7d3a3a819be0` ·
branch `research/switch-horizon-feasibility`.

**Research checkpoint preserved; original game execution NO-GO.** This freezes
Stage23 research and the existing Stage15 synthetic runtime. It starts no new
engineering stage. The repository remains available for inspection and future
evidence review. G4 **BLOCKED**, G5 **UNKNOWN**, G8 **BLOCKED**; decision
**BLOCKED PENDING EXTERNAL EVIDENCE**. No proprietary game instructions have run.

## Objective and verified achievements

The project investigates whether a legally supplied Xbox360 title can eventually
use static PPC recompilation and a bounded Horizon runtime. Research establishes
tool and synthetic contracts, not boot, graphics, audio, gameplay or compatibility.
The preserved Xbox Phase22 branch remains blocked and separate from Switch
roadmap Stages20–23.

| Evidence | Classification and exact scope |
| --- | --- |
| Candidate0.15.0, build `80407778989c` | **HARDWARE VERIFIED**, by explicit user observation: PASS427/FAIL0, generated PPC result197650, SDK entered64/completed61, allocations0/leases0, four synthetic guest workers, separate PCR/TLS, PLUS returned normally to hbmenu. No new hardware run during this freeze. |
| Launch description | User calls it “libnx Applet Mode.” Recorded raw libnx type0 maps to `AppletType_Application` in the pinned headers. Human launch procedure, Title Override and firmware identity remain **UNKNOWN**; these statements are preserved separately. |
| Original synthetic XEX → production ReXGlue → unchanged generated C++ | **HOST VERIFIED** in the selected fixtures; historical AArch64 compilation/link and existing NRO structure **BUILD VERIFIED**. No hand-written imitation of generated instruction bodies. |
| Original Brütal Legend | **NO-GO**. Static analysis/private generation and selected historical compile-only results do not establish loaded ownership or authorize execution. |

The hardware attestation is user-supplied; no photograph was independently
inspected in this freeze. Original build manifests still say hardware UNKNOWN
at their creation time. They are immutable; the later attestation is a separate
checkpoint record, not a rewritten build result.

## Synthetic implementation and limits

The finite acceptance fixture has21 functions/203 instruction words,49 checked
memory sites,97 generated hooks, four backward branches,17 calls,20 returns and
11 branch edges. The current workflow regenerates twice, compares exact emitted
instruction comments with an independent decoder, and compares instruction C++
against the default production template. SDK/header/runtime identities and
unsupported code paths are checked before compiling the fixture.

The Horizon adapter supplies bounded logical guest memory, ownership generations,
leases, checked endian accesses, admitted generated calls and cooperative stops.
Each synthetic guest worker has a16-KiB guest stack and separate PCR/TLS with
selected r1/r2/r13 contracts. Native workers request64-KiB stacks, the controller
256 KiB, and the application16 MiB of heap. Main-stack size is loader-owned and
**UNKNOWN**. These are selected contracts, not the complete Xbox ABI or XThread
scheduler.

Four imports use verified pinned SDK function bodies and bounded adapters:

| Import / ordinal | Admitted contract |
| --- | --- |
| `RtlCompareMemoryUlong` / `0x11b` | Selected aligned IN-buffer comparison. |
| `RtlInitializeCriticalSection` / `0x12e` |28-byte initialization only; no Enter/Leave synchronization contract. |
| `NtQueryVirtualMemory` / `0xee` | Selector0, admitted uniform whole-page query, checked28-byte output publication. |
| `RtlFillMemoryUlong` / `0x126` | Aligned4–64-byte OUT range with checked publication. |

These leaves do not constitute a full Xbox kernel. Refused publication remains
observable and explains the tested entered/completed counter difference. No
unsupported import/signature is admitted as fake success.

**PARTIAL / unsupported:** complete ABI, typed-vector signatures, partial-vector
transfers, reservation atomics, MMIO, instruction-fetch paths, general callbacks,
guest exception dispatch/unwind and unverified exceptional continuations. Logical
checks are not complete physical Horizon guest isolation; the desktop SDK's
4-KiB guest/16-KiB host protection limitation also remains. Cooperative stopping
cannot interrupt arbitrary native calls. Diagnostic context and scope cleanup
do not prove universal fault-time register capture or safe guest continuation.
The linked libnx SVC0x52 wrapper does not establish invocation or permission.

## Frozen artifacts and reproducibility

The external checkpoint separates `public/` recovery material from `private/`
paths, logs and toolchain snapshots. Nothing in that private directory belongs
in Git. Its `public/git/SOURCE-REFS.json` pins the final documentation commit,
protected refs and all21 historical tags. Before freeze publication, all three
worktrees were clean; local and remote research HEAD matched the baseline.
`main` remains `1da0e11a54c3cd8b950d379c8045646bfc9e5051` and
`research/phase-22-blocked` remains
`f3c6affb913fa4fb4da1031fdbf6a9b4fca73416`.

| Existing artifact | SHA-256 |
| --- | --- |
| `package/switch/brutal-legend-nx-acceptance/brutal-legend-nx-acceptance.nro` (794018 bytes) | `b95c5287dd970d55da687f938c7360888d7f28ed8af5cbfbc54270b6d030faf2` |
| `brutal-legend-nx-acceptance.zip` | `ba84bd1595ef27f7936c5ecd0a034f0870f43b00467c566d749ae63dbc0b60e3` |

Both match the saved `SHA256SUMS`; the ZIP's embedded NRO matches too. Exact
copies retain the original manifests/checklist/license notices and build identity.
All90 source-file hashes in the hardware package's build manifest still match.
The existing two clean-build ELF/NRO pairs also agree; no NRO was rebuilt.
Artifact identity, selected source regeneration and historical hardware evidence
are three separate claims. Cross-machine byte-identical SDK rebuilding is not
claimed.

Pinned inputs, also recorded in external `public/DEPENDENCIES.json`:

- ReXGlue SDK v0.10.0, `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`;
  [ordered nineteen-patch lock](../../tools/sdk/xex-binding-lock.json), SHA-256
  `18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
- [Horizon leaf overlay](../../patches/rexglue-sdk/horizon-leaf.patch), SHA-256
  `1391a78256b14bf6ae9cb212298816c9067e2a71704db601b30724a7378f7547`.
  Prepared macOS CLI SHA-256
  `6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c`;
  SDK runtime `863f2fbc16d765217fbb3164df0aafb4628c08cb5e51d2df387084915b705872`.
- [Switch toolchain lock](../../tests/switch-horizon/toolchain.json):
  `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`,
  `linux/arm64`; devkitA64 r29.2-1/GCC15.2.0, libnx4.12.0-1
  (`7644c9b26099aa2d2145bc72a21ee24190e92085`), switch-tools1.13.1-1,
  devkita64-rules1.1.1-1. Installed package database and libnx archive hash were
  rechecked in the cached read-only container.
- This host verification: macOS15.7.7 ARM64, Python3.9.6, Apple Clang17.0.0
  (`clang-1700.6.4.2`). SDK builder pins CMake3.31.6 and Python Ninja1.11.1.4
  (binary `1.11.1.git.kitware.jobserver-1`). The separate22-patch Xbox SDK and
  its locks remain unchanged; they are not the Horizon candidate dependency.

The [actual Makefile](../../tests/horizon-acceptance/Makefile) is authoritative:
`-march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE`, GNU C++23,
`-O2 -g`, exceptions/RTTI, section splitting, `-Wall -Wextra -Werror`,
`__SWITCH__`, `STAGE_MAX=15`, pinned includes, source/build prefix maps;
link through libnx `switch.specs` and `-lnx`, sanitize ELF, then `elf2nro`
with NACP0.15.0 and the preserved icon. See the manifest for complete flags.

## Freeze validation

| Fresh selected check | Result |
| --- | --- |
| Acceptance production-generation/host workflow, Debug/Release/ASan+UBSan | Each423/0 smoke and759/0 stress; controller and disabled-trace variants agree. Result197650, SDK64/61 smoke,232/229 stress, no outstanding allocations/leases. |
| Portable memory/allocation/TLS/failure boundary |6/0 groups in each of three profiles, including4,000 seeded memory operations. Expected unsupported-exception refusal is a modeled event, not a native signal. |
| Negative compilation |7 expected refusals; no unexpected exit or timeout. |
| Existing ELF/NRO on copies | ELF64 AArch64 PIE/NRO0; zero unexpected strong symbols, zero dynamic dependencies, RX/R/RW loads, no W+X. |
| Preservation |23 original/SDK/package sentinels, four old-checkpoint manifest/report sentinels and31 changed SDK22 source hashes unchanged. No whole74-GB private tree rehash claimed. |

The25 acceptance subprocesses used30-second external process-group watchdogs:
45.914 seconds summed child duration, maximum RSS412041216 bytes including
compiler; maximum runtime-child RSS224526336 bytes. Zero unexpected signals,
timeouts or sanitizer findings. Portable foundation driver durations were
1.781/1.316/2.001 seconds. Detailed counts/metrics are in external
`public/TEST-INVENTORY.json`; raw paths and logs stay private.

This is the smallest selected consistency set, not a rerun of all historical
research. The earlier unmodified reference's13 UBSan failures remain failures;
the explicit offset-only observation variant does not make it sanitizer-clean.
The initial container version-query attempt lacked `pacman` on PATH; its exit127
is retained. A read-only package-database query resolved version verification.
No code or toolchain was changed to pass freeze checks. Final exact-commit CI is
recorded externally in `public/CI.json`; CI is host/static, not console execution.

## Xbox gates and minimum external dependency

**G4 BLOCKED:** the disputed original virtual tail lacks independent image-owner
and load-generation authority, reservation, commitment, valid initial contents/
zero-fill, permissions/order, neighbor integrity, reference lifetime, rollback
and unload proof. Required observation is after mapping/fixups/protection and
before any TLS/initializer/entry. Declared PE size, page rounding, security size,
emulator requests or synthetic bytes cannot establish original-title ownership.

**G5 UNKNOWN:** independent provenance for consumed initial title state and
startup assumptions is missing. Synthetic constants do not supply it.

**G8 BLOCKED:** exact fault/refusal context for the bounded region and matched
helper/return, implicit access faults, unsupported transfers, fault PC/registers,
error propagation, cleanup and deterministic safe termination/continuation remain
unproven. Debugger metadata and host C++ catches are not Xbox exception dispatch
or unwind contracts. Loader/binder refusal must remain intact.

[Stage23](xbox-evidence-acquisition-feasibility.md) found no qualified accessible
path. Public clients are transport implementations; existing emulator/source/
synthetic evidence does not independently resolve these gates. Minimum dependency:
legitimately supplied, version-matched Xbox360 loader/exception contracts with
proven applicability, or an already qualified authorized observation environment
capable of the required pre-TLS ownership/lifetime and fault-context evidence.
Neither is currently available. No experiment is activated by finding a client.

## Recovery without altering existing work

Set `CHECKPOINT` to the external versioned checkpoint and `NEW_CHECKOUT` to a
nonexistent directory. These steps verify/copy public data only; they run no guest
code and need not touch any live worktree:

```sh
cd "$CHECKPOINT/public"
shasum -a 256 -c SHA256SUMS
git clone --branch research/switch-horizon-feasibility \
  "$CHECKPOINT/public/git/research-history.bundle" "$NEW_CHECKOUT"
git -C "$NEW_CHECKOUT" branch main origin/main
git -C "$NEW_CHECKOUT" branch research/phase-22-blocked origin/research/phase-22-blocked
git -C "$NEW_CHECKOUT" show-ref --heads --tags
cd "$CHECKPOINT/public/artifacts/ready"
shasum -a 256 -c SHA256SUMS
```

Compare all refs with `public/git/SOURCE-REFS.json`. The clone's `origin` is the
local bundle, so restoring does not publish anything. `source.tar` is a second
public snapshot without Git metadata. The original NRO package can be inspected
or copied later without regeneration; restoration does not deploy or execute it.
The external `public/RECOVERY.md` records the complete sequence and validation.

Private restoration is separate. Read `private/PRIVATE-PRESERVATION.md` and
`private/LOCATIONS.json` locally; verify the private manifest first. The old full
checkpoint has its own guarded restoration helper and complete manifest. Use
only fresh private destinations outside Git. The freeze records locations and
sentinels, not a new complete game-data backup. All checkpoints remain on the
same disk, so disk-loss recovery still requires a separately verified off-device
copy. Original logs, manifests and generated title artifacts remain private.

Optional future **synthetic-only** reproduction, after manual inspection, on
macOS ARM64 with Clang/Python and recovered matching inputs:

```sh
export DYLD_LIBRARY_PATH="$CHECKPOINT/private/toolchain/sdk19-release/lib"
python3 tests/horizon-acceptance/run.py \
  --toolchain "$CHECKPOINT/private/toolchain/prepared" \
  --sdk-source "$CHECKPOINT/private/toolchain/sdk19-source" \
  --workdir "$NEW_HOST_OUTPUT" --stage 15
python3 tests/runtime/run.py --workdir "$NEW_FOUNDATION_O0"
python3 tests/runtime/run.py --optimization 2 --workdir "$NEW_FOUNDATION_O2"
python3 tests/runtime/run.py --sanitize --workdir "$NEW_FOUNDATION_SANITIZED"
```

Every output directory must be fresh and outside the repository. The copied CLI
was launched successfully with the copied runtime selected by the loader. Its
source snapshot supports this workflow but does not include dependency submodule
trees or the SDK build cache. No container image archive is claimed. A future
new AArch64 build uses the same acceptance command with `--switch-build --icon
"$CHECKPOINT/private/toolchain/icon.jpg"`, a fresh output and the exact pinned
cached/pulled image. Never point it at the preserved hardware package.

For a clean toolchain rebuild, follow [SDK build instructions](../../tools/sdk/README.md),
use the nineteen-patch lock and pinned dependencies in a new external workspace,
then [prepare the Horizon CLI](../../tests/horizon-runtime/prepare.py). Existing
relink recipes also require the matching SDK build/cache and original CLI hash;
they may refuse a different host/path output. Stop on fingerprint drift rather
than changing checks. The external dependency manifest preserves submodule pins,
wheel hashes and exact build/prepare commands; no global SDK install is needed.

## Research index and reopening conditions

- [Hardware acceptance and historical build evidence](switch-hardware-acceptance.md),
  [semantics](switch-acceptance-semantics.md),
  [services](switch-acceptance-services.md),
  [reliability](switch-acceptance-reliability.md).
- [Compatibility matrix](switch-title-compatibility-matrix.md),
  [residency evidence](xex-residency-evidence.md),
  [services limits](switch-compatibility-services.md),
  [readiness decision](switch-next-readiness-gate.md).
- [Loader source audit](xex-loader-reference-audit.md),
  [synthetic differential](xex-synthetic-loader-differential.md),
  [design-only hardware protocol](xbox-hardware-observation-protocol.md),
  [gate proof ledger](xex-gate-evidence-review.md),
  [acquisition decision](xbox-evidence-acquisition-feasibility.md),
  [roadmap](../roadmap.md).

Resume original-title research only after genuinely new, independently qualified
G4/G8 evidence and required G5 startup provenance are obtained and reviewed for
the proposed scope. Any later bounded execution needs its own explicit admission
review/authorization, observable failures and finite supervision. Unrelated
synthetic passes, additional imports, clients or a successful restore do not
reopen gates. No completion tag, Release, main merge or Stage24 is part of this
checkpoint. **Investigation stopped pending external evidence.**
