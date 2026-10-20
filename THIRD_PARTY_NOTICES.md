# Third-party research and reuse boundaries

No game code or compiled dependency is included. Phase 5 includes a focused source patch for the open-source ReXGlue SDK, with upstream BSD notices retained; the SDK itself is not vendored. The license file covers original project material only. The following are **research tools or candidates**, not integrated game dependencies. Phase 2 used a local prebuilt ReXGlue SDK and a separately cloned xverter; neither is vendored in this repository. Phase 1 coordinates are in the [reference snapshots](docs/research/reference-snapshots.json); [Phase 2 commands](docs/research/phase2-commands.md) record the newly used tools and release fingerprints.

| Component | License evidence / constraints | Intended use |
| --- | --- | --- |
| ReXGlue SDK | Inspected LICENSE: BSD three-clause terms, including Xenia attribution; GitHub metadata reports NOASSERTION | First host analysis/codegen and runtime candidate |
| project8-sw | Root GPL-3.0; native code includes GPL notices; SDK patches documented as BSD-3-Clause and Mesa patches as MIT | Primary Horizon/runtime and native-renderer architecture reference |
| the-simpsons-game-nx | GPL-3.0 root; per-component SDK, Mesa and LGPL dependencies remain distinct | Second integration reference; not a compatible patch drop-in |
| Mercenaries2-NX | GPL-3.0 root; native-renderer README states GPL-3.0-or-later; component notices differ | Independent integration and renderer diagnostics reference |
| UnleashedRecomp-NX | GPL-3.0 project; XenonRecomp and XenosRecomp have separate MIT licenses | Alternative architecture and renderer design reference |
| XenonRecomp / XenosRecomp | MIT in their inspected repository metadata and license files listed in trees | Fallback CPU/shader tools, not generic turnkey game support |
| Mesa / mesa-switch | Mixed component licenses; inspect every imported file and preserve notices; root MIT shorthand is insufficient for the whole dependency closure | Candidate Horizon Vulkan driver (NVK/NAK) |
| libnx | ISC in inspected metadata; source contains per-file notices | Homebrew platform APIs |
| deko3d | Zlib in inspected metadata | Alternative GPU backend, would require a separate shader/renderer plan |
| xverter | MIT at commit `75c196aefe28692bd02dbdf8fb552485759733cd`; used locally for read-only GOD/SVOD and STFS verification/extraction, no code bundled | Actual Phase 2 extraction tool |
| extract-xiso | Nonstandard/custom license in LICENSE.TXT, including advertising/attribution clauses; metadata NOASSERTION | Separately acquired local extraction tool; not bundled |
| FFmpeg / libmspack / SDL / SIMDe | Configuration and version dependent; reference notices include LGPL-2.1, zlib and MIT components | Audit actual linked configuration before reuse or distribution |

The detected license column in the wider comparison is a discovery aid. `UNKNOWN` or `NOASSERTION` gives no permission to reuse. Root GPL text alone may not establish an “only” versus “or later” choice for individual upstream files; retain their actual SPDX/copyright notices.

Future static linkage to LGPL libraries needs a component-specific compliance and relinking plan. Generated C++ and translated shaders originate from the user's game and remain private even when emitted by an open-source tool. An NRO can contain game-derived machine code; the absence of assets does not by itself establish redistribution permission. This phase creates no release and makes no redistribution conclusion for a future game binary.

Phase 4 adds original GPL-3.0-only VMX test adapters and a numeric oracle. The tests consume a hash-pinned BSD-licensed SDK emitter case and existing fmt headers locally, creating synthetic non-game output outside Git; SDK sources, fmt and generated artifacts are not vendored. Preserve upstream notices/conditions if redistributing those SDK-derived local artifacts. Reference runtime patches were inspected but not copied or applied. [Audit and per-component reuse limits](docs/research/runtime-audit.md).

Phase 5 patch contributions are BSD-3-Clause in `patches/rexglue-sdk/`, retaining Tom Clay/Xenia attribution and full conditions. Original checked foundation/harness and comparison adapters remain GPL-3.0-only. The Canary helper is consumed locally from an exact source fingerprint and is not vendored; preserve its upstream notices if redistributing local SDK-derived artifacts. No reference-port implementation was copied. [Patch boundary](patches/rexglue-sdk/README.md) · [Runtime findings](docs/research/runtime-foundation.md).

Phase 6 adds original GPL-3.0-only PPC/fault/supervision fixtures and a second focused BSD-3-Clause SDK patch. Exact SDK translation units and two method bodies are consumed only in private test outputs, using real SDK types/objects; no SDK binary or source tree is vendored. Both patch files share the retained notices above. [PPC execution and integration limits](docs/research/ppc-execution.md).
