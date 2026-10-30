# Title native image — T0 reconciliation and T1–T3 readiness

2026-10-30 · baseline `d8f37d4ac24edd99d8f20f936d7aa546fafc0f8a`.
**Historical G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED; game execution NO-GO.**
No original instruction was executed; no game-derived C++ was compiled/linked;
no title-specific artifact was made executable or deployed. The strict historical
loader and its 0xC0000001 refusal remain unchanged. This is offline evidence work.

## Hardware evidence kept separate

The user's T1–T3 request explicitly confirms Native Image Probe PASS=42 / FAIL=0,
result=32, SDK=2/2, allocations=0 / leases=0 and normal PLUS return. This is **HARDWARE
VERIFIED by user attestation for the finite original-synthetic workload only**;
no photograph was independently inspected in this task. The preserved NRO hash is
`5be3f0c1da9ba5aff4e2a6564508c09b66ca591744cd6d5d0a545ae7b619bc29`.
Stage 15's earlier hardware evidence and both original packages remain unchanged.
Historical build-time manifests saying UNKNOWN are not rewritten.

## T0 — two valid stages, not interchangeable images

The original XEX hash was reverified as
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
Both compared representations are 14,090,240-byte flat image views with PE metadata:

| View | Identity and intended semantics |
| --- | --- |
| A — pre-binding decoded bytes | SHA256 `9c85bd88281c7742cf90b2b885e0fcc6829402cef0b624b925cb3ac1d5a64dc5`; independently framed/AES-decoded input through the pinned shared LZX backend; equals the preserved later SDK pre-import image. Source-offset mapping is logical base+offset. |
| B — historical tool-mode loader snapshot | SHA256 `482edb9e019bd61e17eb6a7024ed32c4e8237ab2b142459dea2fac6e3b672ec5`; the original producer dumps memory after LoadXexImage and implemented variable-export resolution. Same base+offset mapping, not a different PE raw-section remap. |

**PROVEN EQUIVALENT content:** 14,090,190 byte positions. **PROVEN DIFFERENT:** 50
bytes in 12 differing runs within 13 variable-import words (52 container bytes).
All changed words independently match original-header type-0 variable imports;
no function thunk or frozen entry/helper code differs. Header/section metadata
agrees. Replaying those observed binding words reconstructs B exactly.

The pristine pinned SDK source explains the variable_ptr write mechanism. Exact
historical provider values/lifetimes are not independently transferable into a
new native runtime. Therefore the relation is proven for bytes/transform class,
not full execution semantics or whole-image interchangeability. No unexplained
byte differences remain; native reuse of those 13 resolved values remains UNKNOWN.
Neither representation is declared incorrect. B failing the original pre-binding
content chain is expected after binding and is not evidence of corruption.

A passes all 215 original descriptor content-chain checks. A separately implemented
PE header parser agrees on 14 sections. The AES/framing frontend is independent;
LZX is shared, so this is not two independent LZX decoders or RSA verification.
Neither image contains the disputed extra section tail. Oversized failed decoder
outputs remain excluded diagnostics; their padding is not source authority.
Detailed creation receipts, tool/source identities, mappings and 28 explicit
comparison intervals remain private.

## T1 — complete accounting, partial provenance

Ledger domain is the conservative declared PE image extent, not an asserted
required native allocation. 267 exact intervals: 265 DERIVED, one UNBACKED, one
UNKNOWN. Original-container-relative accounting:

| Classification | Bytes |
| --- | ---: |
| FILE-BACKED directly from encrypted original container | 0 |
| PROVEN DERIVED decoded source content | 14,090,240 |
| Independently justified EXPLICIT ZERO-FILL | 0 |
| UNBACKED declared section tail | 185,980 |
| UNKNOWN outer PE extent-only claim | 1,474,436 |
| Total audit domain | 15,750,656 |

The last two rows total 1,660,416 unresolved bytes but represent different claims:
185,980 belong to a declared section overrun; 1,474,436 follow the last section and
come only from SizeOfImage. Their necessity must be established rather than
assumed. There are 0 overlapping content-authority conflicts; the known 50
cross-stage byte differences are reported separately, not hidden as conflicts.

The decoded 14,090,240-byte region is known source content, **not proven live Xbox/native
reservation or commitment**. Within it, 52 bytes are pre-binding variable tokens
whose future native binding remains a separate initialization obligation.
Zero-valued decoded/BSS contents are retained as DERIVED, not reclassified as
permission to invent zero fill outside the successful decoded extent.

Both section/descriptor bounds and independent header parsing agree on one
uncovered section. PE raw offsets fit elsewhere in the decoded buffer but are
not equivalent to virtual RVAs or compressed-XEX offsets. No remap was applied.
The conventional PE relocation-directory interpretation is incomplete/unrecognized
inside available bytes; no verified relocation entries are asserted. Observed
unsupported type count 0 does not mean the missing/unrecognized table has none.
Preferred guest-base placement is kept distinct from ordinary host allocation;
no guest relocation was applied or speculatively simulated.

## T2 — rejected draft is a valid research result

Private candidate binds original XEX, decoded-source and semantic draft identities,
all source intervals/holes, expected permissions as metadata, intended ownership/
lifetime, TLS references and unresolved relocation/import/startup/code obligations.
**AUDIT DRAFT STRUCTURE / PROVEN INTERVALS VERIFIED; FULL TITLE ADMISSION BLOCKED.**

