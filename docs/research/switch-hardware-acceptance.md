# Horizon hardware acceptance candidate — consolidated Stages 12–15

2026-10-28. **HOST VERIFIED / BUILD VERIFIED / HARDWARE UNKNOWN** in the declared
original-synthetic scope. This is the final host-only expansion batch. **STOP
further feature work: mandatory Nintendo Switch hardware acceptance is next.**
No Stage16, proprietary integration, game XEX load or game-code execution follows.
Xbox Phase22/G4/G8 remain BLOCKED; G5 UNKNOWN; original game execution NO-GO.

## Sequential checkpoints and actual scope

| Stage | Implementation | Debug / Release / ASan+UBSan smoke; stress |
| --- | --- | --- |
| 12 | selected scalar/VMX, checked full blocks and original multicall | 326/0; 445/0 each |
| 13 | real bounded RtlFillMemoryUlong OUT + query/lifetime contracts | 380/0; 499/0 each |
| 14 | explicit lifecycle, contextual/canonical diagnostics, cancel channel | 423/0; 759/0 each |
| 15 | freeze, native-controller UI, reproducible acceptance package | 423/0; 759/0 each |

[Stage12 semantics/evidence](switch-acceptance-semantics.md),
[Stage13 SDK contracts](switch-acceptance-services.md),
[Stage14 lifecycle/uncertainties](switch-acceptance-reliability.md).

The new operations are cntlzw non-record, one explicitly admitted rlwinm mask,
lvx/stvx full 16-byte blocks, vadduwm and vxor. Independent bit/word/endian oracles
validate the declared cases; typed-vector SDK ABI and partial vectors remain
denied. Reservation atomics stay BLOCKED: no Xenon reservation model is invented
from raw CAS or a different processor manual. General callbacks, full FPSCR/Xbox
ABI and guest exception dispatch/unwind remain unverified/unsupported. A discovered
production entry re-entry symbol inconsistency remains blocked without an alias
shim. This is a selected runtime subset, not complete Xbox emulation.

Exactly four admitted xboxkrnl.exe contracts: 0x11b RtlCompareMemoryUlong,
0x12e RtlInitializeCriticalSection (initializer only), 0xee NtQueryVirtualMemory
(selector0 in retained uniform nonexecuting regions), 0x126 RtlFillMemoryUlong
(aligned OUT 4..64 bytes). Actual pinned open-source SDK bodies execute with
checked scratch/leases/publication, not fabricated OS results. Other selectors,
ordinals and signatures remain denied. Permissions are logical software guest
checks, not a proof of physical Horizon VM isolation.

Production fixture: **21 generated PPC functions / 203 instructions / 49 checked
memory sites / 97 hooks / four backedges / 17 calls / 20 returns / 11 branch
edges**. Memory count includes four full-vector sites. Codegen repeats exactly,
instruction C++ matches production-default output, independent word/comment and
site/hook census agree, and emitted C++ is never manually edited. The original
XEX entry still separately returns24. The extended generated multicall result
is197650, including an independently checked FP result3.0 and vector outputs.
The static function count is distinct from cumulative executed prologue entries.

## Frozen candidate and UI ownership

One finite default smoke workload runs on a native controller while the main
thread services input/progress rendering. Four guest workers use their existing
64-KiB native stacks; the controller requests256 KiB, heap16 MiB. The main stack
is loader-owned and UNKNOWN. libnx's actual applet type integer is displayed;
this does not infer the human launch method or Atmosphere version.

The main UI does not read result data until actual native join. A release/acquire
completion flag publishes the result explicitly. A small portability correction
also acquire-checks the native exited state after the Horizon exit wait before
closing/publishing the joined worker; callback writes precede its release store.
Host std::thread join already supplies synchronization. Cross-compilation of this
Horizon branch is evidence of build compatibility, not proof of console behavior.

PLUS during execution or a 45-second UI budget requests cooperative cancellation
at generated safe points. The UI retains native ownership and remains responsive
while waiting; no live thread is force-killed, detached or reclaimed. The deadline
cannot interrupt a blocked native syscall or guarantee termination between hooks.
On applet shutdown, cancellation is requested and cleanup returns only after join.
On normal completion the full result screen persists until PLUS. No failed test
automatically restarts. Device failure labels use a bounded16x96-byte queue with
explicit truncation/drop reporting; background workers do not render the console.

A cancellation/timeout invalidates acceptance, even if earlier assertions passed.
If a native call remains blocked, photograph the last progress stage and use
HOME/Close Software; report the failure before retrying. No firmware, NAND,
Atmosphere/Hekate configuration or unrelated SD files are changed by the NRO.
Only the ordinary console/input/thread/synchronization/tick facilities are used
by the application. The inherited libnx binary still contains privileged SVC0x52
(svcUnmapTransferMemory), called from its tmemUnmap helper; the map-transfer SVC
wrapper is absent. Presence is not proof of execution or permission. No new direct
privileged SVC call is added; actual libnx cleanup behavior remains a hardware
gate, not a claimed host PASS.

