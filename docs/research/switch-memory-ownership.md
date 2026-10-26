# Horizon checked memory and ownership — Stage 4

Architecture map at baseline `712780a`:

| Component | Reused contract | Gap addressed or retained |
| --- | --- | --- |
| `Memory` | Bounded arena, epochs, permissions, checked BE copies | Add write-only; no host pointer escapes |
| `Space` | Logical mappings, ownership, leases, atomic release group | Add epoch-bearing temporary views and lease counters |
| `NativeThread` / `GuestThread` | libnx thread + guest stack/PCR/TLS | Stage 5: full register initialization and explicit states |
| `Scope` | Actual generated calls and checked scalar access | Stage 6: expanded original fixtures and exact site audit |
| `XexImage` / `ImportFrame` | Strict original flat image; real compare leaf | Stage 7: reassess additional exact SDK contracts |
| NRO / deployment | Separate app, pinned container, verified copy | Stage 8: one integrated smoke application |

Software permissions govern checked accesses only. No Horizon VM isolation,
fixed guest mappings, RWX storage or global protection bypass is provided.
Read-only image, stack and heap are ownership capabilities over the same bounded
arena. Cross-page access within one allocation is supported; cross-allocation
access is refused. Address quarantine is bounded and not a reusable general heap.

Temporary views revalidate backing owner, allocation identity, epoch, bounds and
current permissions on every operation. Protection changes take immediate effect;
decommit/recommit invalidates old views. Views do not themselves pin memory;
leases prevent decommit/release. Full write validation precedes any copy.

HOST VERIFIED: 50/0 new assertions in each of Debug, Release and ASan+UBSan;
original Stage 2 108/0 and Stage 3 118/0 in each profile. Five negative compile
fixtures refused. No unexpected faults, timeouts or sanitizer findings.
BUILD VERIFIED: two clean matching AArch64 ELF/NRO builds, no strong undefined
symbols, dynamic dependencies or WX segments. HARDWARE UNKNOWN.

The reused production fixture has 6 functions / 65 instructions and exactly 20
checked scalar memory sites (6 load32, 8 store32, 3 load64, 3 store64). Actual SDK
compare returns 3; generated wrapper returns r3=17. This establishes the declared
scalar scope only; vectors, atomics and arbitrary PPC remain unsupported.

Reproduce outside the checkout using `python3 tests/horizon-expansion/run.py
--stage 4 --toolchain "$PREPARED_TOOLCHAIN" --sdk-source "$ISOLATED_SDK"
--workdir "$PRIVATE_OUTPUT" --switch-build --icon "$ORIGINAL_ICON"`.
All child commands have independent 30-second monotonic process-group watchdogs.
Private diagnostics retain an initial libnx naming conflict and a corrected
Makefile source reference; neither was a runtime fault.

Xbox Phase 22 remains BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN. Game execution NO-GO.
