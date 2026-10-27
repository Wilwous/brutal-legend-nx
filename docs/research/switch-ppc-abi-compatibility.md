# Horizon Runtime II: ABI implementation matrix

Baseline `23f8c8e`; Stage 8 hardware remains UNKNOWN. Only the earlier four-word
PPC probe is HARDWARE VERIFIED. This matrix precedes implementation.

| Area | Existing evidence | Missing work / uncertainty |
| --- | --- | --- |
| GPR32/64, r1/r2/r13, LR/CTR | Selected generated calls and preservation | Expand signs, carries, ten scalar arguments and struct pointers |
| FPR | Exact finite add, nonfinite refusal | Original multiplication corpus with literal IEEE-bit oracles; full FPSCR unknown |
| CFG | 25 scalar sites, 52 hooks, four backedges | Extend independent census; keep unknown/computed non-call transfers denied |
| Callbacks | SDK wrapper needs ThreadState/kernel memory | Typed/general guest callbacks remain denied; do not emulate missing SDK runtime |
| Typed vectors | 128-bit context representation exists | Marshalling, returns and lane/host convention independently unverified: BLOCKED |
| Atomics/reservations | SDK emitter inspected | C++ atomic operations do not establish Xenon reservation semantics: BLOCKED |
| Partial vectors | SDK emits raw-address shuffles | No checked cross-boundary/lane contract: BLOCKED |
| Memory | Checked logical arena, views/leases/transactions | Query metadata and OUT-only publication need explicit state contracts |
| SDK imports | Real compare and initializer | Investigate real NtQueryVirtualMemory with a Space-backed heap-query interface |
| Diagnostics | Counts and controlled Error | Bounded structured provenance/site/refusal and lifecycle events |
| Platform | Pinned AArch64 build and old PASS38 | Runtime II hardware UNKNOWN, no ordinary-homebrew VM isolation claim |

Implementation will reuse Memory, Space, GuestThread, Scope and XexImage. Old
fixtures, packages, SDK locks and protected branches remain preserved. Production
codegen only, no generated C++ edits. Xbox Phase 22 BLOCKED; G4/G8 BLOCKED,
G5 UNKNOWN; original game execution NO-GO.

## Stage 9 selected contracts and semantic evidence

The new original XEX has 16 functions / 138 words: carry/sign/CR, ten u32
arguments, an eight-byte record pointer, finite FMUL and a nested root calling
the preserved Stage 8 function. Primary result is 24; the generated bodies are
compared byte-for-byte with production-default instruction output and independently
mapped to words/comments/sites. No handwritten replacement caller is used.

The integer oracle uses wide integer division to compute carry, modulo-64 GPR
results and explicit sign-bit extension. CA is deliberately tested for the
selected default SDK MSR `0x0200a000`/32-mode contract; other modes and full Xenon
conformance remain UNKNOWN. OE=0 preserves OV/SO; OE overflow forms are denied,
not silently treated as plain add. CR comparisons inherit SO. Ten u32 arguments
use r3–r10 and the pinned SDK's +0x54/+0x5c stack slots through checked memory.
This does not validate arbitrary 64-bit stack/struct-by-value signatures.

Semantic references: [IBM addc specification](https://www.ibm.com/docs/en/aix/7.2.0?topic=set-addc-add-carrying-instruction)
and [PowerPC programming environments](https://www.nxp.com/docs/en/user-guide/MPCFPE_AD_R1.pdf).
Actual emitter/context/wrapper files are fingerprinted in
`tests/horizon-runtime-ii/compat-lock.json`. The ISA distinguishes mode-dependent
carry/overflow; these tests do not infer a new Xbox mode from host C++ behavior.

Six literal IEEE bit-pattern FMUL oracles cover signed zero, normal limits and
subnormal multiplication by two under the selected host FP control. Nonfinite
inputs remain refused. f14 and host control are preserved. Full FPSCR, arbitrary
rounding and guest FP exceptions remain unverified. Hardware behavior is UNKNOWN.

SDK VRegister is 16-byte aligned with 16 bytes / four u32 lanes. Neither that
representation nor host SIMD ABI establishes Xbox typed-vector marshalling.
Actual SDK typed-vector parameter and return compile probes fail at conversion
of VRegister in the generic scalar translator. Both remain denied. General SDK
callbacks require ThreadState/kernel memory not supplied by this small Horizon
backend; foreign/misaligned contexts and unknown targets refuse.

The SDK reservation emitter stores a raw loaded value and performs raw-address
CAS, without this arena's range/lease validation or a complete reservation monitor.
Partial-vector emitters use raw aligned block shuffles. Both fail this checked
scope's opcode/raw-access gates; neither is replaced by guessed C++ atomics.
Reservation granularity/invalidation and partial-vector lane/cross-page semantics
remain BLOCKED. Non-call computed branches and OE forms remain denied.

All 34 declared scalar memory hooks and four backedges are observed; stop is
separately exercised at every backward branch. Exact supported ABI is scoped
GPR u32/u64, selected finite doubles and explicitly checked pointers, not full
Xbox ABI. Earlier image/ownership/permission rules remain intact.

Stage 9 acceptance: HOST VERIFIED **121/0** per Debug/Release/ASan+UBSan profile;
7 controlled compile denials, including actual SDK vector parameter/return probes.
Two matching clean AArch64 ELF/NRO builds, no strong unresolved symbols, dynamic
libraries or WX segments. Nine original public encoding/policy units pass.
HARDWARE UNKNOWN. The initial GCC indentation failure is retained privately;
explicit braces fixed the test without suppressing warnings.
