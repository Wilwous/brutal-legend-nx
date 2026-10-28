# Integrated Horizon synthetic runtime — current Runtime II and preserved Stage 8

2026-10-28: Stages 9–11 are HOST/BUILD VERIFIED in their declared subset;
HARDWARE UNKNOWN. [Runtime II evidence, commands and limitations](switch-runtime-diagnostics.md)
records 268/0 smoke, 387/0 stress per profile, 17 functions/158 instructions,
37 memory-sites/76 hooks and three admitted real SDK imports. Expected new screen
is PASS269/FAIL0, result=24, SDK entered38/completed37, guest entries155, allocations0,
leases0. The separate `brutal-legend-nx-runtime-ii` package preserves Stage 8 and all
older probes. This expected screen has not been observed on hardware.

Xbox Phase 22 BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN; original game execution NO-GO.
The following Stage 4–8 record is preserved as historical evidence and regression.

## Preserved Stage 4–8 checkpoint

2026-10-27. **HOST VERIFIED / BUILD VERIFIED / HARDWARE UNKNOWN.** The result is
PARTIAL as an Xbox runtime. Original game execution is NO-GO; Xbox Phase 22 stays
BLOCKED, G4/G8 BLOCKED, G5 UNKNOWN. This work neither resumes Phase 22 nor starts
Phase 23. Only the older PASS38 probe has external hardware evidence.

## Engineering checkpoints

| Stage | New smoke assertions per profile | Checkpoint evidence |
| --- | ---: | --- |
| 4 — memory/ownership | 50/0 | Read/write/write-only, epochs, leases, atomic failures, checked generated scalar accesses |
| 5 — threads/PCR/TLS | 67/0 | Four concurrent guests, full context initialization, startup refusal, join/lifetime |
| 6 — ABI/control flow | 41/0 | Production codegen, selected GPR/FPR, exact sites/edges/returns, four stopped backedges |
| 7 — SDK/XEX | 40/0 | Actual compare + initializer, IN/OUT/INOUT, image identity, close admission |
| 8 — integration | 58/0 | Seeded multicall, rollback, malformed image, pressure, stale ownership, stop during memory |

Each checkpoint passed Debug, Release, ASan+UBSan and AArch64 compile/link before
commit. Stage 8 cumulative smoke is **256/0 per profile**; bounded host stress is
**382/0 per profile**, using eight load/unload rounds and 32 integrated workers.
The Switch default uses one integrated round/four workers; expensive repetition
and benchmarks are host-only. Independent encoding/auditor units are 11/0,
integrated deployment mocks 6/0 and package integrity fixtures 9/0.

Earlier host regressions remain: original portable foundation 6/0 groups (including
4,000 model operations), native Horizon 21/0 plus UI 6/0, original generated leaf
59/0, public foundation core 20/0, Stage 2 generated 108/0 and Stage 3 XEX 118/0,
each in all three profiles. Additional original public tests: USB deployment
45/0, leaf contracts 21/0, old XEX encoding 8/0, ELF sanitization 8/0, watchdog 6/0.
Final runs have zero unexpected process faults, zero timeouts and no sanitizer
findings. Five unsupported compile fixtures deliberately fail compilation. The
portable foundation's guest-exception fixture records expected controlled refusal,
not a native fault. No new native signal recovery is claimed.

Earlier development diagnostics are preserved privately: a deliberately malformed
section test initially changed an already-zero byte and failed its assertion;
it now changes the actual RVA byte. Strict GCC also rejected an ambiguous libnx
name, a Makefile source reference and misleading indentation, all corrected.
No loader, ownership rule or compiler warning gate was weakened.

## Actual pipeline and declared coverage

Host-authored flat XEX → pinned production ReXGlue codegen → AArch64 static link
→ strict image/code binding → original guest stack/PCR/TLS → generated PPC caller
→ exact import admission → checked pointer adapter → real SDK leaf → PPC return
→ cooperative stop → native join → guest/image cleanup.

