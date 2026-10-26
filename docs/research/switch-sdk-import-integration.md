# Synthetic XEX and SDK imports — Stage 7

HOST VERIFIED: 40 new assertions / cumulative 198/0 in Debug, Release and
ASan+UBSan. Two clean matching AArch64 builds. Five unsupported compile cases
refused. No unexpected faults, timeouts or sanitizer diagnostics. Hardware UNKNOWN.
The original Stage 2/3 fixtures retain 108/0 and 118/0 per profile.

The original flat image is now 11 functions / 99 PPC instructions with two
imports: xboxkrnl.exe ordinal 0x11b RtlCompareMemoryUlong and ordinal 0x12e
RtlInitializeCriticalSection. Production codegen emits 25 scalar memory sites,
52 hooks, 8 calls and 10 returns. The four backward branches remain covered.
The new entry calls the former generated entry, then the actual SDK initializer;
its initial r3 result is 18. Construction of the XEX remains host-side.

The pinned SDK export table, RTL source, dispatcher header and extracted bodies
are fingerprinted in `tests/horizon-expansion/sdk-lock.json`. Actual SDK types,
initializer helper and entry are compiled privately without editing their bodies.
The narrow unused-parameter diagnostic accommodation surrounds only the SDK's
unused cs_ptr parameter. A trace hook records entry, with no fabricated SDK result.
Initializer signature is void; no success code is invented in r3.

The IN compare adapter snapshots retained aligned read-only image storage.
The INOUT initializer is restricted to 28 aligned bytes in retained writable image
data, excluding import metadata. A Space transaction validates read and write
capabilities before invoking the leaf on typed scratch storage. It holds the Space
lock across snapshot, actual SDK call and writeback, preventing lost updates to
untouched fields. Exceptions publish no output. The trusted bounded leaf must
not reenter Space or retain scratch. The initializer preserves SDK-untouched
header bytes; lock_count becomes all ones, recursion and owner zero.

OUT is separately validated through the original generated store fixture over
write-only guest storage, with zero reads. This is not a third SDK export.
NtQueryVirtualMemory (ordinal 0xee), critical-section Enter/Leave and scheduler
semantics remain BLOCKED. No heap-query/status emulation is supplied.

Import admission checks image bytes, metadata, generated function and import
identity, ordinal/signature, native thread/context, selected call edge, pointer
capability and shutdown state. Closing admission refuses new calls while already
admitted calls retain leases. A controlled worker requests stop during an admitted
initializer: the accepted SDK leaf commits, the next production hook stops, join
completes, and only then can image storage be reclaimed.

Review found that temporary views exposed a release handle in their public
representation. Stage 7 makes that representation private and checks it at compile
time; a read view still permits checked reads without exposing release authority.
Global SDK entered/completed and logical allocation/lease counters provide actual
telemetry, including completed SDK calls in negative fixture paths. The cumulative
Stage 7 smoke executes 20 actual completed SDK calls, with zero remaining guest
allocations/leases. No host pointers are guest-visible.

Reproduce `tests/horizon-expansion/run.py --stage 7` with pinned private toolchain,
isolated SDK, fresh private output, `--switch-build` and the original icon. Run
`tests/horizon-expansion/test_fixture.py` for 11 public encoding/auditor units.
Synthetic image SHA-256:
`c64f6cb388bd4a81b821f9c23e478c337d20655e2fbc8b6ec3cb11b88a2585bd`.

The full supported path is original image → actual production codegen → AArch64
build → strict synthetic binding → native/guest TLS → generated caller → checked
pointer adapter → real SDK leaf → generated return → safe stop/join/cleanup.
HOST and BUILD evidence do not establish execution on Switch.

Xbox Phase 22 BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN. Game execution NO-GO.
