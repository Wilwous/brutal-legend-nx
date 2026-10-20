# Compatibility — v0.0.8-pre-entry-safety

No supported playable editions or game builds. The following are **analysis fingerprints**, not a compatibility allowlist.

| Item | VERIFIED result / remaining status |
| --- | --- |
| Input | LIVE GOD/SVOD, 33 fragments; three DLC packages and separate title update |
| Base title/media IDs | `454108C5` / `7DD5E14B` |
| Base XEX version | Execution/base raw `0x2` (`0.0.0.2`), not a marketing label |
| Main XEX SHA-256 | `636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456` |
| Header variant | `live.dll`: different headers/hash, identical stored payload; not loaded as a module |
| Title update | Extracted separately; source/target version fields read; not applied |
| Integrity/extraction | Internal GOD/package checks and extraction passed; all source hashes unchanged |
| ReXGlue host | Official 0.10.0 macOS ARM64 binary runs; selected XEX loads |
| Imports | 274 recognized; 261 function shims present; required stubs remain |
| Function analysis | Validate passes with five explicit function hints; default configuration fails on five branch targets |
| Game C++ / host game binary | GENERATED PRIVATELY / NOT BUILT OR LINKED; two selected partitions compile to Mac objects |
| Instruction semantics | Isolated fixtures reproduce alias and special-value concerns; mask warnings are diagnostically restrictive; hardware equivalence UNKNOWN |
| Foundation | Original checked memory/TLS and SDK safety patch tested; 13 original real-export PPC fixtures and selected XThread/PCR/TLS behavior pass; game runtime not integrated |
| Native protection faults | VERIFIED host cause: unclaimed SDK signal retry; isolated fail-stop/recovery pass. Selected coherent runtime integration passes; complete signal safety PARTIAL |
| Unified SDK | Eleven ordered patches; clean Debug/Release/sanitized builds and bounded defensive regressions; full ABI PARTIAL |
| Patched private loader | Debug/Release/Sanitized loader-only PASS with explicit XDBF acceptance; no guest entry |
| Runtime audit | 261 source classifications: 7 IMPLEMENTED / 184 PARTIAL / 61 STUB / 9 UNKNOWN; no game execution |
| Horizon NRO / launch | NOT BUILT / NOT TESTED |
| Graphics, sound, movies, gameplay | NOT TESTED |
| Input, saves, suspend/resume, dock/undock | NOT TESTED |
| Game memory, stock-clock performance, thermals | NOT MEASURED |

[Phase 8 safety results](research/pre-entry-safety.md) document checked allocation/XAM ownership, parser regressions and Phase 9 NO-GO. [Phase 7 unified results](research/unified-sdk.md) retain historical integration evidence. [Phase 6 PPC/fault results](research/ppc-execution.md) record the diagnosed hang, 57 passing final checks and limited ABI/runtime contracts. [Phase 5 foundation](research/runtime-foundation.md) remains historical component evidence. [Phase 4 tests](research/vmx128-validation.md) and [runtime matrix](research/runtime-audit.md) record current semantic/runtime gates. [Phase 3 report](research/rexglue-analysis.md) records coverage, warnings, commands and a calendar-budget measurement defect. [Binary report](research/xbox360-analysis.md) includes the full variant/update hashes, metadata, resources and limitations. Header/internal-hash consistency does not authenticate a retail dump or establish every edition's compatibility. Future hardware results require exact game/update/module hashes, compiler/SDK/driver/patch revisions, console/firmware/Atmosphère, launch mode, clocks and a reproducible scenario.