The XEX is original, unencrypted/uncompressed and built on the host. Its SHA-256
is `c64f6cb388bd4a81b821f9c23e478c337d20655e2fbc8b6ec3cb11b88a2585bd`.
There are **11 functions / 99 instructions**, independently matched to generated
comments and the unmodified production-default instruction output. All **25 scalar
memory sites** have checked macros and emitted hooks (7 load32, 10 store32,
4 load64, 4 store64). There are **52 hooks**, **8 calls**, **10 returns**, **9 branch
edges**, and **4 backward branches**. All declared scalar memory hooks and all four
backward hooks are observed across the suite; every backward branch is separately
stopped. A hook hit is not a claim that a refused access completed.

The genuinely executed imports are xboxkrnl.exe **0x11b RtlCompareMemoryUlong** and
**0x12e RtlInitializeCriticalSection**. Bodies/types/export identities are pinned;
no fake success function or full desktop SDK runtime is linked. The smoke records
**28 completed actual SDK calls**, including calls in negative paths; host stress
records **84**. Selected initial PPC result is **r3=18**, with peak four concurrent
guest workers. Logical allocation and lease deltas are measured and must be zero.

The supported ABI subset is documented in the [ABI report](switch-ppc-abi-control-flow.md).
[Memory](switch-memory-ownership.md), [threads/TLS](switch-thread-tls-runtime.md)
and [SDK contracts](switch-sdk-import-integration.md) define the other boundaries.
No complete Xbox ABI, typed-vector signatures, arbitrary callbacks, general
XThread scheduler/APC/PRCB, guest SEH/unwind, atomics/MMIO/fetch or critical-section
Enter/Leave is provided. NtQueryVirtualMemory remains denied. Only selected
finite FPR behavior is tested; special values/rounding/FP exceptions are UNKNOWN.

Guest permissions are software checks over ordinary homebrew storage. No fixed
4-GiB mappings, VM isolation, raw fallback, RWX allocation or global protection
bypass exists. Default backing addresses are quarantined; explicit logical XEX
addresses may be reused after full teardown. Old handles/views/closed scopes
remain invalid, but arbitrary numeric-pointer ABA prevention is not established.

## Build and resource evidence

Pinned image: `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`,
linux/arm64; devkitA64 r29.2-1/GCC 15.2.0, libnx 4.12.0-1, switch-tools 1.13.1-1,
rules 1.1.1-1. Actual libnx archive hash matches the public lock. SDK revision
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` plus the existing 19 locked patches;
prepared CLI `6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c`.
No global or original isolated SDK source is modified by these stages.

Two clean final builds match: NRO 699,810 bytes; private ELF 5,912,040 bytes.

- ELF SHA-256: `0cc98c88d38bfc10648382df11f579d04119ca361fe523d3c08c81c7b1c35b47`.
- NRO SHA-256: `d00df975e380653a3e7a77461c18784de642e3e3ce52be103cc67b253f8bb22b`.

Inspection verifies AArch64 PIE/NRO0, linked generated entry alias, zero undefined
strong symbols, zero dynamic library dependencies and no WX segments. Linked
libnx contains its normal svcUnmapTransferMemory wrapper (0x52); static presence
is not a permission or execution claim and no such API is used for guest memory.
The NRO requests a 16-MiB heap, each native worker a 64-KiB stack, and each guest
has a separate 16-KiB checked stack. Largest reported probe static frame is 12,128
bytes; this is not total stack high-water usage. Loader main-stack capacity and
actual Horizon memory consumption remain UNKNOWN.

Final runtime child observations (seconds / peak host RSS bytes):

| Profile | Smoke (256/0) | Host stress (382/0) |
| --- | --- | --- |
| Debug | 3.339336 / 31,686,656 | 1.036126 / 32,129,024 |
| Release | 0.192086 / 23,330,816 | 0.026376 / 31,670,272 |
| ASan+UBSan | 1.052396 / 144,080,896 | 0.378623 / 178,159,616 |

These are watchdog-observed process values, not console memory/timing or container
compiler peak RSS. Zero final unexpected native faults/timeouts/sanitizer findings.

Host benchmark means, microseconds, Apple Clang 17 / Release (not Switch timings):

| Operation | Samples | Mean µs |
| --- | ---: | ---: |
| Generated add64 + strict dispatch | 2,000 | 0.120480 |
| Checked BE store/load32 | 2,000 | 0.287724 |
| Checked 64-byte pointer snapshot | 1,000 | 0.155832 |
| Generated XEX call + two SDK leaves | 64 | 6.800141 |
| Guest stack/PCR/TLS init + cleanup | 32 | 3.872281 |
| Host native guest start/join + cleanup | 8 | 73.130125 |
| Image load/unload including arena | 8 | 501.312625 |
| Host stop request → joined | 8 | 69.552125 |

Stop observations span 55.333–126.292 µs, including host scheduling. Timer overhead
and small synthetic workloads limit interpretation. The benchmark process peak
RSS is 14,319,616 bytes. No FPS, game performance or console timing is inferred.
Every test/build child has an external monotonic 30-second group watchdog. Docker
is offline, read-only except private output/tmp, limited to two CPUs/1 GiB.

## Reproduction and one hardware acceptance

Use the existing pinned prepared toolchain and isolated SDK. For a fresh prepared
copy, `tests/horizon-runtime/prepare.py` takes `--sdk-source`, `--sdk-build`,
`--sdk-stage`, `--ninja`, `--workdir`; it verifies the existing patch locks first.
All output variables below name fresh private directories outside Git.

```sh
python3 tests/horizon-expansion/run.py --stage 8 \
  --toolchain "$PREPARED_TOOLCHAIN" --sdk-source "$ISOLATED_SDK" \
  --workdir "$PRIVATE_BUILD" --switch-build --icon "$ORIGINAL_ICON"
