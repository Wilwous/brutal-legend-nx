# Switch roadmap Stage 22 — optional Xbox observation protocol

**DESIGN ONLY / NOT EXECUTED / hardware access unavailable.** This document is
Switch research Stage22, not a restart or completion of preserved Xbox Phase22.
It proposes no firmware flashing, hardware modification, installation, console
connection or proprietary-title execution. Ordinary retail/user-space access is
not assumed to expose the needed fields. No request to buy or modify a device.

## Why host evidence is insufficient

The [source audit](xex-loader-reference-audit.md) and
[finite synthetic observations](xex-synthetic-loader-differential.md) establish
path differences and stricter project checks. They do not establish the original
image tail's actual allocation owner, committed backing, contents, protections,
neighbor integrity or lifetime. Xenia has no complete independent Xbox unwind
oracle in the reviewed RtlRaiseException implementation. A second emulator with
the same ancestry cannot replace missing console evidence.

## Capability gate before any experiment

A qualified lab must first document an already available, owner-authorized Xbox
360 debug environment and supported debugger/interface. Record model, kernel,
hypervisor, debug monitor and applied patch identities, observer/tool versions,
field definitions and known limitations. This task does not assume such access.
Source names such as walkmem, module sections or a context structure are not
proof that a live interface returns reservation owners or complete PPC registers.

Require a supported hold boundary **after complete image mapping/import fixups/
initialization/protections, before any guest instruction of the candidate**,
including TLS callbacks and initializers. A module notification, entry breakpoint,
manual pause or sentinel alone does not prove this ordering. A candidate's own
program cannot report its state before its first instruction: self-reporting
would already have crossed the observation boundary.

The observation channel must distinguish page reservation, commit, permissions,
allocation base/extent, owner or equivalent loader allocation provenance and
load-instance generation. It must also observe normal rejection and cleanup
without resuming candidate entry. A protection-only query or declared module
span is insufficient. If owner/PTE provenance or pre-entry ordering is unavailable,
record the limitation and keep experiment **NO-GO**. Do not probe by touching an
unknown address, force an exception, or substitute default values on API failure.

## Minimum safe experiment, conditional on that gate

1. Hash an independently authored control XEX and a single-variable overhang
   fixture from the public original corpus. Keep input and manifest identities.
   Use no Brütal Legend content, decrypted image, title update or console secrets.
2. Establish baseline loader-owned ranges and a known optional adjacent control
   allocation through supported interfaces only. Record validity and owner/lifetime
   identity. If the debugger cannot establish these, stop without loading.
3. Request a documented deferred/pre-entry load hold. Retain raw acknowledgements
   and monotonic observer timestamps. Prove that no TLS/initializer/entry ran;
   the entry marker supplements ordering evidence but cannot replace it.
4. For the known candidate span, collect supported reservation/commit/PTE or
   allocator events, owner identity, protection changes and known-byte validity.
   Read only ranges already proven safe by those interfaces, with explicit
   per-range errors. No private memory dump, MMIO scan or unreadable-byte-to-zero
   conversion. Do not infer zero-fill from VirtualSize or rounded page size.
5. Compare control/overhang accept-or-refuse, exact allocations, contents and
   permissions, neighbor preservation and whether the loader granted extra
   backing. A returned load success is not an owner trace or permission contract.
6. Request documented unload/cancel while entry stays held. Observe reference
   lifetime, removal or retention of reservation/commit/protection/owner records
   and unchanged neighbor allocation. Exercise only a normally rejected metadata
   fixture for failure rollback; do not inject raw faults or kernel failures.
7. Record raw status/errors and whether mapping was partial before cleanup.
   Stop if the target resumes unexpectedly, loses hold, reports ambiguous state
   or cannot cancel without candidate execution. Target-safe abort must be
   independently qualified; terminating a host collector does not stop a console.

No candidate entry is needed for residency observation. The marker/trap sequence
is never authorized to run by this protocol. Any later synthetic CPU exception
experiment requires separate authorization and a proven safe termination plan.
No patch or safety bypass is proposed to make a debugger expose missing fields.

## Observable versus unavailable exception facts

| Fact | Supported observation, if available | Limitation |
| --- | --- | --- |
| Normal malformed-header/load rejection | Raw loader status + absence of entry + ownership cleanup | Not guest exception dispatch |
| Mapping/protection failure | Explicit API status and recorded rollback | No inference that a guest CPU fault was raised |
| Pre-entry host/debugger event | Matched interface's documented event/context fields | Completeness, widths and ordering must be demonstrated |
| Guest CPU exception, kernel dispatch, title handler/unwind | Exact independent documentation/implementation or separately qualified original CPU experiment | Not measured by this load-only protocol or ordinary user-space synthetic self-report |

The user's Switch can exercise Horizon behavior; it cannot supply Xbox loader
owner/PTE or original startup/exception provenance. No further Switch test is
requested for this research. Without a valid supported pre-entry interface, the
desired original Xbox state is **not observable here**, and G4/G8 stay BLOCKED.
