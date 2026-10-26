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

Phase 7 privately builds the pinned full SDK dependency closure and adds original GPL-3.0-only build/supervision/integration fixtures. All 22 submodule revisions, CPM revisions/bootstrap hash and tool wheel hashes are in [the lock file](tools/sdk/lock.json); no dependency source/binary is vendored. Seven ordered patches retain the same BSD-3-Clause notice boundary; no reference-port code was imported. The actual SDK codegen library emits original synthetic operands only into private test output. CMake/Ninja and each SDK dependency retain their respective licenses; this private research build is not a completed redistribution/license-closure plan for a game or SDK binary. [Build scope and evidence](docs/research/unified-sdk.md).

Phase 8 retains those boundaries and adds four BSD-3-Clause SDK patches (0008–0011), including bounded parser/allocation/ownership corrections. Original synthetic fixtures remain GPL-3.0-only; game resources are not test corpus material. [The pre-entry lock](tools/sdk/pre-entry-lock.json) preserves the seven prior patch hashes and pins the complete series. All SDK/dependency binaries, borrowed source comparisons, generated synthetic output and proprietary material stay private. No game/SDK distribution license closure or Release is claimed. [Safety evidence](docs/research/pre-entry-safety.md).

Phase 9 adds two BSD-3-Clause SDK patches (0012–0013), retaining upstream Xenia/Tom Clay notices, and original GPL-3.0-only checked-access/entry-audit fixtures. The [entry-readiness lock](tools/sdk/entry-readiness-lock.json) preserves earlier patch hashes. No proprietary body, derived source, SDK binary or private log is published. [Scope and limitations](docs/research/guest-memory-entry-readiness.md).

Phase 10 adds original GPL-3.0-only instrumentation/coverage fixtures and one BSD-3-Clause SDK generator/template patch (0014), retaining upstream notices. The [instrumentation lock](tools/sdk/instrumentation-lock.json) preserves prior patch hashes. Locally emitted synthetic bodies derive from original PPC inputs and the licensed SDK; they remain private build outputs. No proprietary generated body, address, snippet or asset is published. [Boundary and evidence](docs/research/generated-memory-instrumentation.md).

Phase 11 adds original GPL-3.0-only raw PPC encoding, fixture loading/execution and explicit host scope loans. SDK patch 0015 is BSD-3-Clause and retains upstream notices; the [synthetic lock](tools/sdk/synthetic-lock.json) preserves all fourteen previous patch fingerprints. The actual upstream raw-test generator is invoked without copying its implementation into this repository. Synthetic generated bodies remain private build outputs. No game-derived code or assets are included. [Evidence](docs/research/synthetic-execution.md).

Phase 11 recovery adds original GPL-3.0-only lifetime, native-failure injection and read-only debugger fixtures. SDK patch 0016 retains the BSD-3-Clause/upstream notice boundary; the synthetic lock retains the preceding fifteen patch hashes. Only bounded original test children load the injection library, and all debugger/generated/build outputs remain private. The owned Mac exit request is host control flow, not a guest exception implementation. No proprietary code or assets are included.

The Horizon leaf adapter uses the pinned ReXGlue PPC context headers (BSD-3-Clause) and SIMDe headers (MIT), with no desktop runtime library linked. The optional template-directory/platform patch retains the SDK BSD-3-Clause boundary. The independently authored leaf input, adapter and templates are GPL-3.0-only; generated original-synthetic output and compiled NRO remain local artifacts. [Package notices](tests/horizon-runtime/NOTICE.txt) retain the upstream license texts; [build evidence and scope](docs/research/switch-horizon-runtime-adapter.md).

The independent Horizon foundation/XEX stages add original GPL-3.0-only runtime, parser, fixture and template code. One exact BSD-licensed SDK RtlCompareMemoryUlong body is fingerprinted and extracted only into private build output; packaged notices retain Tom Clay and Ben Vanik/Xenia attribution. No full desktop SDK runtime or proprietary binary is distributed in Git. [Stage 2](docs/research/switch-horizon-runtime-foundation.md) · [Stage 3 and admitted subset](docs/research/switch-synthetic-xex-integration.md).

The Horizon expansion retains those GPL/BSD/ISC/MIT boundaries. Two exact SDK
exports (compare and critical-section initialization only), the SDK dispatcher
structure, and original synthetic generated callers are compiled into the local
integrated probe. The 2013/2022 Ben Vanik and 2026 Tom Clay notices are retained in
[the integrated package notice](tests/horizon-expansion/NOTICE.txt). Full desktop
runtime, proprietary XEX/code and assets remain excluded. The source recipe and
original PPC fixtures are public; build outputs remain local, with no Release.
