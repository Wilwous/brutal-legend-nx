# Horizon Runtime II: selected SDK query service

Stage 10 is HOST/BUILD VERIFIED in the declared original-synthetic subset;
general Xbox service compatibility is PARTIAL and hardware UNKNOWN.

Exactly three imports are admitted: xboxkrnl.exe 0x11b RtlCompareMemoryUlong,
0x12e RtlInitializeCriticalSection, and 0xee NtQueryVirtualMemory. The first two
contracts are retained. The query signature is the pinned SDK's actual three-arg
`u32(u32, ppc_ptr_t<X_MEMORY_BASIC_INFORMATION>, u32)`, not a guessed desktop NT
signature. The actual SDK query body, ToXdkProtectFlags, types/constants and export
table are fingerprinted in `tests/horizon-runtime-ii/query-lock.json` and extracted
only into private build output. Their bodies are not replaced or edited.

## Real state and narrow admission

A dedicated original Horizon backend supplies exactly LookupHeap and
QueryRegionInfo to the real SDK leaf. These are not fake success APIs or a full
SDK BaseHeap. Values originate in Memory/Space under their existing lock order:
logical base/allocation base/size, uniform region extent, commit state, first
committed allocation permission and current stored permission. Permission changes
preserve allocation protection; decommit retains stored protection while checked
access refuses because commit is false, matching the reviewed SDK decommit path.
No host virtual-memory values are substituted.

Only selector **0**, page-aligned addresses in retained, uniform, whole-page,
non-executable private regions are supported. Selected heap, stack and image data
are representable; arbitrary free space, sub-page PCR/TLS extents, executing code,
write-only query permissions, cache flags and mixed per-page attributes remain
denied/unverified. Selectors 1/2 remain denied despite the SDK accepting them via
the same body: their intended contracts are not independently established.
UINT32_MAX is separately admitted as a negative test; the real SDK returns
`0xc000000d`, with zero output publication. No Xbox error code is invented.

The SDK's 28-byte structure contains seven big-endian u32 fields. OUT preflight
checks ownership, nullability, alignment, bounds, commitment and write permission.
The input range and output range remain pinned; only a local typed scratch object
reaches the SDK. Successful output is checked and copied atomically before the
status is published to r3. Failure leaves guest output untouched. Write-only OUT
requires no read. Cross-page output inside one retained allocation is tested.
Unrepresentable or unowned cases fail closed at the host boundary.

The metadata snapshot is linearized under Space/Memory locks, not guaranteed to
remain current after return. Pinning prevents release/decommit, not later protect
changes. A controlled test changes OUT to read-only after admission: the real SDK
returns success but publication refuses, the callback records failure, output
remains unchanged, and no guest continuation runs. Entered/returned/committed
counters distinguish these events. A second worker closes image admission and
requests stop during an accepted query; that query completes, the next generated
hook stops, native join completes, then resources reclaim. New calls after close
are refused. This does not implement guest exception unwinding.

## Image and test evidence

Production original XEX: **17 functions / 158 instructions / 37 scalar memory
sites / 76 hooks / four backedges**. The generated root calls the two prior SDK
leaves and query, returning 24. A separate generated query caller covers pointer
and status cases. Exact image, generated symbols/signatures, TLS metadata,
section ownership, leases, rollback and unload remain mandatory. The fixed
.pdata window can contain up to 32 records; four 64-KiB resident sections and
whole-image/code identity rules remain unchanged. No additional residency is
inferred from PE declarations or proprietary inputs.

HOST VERIFIED: **50 new checks / cumulative 171/0** in each Debug, Release,
ASan+UBSan profile; seven controlled compile refusals. Public portable metadata
contracts pass **15/0 per profile**, original encoding/policy units **11/0**.
Two clean AArch64 builds match, with no strong unresolved symbols, dynamic
libraries or WX. Zero final unexpected faults/timeouts/sanitizer findings.
Initial macro/indentation build failures and the unaligned original fixture
failure remain private. The fixture now queries a real page base; no guard was
relaxed. Hardware remains UNKNOWN.

Reproduce `tests/horizon-runtime-ii/run.py --stage 10 --toolchain "$TOOLCHAIN"
--sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_OUTPUT" --switch-build
--icon "$ORIGINAL_ICON"`. Public tests use `test_fixture.py` and `query_core.py
--workdir "$PRIVATE_CORE_OUTPUT"`. All generated/build/log output stays private.

Xbox Phase 22 BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN; original game execution NO-GO.
