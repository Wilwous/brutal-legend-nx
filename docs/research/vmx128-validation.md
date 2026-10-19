# VMX128 float16_4 validation — Phase 4

Assessment: **2026-10-20 · v0.0.4-runtime-audit**. **PARTIAL:** isolated SDK emission has been executed and characterized against an independent numeric reference. Complete Xbox 360 hardware equivalence remains **UNKNOWN**. Full game compilation, execution and Switch deployment were not attempted.

## Baseline and warning classification

The preserved `default.xex` SHA-256 is `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`. Phase 3's manifest, five boundary corrections and 1,453 output files were fingerprinted before/after this phase and remain unchanged. No title update or additional analyzer run was used. All game inputs, generated game source and raw diagnostic inventories remain private.

**VERIFIED:** all **109 warning records correspond to 109 unique guest instruction sites**. They are `vpkd3d128` with type **5 (float16_4)**, mask **3**, shift **0**. The generator warns because its guard accepts mask 2; it still emits conversion code. Prior generated source contains **237** float16_4 operations total, including these 109, and **zero source/destination register aliases** among all 237. These are static source/operand counts, not execution frequencies.

The 109 warnings occur in these 14 emitted functions (address-based analysis identifiers, not recovered source names):

| Function | Warning sites |
| --- | ---: |
| `sub_821E1740` | 8 |
| `sub_8229FEA8` | 68 |
| `sub_822EA0D8` | 12 |
| `sub_82395828` | 1 |
| `sub_82396898` | 2 |
| `sub_82396C40` | 2 |
| `sub_82396DC0` | 3 |
| `sub_82396F10` | 3 |
| `sub_82397098` | 3 |
| `sub_82397240` | 3 |
| `sub_82397418` | 1 |
| `sub_82397580` | 1 |
| `sub_82397740` | 1 |
| `sub_82397950` | 1 |

