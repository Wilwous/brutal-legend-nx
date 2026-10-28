# Horizon Runtime II: diagnostics and lifetime hardening

2026-10-28. **HOST VERIFIED / BUILD VERIFIED / HARDWARE UNKNOWN** in the
original synthetic scope. Complete Xbox compatibility is PARTIAL. No proprietary
XEX is loaded, compiled or executed. Xbox Phase 22 BLOCKED; G4/G8 BLOCKED,
G5 UNKNOWN; original game execution NO-GO. No Phase 23 work follows.

## Provenance and implementation boundary

The existing Stage 8 arena, checked views/leases, native libnx adapters,
GuestThread/PCR/TLS and strict synthetic XEX binder are reused. Stage 9 adds
[scoped ABI/instruction evidence](switch-ppc-abi-compatibility.md); Stage 10 adds
[the narrow real SDK query contract](switch-sdk-service-contracts.md).
The production ReXGlue generator emits an original synthetic XEX fixture;
function instruction output matches production-default output byte-for-byte.
Only PCH/binding templates and original adapters differ. Generated C++ is never
edited. The generator, SDK source/extracted bodies and toolchain remain pinned.

The final fixture has **17 generated functions, 158 PPC instructions, 37 scalar
memory sites, 76 hooks, four backedges, 12 calls, 16 returns and 11 branch edges**.
The memory census is 13 u32 loads, 14 u32 stores, five u64 loads and five u64 stores;
11 direct call sites and one admitted indirect call. Independent decoding/comment
and emitted-macro/hook checks agree; execution observes every declared memory
site and backedge. This is coverage of this exact fixture, not arbitrary PPC.
Typed vectors, reservation atomics, partial vectors, non-call computed branches,
MMIO/fetch, OE overflow forms and arbitrary guest exceptions remain denied.

## Bounded diagnostics and refusals

`runtime/diagnostics.h` holds 2,048 fixed-size events per Trace. Each event contains
an enum, guest PC, numeric reason/details, module-instance ID and guest-thread ID.
Immutable image/codegen/SDK SHA-256 fingerprints accompany the trace. No host
pointer, private path, guest memory payload or arbitrary error string is logged.
A mutex serializes writes; native thread-local scoped bindings restore prior
context. Guest/module owners capture the trace; the controller must outlive all
users and join workers before destroying it. This is a scoped synthetic contract,
not a general shared logging service. Clearing is used only outside measurements
or after joins. Trace memory is roughly 64 KiB plus metadata/mutex; snapshot
allocation can fail visibly. Logging failures mark the trace broken; the original
refusal still propagates. Overflow increments an explicit drop count; the
integrated probe treats drops or a broken trace as failure. Optional per-site
logging is off by default; function/import/lifetime events are enabled.

Events cover admitted functions, import admission/entry/return/publication,
refusal reasons, unknown branches/returns, bad ABI context, stop request and
observation, thread startup/callback errors, join, cleanup, close and unload.
Resource events report logical arena allocation/lease balances, not host RSS.
Eight fresh diagnostic cases assert exact module/thread/reason/site where
applicable: bounds/capability refusal, unknown bctrl target, invalid return,
exception gate, unsupported-instruction gate, incomplete MSR context, native
callback failure and injected native creation failure. The exception/instruction
cases call explicit denial gates from production hooks before continuation;
they do not execute an unsupported opcode or implement Xbox unwind/dispatch.
Errors never produce a successful guest continuation. Seven compile-negative
probes also refuse raw access, unsupported vectors/transfers and actual SDK
vector parameter/return translations.

## Lifetime and memory guarantees

Four simultaneous original guests have separate stacks/PCR/TLS and generated
scopes. Seeded repeated multicalls use all three admitted SDK imports. Tests
cover allocation pressure, four partial image stages, stale scopes/function
pointers, close with active calls, permission changes during OUT publication,
stop during import/at every backedge, failed join retaining resources and final
reclamation only after a successful native join. A BatchGuard releases the gate
and joins every started worker before captured arrays die on controller errors.
An injected third-worker creation failure tests partial startup with three other
workers joined. A failure to join safely fail-stops; no live worker is detached
or forcibly reclaimed. OS resource exhaustion and arbitrary scheduling remain
unverified. ASan/UBSan do not establish race freedom; no TSan claim is made.

