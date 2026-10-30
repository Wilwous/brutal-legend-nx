# N1 — validated native image plans

2026-10-30. Original synthetic scope only. Historical G4/G8 BLOCKED, G5 UNKNOWN;
original title execution NO-GO. This is separate from the strict XEX loader.

`runtime/horizon/native_image_plan.h` defines an explicit version1 value schema:
source/plan/generated-code/SDK/runtime identities; synthetic provenance; module
and lifetime identity; guest limits/base/entry; ordered region owner/address/size,
source offsets/file lengths/authored zero lengths/alignment/rights/init policy;
reservation/commit requirements; fixed-address relocation refusal; TLS metadata;
closed ordinal/signature pairs for the four existing import contracts.

Plan identity is SHA256 of documented canonical bytes: each scalar is an unsigned
big-endian64-bit value, in declaration order (schema, provenance, module, lifetime,
base, limit, entry, TLS address/file/total/slots, relocations), followed by the four
32-byte digests (source/code/SDK/runtime), region count, each region's owner and
fields in declaration order, import count and ordinal/signature pairs. No struct
padding, host pointers or host endian encoding. Region ordering is semantic and
must be ascending/nonoverlapping. A trusted expected identity is required: an
untrusted plan's self-calculated hash is not independent authority. Code/SDK/
runtime digests are required but their actual binding belongs to N2.

Validation completes before allocation: schema/origin/identity, bounded ranges,
owner uniqueness, supported alignments, file+zero coverage, checked source extent,
RO/RW/RX-only guest rights, reserve+commit policy, file-backed executable entry,
file-backed non-executable TLS, fixed32-byte static/64-slot TLS subset, allowed
import signatures. Relocations, arbitrary origin, permission unions and unknown
initialization refuse. This version uses page-sized whole regions and denies
unmapped holes; partial-page layouts remain unsupported. Limits are representable
32-bit exclusive ends; the final4-GiB boundary itself is unsupported.

Materialization registers exclusive Space/module ownership; reserves each region,
commits trusted construction backing, writes exact file bytes plus explicit zeros,
then applies guest permissions. Distinct state fields record declared/reserved/
backed/initialized/explicit-zero/committed/accessible/executable/released. The private
arena already has host storage; reserve assigns a logical backing loan while
guest commitment remains separate. Neither backing nor commitment substitutes
for initialized/admitted. No public admission until all regions complete. RX is
software code admission with readable data backing, never native host execution
permission. This is not Horizon VM page isolation.

Image owns a copied immutable Plan. Const getters expose only read-only admission
metadata. Space capabilities retain actual owner/generation identity. Duplicate
module owners, foreign reservations, active leases and stale views refuse.
Allocation, initialization and injected stage failures release only this image's
handles and unregister its module. Unload checks every lease before releasing
any handle. Space must outlive the image and all capabilities. The existing arena
is a bounded bump allocator: release clears live ownership but does not reset its
allocation cursor. Repeated transactions therefore use a finite budget or a fresh
Space; no unbounded allocator reuse is claimed.

## Independent regression and checkpoint

Run in a fresh external directory:

```sh
python3 tests/native-image-plan/run.py --workdir "$NEW_N1_OUTPUT"
```

The Python oracle independently creates8192 source bytes with `(17*i+3)%256`,
serializes canonical input with `struct.pack`, and uses hashlib SHA256. C++ must
match both source and semantic plan hashes. Three regions represent file-backed
RX, file-backed RO/TLS and8192 explicitly authored zero bytes RW. The guest address
range contains an inaccessible hole. Expected bytes derive directly from the
producer formula, not the mapper's initialization implementation.

Debug/O0, Release/O2 and ASan+UBSan exercise schema, all identity classes, source
mutation, overflow/alignment/overlap, duplicate owner/module, initialization,
unsupported permissions/relocation/imports/TLS/entry, nine partial construction
points, allocator exhaustion, adjacent sentinel preservation, endian bytes, RO
refusal, unknown/cross-end pointers, active leases, stale capabilities, repeated
initialization, final states and complete live-resource balance. Semantic counts
and16384 byte comparisons are reported separately in each results.json row.
Every compile/run is externally supervised with a30-second group watchdog.

The initial harness used a64-KiB Space across repeated rollback cases and exhausted
its bump cursor. Its failure remains private; the corrected harness budgets1 MiB
without changing allocation or failure assertions. This is a test-capacity fix,
not a relaxed allocation contract. Final results are recorded in the N1 checkpoint
receipt. No generated PPC, native guest thread or Switch hardware is exercised in
N1. No new imported ABI, global SDK or original title data is modified.
