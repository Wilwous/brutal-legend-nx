# Switch roadmap Stage 22 — residency and exception proof assessment

**G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED. Original game execution NO-GO.**
This is the assessment for Switch roadmap Stages20–22, not preserved Xbox
Phase22. That branch, baseline, tags and blocking decision remain unchanged.
No original game XEX was loaded or executed; only existing private metadata/
receipts were read and original source SHA256 preserved. No new Switch NRO.

## Work completed and reference limits

- Stage20: source audit complete; pinned Xenia loader/memory/POSIX backend,
  RXDK producer, relevant exception/debug source and external hardware notes.
- Stage21: finite original corpus/strict SDK and source-slice observations
  **HOST VERIFIED**; full independent loader comparison remains **PARTIAL**.
  Complete Xenia heap/CPU/module lifetime, second independent consumer, encrypted/
  NORMAL-LZX fixtures and real Xbox are unobserved, not simulated as successes.
- Stage22: proof assessment and optional acquisition protocol complete as a
  design; external acquisition/hardware experiment remains **BLOCKED**.

[Reference audit](xex-loader-reference-audit.md),
[differential results](xex-synthetic-loader-differential.md),
[optional hardware protocol](xbox-hardware-observation-protocol.md).

## G4 property ledger

Classifications distinguish implementation source from observation. None of
these rows establishes the original title's missing tail from synthetic evidence.

| Required property | New evidence class / scope | Original-tail classification / missing proof |
| --- | --- | --- |
| Owner | PROVEN BY SOURCE for project opaque owned ranges and leased copy; OBSERVED IN SYNTHETIC REFERENCE only allocation requests, adapter has no real Xenia owner token | UNKNOWN. Need independent original-image-class allocation owner/provenance, no adjacent-owner substitution |
| Reservation | PROVEN BY SOURCE: NONE P, BASIC D, NORMAL D requests and guest heap rounding; observed exact project reservation queries | UNKNOWN outside the original descriptor extent. V or stored S alone is insufficient |
| Commitment | Observed actual SDK synthetic committed pages, decommit refusal and rollback; Xenia metadata versus host accessibility distinction PROVEN BY SOURCE | UNKNOWN for disputed title tail; require image-capable guest/PTE observation |
| Contents / zero-fill | OBSERVED IN SYNTHETIC REFERENCE: BASIC memset inside D; NONE raw-size declarations do not erase payload | UNKNOWN outside title backing; all-zero guessed allocation forbidden |
| Permissions | PROVEN BY SOURCE: descriptor-based reference requests ignore returned failure; actual SDK synthetic R/R/RW and PE refusal observed | UNKNOWN for disputed span; physical accessibility/protection-only query is not owner authority |
| Lifetime | Actual project synthetic leases block unload and release restores balance; reference adapter cleanup is explicitly not native Xenia unload | UNKNOWN for title tail; require load generation, active references, owner lifetime |
| Rollback / neighbors | Actual SDK collision, foreign neighbor, partial initialization and failed load cleanup regressions pass | UNKNOWN for an extra original range; must prove neighbor preservation and supported unload/cancel |

**OBSERVED ON REAL HARDWARE:** no Xbox observation from this task. Stage15's
user-attested Switch synthetic result is genuine hardware evidence for that
finite Horizon workload only. The xsdk author's reported image-query refusal
does not supply this title's owner or pre-entry state.

H1 metadata/discarded tail, H2 separately established Xbox backing and H3
representation mismatch remain **HYPOTHESIS/UNKNOWN** for the original tail.
Owned synthetic V>raw is legitimate when allocation covers it; unowned V cannot
supply its own authority. Security-size mismatch and page rounding are not a new
original-title transformation. Preserved original security/descriptor/decoded
extent consistency remains unchanged; no independent Xbox allocation rule was
obtained that justifies changing the loader.

## G8 minimum boundary obligations

The frozen earliest region is still the preserved19 normal instructions with
8 stores, one save-helper/matched return and zero ordinary imports. It does not
establish a required RtlRaiseException import or license a universal dispatcher.
The smallest actual proof obligations are:

1. Reject a failed/unknown map before admitting **any** guest PC. This can avoid
   the need for guest exception dispatch for that rejection. Actual synthetic
   load refusal + rollback proves this selected host path, not original execution.
2. At proposed entry, prove exact PC/CFG, live stack/PCR/TLS and store ownership/
   permission; preserve observable fault-time state and deterministic refusal
   before an unsupported transition. G4 and G5 currently prevent that proof.