python3 tests/horizon-expansion/benchmark.py --build "$PRIVATE_BUILD" \
  --toolchain "$PREPARED_TOOLCHAIN" --workdir "$PRIVATE_BENCHMARK"
python3 tests/horizon-expansion/package.py --build "$PRIVATE_BUILD" \
  --benchmark "$PRIVATE_BENCHMARK" --workdir "$PRIVATE_READY"
python3 tests/horizon-expansion/deploy.py --package "$PRIVATE_READY/package"
```

The new destination is exclusively
`sdmc:/switch/brutal-legend-nx-runtime/brutal-legend-nx-runtime.nro`.
Earlier applications remain untouched. Deployment uses the existing tested
Hekate USB copy/backup/temp/flush/hash mechanics and independently verified
ULTRASWITCH USB metadata/structure. No mounted verified card means no writes.
A different existing runtime NRO requires explicit destination replacement;
normal ejection is attempted, never forced. No firmware/configuration changes.

Expected final screen, still **unobserved**:

```text
Build 0.8.0 / synthetic c64f6cb388bd / stages through 8
Stage 4: PASS=50 FAIL=0
Stage 5: PASS=67 FAIL=0
Stage 6: PASS=41 FAIL=0
Stage 7: PASS=40 FAIL=0
Stage 8: PASS=58 FAIL=0
Generated PPC result=18; actual SDK calls=28
Peak concurrent guest workers=4; allocations=0 leases=0
PASS=257 FAIL=0
```

The extra assertion is successful console initialization. No counts are hardcoded
into the probe summary. Exit USB storage, boot the existing Atmosphère setup,
open hbmenu (prefer Title Override if available), launch Integrated Runtime,
photograph the complete screen and record launch mode/platform versions. PLUS
must return normally. No intermediate Stage 2/3/4–7 device test is required for
this request. Hekate storage cannot run or observe an NRO.

## Publication and next gate

Public contents are original source/tests, templates, SDK fingerprints, retained
notices and sanitized documents. Proprietary files, generated game C++, synthetic
build outputs, title objects, manifests and private diagnostics are excluded.
Original XEX SHA-256 and checkpoint sentinels are preserved. Main, Xbox blocked
branch, tags and all earlier working probes are preserved. Five independently
reviewable commits belong only to the Switch research branch; no Release/tag/merge.

Exact next action: one external hardware acceptance of this integrated smoke
application. Only then consider a bounded synthetic runtime refinement based on
observed failures/resources. Proprietary execution remains NO-GO regardless of
synthetic success or a future positive console photograph.