## Final regression evidence

Each column is real PASS/FAIL. Counts are separate test executions, not unique
capabilities to add together. Generated suites: Debug O0, Release O2, ASan+UBSan
O1. Historical portable foundation sanitized driver uses O0. Apple clang17.0.0;
pinned devkitA64/GCC15.2.0/libnx4.12.0/switch-tools1.13.1 remain unchanged.

| Suite | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| Final direct smoke | 423/0 | 423/0 | 423/0 |
| Actual native UI-controller smoke | 423/0 | 423/0 | 423/0 |
| Trace-off smoke | 423/0 | 423/0 | 423/0 |
| Final stress, eight rounds | 759/0 | 759/0 | 759/0 |
| Stage12 smoke / stress | 326/0;445/0 | 326/0;445/0 | 326/0;445/0 |
| Stage13 smoke / stress | 380/0;499/0 | 380/0;499/0 | 380/0;499/0 |
| RuntimeII smoke / stress | 268/0;387/0 | 268/0;387/0 | 268/0;387/0 |
| Stage9 / Stage10 checkpoints | 121/0;171/0 | 121/0;171/0 | 121/0;171/0 |
| Stage8 smoke / stress | 256/0;382/0 | 256/0;382/0 | 256/0;382/0 |
| Stage2 / Stage3 generated | 108/0;118/0 | 108/0;118/0 | 108/0;118/0 |
| Original PPC leaf | 59/0 | 59/0 | 59/0 |
| Native components / PLUS UI | 21/0;6/0 | 21/0;6/0 | 21/0;6/0 |
| Foundation / query / diagnostic cores | 20/0;15/0;22/0 | 20/0;15/0;22/0 | 20/0;15/0;22/0 |
| Lifecycle/controller core / supervisor UI mocks | 35/0;20/0 | 35/0;20/0 | 35/0;20/0 |
| Portable foundation groups | 6/0 | 6/0 | 6/0 |

Additional public units pass: acceptance encoding14/package13/deployment6;
RuntimeII encoding11/package13/deployment6; Stage8 encoding11/package9/deployment6;
original USB45, PPC leaf contracts21, original XEX encoding8, ELF sanitization8,
watchdog6 and existing static audits/manifests/import-policy units. UI mocks verify
that PLUS/deadline/shutdown request cancel and retain task ownership until join;
the four platform booleans independently match all sixteen input combinations.
Those tests do not execute libnx on hardware.

Final integrated run has **49 bounded processes, seven expected compile refusals,
zero unexpected process faults, zero timeouts, zero sanitizer findings**. Each
child has a 30-second external monotonic watchdog/process-group limit, including
each individual cross-build object/link step. Final SDK completed/entered counts
are61/64 smoke and229/232 stress; three intentional OUT publication refusals
explain the difference. Four distinct imports are genuinely used. Final guest
entries282/1016, controlled refusal assertions72/93, diagnostic event totals1218/
3974, drops0; final allocations0/leases0. Sequential trace snapshots drain only
after all workers join and image unload completes. Total events are not one
simultaneous buffer occupancy. No TSan/race-freedom claim is made.

Original failed evidence stays private: initial fixture displacement/register-save
bugs, entry-symbol incompatibility, macro/name/compiler failures, one30-second
all-in-one build timeout and the24-event trace overflow. None is suppressed or
counted as a final successful run. The watchdog unit's deliberate timeout controls
are separate from unexpected runtime timeouts.

Observed final host process elapsed time / peak RSS (MiB):

| Profile | Direct smoke | Native controller smoke | Stress |
| --- | ---: | ---: | ---: |
| Debug | 2.0798s / 26.55 | 1.1470s / 23.28 | 1.4065s / 23.38 |
| Release | 0.2118s / 22.20 | 0.0318s / 26.12 | 0.0317s / 26.91 |
| Sanitized | 0.7563s / 176.28 | 0.3093s / 176.78 | 0.3900s / 214.17 |

These include startup/watchdog/scheduling overhead, are not per-operation latency,
and provide no Switch memory/performance/FPS estimate. Docker client RSS is not
advertised as cross-compiler peak memory.

## Reproduction, immutable outputs and publication boundary

Use new private directories and the same prepared pinned toolchain/isolated SDK.
Previous lock files, game assets, generated game output and all older NRO/ZIP
manifests remain untouched. No proprietary material belongs in any command below.

