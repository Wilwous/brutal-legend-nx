# Stage 2 — Horizon runtime foundation

2026-10-26. **HOST VERIFIED / BUILD VERIFIED / HARDWARE EXECUTION UNKNOWN.**
The user reports the preceding four-instruction PPC Runtime Probe at
`PASS=38 FAIL=0`, r3=42, checked memory/cleanup PASS and normal PLUS exit on a
real Switch. This is HARDWARE VERIFIED for that original scope only; launch mode
(Applet versus Title Override) remains UNKNOWN. The new foundation NRO has not
been observed on hardware. Xbox Phase 22 remains BLOCKED and game execution NO-GO.

## Implemented subset

The original checked arena remains unchanged. New `Space` adds logical guest
mappings over its private bounded storage: no fixed host mappings, host executable
memory, MMIO or RWX allocation. Owner/id tokens, permissions, commitment, full-span
bounds, address-wrap rejection, leases and transactional multi-region release
mediate every admitted generated access. Address quarantine is intentional:
released backing addresses are not recycled within the bounded arena. Rollback
releases live resources but does not reset its allocation cursor.

`NativeThread` uses actual libnx threadCreate/threadStart/kernel wait/join/close
on Horizon and std::thread on the host. Its callbacks/stack remain owned until
termination and successful join. Duplicate start/join reject. Timeout returns
failure while retaining ownership; destroying an unjoined worker fail-stops.
`Event` uses libnx Mutex/CondVar and monotonic bounded waits, with a host condition
variable implementation. There is no detached worker or busy-spin-only protocol.
The creation-failure test is an explicit injected failure, not an observed kernel
out-of-memory condition. Real native API errors propagate; kernel exhaustion
itself is untested. Native worker stack request is 64 KiB; main stack is loader-owned.

`GuestThread` is an original synthetic XThread **subset**, not an instance of the
full desktop SDK XThread. It associates a guest context with module lifetime,
16-KiB guest stack, 0x2d8-byte PCR and 32-byte static TLS followed by 64 four-byte
dynamic slots. The selected fixture admits registered dynamic slot zero. PCR TLS
pointer +0 and 64-bit self pointer +0x30 follow the inspected pinned SDK X_KPCR
layout and XThread initialization. r1 starts aligned inside owned guest stack,
r2 is the explicit synthetic TOC value, and r13 is the numeric guest PCR address.
Other unconsumed PCR fields stay zero; KTHREAD/PRCB scheduling, APCs, fibers,
Xbox handles, guest exceptions and a complete scheduler are not supplied.

Native C++ thread-local binding, libnx/newlib native TLS, guest TLS bytes/slot
registration and guest r13 are distinct. Registration tokens include owner and
generation; foreign/stale tokens reject. Live guest threads pin a slot against
freeing, a conservative fixture admission rule rather than an Xbox return-code
claim. Per-thread PCR/TLS/stack allocations are independent. Cleanup refuses
before join or during calls. Partial allocation failure rolls back prior regions;
a pinned region prevents the whole cleanup transaction before any region is freed.
Module close denies new admission while retaining all active thread/call leases.

The actual SDK PPCContext and existing checked-memory/CooperativeStop definitions
are reused. No new SDK patch is required after the separately pinned Horizon
leaf patch. No installed/global SDK file is changed. Desktop mmap/mprotect,
signal recovery, shared-memory aliases, dynamic loading, SDL/Vulkan/audio,
spdlog/tm_gmtoff and full KernelState/Processor/XThread are excluded, not replaced
by successful stubs. Software mediation is not hardware guest-memory isolation.

## Production generation and stop coverage

An independently authored raw-test module has **three functions / 30 PPC
instructions**, direct helper calls, branches, finite and continuing loops,
stack load/store, LR and nonvolatile GPR save/restore, and PCR/dynamic TLS access.
Input SHA-256:
`a52e2d0859518945ba414de425ad6c00d584617888a502390e82ea060d8d2b2e`.
Production `rexglue recompile-tests` generates it with explicit original Horizon
PCH templates. All emitted instruction bodies are byte-identical to default
production output; ordered decoded instruction comments, fields and CFG match
the independent original encoding. Repeat generation hashes match. Generated C++
is never edited. Binding headers contain metadata and declarations only.

Static memory census is four load32, four store32, one load64 and one store64
site. All ten use the scoped checked layer; there is no raw guest base fallback.
Direct and indirect dispatch only accept registered context/base functions.
Raw pointer helpers, vectors, unsupported timebase, longjmp and typed-vector
entry calls are compile refusals. No vector ABI or complete Xbox ABI is claimed.
Selected r1/r2/r13/LR, saved GPR/FPR/vector state and synthetic returns are tested.

