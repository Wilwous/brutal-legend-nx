# N3 — synthetic native image Horizon integration

2026-10-30. **HOST VERIFIED** for the finite original fixture. AArch64 inspection
and hardware observation are separate claims. Historical G4/G8 BLOCKED, G5 UNKNOWN;
no original title load/execution. This is native-route N3, not Xbox Phase23.

## Actual pipeline

Original descriptor-covered XEX → production generation and independent encoding
census → canonical NativeImagePlan → exclusive Space-backed materialization →
exact plan/code/control/function binding → GuestThread/PCR/static TLS setup →
leased Scope dispatch → real selected SDK body → checked writes → cooperative
loop stop → native GuestTask join → guest cleanup → module close → image unload.
No strict-loader failure is automatically retried using this path.

The N3 review strengthened binding beyond the first N2 checkpoint: a canonical
plan hash is independently serialized by Python, compiled into CodeIdentity and
compared with the admitted image fingerprint. A different lifetime/semantic plan
with identical source/code now refuses binding. The actual function map also
must equal the trusted compiled map (addresses, implementation pointers, names
and signature flags), not merely have plausible boundaries. Contract SHA256
covers boundaries, site ownership/kinds/widths/targets, roots and terminal return.
These are trusted-build manifests, not authentication of arbitrary user manifests
or attestation of a malicious compiler. No untrusted runtime JSON parser exists.

NativeImagePlan now bounds identity work to64 regions, four imports and64 MiB
source before hashing. Region backing records the logical private arena loan at
reservation; commitment, initialization and final accessibility stay distinct.
An explicit `<cstring>` makes the header self-contained for devkitA64.

Existing Stage15 components are reused: Space/checked memory, GuestThread/Slots,
Scope/CFI, NativeThread/GuestTask, ProbeControl, structured Trace, UI supervision
and release-then-press PLUS exit. No Stage15 source or hardware package is
replaced. Only the new test uses plan-driven placement. Its one SDK import uses
the real pinned RtlCompareMemoryUlong body with bounded IN scratch; the remaining
three SDK imports are covered by unchanged Stage15 regressions, not claimed used
by the new PPC workload.

## Final host matrix and count meaning

Per Debug/O0, Release/O2, ASan+UBSan profile:

| Mode | Positive semantic assertions | Expected negative refusals | Bulk byte comparisons |
| --- | --- | --- | --- |
| Smoke |19 |19 |327680 |
| Eight fresh load/execute/unload cycles |152 |152 |2621440 |
| Two parallel independent images/Spaces |38 |38 |655360 |

All rows have0 failures, actual result32, zero outstanding allocations/leases.
Repeated/concurrent counts repeat the same finite contracts; they are not new
independent ABI claims. An independent complete-image oracle compares contiguous
producer XEX payload bytes with all admitted native bytes before execution; the
producer separately verifies its authored zero run. N1 additionally retains75
semantic checks (26 positive/49 refusal) plus16384 byte comparisons per profile.

The source workload is3 PPC functions /35 words,9 checked memory sites,15 hooks;
one import adds a fourth map record. Successful entry performs4 dispatches (root,
helper direct, helper indirect, SDK). All three PPC functions are observed across
success, stop and rejection tests. Each complete smoke executes two successful
entries (host plus joined native GuestTask), SDK entered2/completed2. Other modes
report these counters for the last individual cycle; semantic/byte counts are
aggregated. Trace events are bounded and owned; snapshots are freed before return.

Tests cover the N1 plan/source/zero/permission/collision/rollback matrix; wrong
plan and stale code identity; altered function map/boundary/site metadata;
wrong static TLS/PCR; invalid entry/indirect/return; unsupported exceptional
adapter transfer; injected native thread creation failure; active code leases;
actual generated unowned loads and RO stores; stop at a generated loop hook;
joined cleanup and independent concurrent image lifetimes. Failed generated
contexts are destroyed, not resumed as if guest unwind succeeded. The exception
probe calls the explicit denial seam; it is not a hardware fault or Xbox dispatcher.
N1 cannot support arbitrary partial-page unions/relocations and refuses them.

