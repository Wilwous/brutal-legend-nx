# Original instrumentation preflight and loader diagnostics

```sh
python3 tests/entry-audit/test_audit.py
python3 tests/entry-audit/run.py --sdk "$SDK" --input-suite "$ENTRY_RESULTS" --workdir "$AUDIT_RESULTS"
# Add --sanitize only for matching Sanitized SDK and Phase21 input-suite results.
```

The input suite must be a complete current-source original Phase21 production
codegen/runtime result. The auditor independently reconstructs the original
fixture, reconciles real emitted source, checked PCH macros, immutable hook
indices/PCs and canonical policy. Seventeen mutated candidates must refuse without
being executed. Portable units separately check original data/snapshots; they
are not production codegen or a live ownership proof.

The policy is compiled to a data-only object and LLVM IR. Decoded immutable IR
bytes must match the authority. No policy object is executed or dynamically
loaded; this is not disassembly of every compiled hook or an arbitrary native
sandbox. The existing Phase21 results provide actual compiled hook/runtime
observations. Runtime owner/register/exception negatives remain mandatory there.

Two original native probes use the actual SDK: loader-only metadata and a fresh
XThread context observer before guest argument/APC dispatch. Neither links or
invokes a generated guest function. The observer verifies SDK defaults and
reclaims its stack/TLS/PCR/image; it supplies no title startup provenance.

Every compiler/runtime child has an external 30-second watchdog and native join
has a 15-second bound. Output directories must be new and private. Complete
workflow supervision uses a 300-second ceiling for this suite. No private input
or artifact is a public test fixture. A title loader rejection is a real failure
(exit 4), even when the diagnostic probe itself cleaned up correctly.

The strict zero-default context challenge is intentionally retained as a failing
oracle when the SDK does not establish all 64-bit register values. A separate
placement-initialization diagnostic calls the actual SDK ThreadState constructor
on a known prefilled original host buffer and compares a direct aggregate control.
It records byte coverage, not PASS or title
provenance. Failed native children remain failed_groups and nonzero suite exits;
independent loader checks may still finish. No assertion is removed or weakened.

## In-place recovery

The separate [recovery lock](../../tools/sdk/entry-audit-lock.json) retains the
nineteen-patch baseline and adds complete scalar ThreadState initialization and
PE section preflight. The original strict observer remains unchanged. Its result
must be recorded both before and after rebuilding the isolated SDK.

```sh
python3 tests/entry-audit/recovery.py --sdk "$SDK" --workdir "$RECOVERY_RESULTS"
# Add --sanitize for Sanitized. Each child is bounded by 30 seconds.
```

Eighteen loader cases use actual UserModule/XexModule operations on the original
unencrypted fixture, without guest function linkage or dispatch. Supported
sub-page section boundaries, zero-filled data, owned holes and an overstated PE
extent are distinguished from unbacked extensions, overlap, wrapping arithmetic,
unsupported unreadable section declarations, short/extra uncompressed payloads,
reservation collision and failed
partial loads. Accepted cases verify image leases, checked byte permissions,
outside-section refusal and final image/virtual allocation balance. An owned hole
may remain mapped at the XEX page level; checked image copies must reject it.
This is not complete guest memory isolation or support for arbitrary sparse XEX.

The field diagnostic checks 32 GPRs, 32 FPR bit patterns, CTR/LR, CR/XER fields,
128 vectors, SDK MSR policy, FPSCR storage and dispatch bookkeeping under three
different host-storage prefills. It checks semantic fields, not padding. Seven
incomplete-field candidates per prefill must refuse its original default-state
oracle. Those are static field checks, not independent execution-admission APIs.
FPSCR storage before host initialization is separate from the existing per-thread
host FP policy. SDK defaults do not supply real title startup provenance.
