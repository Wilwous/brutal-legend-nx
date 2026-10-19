# Compatibility — v0.0.2-binary-analysis

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
| Function analysis | Reached GapFill; stopped at 45-second bound; validation incomplete |
| Game C++ / host game binary | NOT GENERATED / NOT BUILT |
| Horizon NRO / launch | NOT BUILT / NOT TESTED |
| Graphics, sound, movies, gameplay | NOT TESTED |
| Input, saves, suspend/resume, dock/undock | NOT TESTED |
| Game memory, stock-clock performance, thermals | NOT MEASURED |

[Binary report](research/xbox360-analysis.md) includes the full variant/update hashes, metadata, resources and limitations. Header/internal-hash consistency does not authenticate a retail dump or establish every edition's compatibility. Future hardware results require exact game/update/module hashes, compiler/SDK/driver/patch revisions, console/firmware/Atmosphère, launch mode, clocks and a reproducible scenario.