The checked arena models guest ownership/permissions explicitly; it does not
provide full host address-space isolation. No RWX or global protection bypass is
introduced. The real NtQueryVirtualMemory leaf receives a frozen checked metadata
snapshot and local 28-byte scratch, then checked OUT publication. Exactly three
imports are admitted: xboxkrnl 0x11b compare, 0x12e critical-section initializer
only, 0xee query selector 0 in the documented subset. Resolved symbols alone do
not authorize imports; unsupported signatures/selectors remain denied.

## Final host regression evidence

Each column is actual PASS/FAIL; repeated rows are separate executions, not added
together as a count of unique capabilities. Host compiler: Apple clang 17.0.0.
Debug O0; Release O2; sanitized O1 ASan+UBSan for generated/portable suites (the
historical portable foundation sanitized driver uses O0). No full SDK/title
build or deliberate hardware protection fault is run here.

| Suite | Debug | Release | ASan+UBSan |
| --- | ---: | ---: | ---: |
| Runtime II smoke, trace on | 268/0 | 268/0 | 268/0 |
| Runtime II smoke, trace off | 268/0 | 268/0 | 268/0 |
| Runtime II stress, eight rounds | 387/0 | 387/0 | 387/0 |
| Stage 9 checkpoint | 121/0 | 121/0 | 121/0 |
| Stage 10 checkpoint | 171/0 | 171/0 | 171/0 |
| Stage 8 smoke / stress | 256/0; 382/0 | 256/0; 382/0 | 256/0; 382/0 |
| Stage 3 synthetic XEX | 118/0 | 118/0 | 118/0 |
| Stage 2 generated foundation | 108/0 | 108/0 | 108/0 |
| Original generated PPC leaf | 59/0 | 59/0 | 59/0 |
| Native Horizon / exit UI | 21/0; 6/0 | 21/0; 6/0 | 21/0; 6/0 |
| Portable foundation groups | 6/0 | 6/0 | 6/0 |
| Foundation / query / diagnostic cores | 20/0; 15/0; 22/0 | 20/0; 15/0; 22/0 | 20/0; 15/0; 22/0 |

Final Runtime II has zero unexpected faults, zero timeouts and zero sanitizer
findings; 26 supervised generation/compile/test/build/inspection processes,
including seven expected compile refusals. Each child has a 30-second external
monotonic watchdog/process-group kill. The stress is eight bounded rounds,
32 added integrated workers, four simultaneous. Default Switch workload uses
one round only. All final diagnostics remain below capacity: smoke 739 events,
stress 1,885; dropped 0. Smoke has 44 counted controlled refusal assertions,
155 generated prologue entries, SDK entered 38/completed 37; stress has 441
entries, SDK entered 122/completed 121. The intentional OUT-protection-change
case explains the one uncompleted SDK call: real SDK success is not published.
The invalid selector status is a completed real error response, not a success.

Original public unit suites also pass: Runtime II encoding/policy 11, package 13,
separate deployment 6; Stage 8 encoding 11/package 9/deploy 6; original USB 45,
leaf contracts 21, XEX encoding 8, ELF sanitization 8, watchdog 6. Existing audit,
manifest, generated-caller and loop-stop units pass unchanged. These mock/package
counts are not PPC or hardware execution counts. Original earlier failed macro,
alignment and compiler runs remain private; no diagnostics are suppressed.

Runtime II supervised process wall time / peak RSS (RSS is host-only):

| Profile | Smoke seconds / MiB | Stress seconds / MiB |
| --- | ---: | ---: |
| Debug | 1.4593 / 22.25 | 1.0543 / 22.38 |
| Release | 3.0773 / 22.05 | 0.0323 / 22.09 |
| Sanitized | 0.9514 / 136.81 | 0.4153 / 169.41 |