The previous Stage15 production workflow was rerun unchanged:423/0 smoke and759/0
stress per profile, controller/trace-off agreement, seven expected negative
compiler refusals, build fingerprint80407778989c and result197650/SDK64/61.
Its original hardware NRO/ZIP remain byte-identical. No whole historical rebuild.

Every child compile/generator/run uses an external30-second process-group watchdog.
ASan/UBSan and unexpected signal/timeout results are checked separately from
expected semantic exceptions. Early failures remain private: source CTR union
compile spelling, authored pattern initialization, GCC misleading-indentation,
missing explicit `<cstring>`, and libnx's global Permission name collision. The
last was fixed by qualifying the existing software permission type. No SDK,
libnx flag, warning level, old assertion or memory protection was weakened.

## AArch64 and package workflow

After passing the host matrix, use a fresh build directory:

```sh
python3 tests/native-image-horizon/build.py --workdir "$NEW_N3_BUILD" \
  --host-output "$VERIFIED_N3_HOST_OUTPUT" --toolchain "$PREPARED_HORIZON_CLI" \
  --icon "$ORIGINAL_PROBE_ICON"
```

It verifies the immutable devkitA64 container digest and19 SDK patch hashes,
checks generated/source-contract drift and SDK-body integrity, then makes two
clean AArch64 builds in isolated read-only-input, network-disabled containers.
Every object/link/inspection child is bounded. The existing ELF/NRO inspector
runs with the new title/version: requires ELF64 AArch64 PIE/NRO0, correct NACP,
generated entry alias,16-MiB heap, static libnx threads, no strong unresolved
symbols, no dynamic dependencies, no W+X or forbidden platform symbols. Both ELF,
NRO and all selected objects must hash identically before packaging succeeds.

Output is a separate app:
`switch/brutal-legend-nx-native-image/brutal-legend-nx-native-image.nro`, version
`0.1.0-n3`. Ready directory contains NRO, SD ZIP, SHA256SUMS and build manifest;
private absolute locations and raw records are never committed. No SD deployment,
firmware action or overwrite of the old application is performed.

The screen uses a256-KiB native controller and the existing responsive supervisor.
The synthetic workload is finite; PLUS during startup requests cooperative
cancellation and makes acceptance invalid. There is no forceful in-process native
thread kill; arbitrary native hangs remain outside this proof. After completion,
actual platform checks, result, SDK counters, diagnostics, allocation/lease balance
and semantic PASS/FAIL are displayed. Expected normal result is32, SDK2/2,
allocations0/leases0 and PASS42/FAIL0 (38 measured runtime checks +4 platform).
Counts are calculated from actual assertions, never printed as predetermined PASS.

**HARDWARE UNKNOWN:** a built NRO cannot establish execution or PLUS exit on a
Switch. Hardware acceptance, when separately performed, needs the complete screen
and normal PLUS return to hbmenu. The preserved Stage15 hardware evidence does
not transfer to this new image path. No claim of game compatibility follows.

## Verified build checkpoint

Two clean builds passed all inspector checks with matching ELF/NRO and five
object hashes. ELF SHA256:
`22f49b7037b187d85529cfe8a8facc7fe427c2b2571e9e080405cf9c0d6cfdd6`.
NRO SHA256:
`5be3f0c1da9ba5aff4e2a6564508c09b66ca591744cd6d5d0a545ae7b619bc29`.
There are0 unresolved strong symbols,0 dynamic dependencies and no W+X segments.
The existing pinned libnx privileged SVC wrapper presence does not prove it is
invoked or permitted. Hardware runtime and PLUS return remain UNKNOWN.

A further link-time privacy failure was preserved: `iostream` diagnostics pulled
in a third vendor-newlib source path not accepted by the existing exact sanitizer.
The new probe now uses the existing bounded report_failure mechanism instead.
The sanitizer and its refusal census were left unchanged. Only the known two
RO diagnostic prefixes are normalized; executable bytes/layout are unchanged.