3. Establish behavior for implicit store faults/unsupported control flow, error
   propagation and host cleanup without resuming guest code. No unproven catch,
   handler lookup, register rollback or fake successful return is acceptable.
4. Distinguish a native signal/tool failure from a guest exception. Child watchdog
   containment is not Xbox kernel dispatch, CONTEXT preservation or guest unwind.

No explicit exceptional opcode in a static normal slice proves that implicit
memory faults or asynchronous behavior cannot occur. Required live exceptional
transitions and private hook bodies remain unestablished. The Xenia source
RtlRaiseException is a debugger/stub path with unwinding TODO, not the missing
oracle. The project DiagnosticContext test is host-supplied and incomplete;
C++ cleanup cannot close G8. Mapping success with an invalid entry in the corpus
confirms why a separate strict binder is mandatory; that entry was never invoked.

G8 remains BLOCKED. G5 startup provenance remains UNKNOWN. G1/G10/G12 remain
PASS; Recovery III offline G2/G3/G6/G7/G9/G11 remain PARTIAL, not live admission.
No mandatory gate is promoted because this source inventory or test count grew.

## Regression evidence and retained failures

| Scope | Debug | Release | ASan+UBSan |
| --- | --- | --- | --- |
| New14-case differential contracts |199/0, original spans |199/0, original spans |199/0, explicit offset-only observation variant |
| Unmodified Xenia source-slice PE path | native observations only | native observations only |13 UBSan failed processes;1 early bad-PE return; **BLOCKED** |
| Existing23-case SDK residency |337/0 |337/0 |337/0 |
| Existing image ownership/TLS/import/rollback/lifetime,20 groups |504/0 |504/0 |504/0 |
| Existing host refusal/context/cleanup |30/0 |30/0 |30/0 |

SDK-free corpus layout/hash tests13 PASS. Fixture generation is byte-identical;
original Debug/Release and offset-only Sanitized observations match exactly.
The original UBSan null-member defect is retained unsuppressed. The explicit
standard-offset patch changes one macro in private extracted code only, and
mechanical patch application reproduces that variant. No reference acceptance
policy, original upstream source, project runtime or SDK lock is altered.

A stale old rollback fixture expected descriptor/payload mismatch to reach
LoadContinue. Existing strict SDK rejects it at Load. One test expectation was
corrected; cleanup, empty-metadata and reuse assertions stay required. Initial
failed adapter builds, invalid CLI-directory setup, patch-format attempt and
19-pass/1-fail old rollback run remain privately preserved. They are not passes.
The existing locked SDK ARM64 FPSCR width warning is retained in compiler logs.
No unbounded raw fault experiment was attempted; all dangerous children use30s
external process-group watchdogs. The unmodified-reference UBSan failures are13 sanitizer-triggered SIGABRT
terminations (watchdog exit_code=-6), not ordinary exit1 and not zero sanitizer
findings for the whole task. Final validated/offset-variant runs have no signals.

## Ranked evidence acquisition paths

| Rank / blocker | Cheapest useful path | What can change / what remains insufficient |
| --- | --- | --- |
| 1 G4 owner/residency | Qualify existing trusted Xbox loader documentation/implementation or a lab's supported image-owner/PTE observation channel | Could establish a general extra-range allocation rule. Source-slice emulator agreement already cannot; no device purchase/modification is requested |
| 2 G8 required transitions | Exact Xbox exception/context documentation or independent implementation covering the frozen boundary; later separately qualified original-fixture observation | Stub export names/C++ catches/debugger pause are insufficient; avoid full speculative dispatcher |
| 3 G4 protocol | If rank1 lacks authoritative rule, conditional original control/overhang post-mapping/pre-entry protocol | Must prove owner, commit/content/permissions/lifetime/neighbor/rollback; user-space self-report cannot precede its own entry |
| 4 G5 startup | After owner/exception obligations, independent provenance for consumed initial values | Synthetic initialization does not establish title initial state |
| Supporting only | Further emulator tracing, source inspection or another producer | Cheap and useful for contradictions; not promised to satisfy title-specific proof |

**Single most valuable next action:** qualify an independent Xbox-capable
**pre-entry owner/allocation observation channel** or an equivalently authoritative
loader contract before requesting any experiment. If that supported interface
cannot expose owner/commit/lifetime and ordering, explicitly keep acquisition
BLOCKED. Ordinary retail hardware and the user's Switch do not provide this
proof. Do not compensate with guessed allocations, permissions or exception code.
No game experiment or new development stage is authorized by this review.