The pinned [ReXGlue builder](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/builders/vector.cpp#L1325) emits the same conversion body for masks 2 and 3 at shift 0. Xenia's [instruction permutation handling](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/ppc/ppc_emit_altivec.cc#L2054) treats these two combinations alike. **INFERRED:** the mask guard is overly restrictive for the observed combination. The warning itself does not prove a semantic defect, and changing it would not fix the conversion findings below.

## Independent test methodology

Published original sources: [test runner](../../tests/vmx128/run.py), [emitter adapter](../../tests/vmx128/emit_fixture.cpp), [reproduction instructions](../../tests/vmx128/README.md).

**Implementation under test:** the runner checks the exact SDK vector-builder SHA-256 `b3528ed4a7e0642e2f390cf3d745e18d6e496f14d1707afef6f7049bb84fb39d`, extracts its float16_4 case unchanged into a private temporary include, and compiles an original minimal BuilderContext adapter. The adapter runs that emitter on synthetic, game-independent operands to produce C++. That synthetic C++ is compiled and executed. There is no hand-rewritten model of the SDK conversion serving as the subject, no game file input and no SDK runtime link. The SDK source and fmt headers are local open-source inputs; neither is vendored in this repository.

**Expected results:** a separate Python numeric oracle enumerates every positive finite binary16 value from its mathematical definition: subnormal `fraction × 2^-24`, normal `(1 + fraction/1024) × 2^(exponent-15)`. It converts binary32 exactly to a Python binary64 number and uses binary search for the greatest representable magnitude no larger than the input. It restores sign, preserves infinities, saturates finite overflow for IEEE round-toward-zero, and selects a documented quiet-NaN encoding. It shares no conversion code or exponent-shift algorithm with the SDK. Hard-coded independent identity/zero/subnormal checks sanity-check the oracle.

IEEE binary16 round-toward-zero is an independently specified **comparison contract**, not an assumed Xbox hardware specification. The current Xenia [backend](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/backend/x64/x64_seq_vector.cc#L2173) uses round-toward-zero conversion, making it a useful comparison. Emulator code and repository expected-value fixtures are not new Xbox measurements. No Xbox hardware, authoritative exhaustive VMX conversion specification or hardware-derived special-value corpus was available here. Differences from IEEE or emulator behavior must not be called proven hardware mismatches.

**Inputs per configuration:** 105,254 scalar bit patterns: 25 directed patterns, every nonzero positive finite half value expanded to binary32 and its immediate binary32 neighbors (95,229 patterns), plus **10,000 randomized 32-bit patterns**, deterministic seed `0x42524c4e58`. Both masks are exercised at shift 0, giving **210,508 scalar vector invocations**. Another **130 distinct vector inputs × 12 operand combinations = 1,560 invocations** cover masks 2/3, shifts 0/1/2, distinct registers and same-register aliasing. Total **212,068 invocations per configuration**. Random patterns include both signs and special values; exhaustive boundary enumeration is positive, not an exhaustive sweep of all 2^32 inputs.

Vector expectations are computed from a **snapshot** of the input lanes and an independently constructed packed-word layout. Sentinels check preservation of the other destination words. Guest words are serialized most-significant first; the subject explicitly requires a little-endian host and reverses guest/host lane indices as the emitter does. This tests the relevant macOS ARM64 layout, not a big-endian host build.

The isolated case excludes the outer FPSCR flush-mode helper and full PPC context/runtime. FP exception flags, hardware rounding/flush control, signaling-NaN traps and guest state preservation outside the operation remain UNKNOWN. Sanitizers can detect some memory/undefined behavior, not logical corruption of legal vector indices.

## Build and execution results

All builds used Apple Clang **17.0.0 (clang-1700.6.4.2)**, C++20, with no fast-math flags. Each compiler/subject subprocess has a 30-second timeout; this is not a whole-SDK or whole-game build.

| Configuration | Build / execution | Numeric and vector observations |
| --- | --- | --- |
| `-O0` | PASS | Same results below |
| `-O2` | PASS | Identical counters and examples |
| `-O0 -fsanitize=address,undefined` | PASS; no sanitizer diagnostic | Identical counters and examples |

The initial adapter missed an 8-bit scratch-vector view and failed compilation; adding that view corrected the **test adapter**, without modifying the SDK. The final adapter has no compiler warnings. Raw build files/logs remain private. No second compiler vendor or Horizon compiler was tested.

| Comparison bucket, per configuration | Cases | Differences from reference |
| --- | ---: | ---: |
| Finite values within half range, including zeros/subnormals/boundaries | 201,636 | **0** |
| Finite overflow | 8,776 | **8,776** |
| Infinity | 4 | **4** |
| NaN encoding | 92 | **92** (encoding differences, not proof of incorrect NaN category) |
| Distinct-register vectors, each shift 0/1/2 | 260 per shift | **0** |
| Aliased vectors, shift 0 | 260 | **260** |
| Aliased vectors, shift 1 | 260 | **260** |
| Aliased vectors, shift 2 | 260 | **136** |

Masks 2 and 3 gave byte-identical outputs for every paired scalar input, including the unverified special cases. This supports classifying the 109 mask warnings as **diagnostic overrestriction**, while retaining separate semantic gates. Tests pass as **characterization regressions**: they assert the observed gaps and do not report IEEE/hardware equivalence as PASS. Unexpected finite in-range, distinct-lane or mask-equivalence failures cause a nonzero exit with expected/observed diagnostics.

## Expected versus observed examples

Values below are scalar input/output bit patterns; vector examples are listed in guest word order. The expected scalar column is the independent IEEE round-toward-zero contract.

| Binary32 input | Meaning | Expected half | SDK observed half |
| --- | --- | --- | --- |
| `00000000` | +0 | `0000` | `0000` |
| `80000000` | -0 | `8000` | `8000` |
| `3F800000` / `BF800000` | +1 / -1 | `3C00` / `BC00` | Same |
| `3F801000` | Halfway above +1 | `3C00` | `3C00` |
| `3F803000` | Next halfway boundary | `3C01` | `3C01` (nearest-even would differ) |
| `00000001` | Minimum binary32 subnormal | `0000` | `0000` |
| `33800000` | Minimum positive half subnormal | `0001` | `0001` |
| `33000000` | Half of minimum half subnormal | `0000` | `0000` |
| `477FE000` | +65,504, maximum finite half | `7BFF` | `7BFF` |
| `477FE001` | Just above maximum finite half | `7BFF` | **`7FFF` (NaN)** |
| `7F800000` / `FF800000` | ±infinity | `7C00` / `FC00` | **`7FFF` / `FFFF` (NaNs)** |
| `7FC00000` | Quiet NaN | Chosen canonical `7E00` | `7FFF`, also quiet NaN |

**VERIFIED:** finite overflow and infinity map to a NaN representation in the actual isolated SDK output. This contradicts the stated IEEE comparison contract. **UNKNOWN:** the exact Xbox saturation/overflow, infinity and NaN-payload requirements; game values reaching the 109 sites were not measured. The SDK's source comment describing overflow as clamping must not be confused with finite saturation: `0x7FFF` has the half NaN encoding.

**VERIFIED alias defect relative to snapshot vector semantics:** with source and destination the same vector, mask 3 / shift 0, input `[3F800000, 40000000, 40400000, 40800000]` (1, 2, 3, 4), expected output is `[3F800000, 40000000, 3C004000, 42004400]`. Observed output is `[3F800000, 40000000, 3C004000, 20020000]`. The SDK register-name mapper returns the same local/context variable for the same register index; it does not insert an alias snapshot. The emitted loop writes destination halfwords into still-needed source words; sign reads can also observe modified source data. This is a logical dependency failure, not an out-of-bounds access. Similar failures occur at shifts 1 and 2. No aliased operation occurs among the 237 sites in the prior game output, so it is **not a confirmed defect at those game sites**.

## Decision and next task

- **VERIFIED:** warning sites/affected functions, mask-2/3 diagnostic equivalence in the tested combination, finite in-range truncate behavior, tested lane order/preservation, special-value differences and same-register dependency failures.
- **INFERRED:** mask 3 / shift 0 should not require a warning solely for its mask, based on the tested emission and independent permutation reference. A warning-only patch would leave numeric and alias concerns unresolved.
- **UNKNOWN:** full Xbox hardware semantics, dynamic game inputs/site execution, FP status/control behavior, all VMX instructions, other toolchains and Switch correctness.
- **BLOCKED:** claiming VMX/runtime-correct game execution from the Phase 3 codegen pass. No speculative SDK correction or warning suppression was applied.

Next obtain independently justified Xbox special-value/conversion results or a trustworthy hardware fixture corpus, and propose an isolated alias-safe emitter regression/fix separately. Then validate the memory/TLS/exception contracts prioritized in the [runtime audit](runtime-audit.md). Keep full game and Switch implementation behind those gates.