These are supervised elapsed/RSS observations, not benchmark latency; launch,
watchdog polling and host scheduling affect them.

## Host profiling

One validated warm-up is excluded, then four runs alternate off/on order. O2,
steady_clock; identical work and result/cleanup checks in both modes. Trace clear
is outside timing. Small operations use 20 samples x 32 operations per mode/run;
TLS initialization 8x4, native startup/join 8x1, module load/unload 12x1 and actual
stop-to-join 8x1. Table is mean +/- population standard deviation of four run
means, microseconds per operation; raw per-run min/max/sample variation stays in
private results. The initial fixed-order experiment is retained but excluded
because the second mode benefited from warm-up. No universal overhead percentage
is inferred from noisy overlapping distributions. The admission and running-stop
rows are isolated components; actual generated query and loops include their
hooks/SDK work. No uninstrumented unsafe reference is used.

| Operation | Trace off, us | Trace on, us |
| --- | ---: | ---: |
| generated dispatch | 0.1040 +/- 0.0270 | 0.1261 +/- 0.0394 |
| checked memory store load32 | 0.1968 +/- 0.0484 | 0.1968 +/- 0.0631 |
| pointer snapshot64 | 0.1085 +/- 0.0272 | 0.1082 +/- 0.0349 |
| sdk admission component | 0.0294 +/- 0.0074 | 0.0293 +/- 0.0093 |
| generated query sdk out28 | 1.0739 +/- 0.2581 | 1.1319 +/- 0.3732 |
| guest tls read | 0.1163 +/- 0.0285 | 0.1196 +/- 0.0384 |
| running stop poll component | 0.0031 +/- 0.0008 | 0.0033 +/- 0.0010 |
| generated nested loops hooks | 0.5181 +/- 0.1238 | 0.5697 +/- 0.1827 |
| guest stack pcr tls init cleanup | 2.6491 +/- 0.3877 | 2.8304 +/- 0.7736 |
| host native guest start join cleanup | 32.9024 +/- 6.8124 | 35.7513 +/- 10.6527 |
| image load unload with arena | 307.4001 +/- 18.1152 | 301.0218 +/- 48.1086 |
| host stop request to join | 35.4727 +/- 6.3699 | 35.1092 +/- 7.1384 |

These are macOS host measurements only, with no CPU affinity/frequency control
and appreciable scheduler/cache noise. Horizon timing, physical memory behavior,
libnx synchronization and FPS are UNKNOWN. Benchmark workloads finish with
zero failures/allocations/leases and no trace drops.

## Reproduce and package

Use Python 3.9+ on the verified host, clang++, the prepared pinned ReXGlue toolchain
and isolated nineteen-patch SDK described in the earlier Horizon reports.
The pinned Docker image is in `tests/switch-horizon/toolchain.json`: GCC 15.2.0,
devkitA64 r29.2-1, libnx 4.12.0-1. Builds have no network and source/SDK mounts are
read-only. All variables below are explicit private output locations; do not use
a game directory as an input. Preserve old packages; output directories must be new.

```sh
python3 tests/horizon-runtime-ii/test_fixture.py
python3 tests/horizon-runtime-ii/query_core.py --workdir "$QUERY_CORE_OUTPUT"
python3 tests/horizon-runtime-ii/diagnostics_core.py --workdir "$DIAGNOSTIC_CORE_OUTPUT"
python3 tests/horizon-runtime-ii/run.py --stage 11 --toolchain "$TOOLCHAIN" \
  --sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_BUILD" --switch-build \
  --icon "$ORIGINAL_ICON"
python3 tests/horizon-runtime-ii/benchmark.py --build "$PRIVATE_BUILD" \
  --toolchain "$TOOLCHAIN" --workdir "$PRIVATE_BENCHMARK"
python3 tests/horizon-runtime-ii/test_package.py
python3 tests/horizon-runtime-ii/test_deploy.py
# After verified public sources are committed and the tree is clean:
python3 tests/horizon-runtime-ii/package.py --build "$PRIVATE_BUILD" \
  --benchmark "$PRIVATE_BENCHMARK" --workdir "$READY_OUTPUT"
```

