# Hardware acceptance batch — Stage 12 selected PPC semantics

2026-10-28. HOST VERIFIED / BUILD VERIFIED; HARDWARE UNKNOWN. Baseline f6a9d1c.
This is the final host-only batch; hardware acceptance is mandatory after Stage 15.
No game XEX is loaded/executed. Xbox Phase22/G4/G8 BLOCKED, G5 UNKNOWN, game NO-GO.

## Evidence and bounded scope

The existing scalar/carry/sign/CR/XER/LR/CTR/stack/finite-FP contracts remain the
selected Stage 9–11 subset. Newly admitted: non-record cntlzw and rlwinm with
SH=8/MB=8/ME=23; lvx/stvx full blocks; vadduwm and vxor. There is no typed-vector
SDK ABI, partial-vector support or general FP/exception expansion.

[IBM assembler reference](https://public.dhe.ibm.com/systems/power/docs/aix/53/alangref.pdf)
provides scalar instruction contracts. [NXP AltiVec PEM, Rev.3](https://www.nxp.com/docs/en/reference-manual/ALTIVECPEM.pdf)
provides full-block address rounding and vector operation semantics. The original
oracles count bits individually, select rotated bits individually and use scalar
wide modulo arithmetic/bitwise XOR for four distinct big-endian words. They do
not reuse the emitter helper or SIMD intrinsic as their oracle. There is no
independent Xenon execution reference with matching environment here; hardware
conformance remains UNKNOWN. Other rotate/Rc forms are not admitted by the policy.

Production lvx/stvx round the low four EA bits to zero. A new narrow Scope seam
consumes exactly one admitted 16-byte memory site, validates the retained full
block/permission, and copies through Space. The SDK's reversed-byte register
representation is explicitly converted at the boundary. All 16 address low-bit
values are exercised; modulo carries, distinct lanes, xor, v14 preservation,
stack/PCR preservation, RO publication refusal, unknown and uncommitted pointers
are checked. Each individual write is atomic under the arena lock; a sequence of
two stores is not advertised as a transaction. An aligned 16-byte block cannot
straddle a 4-KiB boundary. Existing templates continue to deny vector access;
only the acceptance template enables the exact full-block seam.

## Atomics remain BLOCKED

The pinned SDK uses a raw loaded-value/CAS path, outside checked ownership and
without a complete reservation monitor. [NXP e600 manual section 3.3.6](https://www.nxp.com/docs/en/reference-manual/E600CORERM.pdf)
shows implementation-specific reservation behavior, not evidence for Xenon.
No granule size, reservation invalidation, ordering, alignment exceptions or
cross-thread semantics is invented. lwarx/ldarx/stwcx./stdcx. and partial-vector
opcodes fail admission before generation/execution. C++ CAS is not substituted.
Typed-vector SDK parameter and return compile probes still fail explicitly.

## Actual production pipeline and discovered limit

Three original functions are appended without moving the 158 earlier words:
word_logic, vector_blocks and acceptance_multicall. The latter calls the existing
internal Stage8 callee, finite FMUL, word logic and full-vector function through
actual production-generated C++. Its independently expected scalar result is
197650 and FP result 3.0; the original XEX entry separately still returns 24.
Total **20 functions / 195 instructions / 46 memory sites / 92 hooks / four
backedges / 16 calls / 19 returns / 11 branch edges**. Nine appended memory
sites execute. Instruction output matches production-default codegen byte-for-byte
and repeats deterministically; no generated C++ is edited.

A direct bl to the named XEX entry reproduces a generator symbol inconsistency:
the body is xstart, but the emitted call references an absent sub_* name. This
entry re-entry path stays BLOCKED; there is no alias shim or SDK patch. The valid
multicall uses the ordinary internal callee. Initial fixture branch displacement,
register-save and libnx Permission name-collision failures are retained privately;
strict audits/oracles caught them, and source fixture/adapters were corrected.

## Checkpoint acceptance and reproduction

Debug O0, Release O2, ASan+UBSan O1: **326/0 smoke, 445/0 bounded stress**, each;
trace-off semantic results match. Seven expected compile denials. Twelve public
encoding/policy tests pass. Existing RuntimeII independently remains **268/0
smoke, 387/0 stress** in each profile. No unexpected process fault, timeout or
sanitizer finding in final checkpoint runs. Two clean pinned AArch64 ELF/NRO
builds match; no strong unresolved symbols, WX or dynamic dependencies.
Toolchain verified via its actual dkp-pacman: devkitA64 r29.2-1/GCC15.2.0,
libnx4.12.0-1, switch-tools1.13.1-1, rules1.1.1-1; immutable container digest and
SDK revision stay unchanged. Additional reviewed builder hashes are in the new
compat-lock; no previous lock or globally installed SDK is edited.

```sh
python3 tests/horizon-acceptance/test_fixture.py
python3 tests/horizon-acceptance/run.py --stage 12 --toolchain "$TOOLCHAIN" \
  --sdk-source "$ISOLATED_SDK" --workdir "$PRIVATE_STAGE12" \
  --switch-build --icon "$ORIGINAL_ICON"
```

All generated XEX/C++/objects/logs stay outside Git. This checkpoint is not a
hardware acceptance package and is not deployed. Original probes/NROs and private
checkpoints remain preserved. Stages 13–15 follow only within synthetic scope.
