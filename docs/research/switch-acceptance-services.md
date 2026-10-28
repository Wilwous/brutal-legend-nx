# Hardware acceptance batch — Stage 13 real SDK contracts

2026-10-28. HOST VERIFIED / BUILD VERIFIED in the declared original-synthetic
scope; HARDWARE UNKNOWN. Xbox Phase22/G4/G8 BLOCKED, G5 UNKNOWN; game NO-GO.
The three earlier imports and selector-zero query admission remain unchanged.

## Fourth real import: RtlFillMemoryUlong

Exact xboxkrnl.exe ordinal **0x126**, pinned SDK signature
`void(mapped_void,u32,u32)`. The unchanged executable leaf is extracted into
private build output, with full-file/body hashes in `fill-lock.json` and exact
export-table validation. This is neither a handwritten replacement nor a fake
Xbox success response. The existing SDK and nineteen-patch locks are unchanged.

The OUT contract requires a non-null four-byte-aligned address and a nonempty
multiple-of-four length from 4 through 64 bytes. Alignment/whole-word intent is
independently described by [Microsoft's RtlFillMemoryUlong contract](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-rtlfillmemoryulong);
its desktop SIZE_T/IRQL conventions are not substituted for the pinned Xbox SDK
signature. Four literal patterns, exact big-endian bytes and a 64-byte cross-page
span are independently checked. Length zero/nonmultiples and larger spans remain
unadmitted rather than assuming their platform behavior.

A retained checked guest OUT view and allocation/module leases protect the call.
Only aligned local typed scratch reaches the actual SDK. No guest-memory read is
required, so write-only OUT is supported. A successful leaf return is followed by
checked atomic output publication; then completed count increments. It has no
guest status return and the selected void wrapper leaves r3 unchanged. Invalid
pointers, overflow, RO, out-of-capability spans, wrong ABI, missing generated
symbol, stale module and failed publication refuse observably. There is no raw
host pointer escape. Multi-output/nested-pointer transactions remain unsupported;
the flat query/initializer/fill contracts do not imply them.

Native-worker gates independently hold an accepted fill. Releasing its OUT
allocation and zero-time join both refuse while active. A concurrent protection
change makes publication fail, leaves output unchanged and records callback
failure without guest continuation. A second case closes module admission and
requests stop: the accepted fill publishes, the next production hook observes
stop, the native worker joins, then allocations/module unload reclaim. New fill
admission after close fails. This is cooperative host unwinding, not Xbox guest
exception dispatch.

## Query consistency and image binding

New queries cover actual guest stack metadata, retained writable image data and
a committed region ending exactly at 2^32. An admitted metadata snapshot is
linearized before a controlled permission mutation; the result reflects that
snapshot and subsequent checked writes correctly refuse the new RO state.
Freed mappings and overflowing read spans refuse. Existing reserved/committed,
initial/current permission, cross-page OUT, failed publication and unload tests
remain intact. These are logical software permissions, not proven physical
Horizon VM isolation. Query selectors 1/2 remain denied.

The strict original XEX family accepts one exact additional import pair at its
fixed location. Module name, ordinal/type, thunk, complete image/code identity,
generated symbol/signature and four resident 64-KiB owned sections are still
mandatory. No arbitrary PE extent, title loader, relocation or residency rule is
broadened. The query binding now uses its exact index before the fourth import;
missing/wrong fill/query symbols fail before image allocation.

Exactly four imports: 0x11b compare, 0x12e initializer-only, 0xee basic query,
0x126 bounded fill. Enter/Leave critical section, scheduler/TLS exports requiring
full SDK ThreadState, other query selectors and unknown ordinals stay denied.

## Checkpoint evidence

**380/0 smoke, 499/0 stress** in each Debug O0/Release O2/ASan+UBSan O1 profile;
trace-off results agree. Four imports, smoke SDK entered52/completed49, generated
entries215; zero final allocations/leases/dropped events. Three intentional OUT
publication refusals explain the entered/completed difference. Stress counts are
entered136/completed133 and entries501. Diagnostic event totals sum sequential
independently bounded buffers, so totals may exceed one buffer's capacity.
Seven expected compile denials; fourteen public encoding/policy units pass.

Original XEX regression: 118/0 per profile. RuntimeII regression: 268/0 smoke,
387/0 stress per profile. Final checkpoint has zero unexpected faults, timeouts
or sanitizer findings. One earlier all-in-one cross-build timed out at 30 seconds;
its process group/container were terminated and diagnostics retained. Builds now
supervise each object and link independently at the same 30-second limit. Both
clean AArch64 ELF/NRO builds match, no strong undefined symbols, WX or dynamic
libraries. Earlier failed evidence is retained, not counted as a final PASS.

Production fixture: **21 functions / 203 instructions / 49 checked memory sites /
97 hooks / four backedges / 17 calls / 20 returns / 11 branch edges**. Full-vector
memory is included in that count. Output still matches production-default
instruction C++ and repeats without editing generated code.

```sh
python3 tests/horizon-acceptance/test_fixture.py
python3 tests/horizon-acceptance/run.py --stage 13 --toolchain "$TOOLCHAIN" \
  --sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_STAGE13" \
  --switch-build --icon "$ORIGINAL_ICON"
```

Use new private output directories and the pinned toolchain from Stage12. No SD
scan/deployment or intermediate hardware test occurs in this stage. Stage14
lifecycle/diagnostics and Stage15 packaging remain within synthetic scope only.