Historical regressions use `tests/runtime/run.py` with optimization 0/2 and
`--sanitize`; `tests/switch-horizon/run.py`, `tests/horizon-runtime/run.py`,
`tests/horizon-foundation/core.py`, `tests/horizon-foundation/run.py`,
`tests/horizon-xex/run.py`, `tests/horizon-expansion/run.py --stage 8` and
`tests/horizon-runtime-ii/run.py --stage 9` / `--stage 10`, with the same private
workdir/toolchain/SDK conventions. Each driver records exact commands/results.

Two independent clean final AArch64 builds match ELF/NRO/object hashes. Inspection
verifies ELF64 PIE/NRO0, exact generated entry aliases, no strong unresolved
symbols, no dynamic dependencies and separate RX/R/RW load segments. Requested
heap is 16 MiB, worker stack 64 KiB; loader main stack and SVC permissions remain
UNKNOWN. Linked privileged SVC wrapper presence is not evidence of invocation
or permission. No desktop runtime library is linked.

Build identity `aacb8eeda878`; original synthetic XEX SHA-256
`12927e77f43b352a649461a8372243dc7f32169fa06663520c3447f32d707be0`.
Codegen aggregate `ffdb7a7f4866c8654f170f2af9d2302f245d1853decdb4dbb3830b9b787206e3`;
SDK source aggregate `106e5bf6fd76f0895399debca1f9bb70a8546f2ad179aa5fbfdfba5d59cfa4ad`.
Final NRO SHA-256 `f61065e3421ba21da076b2320534e6781e6c2aff25ceedae0dcbc19ebf0735c3`
(716,194 bytes); ELF SHA-256
`1f6c72ff08f1143f1b2a1bf8d7c82ac9d80d9a6eb2cc0c558d72f78b57c832c2`.

The package contains exactly one new app under
`switch/brutal-legend-nx-runtime-ii/brutal-legend-nx-runtime-ii.nro`, source/build
receipt, checksum, license/third-party notices and launch instructions. SHA-256
manifest and reproducible ZIP accompany it. The validator checks current source
hashes, two builds, clean committed source, three distinct profiles, real test
counts and UNKNOWN hardware status. Unit fixtures are synthetic containers, not
actual device execution. USB tooling permits only the exact new app name; previous
probes and the Stage 8 application remain unchanged. Existing Runtime II replacement
requires an explicit selection and verified backup. No verified ULTRASWITCH was
mounted during preparation; no deployment is implied. Use the existing validated
Hekate workflow only after selecting the final package:

```sh
python3 tests/horizon-runtime-ii/deploy.py --package "$READY_OUTPUT/package"
```

## Single hardware acceptance and next gate

Expected, calculated by actual assertions: **PASS=269 FAIL=0**, result=24,
imports=3, SDK entered=38/completed=37, generated guest entries=155, peak workers=4,
allocations=0, leases=0, controlled refusals=44, diagnostic events=739/dropped=0.
This includes one successful console-initialization check and 268 runtime checks.
The count is an expectation only. Launch the final Runtime II through hbmenu,
prefer Title Override, photograph the complete screen/build identity and record
launch mode, Atmosphere/Horizon versions. Confirm PLUS returns normally to hbmenu.
Hekate mass storage can copy but cannot launch or observe the NRO.

**Next action: one final Runtime II hardware acceptance.** Do not add more services
or claim broader ABI before resolving any observed platform refusal/fault.
Stage 8 and Runtime II hardware remain UNKNOWN; only the original four-word
PASS38/r3=42/PLUS-exit probe is externally verified. General callbacks, typed
vectors, reservation atomics, partial vectors, guest exceptions and original XEX
residency remain blockers. Synthetic success provides no game boot/playability
or game-code execution authorization.
