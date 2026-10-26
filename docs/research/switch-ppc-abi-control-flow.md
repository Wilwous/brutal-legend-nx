# PPC ABI, control flow and stop — Stage 6

Original synthetic flat XEX construction remains host-side. Production ReXGlue
v0.10.0 generates all PPC bodies; no generated C++ is edited. The extended image
has 10 functions / 83 instructions, including 64-bit add, exact finite double add,
two nested loops and an intentionally invalid LR return. The original 65-word
fixture remains unchanged.

The independent instruction/comment census checks every emitted body against the
original PPC words and the unmodified production-default instruction output.
There are 20 scalar memory sites (6 load32, 8 store32, 3 load64, 3 store64), 44
instruction hooks, 6 call sites, 9 return sites and 9 branch edges. All four
backward branches have emitted hooks and are individually observed stopping.
All 20 scalar memory hooks are observed across the declared test scope. A hook
hit means entry to validation, not proof that a refused memory access completed.

Strict Function metadata requires a ControlPlan. Each memory operation consumes
an armed instruction site of the correct direction and width; each call consumes
its exact edge. Direct targets, the selected CTR target and per-call LR return
are checked before transfer/continuation. Raw pointer entry without an admitted
dispatch is refused at the generated prologue. Root exits use an explicit host
sentinel. Site ownership is per active generated function, with bounded call
depth. This is a fixture-specific policy, not a general CFG engine or Xbox SEH.
Older probes keep their narrower original admission contract.

The tested ABI subset covers GPR32/64 argument values, widening, r1/r2/r13,
stack arguments, LR/CTR, selected CR/XER preservation, r14/r15/r31 and exact finite
FPR addition (1.5+2.25=3.75), including f14 preservation. Host FP control is saved
and restored by Scope. Nonfinite inputs to the selected FPR function are denied.
Rounding edge cases, denormals, NaNs, FP exceptions and full Xbox ABI remain
UNKNOWN. Typed-vector signatures, arbitrary callbacks, guest unwind and exception
transfer remain BLOCKED.

Stop uses production midasm hooks and the existing one-shot state machine;
no forced cancellation, longjmp or injected guest exception. Atomic memory/SDK
work completes before the next boundary. Tests cover finite, infinite and both
nested-loop backedges. Host stop latency is measured separately at Stage 8 and
must not be presented as Switch timing.

Verification and reproducible command: use `tests/horizon-expansion/run.py
--stage 6` with the pinned toolchain, isolated SDK, fresh private output and
`--switch-build`. Public original encoding/auditor units:
`python3 tests/horizon-expansion/test_fixture.py` (9/0). Full profile results are
recorded below after the checkpoint gates.

Xbox Phase 22 BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN. Game-code execution NO-GO.

Checkpoint evidence: HOST VERIFIED 41 new assertions / cumulative 158/0 in each
Debug, Release, ASan+UBSan profile. Five compile denials; no unexpected faults,
timeouts or sanitizer findings. Earlier fixtures remain 108/0 and 118/0 per
profile. BUILD VERIFIED: two matching clean AArch64 ELF/NRO builds; no undefined
strong symbols, dynamic dependencies or WX. HARDWARE UNKNOWN. Extended image
SHA-256: `dd8967586f11fd61090337df54fe9655baa89d68c50faf2a7c1dcc2a8342b5b3`.