The actual existing N1 validator rejects the data-only full projection at its
region budget; a separate diagnostic control rejects unsupported title origin.
No check is bypassed. The current synthetic-only origin enum, 64-region/four-import
budget, 4096-byte region requirements, fixed 32-byte synthetic static TLS, missing
module-aware import contracts and absent title code binding prevent a faithful
full lowering. Coalescing/padding cannot be guessed to make validation pass.
A projection with read-only metadata is not a lossless admitted title plan.

The ordinary inert prefix-copy audit populated 14,090,240 verified bytes in 265 source
copies and matched the known decoded identity. Unknown tail bytes were not added,
zero-filled or made executable. Native guest allocations 0; admitted executable
intervals 0. Correct source identity and historical-snapshot mismatch are checked
separately: both projections remain refused. The original synthetic overhang
fixture still validates general ownership/zero/permission/neighbor/rollback rules;
its success is not evidence for the title tail.

## T3 — bounded static readiness

The fresh pre-binding image, historical image and preserved generated-comment
view agree on two frozen ranges / 20 sites. A fresh bounded normal walk covers 19
instructions, one direct helper call/matched return, eight stores (six stack/two
fixed data), zero admitted imports and zero unresolved normal targets **inside
that slice**. It stops before the next unverified initializer. This is not full
startup reachability. Exception metadata intersects the prefix; shared aliases
and exceptional continuations remain excluded.

| Startup property | Status |
| --- | --- |
| Entry and declared stack/TLS metadata | PROVEN metadata only |
| Required aligned owned r1 window and matched helper return | DERIVABLE relative constraints; no literal stack/owner invented |
| Actual full-title r1/r2/r13/PCR, GPR/FPR/LR/CTR/CR/XER state | PARTIAL mechanics / UNKNOWN title context; unused values are not borrowed from probe defaults |
| Static TLS bytes | PROVEN 28-byte source template / 64 slots; index/PCR/padding/runtime integration PARTIAL |
| Module/global initialization and next initializer | UNKNOWN/BLOCKED beyond the stop boundary |
| Imports | 274 identities: 261 functions, 13 variables; four selected synthetic contracts do not admit title argument/lifetime use |
| PPC/ABI | Reviewed scalar/control/store subset only; complete ABI, typed vectors, atomics and broader instructions remain unverified/denied |
| Exception/fail-stop | Synthetic mechanism exists; title-required recovery/continuation/context obligations remain BLOCKED |

The PE TLS payload was also checked in both byte orders: the big-endian
start/end fields corroborate the separate XEX template. The initial index word
and first callback-list word are observed zero. This proves bytes, not platform
callback/index lifecycle or a complete TLS startup contract.

The private inventory retains module/ordinal, source declaration evidence where
found, pointer/return/ownership uncertainty, startup dependency and current
Horizon denial for every import. 270 imports lack an admitted selected native
contract; the four matching names/ordinals are still PARTIAL until title argument
and ownership contracts are established. A source declaration is not ABI proof.

## Targeted tests and evidence limits

- N1: each Debug/Release/ASan+UBSan 75 semantic checks (26 positive/49 refusal),
  separately 16,384 byte comparisons, no remaining allocations/leases.
- Original tail fixture: each profile 45 semantic checks (28 positive/17 refusal),
  separately 327,680 byte comparisons, cleanup balanced.
- Generic DATA reader: each profile two identity controls, four native-validator
  refusal calls and four malformed-protocol refusals; counts describe calls,
  not independent Xbox compatibility contracts.
- Private candidate reader: each profile checks both representations, expected
  refusal and the distinct source hash, with 0 images/guest instructions.
- Provenance units: 8 methods including 12 negative subcases; bounded CFG 22 and
  original metadata/hash-chain 20 unit methods. No title bytes are needed by CI.

All final selected children are bounded; raw exit codes, wall time and RSS remain
private. Expected parser exits 1/2 are separated from faults/sanitizer findings.
The 45 selected compile/test children total 26.825 seconds of measured child
wall time; peak RSS including compilation is 267,993,088 bytes. Peak execution
RSS is 68,993,024 bytes. Unexpected faults, timeouts and sanitizer findings: zero.
The 12 nonzero malformed-input exits (nine exit 1, three exit 2) are expected.
Early new harness issues (an overcounted expected reason and a watchdog budget
rejected before child launch) remain preserved; no runtime check was relaxed.
No whole historical workload, title codegen or console test was repeated.

## Gates and single next action

Historical **G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED** unchanged. Title native
**N-G4 BLOCKED**, **N-G5 PARTIAL mechanics / UNKNOWN startup**, **N-G8 BLOCKED**.
Synthetic hardware verification is preserved separately and closes no title gate.
The earliest future executable slice is **NOT READY**: it still needs exact
restricted source/layout authority, a supported native provenance policy, stack/
context and variable/import contracts, exact code/CFI binding, fault/stop boundaries
and owner/lease cleanup proof. This assignment grants no execution regardless.

**Most valuable next action:** obtain an independently applicable initialization
contract for the unresolved section tail and establish how its section/resource/
relocation declarations apply to a fixed-address native image. Do not substitute
PE raw-pointer remapping, observed old resolver pointers or failed-decode padding.
This is a native functional-content obligation; an exact historical Xbox allocator
trace is a separate proof goal, not silently reinstated as the only solution.