```sh
python3 tests/horizon-acceptance/test_fixture.py
python3 tests/horizon-acceptance/lifecycle_core.py --workdir "$LIFECYCLE_CORE"
python3 tests/horizon-acceptance/supervisor_core.py --workdir "$UI_PROOF"
python3 tests/horizon-acceptance/run.py --stage 15 --toolchain "$TOOLCHAIN" \
  --sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_BUILD" \
  --switch-build --icon "$ORIGINAL_ICON"
python3 tests/horizon-acceptance/test_package.py
python3 tests/horizon-acceptance/test_deploy.py
# Only after verified public source is committed and the tree is clean:
python3 tests/horizon-acceptance/package.py --build "$PRIVATE_BUILD" \
  --ui "$UI_PROOF" --workdir "$READY_OUTPUT"
```

Affected earlier regressions use the preserved drivers with the same private
workdir/toolchain/SDK convention: tests/runtime/run.py (optimization0/2 and
--sanitize), switch-horizon/run.py, horizon-runtime/run.py, horizon-foundation/
core.py and run.py, horizon-xex/run.py, horizon-expansion/run.py --stage8,
horizon-runtime-ii/run.py --stage9/10/11 and horizon-acceptance/run.py --stage12/13.
Receipts retain the exact commands and private diagnostic files.

Two independent clean AArch64 builds match ELF/NRO/object hashes. Inspection
confirms ELF64 PIE/NRO0, exact generated entry aliases, RX/R/RW segments, no WX,
no strong unresolved symbols and no unexpected dynamic dependencies. No desktop
runtime library or game code is linked. The package includes a single new app,
source/build manifest, license/third-party notices, checksum and launch README;
the ready root adds ZIP/SHA256SUMS, hardware checklist and sanitized host results.
Full raw command/process logs remain private. Existing probes are preserved.

Build identity **80407778989c**. Original synthetic XEX SHA-256
`f888eea46f35afe17510dc78ee02b7ef39fe7e8e4d5b80888804bee7a4eddcd0`.
Codegen aggregate `c669e32fe3d79f8ec1fa3df64314a7910b908e8da227ac55af2bb1f4af43a016`;
SDK aggregate `753066c47cbc950881b175f20b6de55dfc07f4ef3645cb50577d1add26225f0a`.
NRO SHA-256 `b95c5287dd970d55da687f938c7360888d7f28ed8af5cbfbc54270b6d030faf2`
(794,018 bytes). ELF SHA-256
`4ea12ecaebc4b8400af72c5cab8ac8f4ad38958b573e627cfa5d43754fa11f0e`.

## Mandatory next hardware acceptance — no Stage16

1. Connect Switch through Hekate -> Tools -> USB Tools -> SD Card. Confirm a
   uniquely verified removable USB FAT/exFAT Switch volume, expected ULTRASWITCH.
   Do not guess another disk. If absent, deployment stays PENDING; no repeated
   scans or writes. The safe wrapper validates package/source hashes, Switch
   markers, mount/device/UUID, temporary copy/flush/byte verification and non-forced
   eject. An existing selected candidate requires explicit replacement/backup.
2. Deploy only `switch/brutal-legend-nx-acceptance/brutal-legend-nx-acceptance.nro`.
   Preserve PASS23, PASS38, Foundation, Synthetic XEX, Stage8 and RuntimeII.
   The ready ZIP contains exactly this new application directory.
3. Safely eject, leave Hekate USB storage, boot the existing Atmosphere setup,
   open hbmenu and launch Hardware Acceptance Candidate0.15.0. Prefer Title
   Override; record the actual launch method. No firmware changes are requested.
4. Run one default workload only. Photograph the complete result/build screen,
   libnx applet type and any FAIL/progress/cancellation labels. Also record
   Horizon/Atmosphere versions; applet type alone does not identify those.
5. Expected only, derived from423 host runtime assertions and four independently
   tested platform-check branches: **PASS=427 FAIL=0**, runtime423/platform4,
   result197650, declared functions21/instructions203/sites49/hooks97, guest
   entries282, imports4, SDK entered64/completed61, peak guest workers4,
   allocations0/leases0, controlled refusals72, diagnostic events1218/dropped0.
   The platform checks are actual console init, controller start, native join and
   completed non-error callback/result publication; the NRO computes these from
   real return paths, not a hardcoded successful total.
6. Press PLUS after the result screen and confirm normal return to hbmenu. PLUS
   during testing cancels and invalidates acceptance. If a native call remains
   stuck after the cancellation message, photograph the last stage, use
   HOME/Close Software and report it; do not assume cleanup succeeded or retry
   indefinitely. A crash/error screen is a failed/UNKNOWN hardware result.

```sh
python3 tests/horizon-acceptance/deploy.py --package "$READY_OUTPUT/package"
```

Hekate mass storage cannot run the NRO or collect its runtime output. **HARDWARE
UNKNOWN until the user's actual console evidence and PLUS exit confirmation.**
Only the older four-instruction PASS38 probe has external execution evidence.
After packaging, CI and preservation: stop host feature development and request
this hardware test. A failure permits only a separately scoped diagnosis/replan;
it does not authorize Stage16, game execution or unblocking Xbox residency.
