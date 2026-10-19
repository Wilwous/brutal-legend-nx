# ReXGlue deep analysis — Phase 3

Assessment: **2026-10-19 · v0.0.3-rexglue-analysis**.

**Verdict: PASS for analysis and minimal C++ generation, with five explicit function hints.** The unconfigured run fails validation. This is a mechanical analysis/generation result; instruction semantics, a complete host link, execution and Nintendo Switch playability remain **UNKNOWN**. The second run also exposed a timing-supervision defect: its recorded calendar duration exceeds the requested 300-second bound. No third analyzer run was made.

## Baseline and preservation

**VERIFIED:** the existing private extraction was reused. `default.xex` SHA-256 was checked before both runs and again after the investigation:

```text
636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456
```

The title update was not applied; `live.dll` was not analyzed as another module. Neither input bytes nor SDK binaries were patched. Extracted files, decoded image, manifests, raw logs/profiles, generated C++, objects and helper programs remain outside public Git. Original source packages were not re-extracted or written. [Phase 2](xbox360-analysis.md) records input format, provenance limitations, metadata and inventory.

Official macOS ARM64 ReXGlue **0.10.0**, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, was used unchanged. CLI SHA-256: `78c1ff17254180e936b701bd97a17ea53b6626a111236dd1cf1aa4a3b1615eea`. The host was the existing Apple Silicon Mac, using Apple Clang 17 for the small compiler checks. [SDK acquisition and fingerprints](phase2-commands.md).

## Budgets and measurement limits

Two fresh private runs were authorized and performed: initially 180 seconds, then one additional 300-second run after the first completed GapFill and exposed five actionable validation failures. The supervisor used Python `time.monotonic()`, sampled child RSS/CPU once per second, disabled core dumps, set CPU limits to budget + 10 seconds, and collected a two-second host stack sample at approximately 30 seconds. Limits were 2 GiB sampled RSS, 96 MiB raw logs and 512 MiB generated output. Logging used 16 MiB rotation / four retained files. No limit-triggered termination occurred in these runs.

| Measurement | Initial, no hints | Additional, five hints |
| --- | ---: | ---: |
| Intended supervisor budget | 180 s | 300 s |
| Exit code | 1 | 0 |
| `/usr/bin/time -l` real time | **78.94 s** | **1,096.56 s** |
| Supervisor monotonic elapsed, including polling | 79.641 s | 77.033 s |
| User + system CPU time | 78.39 + 1.07 s | 76.21 + 1.65 s |
| Maximum resident set size, system timer | **177,487,872 bytes (169.27 MiB)** | **670,859,264 bytes (639.78 MiB)** |
| Peak sampled RSS | 177,471,488 bytes | 421,232,640 bytes |
| Sampled CPU min / median / max | 97.2 / 101.2 / 101.6% | 69.2 / 100.9 / 198.6% |
| Swaps reported by system timer | 0 | 0 |
| Ever-discovered unique function addresses | 50,442 | 50,450 |
| Warning records, audited raw log | 0 | 113 |
| Error records / distinct validation errors | 10 / 5 | 0 / 0 |

CPU percentages are process samples relative to one core; the second run briefly uses more than one core during emission. System peak memory footprint is a different metric from RSS and is not substituted for it. These are **tool** resource measurements, not game RAM requirements.

**VERIFIED timing anomaly:** the second SDK log spans `12:48:35.491` to `13:06:51.908` in the host's recorded local time, and the SDK itself reports about 1,096 seconds. This agrees with the system timer, while the monotonic supervisor and approximately 78 CPU-seconds disagree. **UNKNOWN cause:** host suspension or a clock discontinuity is plausible, but not established. The second run therefore **cannot be claimed to have met a 300-second calendar wall-clock budget**. The supervisor failed to enforce that interpretation of the limit. Its approximately 77 seconds must not be reported as calendar execution time. No additional analysis was used to replace or hide this result. Future runs need a sleep-inclusive continuous deadline and a calendar deadline, with termination on resume if either has expired.

The initial stdout aggregator missed the SDK's `[warning]` spelling. Warning totals above were independently audited from the retained raw log, not taken from its incorrect zero-warning summary. Repeated summary/error records in the first run do not represent ten separate failures.

## GapFill outcome and profile

Both runs advanced through Register → Scan → Discover → **GapFill → Merge → Validate**. Only the hinted run proceeded to Write and completed emission.

