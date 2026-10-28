# Hardware acceptance batch — Stage 14 lifecycle and diagnostic quality

2026-10-28. HOST VERIFIED / BUILD VERIFIED in the scoped synthetic runtime;
HARDWARE UNKNOWN. Stage 15 must produce the final candidate and stop feature
expansion until actual Switch acceptance. Xbox Phase22/G4/G8 BLOCKED, G5 UNKNOWN;
game execution NO-GO. Original/proprietary XEX is never loaded.

## Explicit lifecycle and ownership

The original acceptance controller has exactly this permitted sequence:
uninitialized -> initialized -> image -> contexts -> entry -> running ->
stop_requested -> joined -> cleanup -> unloaded -> terminated. Skips, repeats,
cleanup with memory leases, and unload/termination with outstanding allocations
or leases refuse. This policy accompanies actual owners and native operations;
it does not implement an Xbox scheduler or replace SDK XThread semantics.
Partial-failure cleanup is governed by RAII owners/guards, not a fabricated
successful lifecycle transition. An incomplete run is a failure.

Four workers share one checked Space/image, but own separate guest stacks,
PCR/TLS and OUT allocations. Their actual generated multicall combines integer,
finite FP, full-vector and real SDK execution; a generated fill caller writes
per-worker outputs. Gate scheduling holds all workers at a generated backedge.
Early join, guest cleanup and allocation release refuse; active image close
cannot reclaim resources and closes new admission. Four stop requests precede
safe-boundary observation, actual native joins, guest/buffer cleanup and final
image unload. Eight rounds are host-only; Switch default remains one round.

A controller guard outlives all captured arrays, releases gates, requests only
cooperative stops and joins all started workers on host failure. It never detaches,
force-kills or reclaims a live worker. Failure to join safely fail-stops. Original
partial native creation/image initialization and allocation-pressure tests remain
in the integrated baseline. None of this proves OS scheduling or race freedom.

## Trace identity and bounded retention

Events now include operation lifecycle phase and, when a checked allocation is
known, its numeric Space owner/token. Unowned ranges have no fabricated allocation
identity. Scope startup/dispatch, cleanup attempts and explicit controller phases
supply phase context. Existing events without a supplied phase retain zero
(unspecified); not every legacy event is a full state-machine trace. No host
pointers, private paths or guest memory payload is logged. Metadata retains the
exact synthetic image/codegen/SDK fingerprints.

Capacity remains 2,048 events; expanded event storage is about 112 KiB plus
metadata/mutex. Initial stress correctly failed on 24 dropped events. The fix
accounts/drains snapshots only after all workers join and the module is unloaded;
it neither enlarges capacity nor suppresses overflow. Counts sum sequential
buffers, so total events may exceed capacity. Overflow/broken state remains a
failure. Snapshot allocation failure is observable. Serial refusal traces compare
after normalization of numeric module/thread/allocation owners. This does not
impose deterministic ordering on concurrent interleavings. Tests independently
verify exact phase, PPC site, module/thread and allocation for a real RO refusal.

ProbeControl adds an optional synthetic-only UI cancellation channel. It is
inactive for ordinary callers. Cancellation is polled at generated safe points
and propagates a normal controlled host Error; no guest exception dispatch or
unsafe unwind is introduced. A vector fixture proves cancellation before output
publication and balanced teardown. It cannot interrupt a blocked native syscall
or instructions between hooks. Only one global UI controller is admitted and it
must outlive all joined users. A separate local control supports bounded test
injection without replacing the global controller.

On Switch, an active controller queues at most sixteen 96-byte failure labels;
truncation/drop indicators are explicit. This lets the Stage15 main UI draw
results while the test controller runs elsewhere. Host failures still print to
stderr. Assertion counts and failures are never fabricated from queue contents.
The queue is not a guest memory dump or general logging service.

## Checkpoint verification

Debug O0 / Release O2 / ASan+UBSan O1 each: **423/0 smoke, 759/0 stress**;
trace-off results match. Smoke: result197650, four imports, SDK entered64/
completed61, generated entries282, refusals72, diagnostic events1218/dropped0,
allocations0/leases0. Stress: entered232/completed229, entries1016, refusals93,
events3974/dropped0. The three intentional failed OUT publications explain the
SDK counter difference. Concurrent ordering is not asserted from these counts.

Portable lifecycle/control/core tests **35/0 per profile**; original diagnostic
core22/0 and query core15/0 remain passing. Affected RuntimeII remains268/0 smoke,
387/0 stress; Stage13 remains380/0 and499/0 per profile. Seven expected compile
refusals remain. Final runs have no unexpected faults, timeouts or sanitizer
findings. Earlier trace-overflow, missing-header and compiler-warning failures
are retained privately. Warnings were fixed, not disabled. Two clean bounded
per-object AArch64 builds match, no strong undefined symbols, dynamic libraries
or WX segments. Fixture remains21 functions/203 instructions/49 sites/97 hooks/
four backedges with unchanged production-default instruction C++.

```sh
python3 tests/horizon-acceptance/lifecycle_core.py --workdir "$PRIVATE_CORE"
python3 tests/horizon-acceptance/run.py --stage 14 --toolchain "$TOOLCHAIN" \
  --sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_STAGE14" \
  --switch-build --icon "$ORIGINAL_ICON"
```

All children retain a 30-second monotonic watchdog/process-group budget; generated
output, detailed logs and failed evidence stay private. No SD scans/deployment
or intermediate hardware acceptance are performed.

## Horizon uncertainty matrix

| Assumption | Classification | Evidence / required observation |
| --- | --- | --- |
| checked range/permission/ownership, snapshot publication | HOST-VERIFIABLE | independent oracles, negative cases, sanitizer runs; software model only |
| lifecycle/canonical tracing/control cancellation | HOST-VERIFIABLE | actual generated host callers and portable contract fixtures |
| libnx headers, AArch64 symbols, ABI compilation, segments | CROSS-BUILD-VERIFIABLE | pinned GCC15.2/libnx4.12, matching ELF/NRO; not runtime evidence |
| native scheduling, stack allocation, joins, TLS on Horizon | HARDWARE-REQUIRED | final candidate progress/result screen and zero resource balance |
| hbmenu launch mode and loader-owned main stack | HARDWARE-REQUIRED | report actual applet type where available; record launch method |
| required SVC permissions / libnx cleanup branches | HARDWARE-REQUIRED | link inspection alone cannot prove permission or branch execution |
| PLUS exit and applet teardown | HARDWARE-REQUIRED | normal return to hbmenu after completed checks |
| physical guest VM isolation | BLOCKED | logical checked arena is not complete Horizon page isolation |
| full Xbox ABI, typed vectors, reservations, partial vectors, guest unwind | BLOCKED | narrow selected tests do not establish these contracts |

A blocked native syscall may not respond to cooperative cancel. The Stage15 UI
must remain observable, retain the last progress stage and never force-kill a
worker. Manual HOME/Close Software recovery and reporting are documented for
hardware; host results cannot resolve that dependency.
