# Native module metadata and variable binding — T17–T21

The generic host runtime now provides a minimal sealed module metadata graph and
one selected self-module variable-import contract. This is **VERIFIED for the
original synthetic fixtures**, not a complete Xbox module object or admitted
Brütal Legend provider. No proprietary PPC is generated, compiled or executed.

## Representation and admission

`native_module::Metadata` reserves only the four logical bytes at `M+0x58`.
They contain the BE32 guest address `H` of the retained T16 header buffer.
`M` is a module coordinate, not a readable contiguous native structure. Adjacent
fields remain inaccessible; there is no inferred full loader object layout.

The selected `XexExecutableModuleHandle` representation is an exported four-byte
cell `P`, storing `M`. A separately owned synthetic import slot `S` stores `P`:

```text
S --BE32--> P --BE32--> M
                      M+0x58 --BE32--> H
```

The phrase “variable slot → module object” refers to `P`, not the additional
consumer import slot `S`. T14's pinned SDK and private consumer evidence establish
this depth; synthetic addresses never reuse historical snapshot pointer values.
The provider registry supports only the selected library/ordinal and variable
kind. Other providers and function-kind requests refuse. This is a self-module
policy; original PE import publication, general DLL resolution and title provider
admission remain unsupported.

## Ownership and publication

`Object` copies original native data into exact logical slices of the existing
checked `Space`, commits, seals RO and pins each allocation before publication.
Module-instance and object-generation tokens distinguish reused guest addresses.
Header, metadata, provider and import-slot readers form an acyclic retained graph.
No guest backing pointer escapes. Every read revalidates the current permission
and exact allocation token. Module/space mismatches, stale tokens, overlap and
partial initialization refuse without publishing an object.

A mutex protects object publication/closing and reader counts. Registry/provider
locks serialize publish, binding, withdrawal and replacement. Withdrawal denies
new acquisitions; already admitted readers retain the full graph to completion.
Multi-slice retirement first checks every lease under one Space lock, so a
competing pin cannot cause partial sibling reclamation. Constructor failures
release unpublished allocations. Last-reader retirement releases dependencies
in order and balances module, allocation and registry ownership.

The native `Space` and `ModuleLife` must outlive all graph readers. Low-level raw
Space management pins require retaining the object wrapper until those pins drain
and retrying close. Destroying the wrapper while such a pin prevents retirement
is a contract violation and fails closed. This API is trusted native ownership
infrastructure, not a sandbox for arbitrary host code. Tests cover a competing pin
on one sibling after graph readers drain, and deferred reads on a worker after
parent wrapper destruction. These are deterministic interleavings, not exhaustive
concurrency proof or a ThreadSanitizer result.

## Selected lookup contract

`lookup` admits only the selected inline-word selector on the retained raw XEX
header representation, checks identity, magic, declared extent and catalog bounds,
and distinguishes missing key (address zero) from invalid header/module/selector
(refusal). A present inline word returns its guest address even when its value is
zero; `Field` keeps the graph alive while borrowed. Duplicate selected entries
refuse under the stricter native policy. This is consistent with the selected
pinned SDK contracts tested in T15; it is **not** a newly invoked Xbox SDK export
or proof of historical console behavior. See
[the SDK case distinctions](missing-xex-header-field.md).

## Reproduction and evidence

From a checkout, use fresh external directories:

```sh
python3 tests/native-module/run.py --workdir "$OUT/module"
python3 tests/variable-imports/run.py --workdir "$OUT/variables"
python3 tests/header-buffer/run.py --workdir "$OUT/header"
```

Each driver builds Debug (`-O0`), Release (`-O2`) and combined ASan/UBSan
(`-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all`). Every child has
an external monotonic 30-second process-group watchdog. JSON results, commands,
compiler/runtime diagnostics, time and peak RSS stay outside Git.

| Suite, per profile | Positive assertions | Expected refusals | Bulk byte comparisons |
|---|---:|---:|---:|
| Minimal metadata/lifecycle | 97 | 21 | 4 |
| Selected provider/import binding | 117 | 81 | 16 |

All six runs pass with zero outstanding allocations/leases and zero sanitizer
findings, unexpected faults or timeouts. Bulk comparisons are not independent
semantic tests. Four metadata, four provider and four binding failure points
exercise rollback; the latter two run for each of four header variants. Reuse,
stale withdrawal, selected field absence/zero/duplication, permission revocation,
foreign identity and repeated/concurrent teardown contracts are covered.

A private nonexecuting replay also copies the verified original 12,288-byte header,
constructs only the supported metadata word, reads it through the checked guest
layer and rejects three adjacent byte probes in all three profiles. Cleanup is
zero; imports and PPC entries are zero. Detailed source hashes, plans and guest
coordinates stay private. This replay does not admit the title provider.

## Remaining obligations

All 13 original variable imports remain unadmitted for title execution. One has a
bounded generic mechanism; the other 12 have no new implementations. Their exact
private ledger separates cell/scalar/structure/string/indirection shape, current
value semantics, owner, initialization, rights and lifetime obligations. A known
name or finite consumer does not prove those contracts.

Historical G4 **BLOCKED**, G5 **UNKNOWN**, G8 **BLOCKED**; native N-G4 **BLOCKED**,
N-G5 **PARTIAL**, N-G8 **BLOCKED**. Full image permissions/lifetime, startup values,
unknown continuations and guest exceptions remain independent blockers. Original
game execution is **NO-GO**.