| Phase start, supervisor monotonic seconds | Initial | Additional |
| --- | ---: | ---: |
| Register | 0.348 | 0.505 |
| Scan | 10.174 | 8.905 |
| Discover | 10.180 | 8.910 |
| GapFill | 16.875 | 14.643 |
| Merge | 78.697 | 69.387 |
| Validate | 78.841 | 69.648 |
| Write | Not reached | 69.700 |

GapFill consumed **61.822 / 54.744 monotonic seconds**, respectively; the latter is subject to the calendar-clock anomaly. Observed discovery counts moved from 37,116 to 50,442 in the first run and 37,121 to 50,450 in the second. Long periods without a changed count were followed by completion. These are cumulative debug discovery events, not the final graph size.

**VERIFIED:** the first 30-second profile puts 175 of 176 sampled main-thread stacks in GapFill. Function insertion and pending-target resolution dominate those stacks. Source review shows each new function invokes a graph-wide notification over pending functions. GapFill segments uncovered regions, registers entries, discovers pending blocks and removes absorbed entries. [GapFill implementation](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/phase_gapfill.cpp#L109), [insertion/notification](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/function_graph.cpp#L1286).

**INFERRED:** the previous 45-second timeout was too short for this finite discovery/registration workload. Repeated pending-resolution scans are a credible performance cost. No measured memory-pressure, unsupported-opcode stall or infinite loop explains the observed GapFill runs. This is not a proof that the algorithm cannot behave pathologically on another input. The exact cause of the five missing boundaries is not established as a particular SDK defect.

## Initial failure and bounded correction

The initial run completed GapFill but failed Validate with exactly five `UnresolvedCall` errors, all unconditional relative `b` targets reported as “target not in any function”:

| Branch site | Target | Explicit size used in second run |
| --- | --- | --- |
| `0x82B1DF5C` | `0x82B1DF4C` | `0x8` |
| `0x82A8C61C` | `0x82A9D040` | `0x28` |
| `0x82A839EC` | `0x82A839E0` | `0x8` |
| `0x82B35B70` | `0x82B35B60` | `0x8` |
| `0x82B35C88` | `0x82B35C78` | `0x8` |

Each span was checked privately in the loaded image using the SDK's PowerPC decoder: aligned mapped executable addresses, bounded termination, three short shared return epilogues, one adjustment/tail thunk and one 40-byte conditional leaf function. No instruction bytes or disassembly are published here. These are explicit analysis metadata, not a binary patch or a claim that every span corresponds to an original source-language function.

The second fresh manifest added only those five address/size assertions through the supported `[entrypoint.functions]` configuration. It used no `--force`, ignored-error flag, SDK patch or wider speculative range. Validation and generation then succeeded. These hints are necessary to reproduce the successful result; an unmodified default configuration did not pass.

## Coverage and remaining uncertainty

| Area | Evidence classification and result |
| --- | --- |
| Discovery | **VERIFIED:** 50,450 unique addresses ever reached debug `DISCOVERED`, including imports and entries later absorbed/merged |
| Final function representation | **VERIFIED:** 48,632 unique generated declarations = 48,371 unique generated bodies + 261 imported function symbols |
| Analyzed functions | **INFERRED:** those 48,632 entries represent the final graph accepted by Validate and handed to the writer; 48,371 bodies were translated. No independent count of every original executable function or coverage percentage is available |
| Unsupported instructions | **VERIFIED:** zero generic `Unimplemented:` diagnostics and zero `REX_UNIMPLEMENTED`/`UNIMPLEMENTED` markers in emitted C++. **UNKNOWN:** complete semantic correctness, including unusual VMX operands |
| Unresolved branches | **VERIFIED:** five fatal relative-branch errors initially, zero reported validation errors with hints. **UNKNOWN:** exhaustive conditional/indirect/dynamic target coverage |
| Imports | **VERIFIED:** all 274 logical ordinals recognized, including 261 function shims and 13 variable imports; zero unknown import ordinals. Existence is not implementation correctness |
| Missing generated function symbols | **VERIFIED:** all generated declarations reconcile by name with bodies or known import shims. **UNKNOWN:** complete linker closure; no whole-game link performed |
| Memory references | **VERIFIED:** no explicit invalid-memory-reference error in these logs and no out-of-range literal vector lane found in the emitted source scan. **UNKNOWN:** guest memory safety, dynamic accesses and address/protection correctness |
| Exceptions | **VERIFIED:** analyzer notes 106 functions sharing registers with SEH funclets. **UNKNOWN:** number of unhandled runtime exception patterns; imported unwind/handler stubs remain a blocker |
| Jump-table guards | **VERIFIED:** 321 emitted `__builtin_trap()` defaults all identify switch-case-out-of-range guards. They are not 321 unsupported instructions or 321 unresolved branches |

Validate checks relative unconditional branch instructions in graph blocks, accepting internal/function/import targets; source also skips a block it cannot translate. Passing it is not a general proof of all possible control flow. [Validator](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/phase_validate.cpp#L105). Declaration/body counting follows the [writer's graph and import handling](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/codegen_writer.cpp#L187).

## Generation, warnings and compiler checks

**VERIFIED:** successful private output totals **1,453 files / 199,185,199 bytes**. It contains 721 recompilation `.cpp` partitions and 721 matching partition declaration headers, a common declaration header/PCH, initialization and registration C++, an initialization header, source/CMake metadata and dependency/stamp/partition files. There are 723 C++ translation units in total. The SDK reports 1,449 written files; the filesystem inventory also includes four wrapper/glue/stamp outputs.

The audited 113 warnings comprise:

- **109 distinct `Unexpected float16_4 pack instruction` sites**. Decoding all sites gives type 5, mask 3, shift 0. ReXGlue emits code despite the warning; its float16_4 guard expects mask 2. Literal destination indices remain within the vector array.
- **4 functions exceeding the requested 262,144-byte partition target**. These are nonfatal partition-size warnings, not guest memory errors.

The [ReXGlue vector builder](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/codegen/builders/vector.cpp#L1325) writes a half-float conversion despite the unexpected mask. A source comparison with [Xenia at `95a5c3e`](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/ppc/ppc_emit_altivec.cc#L2054) shows that its mask 2 and mask 3 handling select the same permutation when shift is zero. **INFERRED:** the observed mask combination may be accepted semantics with an overly restrictive warning guard, rather than a confirmed unsupported instruction. Neither that comparison nor compilation verifies half-float values, rounding, special values, aliasing or lane preservation. Keep the warning open pending original differential fixtures; do not silence it blindly.

The generated source uses the pinned SDK's PPC context/dispatch and image metadata, guest memory/MMIO, FP/vector helpers and SIMD support, logging/formatting, timing, threading and kernel import shims. Output is not standalone C++ and does not supply a Horizon runtime or a renderer. Minimal compiler checks used SDK headers, C++23 and `SPDLOG_FMT_EXTERNAL`:

| Check | Result |
| --- | --- |
| Syntax: partition 0 | Exit 0, 1.693 s |
| Syntax: vector-containing partition 599 | Exit 0, 1.185 s |
| Syntax: registration unit | Exit 0, 1.729 s |
| Object compile: partition 0, `-O0 -c` | Exit 0, 2.016 s |
| Object compile: partition 599, `-O0 -c` | Exit 0, 5.052 s |

Each check retained one existing SDK FPCR inline-assembly operand-width warning at `rex/platform/fpscr.h:78`. There were no compiler errors in these samples. Checks were bounded to 30 seconds per file. **No whole-game compilation, link, guest execution, shader compilation or Switch cross-compilation was attempted.** Compiler compatibility for all other units, toolchains and optimizations is UNKNOWN.

## Specific toolchain comparison

Only pinned source was compared; neither alternative was run on the game in this phase. No migration was made.

| Concern | ReXGlue 0.10.0 | XenonRecomp `ddd128b` |
| --- | --- | --- |
| Concrete VMX warning | Emits float16_4 with mask 3 / shift 0 and warns | Same unexpected-mask guard in its float16_4 emitter; not evidence of a fix |
| Instruction coverage | Translation finishes for this configured graph; semantics unproven | PPC/VMX emitter exists; this game's coverage not measured |
| Function discovery | Graph/GapFill/merge; five metadata hints needed | `.pdata` and call/branch heuristics, manual boundaries/jump-table work may still be needed |
| XEX input | Actual encrypted/LZX baseline successfully loaded | Existing XEX path documented; exact baseline not tested here |
| Runtime | Integrated SDK/shims; known exception/STFS stubs | Separate game runtime; README states exceptions unsupported |
| Switch work | Existing reference adaptations, still unverified for this game | ARM64/SIMDe support does not provide Horizon memory, APIs or graphics integration |

Sources: [XenonRecomp emitter](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/XenonRecomp/recompiler.cpp#L1997), [pinned README](https://github.com/hedge-dev/XenonRecomp/blob/ddd128bcca99fe8bfbb99bea583c972351fa6ace/README.md). The exact failing discovery case has a demonstrated ReXGlue configuration remedy. XenonRecomp offers no demonstrated advantage for either that case or the observed warning. Xenia's permutation handling is useful as a semantic reference, not a proposed Switch toolchain migration.

## Reproduction commands

Paths below are placeholders for private directories outside the public checkout. Reuse the existing base extraction and pinned SDK; do not extract or update again. Before any separately authorized future analyzer run, use a supervisor with sleep-inclusive/calendar deadline handling as described above. The two-run allowance for this investigation has already been consumed.

```sh
shasum -a 256 "$BASE/default.xex"
"$SDK/bin/rexglue" --version
```

Use a fresh private `$RUN` per experiment. In the actual layout, `game_root` and `file_path` resolve from the run directory to the pre-existing base. Adapt only these private paths:

```toml
[project]
name = "brutal_legend_probe"
game_root = "../../phase2/extracted/base"

[entrypoint]
file_path = "../../phase2/extracted/base/default.xex"
out_directory_path = "generated/default"
```

The initial run used exactly that manifest. The successful second run additionally used:

```toml
[entrypoint.functions]
0x82B1DF4C = { size = 0x8 }
0x82A9D040 = { size = 0x28 }
0x82A839E0 = { size = 0x8 }
0x82B35B60 = { size = 0x8 }
0x82B35C78 = { size = 0x8 }
```

Analyzer invocation, inside the bounded supervisor and with working directory `$RUN`:

```sh
/usr/bin/time -l -o "$RUN/time.txt" \
  "$SDK/bin/rexglue" --log-level debug --log-file "$RUN/rexglue.log" \
  --log_max_file_size_mb 16 --log_max_files 4 \
  --max_file_size_bytes 262144 codegen "$RUN/brutal_legend_manifest.toml"
```

Compact monitoring used `ps -o rss=,pcpu=,time= -p "$PID"`, phase markers and unique `FunctionNode ... DISCOVERED` addresses. A two-second `sample "$PID" 2 10 -file "$RUN/sample-30.txt"` captured host stacks. Aggregate severity counts from the raw `[warning]`/`[error]` records; do not publish raw logs or treat cumulative discovery events as the final graph count.

For the two selected generated partitions and registration unit, a bounded child process used:

```sh
clang++ -std=c++23 -DSPDLOG_FMT_EXTERNAL -fsyntax-only -ferror-limit=10 \
  -I"$SDK/include" -I"$OUT" "$OUT/brutal_legend_probe_recomp.0.cpp"
clang++ -std=c++23 -DSPDLOG_FMT_EXTERNAL -fsyntax-only -ferror-limit=10 \
  -I"$SDK/include" -I"$OUT" "$OUT/brutal_legend_probe_recomp.599.cpp"
clang++ -std=c++23 -DSPDLOG_FMT_EXTERNAL -fsyntax-only -ferror-limit=10 \
  -I"$SDK/include" -I"$OUT" "$OUT/brutal_legend_probe_register.cpp"
clang++ -std=c++23 -DSPDLOG_FMT_EXTERNAL -O0 -c -ferror-limit=10 \
  -I"$SDK/include" -I"$OUT" "$OUT/brutal_legend_probe_recomp.0.cpp" -o "$CHECK/sample-0.o"
clang++ -std=c++23 -DSPDLOG_FMT_EXTERNAL -O0 -c -ferror-limit=10 \
  -I"$SDK/include" -I"$OUT" "$OUT/brutal_legend_probe_recomp.599.cpp" -o "$CHECK/sample-1.o"
shasum -a 256 "$BASE/default.xex"
```

## Decision and next engineering task

The requested **analysis + minimal generation gate is PASS with explicit configuration**, not an out-of-the-box pass. Unconfigured validation has a reproducible five-boundary blocker. There is no reproduced fatal opcode incompatibility and no evidence supporting migration. Broader port feasibility remains **PARTIAL / unproven** because semantic, runtime, graphics, media and hardware gates remain open.

Next: create small **original, game-independent VMX128 float16_4 fixtures** covering mask 2/3, shift 0, source/destination aliasing, preserved lanes, finite limits, subnormals, NaNs/infinities and signs. Compare against independently justified semantics, treating Xenia as a reference rather than hardware proof. Then retain or change the SDK guard only with evidence. Audit the 106 SEH-related functions and imported unwind/STFS paths separately before attempting a private entry/runtime experiment. Fix future supervision clocks before any new bounded analyzer work. Full Switch development remains out of scope.

[Feasibility/blockers](../feasibility.md) · [Roadmap](../roadmap.md) · [Compatibility](../compatibility.md)