Both independently decoded backward branches target a direct helper call.
The helper's generated prologue is a mandatory safe point on every iteration.
Static loop coverage is **2/2** by this dominance policy; it is not per-branch
midasm instrumentation. Finite-loop guards are observed in positive execution;
the continuing loop executes an iteration before its second helper guard blocks
on a real event. A host request then follows requested → boundary → typed host
unwind → exited → joined → reclaimed → unloaded. No asynchronous cancellation or
longjmp crosses C++ objects. The cancelled guest context is discarded, not resumed
as if its PPC epilogue or a guest exception unwind had executed.

## Final local results

| Suite | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| Actual generated foundation fixture | 107/0 | 107/0 | 107/0 |
| Public portable adapter CI contracts | 20/0 | 20/0 | 20/0 |
| Previous four-instruction generated probe | 59/0 | 59/0 | 59/0 |
| Previous native probe/UI | 27/0 | 27/0 | 27/0 |
| Existing portable foundation | 6/0 groups | 6/0 groups | 6/0 groups |

Additional controls: five new expected compiler refusals; eight previous expected
compiler refusals; 45 deployment mock units, 21 original leaf encoding/package
units and eight ELF sanitizer units pass. New fixture groups total memory19,
native threads26, PCR/TLS14, generated execution18, stop13, ownership/rollback17.
Concurrent memory readers, synchronized increments, allocation/release and two
independent guest contexts pass. Sanitizer findings, unexpected faults and external
timeouts are zero in final runs. A controlled zero-time join refusal is intentional.
This is evidence for these paths, not a proof of race freedom.

Every host execution uses an external monotonic 30-second process-group watchdog.
The final stage runner records 24.410 summed child seconds; host execution
Debug 0.793 s / 10,010,624 bytes peak RSS, Release 0.165 s / 9,666,560 bytes,
Sanitized 0.383 s / 30,097,408 bytes. Earlier missing-include/name-collision/GCC
warning diagnostics remain private. No crashing allocation test is repeated.

The unchanged pinned devkitA64/libnx image builds twice offline with identical
NRO/ELF/object bytes. NRO **347,554 bytes**, SHA-256
`8b1b76ed1c5659bd911eb024d0fb2280a22266575614e11081bc93f12335eebb`.
ELF SHA-256 `01a1240f7f32ae2f858c5ea466447e0af23ae13d8850a5d2792474eef975dcac`.
Inspection verifies AArch64 PIE, RX/R/RW load segments, no W+X, zero DT_NEEDED,
zero unresolved strong symbols, generated alias, 16-MiB heap request and NACP/icon.
Loaded file bytes are 230136 RX + 54804 R + 17456 RW; RW memory extent is 36304.
No fixed-address/SVC protection bypass is introduced. Native allocation/protection
behavior and launch permissions still require hardware observation.

The new application is **Brutal Legend NX - Horizon Runtime Foundation**, separate
path `sdmc:/switch/brutal-legend-nx-foundation/brutal-legend-nx-foundation.nro`.
Its expected successful screen is **PASS=108 FAIL=0**: 107 actual checks plus
successful console initialization, primary PPC r3=12 and zero remaining guest
allocations. The summary is computed, not hardcoded. It waits for PLUS.
This result has not been observed on a Switch. The successful PASS38 NRO/ZIP and
existing USB deployment sources are unchanged. A six-file ZIP is supplied locally;
new packages never overwrite earlier probes automatically.

## Reproduce and acceptance

Use the previously verified isolated `toolchain-verified` CLI/header environment;
see [its pinned preparation method](switch-horizon-runtime-adapter.md#reproduction).
All work/output variables must refer outside the public checkout.

```sh
python3 tests/horizon-foundation/run.py --toolchain "$HORIZON_TC" \
  --workdir "$FOUNDATION_WORK/build" --switch-build --icon "$ORIGINAL_ICON"
python3 tests/horizon-foundation/core.py --workdir "$FOUNDATION_WORK/core"
python3 tools/switch/stage_package.py --application foundation \
  --build "$FOUNDATION_WORK/build" --workdir "$FOUNDATION_WORK/ready"
```

Stage 2 **build/host acceptance passes** in this explicitly bounded subset;
Stage 3 host-side original XEX preparation is eligible independently of device
access. Stage 2 **hardware acceptance is UNKNOWN**. Test this NRO on the existing
homebrew setup, record the entire screen, launch mode and platform versions,
then verify PLUS exit. Do not use it to claim game boot or satisfy Xbox residency.
