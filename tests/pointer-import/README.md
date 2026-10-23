# Original checked pointer import dispatch

These original host-driven PPCContext fixtures load a synthetic XEX2/PE image
through the actual SDK, initialize real XThread TLS from its header, resolve its
imports and dispatch through the Phase 13 admission boundary into checked copy
adapters and actual SDK implementations. No proprietary input, guest entry point
or replacement success implementation is executed. Generated original PPC execution
is separately rerun by the existing Phase 11 suite.

The explicit PointerBinding overload admits three contracts: RtlCompareMemoryUlong
(IN BE words), RtlInitializeCriticalSection (INOUT 28-byte partial initialization),
and NtQueryVirtualMemory (OUT 28-byte information, same-owner query target).
An opaque live allocation token, matching thread/PCR and exact signature are
required. Scalar-only admission remains unchanged; no raw-pointer fallback exists.
The current matrix has six scalar, three pointer-scoped and 24 denied imports.
Full Xbox conformance classifications remain 27 PARTIAL / six BLOCKED.

Use the unchanged eighteen-patch SDK from the Phase 13 lock, with its matching
headers/runtime profile and private interposable test linkage:

```sh
python3 tests/pointer-import/run.py --sdk "$SDK_DEBUG" --workdir "$PHASE14/pointer-Debug"
python3 tests/pointer-import/run.py --sdk "$SDK_RELEASE" --workdir "$PHASE14/pointer-Release"
python3 tests/pointer-import/run.py --sdk "$SDK_SANITIZED" --workdir "$PHASE14/pointer-Sanitized" --sanitize
```

All output directories must be new, private and outside the checkout. Every
build/execution child has an external 30-second process-group watchdog. Only two
allocation-failure children receive DYLD interposition; all ordinary children
scrub inherited overrides. Actual runtime hashes/profile/revision are verified.

Thirteen groups cover exact policy, IN/OUT/INOUT and direct/indirect calls,
cross-page buffers, overflow/null/alignment/extent failures, RO/uncommitted/freed
storage, callback failures, output preservation, scratch allocation failures,
controlled protection change and raw release before copy-out, active-call module
unload, nonblocking allocation release refusal, four XThreads, repeated calls and
eight module reloads. A ninth-argument uint64 callback traverses the actual SDK
stack bridge into pointer dispatch and checks wide return/context restoration.
That original callback is a separate ABI fixture, not a new title import.

The adapter serializes facade operations during the call and uses private,
owner-tagged SDK scratch. Snapshot reads and a final all-or-nothing checked copy
prevent guest pointer escape and unsafe access after external release/protection
changes. IN is snapshot semantics. Arbitrary unmediated SDK/host writes are not
excluded; Reset/Restore/MMIO changes require quiescence. This is no general C++
sandbox or full guest-memory mediation. [Detailed contracts, evidence and limits](../../docs/research/pointer-import-synthetic-dispatch.md).
