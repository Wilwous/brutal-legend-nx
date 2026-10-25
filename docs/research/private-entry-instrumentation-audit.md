# Phase 22 — private entry instrumentation and admission audit

2026-10-24–25 · baseline `v0.0.21-entry-runtime-contracts`, commit
`1da0e11a54c3cd8b950d379c8045646bfc9e5051`.

**Current hardware trace feasibility: PARTIAL; experiment BLOCKED / NO-GO.**
The user confirms no Xbox 360 debugging hardware access. Reliable post-mapping/
pre-entry stopping and image reservation/owner observation remain unqualified. The original-only protocol below is not executed. Residency
remains UNKNOWN; G4/G8 BLOCKED, G5 UNKNOWN. All prior work, selected 1001/0
regressions and offline artifacts are preserved. No loader change, proprietary
execution or Phase 23 occurs. Published baseline and VERSION remain Phase 21.

## Incomplete research preservation

The branch `research/phase-22-blocked` preserves this unfinished research with
the commit subject `chore: preserve blocked phase 22 research`. This is a
preservation commit, not Phase 22 acceptance. Earlier statements about no commit,
push or publication retain the status of their historical research attempts;
this section records the later, explicitly authorized research-branch preservation.
No completed milestone tag, GitHub Release or merge into main is created.
The completed baseline remains Phase 21 and all G1–G12 requirements remain in force.

The complete private working tree is retained locally in an independent
copy-on-write checkpoint, with a file manifest and verification receipts outside
Git. The private report provides exact paths, commands and a restore-to-fresh-
directory procedure; it never restores over a live working tree or runs saved
code. This local same-disk checkpoint is not an off-device backup. Public source,
original fixtures and sanitized documentation are reviewed against the allowlist;
game files, private manifests/logs, generated title code and objects are excluded.

The [two-track roadmap](../roadmap.md#two-independent-research-tracks) keeps
Xbox XEX residency **BLOCKED** pending independent hardware evidence. Original-only
Nintendo Switch/Horizon runtime work is eligible for its own future feasibility
investigation, without proprietary execution; it is **NOT STARTED**. This does
not reopen loading, implement hooks, establish startup state or begin Phase 23.

## Hardware trace feasibility and pause preservation

**2026-10-25 · PARTIAL protocol feasibility; hardware experiment NO-GO.** Public
client implementations expose useful inspection commands, but no accessible
console, matched debugger or image-owner observation channel has been qualified
in this task. Hardware access is **unavailable, confirmed by the user**; no device ownership
is inferred.
No device discovery, device connection, upload, console launch, SDK modification
or proprietary execution is attempted. Pre-entry evidence remains **BLOCKED**.
The following is a proposed original-fixture-only protocol, **not an executed
hardware experiment**. It does not reopen the earlier static residency search.

### Hardware feasibility matrix and reviewed evidence

| Candidate | Public evidence and usable observations | Unverified requirements / verdict |
| --- | --- | --- |
| Xbox 360 development/debug kit with matched authorized XDK debugger | Public [Xenia XBDM export names](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xbdm/xbdm_table.inc) include initial break, thread context, module and committed-memory walks. This verifies names, not console semantics or their exact API payloads. | **PARTIAL candidate / locally BLOCKED.** Obtain legitimate matching debugger documentation and validate image-capable reservation/owner records, stop ordering and rollback observation. Kit type alone does not qualify them. |
| Owner-controlled RGH console with a specific XBDM plugin | Reviewed Xbox 360 clients issue read, region, module, breakpoint, stop and context commands. Earlier reported RGH memory observations below demonstrate that image and heap query behavior can differ. | **PARTIAL candidate / locally BLOCKED.** Record kernel/HV, patches, plugin hashes/load order and debugger version. Plugin compatibility does not imply devkit equivalence or stock-console semantics. |
| [MemoryEngine360 connection](https://github.com/AngryCarrot789/MemoryEngine360/blob/01f9b84a7b9dfe180ad3ad6ca203bf32118ce8f3/Xbox/MemEngine360.Xbox360XBDM/Consoles/Xbdm/XbdmConsoleConnection.cs) and [debug features](https://github.com/AngryCarrot789/MemoryEngine360/blob/01f9b84a7b9dfe180ad3ad6ca203bf32118ce8f3/Xbox/MemEngine360.Xbox360XBDM/Consoles/Xbdm/XbdmConsoleConnection.XbdmFeaturesImpl.cs) | **VERIFIED source only:** region/protection parsing, memory reads, modules/sections, stop/go, breakpoints and selected context parsing. | **PARTIAL.** No owner/reservation field in the reviewed region result; step is a TODO. No verified atomic pre-entry stop. Preserve raw replies: region filtering and failed-read zero substitution make the formatted UI insufficient evidence. |
| [Experiment5X/XBDM client](https://github.com/Experiment5X/XBDM/blob/fd9cc78a5895d0418a4685d9863ccf791029416d/Xbdm.cpp#L1018-L1047) | **VERIFIED source only:** `walkmem` parses base/size/protection; module/section queries parse declared spans. | **PARTIAL.** Cached results need fresh requests. No reservation owner, allocation generation or demonstrated pre-entry/rollback ordering. It is a client, not proof of monitor/kernel behavior. |
| [OXDK Xbox 360 tools](https://github.com/MrMilenko/OXDK/blob/b175c2c16446f0af33b75cb49a7140d6a2cab3b5/docs/xbox360.md) and [client](https://github.com/MrMilenko/OXDK/blob/b175c2c16446f0af33b75cb49a7140d6a2cab3b5/xbox360/tools/xbdm/xbdm.py) | **VERIFIED source only:** a Unix-host build path requiring the user's XDK, plus raw command, deployment, launch and notification transport. | **PARTIAL transport, not an owner tracer.** Normal launch supplies no verified pre-entry hold; listener can wait indefinitely. No XDK is obtained, installed or redistributed here. |
| Original-Xbox XBDM wiki/GDB bridges, generic Windows debugger examples, emulator results | These can suggest questions but target a different platform or implementation. | **Not Xbox 360 pre-entry proof.** No port number, stop flag, query ABI or Windows page-table command is transplanted from them. |

All references are pinned with source hashes privately; third-party clients were
read, not installed or executed. Their shared protocol is not several independent
hardware traces. The inspected MemoryEngine360 path requests control/integer/FP
context; a declared vector structure does not prove complete VMX capture. Raw
field presence, widths and completeness must be checked before a register claim.
No public source reviewed establishes a complete eight-property console trace.

### Required access and acquisition checklist

The laboratory must supply these facts before any future experiment is GO:

1. An explicitly identified, owner-authorized Xbox 360 dev/debug kit, or a known
   modified console whose supported debug configuration is recorded. A retail
   console or network memory viewer alone is insufficient. The user confirms no hardware access.
   The access reply does not authorize any connection or acquisition.
2. Exact console model/debug mode, kernel and hypervisor identity, applied patches,
   debug-monitor binary identity/configuration and plugin load order. Record
   uncertainty rather than infer firmware from a dashboard label. Do not collect
   console secrets or use unknown devices.
3. A matched legitimate host debugger/toolchain and its documentation, or an
   independently authored original-fixture toolchain. OXDK is optional host glue,
   not a replacement XDK or debugger-ownership implementation. Keep required
   proprietary tooling private; no installation or download is performed here.
4. Verified stop-before-entry semantics, including behavior on connection loss,
   and image-capable reservation/commit/owner queries or trustworthy loader
   allocation/PTE traces with documented field meaning and cleanup events.
5. A raw transcript collector with monotonic timestamps, exact byte counts,
   errors and per-byte validity. Disable client region filters, caching and any
   invalid-byte-to-zero presentation. Do not scan MMIO or read an unknown range
   to discover whether it is accessible.
6. A verified cancellation/unload/recovery procedure that does not require title
   entry to run. Stopping a host collector does not stop a console. An unqualified
   target-side abort path is a NO-GO, even with a host watchdog.

Resources verified locally are the pinned isolated SDK, original synthetic test
sources, prior saved results and private offline objects. Hardware access is **unavailable by user confirmation**. A matched console
debugger, allocator/PTE owner introspection and a measured pre-entry boundary
remain **unqualified**. No hardware
inventory or connectivity is inferred from local files or network discovery.

### Strict pre-entry observation gate

The required boundary is **after mapping, initialization, import fixups and final
protection work for the candidate complete, but before its first entry instruction
executes**. Record the exact load instance and absence of earlier candidate TLS,
callback, initializer or other execution that could have changed its pages. A
module-load notification, late attach, manual pause, sentinel value or entry PC
alone does not prove that ordering.

Qualification must use an original control and the actual matched debugger's
supported initial/deferred execution stop. Capture its acknowledgement, stop
reason, module/load identity, loader-completion evidence, all relevant thread
states and raw entry context. Verify whether the reported PC is before a hardware
execution stop or after a debugger trap; do not correct PC by an assumed offset.
If software breakpoints replace original fixture bytes, record the patch and
exclude it explicitly from content comparisons. Do not patch proprietary code.

A first-instruction sentinel or intact data can corroborate a control, but cannot
prove that execution never occurred or that mapping finished. A debugger-created
stop before mapping is also too early. If event ordering cannot be established,
mark **pre-entry evidence BLOCKED** and do not advance to the overhang. API names
such as initial-break/stop-on are candidates for qualification, not proof that
this boundary exists on an untested monitor build.

### Original control/overhang protocol — not run

Use an independently authored minimal fixture with no TLS callbacks, static
constructors or guest imports and with entry that need never execute. No game
bytes, addresses, names or layout are copied. Address choices are original and
must fit the laboratory's verified allocation model.

- **A, ordinary control:** code/RO/RW and explicit initial bytes are entirely
  covered by its payload, security extent and descriptors. Independent host
  layout validation is only a format check. Establish its real ordinary
  reservation, commitment and owner on the console before describing A's
  ownership as verified.
- **B, overhang:** retain A's entry, code, stored data, decoded extent, security
  size, descriptors and module kind. Extend one declared RO data VirtualSize
  past that extent by one qualified image page, with PE SizeOfImage adjusted
  consistently. This is a documented coordinated metadata change, not a
  one-field-only experiment. Recompute required derived integrity/signing fields
  using legitimate tooling; keep the same policy as A. No producer may silently
  enlarge the payload or descriptor ownership to accommodate B.
- **C, refusal control:** an independently authored malformed-header candidate
  checks that a load refusal and cleanup are observable. Use a format violation,
  not an assumed digest rejection on a modified console. A pre-allocation refusal
  cannot validate rollback after a partially allocated load.

The proposed bounded acquisition sequence is:

1. Freeze builder/version, input/output hashes, symbolic ranges, exact A/B delta
   and integrity provenance. Confirm original-only content and that B actually
   has an overhang; do not treat packer normalization as a test of it.
2. Before loading, capture a fresh reservation/commit/owner map for the candidate
   span and immediate neighbors, plus thread/module inventory. Never place a
   fixed allocation or write into unknown memory to create the scenario.
3. Qualify A's pre-entry stop and observation channels as above. Without an
   image-capable owner/reservation view, stop with PARTIAL observations rather
   than infer ownership from reads or matching module ranges.
4. Acquire A's boundary snapshot and temporal allocation records; use only
   supported read-only memory/PTE introspection. After verified cancellation/
   unload, capture final owner/map state and match release to allocation IDs.
5. Repeat for B, preserving its actual load status even if mapping never finishes.
   Read bytes only after the range is independently shown committed, readable
   and properly owned. Foreign-owner content is not dumped.
6. Acquire C's explicit failure and cleanup. For post-allocation rollback require
   either an observed B failure after recorded allocation or a documented safe
   failure mechanism. No arbitrary faults or speculative fault injection.
7. Use at most three fresh A/B/C cycles: nine load attempts. Pre-entry
   qualification must pass before B. Set 30-second host operation and 180-second
   case ceilings, with a verified target-side cancel/hold path. Timeout,
   unexpected resume, missing owner data or mismatched fixtures stops the run;
   do not automatically repeat failed attempts.
8. Compare raw traces across cycles using allocation identity/generation and
   interval boundaries, not reused virtual addresses. Record variation and
   incomplete observations. Do not resume entry to obtain a prettier result.

| Observed outcome | Evidence required to distinguish it |
| --- | --- |
| Refused image | Explicit load failure, its phase and final map/owner comparison; absence from a module list alone is insufficient. |
| Declared but unmapped tail | Declaration plus image-capable reservation and commitment maps both showing no tail mapping; missing `walkmem` output alone is insufficient. |
| Reserved but uncommitted tail | Exact reservation owner covers tail, with commitment explicitly absent; read failure cannot distinguish it from unmapped. |
| Committed initialized tail | Owner/commit/permission records plus complete valid-byte data at the qualified boundary; zero substitution is not zero-fill. |
| Separately owned mapping | A distinct allocation identity and owner/creation trace, correlated to loader action and later release. Spatial overlap with the PE declaration is not enough. |
| Overlap with another allocation | Before/after owner generations and interval intersection demonstrate the foreign neighbor; do not infer it from contents or write to test it. |

### Eight-property evidence checklist

All rows currently have **no acquired console evidence**. “Method” is a
requirement, not a claim that any reviewed client exposes it. Use A versus B
at identical lifecycle points, retaining three bounded cycles if qualification
succeeds. One snapshot cannot establish a platform-wide rule.

| Property | Observation method / evidence source | Required precision and control | Reproducibility and limitation |
| --- | --- | --- | --- |
| Reservation | Qualified image reservation/allocator records, not only module headers | Exact start/end, owner and allocation generation before/load/after; A establishes baseline | Correlate all cycles; committed-only walks cannot distinguish free from reserved. |
| Commitment | Image-capable PTE/commit query tied to that reservation | Every page, granularity, state and validity; compare A/B tail and boundary pages | Repeat at same stop; absent or failed query remains UNKNOWN. |
| Initial contents | Raw debugger reads after owner/readability checks, with validity/error mask | Every in-scope byte, exact length/hash and expected A bytes; B contents observed without a zero-fill assumption | Earliest qualified boundary each cycle; snapshot cannot prove how bytes were initialized without loader evidence. |
| Permissions | Qualified actual page/PTE protection, distinct from PE flags | Every page and alias relevant to the range, before/after loader final protection | Repeat A/B; declared flags or successful reads do not prove effective rights. |
| Ownership | Allocator reservation identity plus loader create/retain/release correlation | Distinguish whole image, separate tail and unrelated neighbor; identity/generation, not base only | Temporal record mandatory; a single snapshot or statistics total cannot establish attribution. |
| Adjacent allocations | Before/after owner maps around tail; original known neighbor only if already safely supported | Exact nonoverlap, preserved owner generations/rights and legitimately readable control bytes | Three cycles do not prove all collisions; if no controlled neighbor case exists, retain PARTIAL. |
| Lifetime | Timestamped owner/refcount or retain/release transitions through load, hold and unload | Same allocation identity retained while live, no premature reclamation | Requires multiple lifecycle observations; snapshot alone insufficient. |
| Rollback/unload | Actual failed-load phase and allocation/release log plus final owner map | Every acquired owner released exactly once, neighbors unchanged; distinguish pre-allocation rejection | Multi-event evidence required; post-entry shutdown or total free-memory equality is not exact rollback proof. |

A final memory snapshot cannot by itself prove reservation attribution,
initialization mechanism, neighbor noninterference across time, lifetime or
rollback. Even a complete snapshot needs a proven stop boundary and query
semantics before it can establish commitment/protection/content at that instant.

### Decision, fallback and exact next action

**Hardware experiment NO-GO with currently verified resources.** Candidate
inspection methods are PARTIAL; sufficient pre-entry/owner observability is
UNKNOWN and its admission gate BLOCKED. No complete trace is promised from
XBDM alone. G4 remains **BLOCKED**, G5 **UNKNOWN**, G8 **BLOCKED**. The original
load's retained `0xC0000001` is not retried; no SDK/binder behavior changes.
Even successful original-fixture captures would not supply Brütal Legend's
startup provenance or private exceptional-transfer enforcement.

**Access update: no hardware is available to this project through the user.**
The hardware path is therefore **BLOCKED**, not merely awaiting a target address.
Preserve the protocol and stop acquisition work. Its next evidence prerequisite
is an independently supplied, original-fixture-only laboratory trace package
meeting the checklist above, or explicitly arranged authorized lab access. No
third party is contacted and no hardware purchase, connection or experiment is
initiated. If such access later becomes available, qualify A's pre-entry ordering
and image reservation/owner observer before B. A late dump or new decoder cannot
substitute for the missing observations.

The smallest independently testable alternative is an **original synthetic sparse
module ownership contract**: two explicit ranges with host-owned generation/lease
identities, checked copies and denied gaps, exercising rollback and final release
without claiming an original XEX was loaded. Evaluate it only under a future
explicit scope; no implementation starts here. A later snapshot-backed variant
would additionally need independently proven bytes, state, owners and permissions.
Neither alternative closes original G4, supplies G5 or implements G8, so neither
authorizes game execution or Phase 23.

### Preservation, verification and resuming after a pause

The complete 38-file dirty state is snapshotted before this task; earlier source,
patch locks, private inputs/generated outputs, native objects and failure receipts
are retained unchanged. The original XEX SHA-256 is reverified against the fixed
baseline. Private artifact identities, staged library hashes and producer/test
source receipts are checked against the existing records. Only this research
report changes during this hardware review. Public review uses a separate private
Git index; the real index remains empty and HEAD stays the Phase 21 commit.

Saved selected evidence remains **1001 PASS / zero FAIL**, with **125** additional
Python units; its earlier failed/superseded attempts are retained. Offline evidence
remains **8/8 checked stores**, **20/20 emitted/compiled references**, **zero hook
implementations** and **two compile-only bodies per profile**. Nothing is
regenerated, relinked or executed. No full regression suite is rerun because no
production behavior changes. Reproduction interfaces are checked with help-only
invocations, not by launching codegen or native tests.

To resume safely:

1. Read this section, then Recovery III and the preserved private progress/seal
   records; distinguish selected final results from raw failed attempts. Check
   HEAD, dirty paths, real index and current hashes before any action. Preserve
   newly discovered differences rather than restoring over them.
2. Use the latest private preservation manifest to locate prior SDK stages,
   source/configuration, frozen inputs, exact command receipts, failure logs and
   source/object hashes. Earlier workspaces are read-only evidence. Keep outputs
   in a new private directory; never reuse or overwrite a completed work directory.
3. Recheck XEX identity, ordered 22-patch lock and staged runtime/header identities.
   The complete regression entry point is shown below **for a justified later
   production change only**; the offline test command uses original public
   fixtures, not the private game extraction.
4. Record confirmed access/capabilities before any device action. This protocol
   is a plan, not permission to discover or contact a device. Do not automatically
   retry original loading, proprietary codegen, old faults or incomplete hardware
   attempts. No Phase 23 or completed Phase 22 publication is authorized.

```sh
# Reference only: use fresh private outputs and the matching pinned SDK.
python3 tests/xex-binding/workflow.py --sdk-workspace "$RESIDENCY_SDK" --lock tools/sdk/residency-lock.json --workdir "$FRESH_REGRESSIONS"
python3 tests/offline-region/run.py --sdk "$SDK" --source "$PINNED_SOURCE" --workdir "$ORIGINAL_ONLY_RESULTS"
# Add --sanitize to the original-fixture runner only for the matching Sanitized SDK.
```

The publication allowlist, documentation links and absence of new private paths/
identifiers are checked. No game bytes, metadata maps, generated proprietary C++,
objects, raw logs, device identity or local paths are added to the public tree.
The narrow review leaves all work in place: **no interim public commit, completion
tag, push, CI publication or GitHub Release**. Phase 22 remains BLOCKED/unpublished.

## Residency evidence gate — outcome B

**2026-10-25 · UNKNOWN semantics / BLOCKED original loading.** Targeted reference
research is complete within the reviewed public sources. No defensible new
section-tail allocation rule emerges, so the gate stops under outcome B. This
is a limit of available evidence, not proof that no such platform rule exists.
No loader correction, patch-lock change, new positive fixture, original load
retry, proprietary generation/compilation/linking or guest execution occurs.
Recovery III's exact offline instrumentation and final selected 1001/0 results
remain preserved historical evidence, not newly rerun tests in this gate.

### References, independence and applicability

| Ref | Reviewed source and precise locus | Evidence class and limit |
| --- | --- | --- |
| R1 | [Xenia CPU loader](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.cc#L741-L764), `ReadImageCompressed`, `ReadPEHeaders`, `LoadContinue`, `Unload`, `ApplyPatch`; [size calculation](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.h#L77-L92) | Explicit emulator behavior. The current upstream revision equals the previous pin. ReXGlue shares ancestry; this is one source family, not independent console confirmation. |
| R2 | [Xenia UserModule](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/user_module.cc#L134-L259), `LoadFromMemory`, `LoadXexContinue`, `Unload` | Same implementation family as R1. It delegates image loading, allocates a separate guest header and records loader metadata; that header allocation does not back a PE tail. |
| R3 | [RXDK360 packing notes](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/docs/xex-packing.md), and [packer](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/tools/elf2xex.py#L250-L350), `build_pe_basefile`, `build_page_descriptors`, `build_security_info` | Independently authored producer, with Xenia/XexTool-derived interpretation. Its reported Xenia successes are not hardware tail tests. It pre-covers its own section ends in image bytes/security size/descriptors; it does not demonstrate allocating an overhang at load time. |
| R4 | [Microsoft PE/COFF](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format#section-table-section-headers), General Concepts, Optional Header, Section Table, Section Data and flags | Documented Windows format definitions. Authoritative for those definitions, not Xbox XEX allocation ownership. A versioned source copy and its hash are retained privately. |
| R5 | [xsdk hardware report](https://github.com/xenosworks/xsdk/blob/2c4599b31066efcf67be099673287fbec786ef14/docs/HARDWARE.md), “XEX loading”, “What the kernel leaves a title”, M55 query follow-up; [packer](https://github.com/xenosworks/xsdk/blob/2c4599b31066efcf67be099673287fbec786ef14/tools/elf2xex.py#L150-L185) | Reported RGH-console observations, not reproduced here. Format reasoning cites Xenia; hardware results are a distinct observation channel. The report notes uncertain kernel/patch provenance. Own packed images are fully covered; no overhang, tail owner or tail rollback test is reported. |
| R6 | Prior pinned IDA/database-loader study in Recovery III below | Reused static evidence, not another parser run. Database segment declarations and copied host bytes do not establish guest ownership. No additional wrapper is counted as independent proof. |

A–E remain distinct: **A**, Windows format definitions but no located Xbox tail
rule; **B**, explicit producer/loader code; **C**, emulator mapping choices;
**D**, preserved original synthetic SDK behavior; **E**, unverified console-tail
hypotheses. No cited producer or emulator is treated as the Xbox specification.

### Field relationships and competing explanations

Use symbolic quantities only: `B` is XEX load base, `D` is summed descriptor
extent using the selected page unit, `S` is security image size, `L` is verified
decoded payload length, and `E` is a declared PE section end. The preserved
private evidence has `D = S = L`, with one `E > B + D`; PE SizeOfImage is also
larger. Header base/load facts are retained unchanged. This gate reuses their
verified inventory; it does not decrypt, decompress or parse the image again.

R1's normal path allocates and decodes `D`; PE section spans are recorded later,
without another allocation. Its BASIC path has explicit zero spans within its
allocated image. Its patch-growth path follows changed XEX headers/descriptors
after a patch, not PE VirtualSize alone. None is a new normal-image tail rule.
The title update remains unapplied. The reviewed heap page unit and page-rounded
extent already agree in the original evidence; alignment does not authorize
another owner. Packer-specific page choices are not universal console rules.

R4 distinguishes RVA/VirtualAddress from PointerToRawData and defines
VirtualSize/SizeOfRawData padding and SectionAlignment/SizeOfImage. A readable
raw-file span at a different offset is not automatically backing at its declared
RVA in an already decoded XEX image. Generic PE zero-fill and discardability
supply neither an Xbox reservation nor permission to omit this particular range.
No separately allocated or demand-mapped tail rule is located.

| Hypothesis | Evidence and verdict | Unsupported action rejected |
| --- | --- | --- |
| H1: the declared PE extent need not be independently resident | Metadata/database recording without allocation exists. **UNKNOWN** whether this is the original console rule for the specific tail. | Dropping/trimming the section or treating metadata acceptance as safe load success. |
| H2: the platform creates extra tail memory | No applicable independent contract. Generic PE padding, producer-owned BSS and patched-image growth are different cases. **UNKNOWN; unsupported as an implementation basis.** | Growing from PE SizeOfImage/VirtualSize, undocumented zero-fill or demand mapping. |
| H3: the XEX needs an unsupported special representation | Flat-RVA basefiles and compression/patch paths exist, but none identifies a missing representation applicable to this unchanged base input. **UNKNOWN.** | Raw-offset aliasing, changing page units, applying an update or inventing remapping. |

These hypotheses are not proven false; each is rejected as grounds for changing
this loader. The current SDK's unsupported whole-section claim is reproducible,
but neither an invalid retail image nor a console-compatible SDK defect is proven.

### Eight unresolved tail contracts

The rule column is limited to the source's stated scope; **every original-tail
answer remains UNKNOWN**. High confidence in source inspection is not confidence
in an unmeasured console behavior.

| Question | Precise available rule/evidence | Applicability and confidence for this tail |
| --- | --- | --- |
| Reservation range | R1 normal allocation covers descriptor-derived `D`; R3/R5 producers include their sections before loading. | Implementation/producer facts verified; no authority for an extra original reservation. **UNKNOWN**. |
| Page commitment | R1 combines reserve/commit for its normal image. | No independent console commitment record beyond `D`. **UNKNOWN**. |
| Initial content | R1 zeroes/decodes inside its extent; BASIC has explicit zero spans; R4 defines generic PE padding. | No file/decode/zero/inaccessible selection proven for the original uncovered bytes. **UNKNOWN**. |
| Permissions | R1 uses XEX descriptor types; PE flags are separate declarations. | No tail page owner/type or actual console protection measurement. **UNKNOWN**. |
| Allocation owner | R1/R2 separate module image from guest header allocation. | No Xbox allocator owner, image reservation identity or separate-tail object established. **UNKNOWN**. |
| Adjacent allocations | Preserved SDK collision/foreign-neighbor negatives refuse and preserve neighbors. | Verified synthetic safety only; no original console neighbor trace. **UNKNOWN**. |
| Lifetime | Preserved SDK leases pin admitted ranges; R1 retains its normal image until unload. | No demonstrated tail retain/release lifetime. **UNKNOWN**. |
| Unload/rollback | Preserved SDK transactions balance known owners; R1 normal unload releases its base. | No exact tail cleanup on success, partial failure or unload established. **UNKNOWN**. |

R5's query follow-up matters to future measurement design: its image queries
returned invalid-parameter while another API reported image protection. That
reported refusal alone cannot prove an image address unallocated. Allocation
owner/commitment must come from a validated image-capable observation, not an
assumed query ABI. Ordinary protected images and heap statistics are not tail
ownership evidence.

### Decision, validation and smallest next action

| Separate decision | Verdict |
| --- | --- |
| Xbox section-tail residency semantics | **UNKNOWN** |
| SDK loader correctness | **PARTIAL**: supported owner/refusal/rollback contracts verified; console parity for this layout unknown |
| Safe original XEX load eligibility | **BLOCKED / NO-GO**; retain last measured `0xC0000001`, not retried |
| G4 image ownership | **BLOCKED** |
| G5 actual startup provenance | **UNKNOWN** |
| G8 actual exceptional-transfer enforcement | **BLOCKED** |
| Original private entry eligibility | **BLOCKED / NO-GO** |

No bounded loader correction is justified. The conditional synthetic-proof step
is **not applicable**: without a supported rule, a new successful fixture would
only demonstrate an invented policy. No new native tests or sanitizer runs are
claimed. Production SDK behavior is unchanged, so the preserved selected
Debug/Release/Sanitized matrix stays **1001/0**, with **125** separate Python units;
it is not rerun. Offline **8/8 stores**, **20/20 emitted/compiled references**,
**zero hook bodies**, and **two compiled bodies per profile** remain hash-sealed.
No hook implementation or private link is attempted. G1–G12 remain as recorded
in Recovery III; offline success changes none of these original admission gates.

The exact missing proof is an independently reproducible loader rule or trusted
console/debugger trace for this class of overhang, establishing all eight rows.
A useful evidence package needs input hashes, kernel/patch identity, a conventional
original control and a changed-overhang case, image-capable allocation/PTE/owner
observations, initialization/protection results, neighboring-owner preservation,
and successful/failed load/unload reclamation. Observing only load success,
protection bits or a memory dump is insufficient. No original game entry is
needed. The next action is **acquiring that observation channel and evidence**;
no new positive fixture or loader rewrite is started without the rule.

One concrete alternative for **future evaluation only** is a snapshot-backed
sparse guest-memory module with explicit per-range capabilities and denied gaps,
rather than a full-XEX-loaded-module contract. Prerequisites are independent
pre-entry byte/state/owner/protection/lifetime provenance and original synthetic
validation of its own binder and rollback. It would be a distinct module type,
never a successful XexModule or a bypass of the existing binder; it would not
close current G4 or establish original startup compatibility. No snapshot,
architecture implementation, hook body, private link or Phase 23 is started here.

The four requested documentation files are the only public edits in this gate.
All 38 existing dirty files, SDK patch locks, diagnostics and private outputs are
preserved. Preservation, documentation links, publication allowlist and absence
of new private identifiers are checked locally; detailed receipts stay private.
Phase 22 remains **BLOCKED/unpublished**. No completion commit, tag, push, CI
publication or GitHub Release is made.

## Recovery III: residency evidence and offline preparation

### Independent static loader study

**UNKNOWN original console residency; UNSUPPORTED BY CURRENT SDK.** The new
comparison reuses previously verified metadata and the independent pefile header
inventory. It does not repeat decryption, LZX or descriptor decoding. No Xenia,
IDA plugin or original title loader is executed. No SDK patch or lock changes.

The pinned [Xenia loader and size calculation](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.h)
use the descriptor sum for normal-image allocation. Its source reserves/commits
that extent, zeroes and decompresses inside it, records PE virtual spans without
the SDK's exact full-section owner check, and applies descriptor page permissions.
Unload releases the image base. A recorded PE span is not another allocation.
ReXGlue's XEX code shares Xenia ancestry, so their agreement is not independent
console evidence. The pinned source and relevant branches are retained privately.

The independently obtained [IDA loader](https://github.com/emoose/idaxex/blob/f388a502cbe84efd25e36d38f20d1a99cb2f166e/idaloader.cpp)
provides a different **analysis database** interpretation: declared segment size
may exceed copied bytes; available bytes are clamped to the decoded buffer, and
PE flags become database attributes. Its host vector uses the larger of stored
payload length and security image size. Neither a wider database segment nor a
host vector establishes Xbox reservation, commitment, initialization or lifetime.
Its optional named-section exclusions are not adopted. No missing tail is mapped,
zeroed, aliased or omitted by this recovery.

The original read-only [comparison tool](../../tools/compare_residency_models.py)
transcribes those specific pinned branches and reconciles all fourteen independent
PE entries against the existing inventory. Its output is a bounded static model,
not an executed reference loader or proof of emulator equivalence. Eight original
units cover disagreement, missing entries, permissions, extent/budget errors and
host-buffer-versus-owner separation. Detailed title-specific results stay private.

The existing page chain corroborates bytes inside descriptors. It does not prove
ownership of the uncovered tail. No independent rule establishes separately
allocated or implicitly zero-filled tail space, and no evidence proves the retail
input invalid. The result is **UNKNOWN** for the tail and **UNSUPPORTED BY CURRENT
SDK** for the complete section claim. Additional speculative loader work stops.
Resolving G4 requires a documented or independently demonstrated Xbox rule for
reservation/commit owner, exact initialization, permissions, neighbor interaction,
lifetime and final release across that specific declared range.

### Supported offline interface and exact scope

The CLI `recompile-tests` path uses a fixed analysis base and its own test config;
using it unchanged would not preserve arbitrary original PCs and hook configuration.
The SDK also exposes `BinaryView::fromModule`, the testing overload
`CodegenContext::Create(BinaryView, RecompilerConfig)`, explicit FunctionGraph
construction, `FunctionNode::emitCpp`, and `TemplateRegistry::render`.
Those actual production implementations are used, without a full-XEX loader.
This is **production core API generation**, not the complete CLI discovery/analysis
or whole-title generation pipeline.

An independently authored analysis-only Module data carrier supplies exactly two
noncontiguous read-only instruction slices. It has no Memory or dispatcher and
reports `is_executable=false`. BinaryView copies only those slice bytes. Its
coordinate envelope is metadata used by the SDK copier; gaps have no copied bytes,
guest allocation or inferred backing. It is not a successful or substitute XEX
module, and it cannot supply an image lease to the unchanged binder.

Private preparation checks the unchanged original XEX SHA-256, frozen manifest,
validator and historical decoded-image fingerprints. Exact selected bytes agree
with the preserved fresh pre-PE payload and all independently decoded frozen words.
Only the selected ranges enter the producer: nineteen admitted instructions plus
one independently identified, unexecuted boundary word. Original PCs, two ranges,
four blocks, nineteen normal edges and the existing namespace remain unchanged.
No state values, data sections, imports, extra function targets or shared suffix
entries are synthesized. The frozen manifest's old configuration binding remains
provenance; a separate hash identifies the actual offline API input/configuration.
It is not represented as execution of the historical CLI configuration.

The original generic [host generator](../../tests/offline-region/generator.cpp)
constructs exact sealed graph nodes/blocks and the admitted helper edge, supplies
production before-instruction hooks, and invokes the real emitter and embedded
SDK configuration template. Input buffers remain alive until BinaryView copies
them; section/function/block coverage and supported raw forms are bounded before
emission. No Runtime, XexModule, guest dispatcher or generated function is invoked.
All original words and resulting source/objects/symbol maps remain private.

### Stores, hooks and boundary correspondence

**VERIFIED static eight-store accounting: 8/8.** The independent auditor verifies
one expected U32/U64 checked store per original site, exact source/value registers,
signed effective-address arithmetic and update-form stack semantics. It requires
no extra/missing memory operation, exact non-memory scalar assignments, explicit
NOP behavior, original LR/helper/return anchors, and no raw pointer or unknown
normal operation. Actual preprocessing selects the existing checked-memory layer
and BoundedCfg direct/indirect admission macros. Native undefined symbols contain
no GuestToHost/function-lookup fallback. This is static/compile-only evidence;
private runtime-observed stores remain **zero**.

**PARTIAL hook integration:** requested **20**, production-emitted call sites
**20**, independently reconciled PC/phase/identity references **20**, compiled
external references **20**, linked hook implementations **0**, runtime-observed
private hits **0**. Canonical events/index order and frozen namespace remain in
unchanged authority data. External hooks deliberately remain unresolved: no
successful runtime policy integration is fabricated from source references.

Production emission retains the unexecuted boundary instruction's unknown-call
error diagnostic **after** its before-hook and unconditional `return`. The auditor
checks that ordering and treats the residue as unreachable, not an admitted call
or invented target. It is preserved in emitted source; no generated C++ is edited.
The selected nineteen admitted operations contain no such fatal/unknown path.
The independently generated wrapper only includes declarations, the actual SDK
config and verbatim emitted bodies, guarded as compile-only. It supplies no hook
stubs, synthetic initial registers, import fallback or runtime entry protocol.

### Nonexecuting native artifacts and admission separation

Two actual emitted function bodies compile into private objects at O0, O2 and
O2+ASan/UBSan using matching patched SDK headers. Repeated production generation
must be byte-identical. The object symbol audit checks every required external
hook reference; unresolved hooks, checked-runtime/C++ dependencies and exact
source/object/input/archive hashes are recorded privately. No native executable
is linked, no object or generated library is loaded, and no proprietary function
is called. Compilation establishes the selected context/base declarations and
host syntax only, not complete Xbox ABI, initial state or native sandboxing.
Typed-vector signatures and guest exception dispatch/unwind remain unsupported.

Independent outcomes: residency **UNKNOWN/BLOCKED**; offline extraction
**VERIFIED**; production-core correspondence **VERIFIED**; eight checked stores
**VERIFIED statically**; twenty-hook implementation **PARTIAL**; native object
compilation **VERIFIED compile-only**. Original image loading **NO-GO** and private
guest execution **NO-GO**. The explicit offline admission function always rejects,
including forged loaded/owner flags; no offline artifact converts to an image
lease or skips the original binder. The last measured original load remains the
preserved `0xC0000001`; it is not retried here. G4 and G5 are not improved by
successful offline generation.

| Gate | Recovery III status | Scope |
| --- | --- | --- |
| G1 | PASS | Frozen original identity and graph unchanged |
| G2 | PARTIAL | 8/8 offline checked stores; live binding absent |
| G3 | PARTIAL | Exact static normal-operation grammar; loaded/runtime coverage absent |
| G4 | BLOCKED | Full original-image ownership unresolved |
| G5 | UNKNOWN | Actual title startup provenance absent |
| G6 | PARTIAL | Original helper/LR/return statically reconciled; live admission absent |
| G7 | PARTIAL | 20 emitted/compiled references; zero linked hook implementations |
| G8 | BLOCKED | Actual private exceptional-transfer enforcement absent |
| G9 | PARTIAL | No admitted normal imports; no loaded private continuation policy |
| G10 | PASS | Existing bounded supervisors retained; compile-only private work |
| G11 | PARTIAL | Genuine private object audit; no linked/bound complete artifact |
| G12 | PASS | Preservation and private/public artifact boundaries retained |

### Recovery III final regression evidence

The usage-limit interruption left final regression reconciliation, independent
store/hook recount and publication review unfinished. Existing extraction,
production emission, private compile outputs and earlier results were inspected
and retained; completed private generation was not repeated on resumption.

**VERIFIED selected final result: 1001 core PASS / zero FAIL.** Composition is
752 prior Phase 5–21 checks, three new host synchronization groups (128 rounds
per profile), 66 entry-audit, 57 loader/context, 45 residency and 78 offline
generation/audit groups. Current fixture/source receipts, staged runtime hashes,
manifest/object hashes and the unchanged strict CPU-context oracle are checked.
This is an explicitly selected matrix: the completed workflow is retained in
full, with only its three synthetic-suite results replaced after the correction
below. It is not represented as one wholly successful workflow invocation.

| Profile | Core PASS | FAIL | Core child seconds | Compile child seconds | Peak core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 334 | 0 | 176.781436 | 117.779669 | 486,883,328 |
| Release | 334 | 0 | 179.121619 | 144.398785 | 440,074,240 |
| Sanitized | 333 | 0 | 209.671574 | 154.129307 | 535,511,040 |

Child seconds sum supervised compiler/runtime intervals; compilation is a subset.
Maximum individual wait4 RSS is not concurrent-process RSS, game RAM or Switch
performance. Selected outer profile-suite totals, including separate component
and VMX controls, are 189.740776 / 199.472387 / 225.707197 seconds. These exclude
historical failed/superseded attempts and are not a whole-session wall clock.
Client configurations remain O0/O2/O2+ASan+UBSan; the Sanitized SDK remains
O1+ASan+UBSan, with existing framework/assembly limits.

Initial Recovery III workflow: **997 PASS / 1 FAIL**, one unexpected Release
SIGABRT, zero unexpected external-watchdog timeouts and no reported sanitizer
finding. Its synthetic `release-before-exit` child failed after 5.180532 seconds
with `fixture gate timeout`; peak RSS was 23,199,744 bytes. The saved stack sample
shows main and worker waiting on the same condition variable. The fixture wrote
`at_gate` and `release` outside the wait mutex; atomic predicates alone do not
prevent a notification between the waiter's predicate check and wait transition.
Both writes now use that mutex. SDK/XThread ownership code, strict teardown
assertions and five-second internal limits are unchanged. The affected synthetic
suite alone was rebuilt/rerun in all three profiles: **21/21 each**, including a
new bounded 128-round host-only regression. Its outer times are 8.872315 /
7.709174 / 11.266129 seconds, under 180-second suite limits and unchanged external
30-second child limits. Original raw failure, binaries, receipts and samples
remain private; the failure is not erased or attributed to a new SDK defect.

Selected final unexpected FAIL/signals/timeouts are **0/0/0**; reported sanitizer
findings are **zero**. The 68 expected fault checks include 44 SIGBUS, three
SIGSEGV, nine SIGABRT and twelve capture/recovery checks; nine separate expected
join fail-stop exits are 70. The existing ASan-owned default-signal exclusion
explains the one-count profile difference; no new exclusion or suppression is
added. Watchdog units deliberately test containment timeouts separately.

Separately pass **125 Python units** (including eight new loader-model units),
27 component/source-control groups, 33-import/signature audits, the Debug
reference trace and three VMX characterizations of 212,068 calls each. All 54
prior loader cases and 45 residency cases preserve refusal and final ownership
balance. All 63 incomplete context candidates and 51 source mutations still
refuse. Offline original fixtures pass 26 groups per profile, including 22
adversarial input/admission/source cases; zero generated proprietary calls occur.

A second private census imports neither the pipeline auditor nor its decoder.
It decodes store fields directly from the frozen words, independently checks each
signed EA/register/width/update operation, and counts source declarations/calls
and native undefined/defined symbols. In every profile it confirms **8 stores
(4 U32 + 4 U64), 20 declarations/calls/compiled references, zero hook definitions,
2 compiled bodies and zero execution**. Repeated production source/config outputs
are byte-identical both within and across profiles. Exact identities and hashes
remain private. Separate private compile/generator child totals are 7.820205 /
6.110379 / 10.432791 seconds; peak individual RSS is 398,540,800 / 476,463,104 /
525,910,016 bytes. These compile-only checks are not runtime ABI or image proof.

### Recovery III preservation and publication boundary

The pre-recovery 31-file public state and complete prior private workspaces are
preserved. Content comparisons cover 102,677 current-prior, 11,398 Phase 22 and
47,201 Recovery I files with zero differences. A further snapshot preserves the
resume state before the synthetic fixture fix. The 1,455 input fingerprints,
299 extracted files, original XEX hash, frozen Phase 19–21 inputs and installed
SDK remain unchanged. No SDK source, patch or lock is changed by Recovery III.

The intended review contains 38 changed files; a private temporary Git index
checks 228 allowlisted public files and documentation links. The real index stays
empty. Only original tooling/fixtures, reviewed SDK patches and documentation are
included; original bytes/PCs, generated proprietary C++, objects, symbol maps,
private logs and local paths are excluded from the intended diff. The identifier
scan also finds pre-existing entry metadata in three unchanged historical documents;
those files equal HEAD exactly and introduce no new disclosure in this recovery.
The initial overbroad scan and its classified findings remain private. The allowlist
and targeted scans are limited checks, not complete copyright or secret detection. No commit,
tag, push, CI publication or Release is made. Phase 22 remains **BLOCKED** and
Phase 23 is not started.

### Reproduction and retained development evidence

```sh
python3 tests/offline-region/test_models.py
python3 tests/offline-region/run.py --sdk "$SDK" --source "$PINNED_SOURCE" --workdir "$OFFLINE_ORIGINAL_TESTS"
# Add --sanitize only for a matching Sanitized SDK.
python3 tests/xex-binding/workflow.py --sdk-workspace "$RESIDENCY_SDK" --lock tools/sdk/residency-lock.json --workdir "$FRESH_REGRESSIONS"
python3 tools/compare_residency_models.py --metadata "$PRIVATE_INVENTORY" --independent-pe "$PRIVATE_PEFILE_REPORT" --stored-payload-bytes "$STORED_SIZE" --report "$PRIVATE_COMPARISON"
```

Public sources contain only generic original tooling and original fixtures. The
actual frozen-slice extraction/production commands and inputs are retained in the
ignored private workspace with hashes. Every compiler, generator and object-tool
child has the unchanged external 30-second process-group watchdog; suite and static
supervisors also remain finite. No globally installed SDK, prior patch/lock or
strict CPU-context oracle is modified.

Development evidence is retained: the first audit counted an SDK diagnostic comment
as an instruction; a read-only audit view now distinguishes the exact boundary
diagnostic without rewriting output. Mutation review exposed an unchecked leading
translation-unit fragment and incomplete declaration accounting; strict prelude,
declaration, instruction and scalar/store checks now reject them. An intended
unsupported-word mutation initially changed a register bit into another valid
instruction; it now uses an explicit unsupported word. The first private
preparation refused an unlisted NOP before generation; only that verified standard
form was added and checked against its actual production emission. No original
SDK failure is suppressed and no unsupported instruction is silently accepted.

## Recovery II: independent section residency evidence

**VERIFIED D: the complete section claim is unsupported by the current safe
SDK image contract. UNKNOWN E: the console loader rule for its uncovered tail
is not established.** Recovery II preserves the existing twenty-one-patch SDK
unchanged as historical evidence. New synthetic UBSan evidence requires a separate
integer descriptor-preflight correction under a new twenty-two-patch lock. It
adds no mapping or execution admission path. Original XEX SHA-256
is reverified as
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The previous 25 public files and both prior private Phase 22 workspaces were
snapshotted before further work; all earlier failures remain available privately.

### Evidence independence and agreement

1. A separate pinned [pefile 2024.8.26 header parser](https://github.com/erocarrera/pefile/releases/tag/v2024.8.26),
   used with fast header loading, agrees with the original explicit struct
   decoder on all fourteen sections, the image base, both alignments, raw-file
   spans and flags. Both identify one virtual section crossing the descriptor
   extent. They share a decoded input, so this is independent **header parsing**,
   not independent decompression. Generic Windows warnings are retained privately
   and are not treated as Xbox validity judgments.
2. An independently authored bounded big-endian XEX framing frontend uses Apple
   CommonCrypto AES rather than the SDK AES implementation. All 97 compressed
   block SHA1 values agree; 430 chunks are recovered. Keys are never recorded.
   A host-buffer-only probe decompresses these chunks through the pinned
   SDK/libmspack LZX routine. The exact security-extent output equals the fresh
   SDK pre-PE payload. This shares an LZX backend and is **not a second decoder**.
   Native output requests extending to either the last claimed section end or
   PE SizeOfImage terminate with decoder read/EOF error 3 (probe exit 5).
   Their partial outputs are untrusted, not evidence of zero-fill or ownership.
3. A separate Python/hashlib implementation verifies the page-descriptor chain,
   following the primary [reverse-engineering equation pinned at d83fe901](https://github.com/landaire/acceleration/blob/d83fe901b68d5b21e3cc29af7bf297f0555aff65/crates/xex2/src/page_descriptors.rs).
   Every one of **215 descriptors** agrees. For each descriptor the verifier
   hashes its covered decoded bytes followed by its raw descriptor, compares
   against the preceding expected hash, then advances that expected hash from
   the descriptor. The root comes from XEX security information. This independently
   corroborates decoded bytes across the entire descriptor extent; it does not
   verify the RSA signature, console authenticity or residency beyond that extent.
4. Pinned SDK source and original production loader regressions establish the
   implemented allocation/commit/rollback/lease contract. Static metadata, chain
   validity and permissive parser acceptance cannot substitute for a live owner.
   No located public loader rule proves a separate allocation or implicit tail
   for this input. The PE specification does not supply that Xbox-specific rule.

The security image extent, descriptor sum and successful decoded output agree.
PE SizeOfImage and the last claimed virtual end exceed that backing. Header
flags and page alignment do not explain the excess. The problematic section has
nonzero descriptor-backed bytes and intersects both declared resource claims;
its raw-file span lies inside the decoded buffer at a different virtual position.
Neither discardability nor that raw span proves permission to drop, alias,
remap or zero the section. Detailed ranges, section identities, resource names,
page types and fingerprints stay private. No extra guest mapping was attempted.

### Candidate models and byte ownership

| Model | Finding | Ownership consequence |
| --- | --- | --- |
| A: ordinary image pages | VERIFIED for the descriptor-covered prefix; not the full section | Prefix has corroborated bytes and descriptor page types; a successfully loaded live owner is still absent |
| B: separately allocated extension | UNKNOWN | No independent reservation owner, commitment, permissions, initialization, lifetime or unload rule found; no implementation justified |
| C: legitimate unbacked/zero region | UNKNOWN for the tail | No authority to create or expose those bytes; nonzero/resource-bearing prefix must not be erased |
| D: unsupported full-section claim | VERIFIED for this SDK contract | The current exact owner/section predicate must continue refusing |
| E: insufficient console semantics | UNKNOWN / remaining blocker | Chain consistency does not establish how the console represents the excess declaration |

Descriptor page types are metadata permission claims. PE flags are separate
claims; neither is a measurement of final permissions on a successfully loaded
original module. No such module exists in this recovery. For the missing tail,
reservation/committed owner, initialization, actual permissions, lifetime,
neighbor interaction and unload behavior remain UNKNOWN. No accessible byte is
admitted on that basis. Possible intended console behavior is not asserted.

**Safe residency solution: retain refusal.** An independently reproduced
invalid-enum parser defect is corrected separately as described below; it does
not supply missing image ownership. This identifies a
reproducible unsupported whole-section ownership claim, not a proven corrupt
retail image, faulty LZX implementation or arithmetic error. A documented,
independently demonstrated residency rule is still needed before a residency
correction.

### Newly reproduced descriptor parser defect

The first complete Recovery II matrix retains **913 core PASS / one FAIL**,
including a real Sanitized failure in the new descriptor-kind fixture. Twelve
other cases in that failed suite pass in its detailed receipt; the outer workflow
rollup omits the failed suite entirely, so it is not a complete per-case total.
A four-bit descriptor value of fifteen is outside
the valid representation range of the C++ `xex2_section_type` enum. The existing
preflight reads `desc.info` to reject it, but that enum load itself triggers
UBSan before a safe return. That child exits with unexpected SIGABRT (-6), not a
normal rejection. A detailed recount corrects the initial private summary
which had assumed zero unexpected signals; both records remain preserved.
Unsanitized rejection did not establish defined behavior. No failure is
suppressed or renamed expected.

Patch 22 reads the endian-converted descriptor as `uint32_t`, masks the type and
shifts the page count, then range-checks those integer values before any later
enum view can be reached. It preserves valid types, extent arithmetic, owner
bounds, permissions and rollback. A fresh isolated SDK uses the separate
[residency lock](../../tools/sdk/residency-lock.json), retaining the first 21
patches byte-for-byte and leaving all prior SDK workspaces untouched. This is a
confirmed parser correction (solution A), independent of the unresolved D/E
residency outcome. The malformed original fixtures are the regression authority;
no proprietary load or codegen retry is required to demonstrate this fix.

### Original fixtures and boundaries

The new production-loader suite adds fifteen cases per profile: ordinary image,
BASIC-compressed explicit in-owner zero-fill, missing payload pages, zero page
count, descriptor types zero/four/fifteen, wrapping descriptor claims, misaligned image base, unsupported permission flags,
overlap, an overrun into a foreign neighboring owner, valid neighboring ownership,
eight repeated load/unload cycles and active-reference unload. Five accepted
case groups exercise twelve module lifetimes per profile. The BASIC fixture
supplies explicit data/zero lengths **inside existing descriptor ownership**;
it establishes no outside-image zero-fill rule. No separate-allocation positive
fixture is fabricated because model B remains unproven.

All accepted fixtures check exact owner bounds, read-only write refusal, lease
closure, final image/virtual page balance and, where applicable, actual zeroed
bytes. The adjacent foreign allocation survives accepted load/unload and rejected
overrun, retains its data and is released with its original owner token. Existing
allocation collision, header-allocation zero/throw, partial-load rollback,
section-boundary and byte-permission cases remain required. Host-page protection
still cannot isolate every 4-KiB guest subpage on a 16-KiB host; these checked
copy tests are not complete native/guest isolation.

Twenty separate pure-Python fixtures exercise static chain integrity, corrupt
root/link/page bytes, metadata bounds, missing/extra backing, wrap, overlap,
resource bounds and invalid descriptors. These are analysis-tool tests, not
console signature or loader acceptance evidence. The original synthetic data
and sources are redistributable; no game-derived fixture is published.

### Original loader, instrumentation and context gates

The original nonexecuting loader is **not retried in Recovery II**: its missing
ownership prerequisite has not been established. The earlier three-profile
`0xC0000001` refusals and reclaimed page counts remain the latest measured
original loader results. Their historical SDK library, headers and patch lock remain
unchanged. The integer-preflight correction has no original-image retest; no
new successful-load or permission measurement is inferred.

Private production codegen remains deliberately **BLOCKED**. The frozen region
retains nineteen instructions, eight stores and twenty requested hook identities;
new private checked stores are **0/8**, emitted private hooks **0/20**, compiled
private guest functions **0**, and game functions invoked **0**. This recovery
makes no new proprietary C++, object or executable. G5 remains UNKNOWN even
though the unchanged strict context observer and full scalar/vector/default
field regressions are rerun on the actual SDK. Typed-vector signatures, complete
Xbox ABI and guest exception dispatch/unwind remain unsupported or unverified.

G1/G10/G12 retain PASS; G6/G9 PARTIAL; G5 UNKNOWN;
G2/G3/G4/G7/G8/G11 BLOCKED, using the unchanged definitions below. A passing
synthetic matrix cannot waive original-image, actual generated-code, startup
provenance or exceptional-transfer gates. Entry remains **NO-GO**. The next task
is still Phase 22 residency evidence; Phase 23 is not started.

### Reproduction and retained diagnostic corrections

```sh
python3 tests/entry-audit/test_residency.py
python3 tests/entry-audit/residency.py --sdk "$SDK" --workdir "$RESIDENCY_RESULTS"
# Matching Sanitized SDK requires --sanitize.
python3 tools/sdk/build.py --workspace "$RESIDENCY_SDK" --lock tools/sdk/residency-lock.json --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$PINNED_CACHE" --interposable-runtime
python3 tests/xex-binding/workflow.py --sdk-workspace "$RESIDENCY_SDK" --lock tools/sdk/residency-lock.json --workdir "$FRESH_TEST_RESULTS"
# Static report is private. This command never loads or executes an image.
python3 tools/audit_xex_residency.py --xex "$ORIGINAL_XEX" --image "$DECODED_IMAGE" --report "$PRIVATE_REPORT"
```

Runtime/compiler children retain external monotonic 30-second process-group
watchdogs; suite supervisors remain finite. Static reports require fresh private
output names. Complete exact local commands, source/dependency hashes and native
records remain private. Two initial static scripts accidentally reused their
watchdog record JSON name; both were reverified with distinct result/record names,
without changing earlier artifacts. A host-only decoder probe initially tested
ifstream EOF state before reaching LZX; the corrected file-open check and all
failed diagnostic attempts are retained. A naive standalone page-SHA1 hypothesis
matched none of the descriptors; applying the actual chained equation yields
215/215. The rejected hypothesis is not reported as input corruption. A private
preservation script used an unavailable Python hashing convenience and was
corrected to streaming hashing; its first failure is retained.

### Recovery II regression evidence

**VERIFIED final matrix: 920 core PASS / zero FAIL**, consisting of 752 historical
Phase 5–21 groups, 66 instrumentation-audit groups, 57 prior loader/context groups
and 45 new residency groups. All suites are freshly rerun against the new coherent
22-patch SDK in Debug, Release and Sanitized. This is a complete successful
workflow invocation, not a selection that substitutes passing suites into the
first failed run. Initial 913/1, its one UBSan finding and one unexpected
SIGABRT remain separately retained. The later signal recount supersedes an initial incorrect private
summary; no original record is rewritten as a pass.

| Profile | Core PASS | FAIL | Core child seconds | Compile child seconds | Peak core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| Debug | 307 | 0 | 165.627921 | 106.844531 | 375,898,112 |
| Release | 307 | 0 | 142.690541 | 112.750737 | 398,475,264 |
| Sanitized | 306 | 0 | 188.303694 | 138.404155 | 493,207,552 |

Times sum supervised compiler/runtime children; compilation is a subset. RSS
is maximum individual core-child wait4 usage, not total concurrent processes,
game RAM or Switch measurements. Separate component, VMX and Python controls
are not folded into those core counts. Client profiles are O0/O2/O2+ASan+UBSan;
Sanitized SDK remains O1+ASan+UBSan with no recovery. System frameworks and
assembly retain their existing instrumentation limits.

All 45 new loader cases pass, including thirty invalid-case refusals and
36 accepted module lifetimes; all verify final measured image/virtual page
balance. The previous 54 loader cases also pass unchanged. The original strict
context observer is byte-identical to its preserved source and passes in every
profile, including actual stack/TLS/PCR/image reclamation. All 63 incomplete
semantic-field candidates and 51 production-source adversarial candidates refuse.

Final unexpected FAIL/signals/timeouts are **0/0/0** and reported sanitizer
findings are **zero**. Historical expected fatal controls remain 44 SIGBUS,
three SIGSEGV and nine SIGABRT, plus twelve capture/recovery checks and nine
separately expected join fail-stop exits 70. The existing ASan-owned default-
signal exclusion explains the one-count profile difference; no new test is
excluded. Watchdog self-tests intentionally test timeouts as containment controls.

Separately pass **117 Python units**, 27 component/source-control groups,
33-import/signature audits, the Debug reference trace and three VMX
characterizations of 212,068 calls each. These results do not establish complete
ABI, race freedom or safety of unexecuted proprietary code.

The focused corrected loader suite independently passes 15/15 in each profile
before the full rerun. New SDK builds take 108.174794 / 133.334043 / 216.974774
seconds with build-command peak RSS 582,402,048 / 549,404,672 / 936,607,744 bytes.
The new ordered lock SHA-256 is
`edd55c8d89851303d779e27f785dd1bcc6141c6d06cba913fb470e11d5ae7bb2`.
Its first 21 patch records equal the preserved recovery lock exactly.

Original/frozen inputs, 299 extracted files, 1,455 preservation fingerprints,
the installed SDK and both earlier private workspaces are reverified unchanged.
Content comparisons cover 11,398 and 47,201 prior private files. Original 25 public
file snapshots match their prior review fingerprints. The intended current diff
contains 31 files; an external temporary index reviews all 223 allowlisted public
files and documentation links, keeping the real index empty. Private mappings,
logs, game bytes, generated proprietary C++, objects and temporary artifacts are
excluded. The allowlist and scans do not constitute a complete copyright/secret
audit. Useful diagnostics and complete command receipts remain private.

Phase 22 stays **BLOCKED**, entry **NO-GO**. No commit, tag, push, CI publication
or GitHub Release is made. The next task is still an independent, documented
Xbox section-residency/initialization/ownership rule with a faithful fixture;
only then may the original nonexecuting loader and private generation gate be
reconsidered. No Phase 23 or game execution is started.

## Historical Recovery I: loader contract and context correction

The original fifteen public work files were snapshotted privately before edits;
the prior private Phase 22 tree, diagnostics and 817 PASS / 1 FAIL result are
preserved. Original inputs and the installed SDK remain immutable. Recovery
uses a separate SDK/source/build/staging workspace and the new deterministic
[twenty-one-patch lock](../../tools/sdk/entry-audit-lock.json). Its first nineteen
entries are identical to the preserved lock. Ordered patch reconstruction,
dependency pins and staged library/header hashes are independently checked.

Both original failures were reproduced before changes: loader child exit 4 and
unchanged strict Sanitized observer exit 1. All diagnostic/runtime children use
external monotonic 30-second process-group watchdogs. No private guest function
is linked or dispatched by these probes.

### Loader diagnosis and supported scope

**VERIFIED: this layout is unsupported by the current safe SDK contract.**
**UNKNOWN: a safe new tail/discard/remap contract is not established.** A valid
layout incorrectly rejected by a proven ownership interpretation has not
been established. These findings do not establish that the retail input itself
is corrupt.

A fresh production decryption/decompression is captured as data before PE
processing, without changing the original method's result. Its length agrees
with both the XEX security extent and summed page descriptors. The current
image base, PE section table, frozen selected code and all other bytes except
thirteen type-0 import slots agree with the historical loaded view. Fifty bytes
differ within those slots; the historical image has loader fixups. Whole-image
hash equality is **not** claimed. The first overstrict equality assertion and
its actual difference inventory remain private development evidence.

The refused section's virtual extent exceeds the verified allocation, with no
address wrap or overlap explaining it. Its raw-file span lies inside the decoded
buffer, but raw offsets differ from virtual RVAs: those bytes cannot simply be
treated as backing at another guest address. The relocation directory repeats
the offending virtual extent. Its discardable flag does not establish authority
to omit the span or expose padding. Page rounding cannot close the discrepancy.
Exact section names, addresses, tables, values and dumps remain private.

The [PE specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
distinguishes raw-file offsets, virtual extents and discardability. It supplies
no Xbox-specific ownership rule for this mismatch. The upstream
[Xenia loader](https://github.com/xenia-project/xenia/blob/master/src/xenia/cpu/xex_module.cc)
allocates the decompressed XEX extent; its more permissive section metadata
parsing is not evidence that the missing virtual span is owned. Neither source
justifies enlarging this image or weakening the frozen binder.

Original loader-only fixtures independently exposed acceptance of overlapping
sections and unreadable PE declarations in the nineteen-patch SDK. Patch 21
adds sorted byte-range overlap refusal and requires READ for the supported
checked-image contract, before PE sections are published. This is a supported
subset restriction, not a claim that every other PE flag combination is invalid
on Xbox. No successful mapping's effective page permissions are changed.
Out-of-owner rejection, real allocation tokens, rollback and lease lifetime
remain intact. No missing span is allocated, no section is dropped and the strict
binder policy is unchanged.

Final review independently reproduced a third issue: a short uncompressed
payload was accepted while a PE section extended into rounded allocation padding.
Patch 21 now requires uncompressed payload length to equal the validated XEX
page-descriptor extent, rejecting both missing and extra payload bytes before
allocation. This is the explicitly supported uncompressed layout contract; it
does not infer missing backing from page rounding. Basic compression retains its
explicit zero-fill contract and the original zero-fill regressions. The first
completed intermediate 869 PASS / zero FAIL matrix and its exact SDK binaries,
lock and source snapshots are preserved; they predate this additional correction
and are not substituted as final evidence.

The original image is still rejected with `0xC0000001` in all three rebuilt SDK
profiles; all measured image/virtual page counts return to baseline. Therefore
the recovery prerequisite for private production codegen is not satisfied.
**No new private codegen attempt is made.** The earlier bounded CLI failure is
retained; new private C++, checked stores, emitted hooks, compiled guest functions
and guest execution all remain zero.

### Exact context defect and correction

**VERIFIED SDK defect:** `Register` is an eight-byte union whose first member
is `s8`. The observed aggregate-member initialization in the actual ThreadState
constructor writes one byte of each scalar union, leaving seven observable
register bytes at their prior storage contents. This affects GPRs, FPR bit
storage, CTR and reserved scalar storage; it is not structure padding.

Patch 20 explicitly assigns the complete `u64` member of all 66 scalar register
unions in the actual constructor, then applies the caller's SP/PCR to r1/r13.
It preserves context layout and existing defaults for LR, CR/XER, vector arrays,
SDK MSR policy, FPSCR storage and dispatch fields. It does not memset the context
or alter per-thread host FPSCR initialization. The upstream
[ThreadState source](https://github.com/xenia-project/xenia/blob/master/src/xenia/cpu/thread_state.cc)
also initializes register storage before applying SP/PCR, supporting an SDK
default-state contract; it does not establish real console launch inputs.

The exact original strict Sanitized observer is unchanged and now passes,
including reclaimed stack/TLS/PCR and image lifetime. The representation
diagnostic now observes 240/240 examined GPR bytes zeroed, zero retained prefill.
A separate field oracle checks 32 GPRs, 32 FPR bit patterns, 128 vectors, LR/CTR,
CR/XER, SDK MSR policy, FPSCR storage and dispatch fields under three prefills.
Seven deliberately incomplete semantic-field candidates per prefill must refuse.
No padding comparison or arbitrary title initialization value is used.
Coverage is limited to fields modeled by the pinned `PPCContext`; it does not
establish every architectural system register, full Xbox ABI conformance or
unsupported guest exception/unwind behavior. Typed-vector marshalling remains
unsupported. Host FPSCR initialization is a distinct existing per-thread stage.

**UNKNOWN real title startup:** r1's real stack owner/layout, initial LR,
consumed GPR values and current private XThread/PCR/TLS/module instances remain
unproven. r2/r13 are not dereferenced in the frozen normal words, but supplied
state preservation and legitimate runtime ownership still matter. SDK scalar
initialization correctness and original-synthetic startup correctness do not
establish title startup provenance. Ordinary XThread allocation still does not
register the checked harness's owner loans automatically. No game values or
missing-owner compatibility behavior are fabricated.

### Recovery test interpretation

The loader-only matrix covers eighteen original cases: contained sections,
sub-page section boundaries, zero-filled data, owned holes, overstated PE extent,
unbacked/discardable extensions, overlap, integer wrap, unsupported unreadable
flags, short/extra uncompressed payloads, reservation failure, partial rollback
and active leases. Accepted cases
check exact image bounds, checked byte permissions, outside-section refusal,
lease-blocked unload and final image/virtual balance. An owned hole can remain
mapped at the XEX page level; checked image copies refuse it. This is not full
guest isolation or arbitrary sparse-image support. Historical header allocation
zero/throw and transactional ownership regressions remain required.

The first recovery Debug metadata test expected loader acceptance of two
overlapping variants before binder refusal. The stricter loader rejected them
earlier, causing that oracle and its dependent audit to fail. Those results are
retained. The two variants now **require** early loader refusal, no lease,
processor or guest header, and allocation rollback. No candidate is newly
accepted. Independent original loader tests support this stronger oracle. The
original strict initial-context assertion remains byte-for-byte unchanged.

The final matrix likewise exposed an older rollback variant that expected an
enlarged descriptor extent to reach PE continuation. Payload validation now
refuses it during Load. That negative now requires the earlier refusal, retaining
unloaded state, empty metadata, allocation balance and successful subsequent valid
reload checks. The failed Debug image run remains retained; the affected image
suite is reverified, and Release/Sanitized use the corrected oracle. No failure
is renamed expected and no malformed candidate becomes accepted.

### Final recovery evidence

**VERIFIED selected current-source regressions: 875 core PASS / zero FAIL.**
The Phase 5–21 suites contribute 752 PASS, the Phase 22 instrumentation audits
66 PASS, and the new loader/field suite 57 PASS. The historical suites retain
their cases; the two overlap variants and descriptor/payload variant now require
the independently demonstrated earlier refusal, as explained above.

| Profile | Historical PASS | Entry audit PASS | Recovery PASS | FAIL | Core child seconds | Compile child seconds | Peak core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Debug | 251 | 22 | 19 | 0 | 156.631542 | 100.437520 | 371,736,576 |
| Release | 251 | 22 | 19 | 0 | 142.408334 | 110.619274 | 401,850,368 |
| Sanitized | 250 | 22 | 19 | 0 | 180.608665 | 130.654115 | 498,810,880 |

Time sums supervised core children, not calendar project duration. Compile time
is a subset; RSS is the maximum individual core child, not process-tree/game
memory. Separate final read-only positive source/table/IR audits take
0.008614 / 0.006893 / 0.007422 seconds (including reads; excluding negatives).

The final evidence is the complete three-profile matrix after the payload fix,
with only its affected Debug image suite reverified against the strengthened
early-refusal oracle. The retained original matrix still records that Debug
image failure; it is not rewritten as a successful workflow invocation. All
selected executable-source receipts, SDK and object hashes match current code.
There are no post-test documentation receipt differences in this final matrix.
The earlier 817/1 and intermediate 869/0 runs remain separately identifiable.

All 54 new loader cases pass, including 36 refusals and eighteen accepted
lease lifetimes; every case balances measured image/virtual pages. The unchanged
strict context observer certifies one additional reclaimed stack/TLS/PCR/image
lifetime in each profile. All 63 incomplete-field candidates refuse the field
oracle. All 51 production-source adversarial candidates refuse before execution.
Original production fixtures reconcile 24 sites, ten scalar stores and 21
normal edges, separate from the zero private instrumentation count.

There are zero unexpected native signals, zero unexpected timeouts and zero
reported ASan/UBSan findings. Historical expected outcomes remain 44 SIGBUS,
three SIGSEGV, nine SIGABRT and nine separately expected join exits 70; fatal
paths do not claim graceful cleanup. The existing Sanitized default-signal
exclusion is unchanged. Watchdog self-tests deliberately exercise timeout
containment. Separately pass 97 Python units, 27 component/source-control groups,
33-import/signature audits and three VMX characterizations of 212,068 calls each.
These scopes do not prove race freedom, complete ABI or private native safety.

The actual private loader continues to refuse with exit 4 in
1.194198 / 0.412483 / 0.952709 seconds, peak RSS
58,753,024 / 53,510,144 / 98,664,448 bytes (Debug/Release/Sanitized).
All three restore the measured image/virtual counts. No proprietary function
executes or compiles, and no private production generation is retried.

Final recovery patch-lock SHA-256:
`09ae65dbe175bc48783afde0ddc84f8cd852cb6f3eab9b05d12651f68403fc52`.
The first isolated build takes 105.977 / 136.833 / 226.610 seconds; the final
payload correction rebuilds incrementally in 1.684 / 1.632 / 3.322 seconds.
Build records retain compiler/dependency pins and peak resource usage separately
from test figures. No global installation or protection bypass is used.

Reproduction uses a new, private SDK workspace; installed or historical SDK
workspaces must not be reused as build output:

```sh
python3 tools/sdk/build.py --workspace "$RECOVERY_SDK" --lock tools/sdk/entry-audit-lock.json --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$PINNED_CACHE" --interposable-runtime
python3 tests/xex-binding/workflow.py --sdk-workspace "$RECOVERY_SDK" --lock tools/sdk/entry-audit-lock.json --workdir "$RECOVERY_TESTS"
python3 tests/entry-audit/recovery.py --sdk "$SDK" --workdir "$LOADER_CONTEXT_RESULTS"
# For matching Sanitized SDK inputs, add --sanitize to the per-suite runner.
```

Actual private section acceptance, eight-store instrumentation, twenty emitted
hooks and a genuine nonexecuting guest compile/link remain **BLOCKED**. G1/G10/G12
retain PASS; G6/G9 PARTIAL; G5 UNKNOWN; G2/G3/G4/G7/G8/G11 BLOCKED. Each PASS is
linked to hashed local evidence in the private recovery checklist. Game-entry
admission remains **NO-GO**. Phase 23 is not started. No Phase 22 completion
commit, tag, push, CI publication or Release follows this incomplete recovery.

## Historical first attempt — preserved evidence

All remaining sections retain the first-attempt snapshot, including its original
checklist and failed counts. Current decisions and selected results are in
Recovery III above; these historical gates are not a second current matrix.

The following findings and measurements describe the pre-recovery Phase 22
attempt. Its context failure is fixed by the isolated correction above; its
private image/instrumentation/build blockers remain unresolved.

**Historical BLOCKED / unfinished milestone.** The frozen region reconciles, but the actual
pinned patched SDK rejects the original image before analysis/emission. No new
private guest C++, checked stores or hooks are emitted. An independent original
context probe also disproves a full-width-zero startup assumption in the
Sanitized runtime. No proprietary function is executed, compiled or linked.
Only private policy **data** is compiled offline. No completed Phase 22 tag,
commit, push or GitHub Release is authorized by these results.

## Baseline and preserved identities

The initial checkout is clean at Phase 21. Public/private Phase 19–21 reports,
inventories and the immutable Phase 20 manifest are reused. Its namespace,
instruction identities and ownership requirements are not rewritten. Original
XEX SHA-256 remains
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The preservation recipe checks 1,455 original XEX/config/generated fingerprints,
299 extracted-file hashes, thirteen reused private report/manifest/view inputs,
and the globally installed SDK runtime. All outputs and useful diagnostics stay
in a dedicated private workspace outside Git, with an ignore-all marker.
About 31 GiB was free before work.

The isolated SDK remains ReXGlue v0.10.0 at
`f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, using the unchanged nineteen-patch
[lock](../../tools/sdk/xex-binding-lock.json), SHA-256
`18a39ee8d39da4e6fb39ff7bf8e1d7782affcceca7b571ee282b7f47bcbee6e1`.
The workflow reconstructs the patched source and checks 22 dependency pins,
staged headers and runtime/CLI fingerprints. No SDK source, patch lock, original
XEX, original assets/configuration or historical generated output is changed.
No title update is applied.

## Exact frozen region reconciliation

**VERIFIED static identity:** independent decoding of the preserved loaded view,
current XEX header/hash, existing production mappings and emitted body anchors
still agrees with the frozen policy. Rebuilding the graph from the addressed
words reproduces its canonical namespace, sites, blocks, edges and boundary.
Source/mapping hashes and exceptional metadata fingerprints remain unchanged.

| Frozen fact | Read-only result |
| --- | ---: |
| Admitted instructions / nonexecuted boundary word | 19 / 1 |
| Selected ranges / blocks / normal edges | 2 / 4 / 19 |
| Admitted helper calls / matched returns | 1 / 1 |
| Intended stores / normal SDK imports / normal loops | 8 / 0 / 0 |
| Requested new hooks | 20 |
| Newly emitted private hooks / checked stores | 0 / 0 |
| Newly generated private guest C++ / compiled guest functions | 0 / 0 |

The six stack and two image store requirements retain their actual private
identities. All eight are identified, **none is newly instrumented**. The legacy
output remains unchanged and unmediated. Shared helper suffix aliases are not
additional roots. The known intersecting exception scope is preserved, never
inferred unreachable. The region is not widened to the historical 108-function
closure.

## Reproducible loader and production-codegen blocker

[The original loader-only probe](../../tools/entry-audit/load_probe.cpp) links
only the actual SDK. It creates no XThread, links no generated title function,
and never requests launch/dispatch. The original file is loaded solely as data
in tool mode. Debug, Release and Sanitized all return the real SDK failure
`0xC0000001`; the probe exits **4**, not success. All three restore the measured
image/virtual page counts, with no native signal or timeout.

An independent PE/XEX preflight finds fourteen declared sections, with one
outside the actual loaded-owner bound. Both selected code ranges and both fixed
writable destinations lie inside the bound with matching section flags; that
does not make the entire image valid. The current production
`XexModule::ReadPEHeaders` requires every section to pass `ImageContains`.
A private original diagnostic interposer, used only in one bounded loader child,
records the production false return **without changing it**. Its refused span
matches the independently parsed out-of-owner section exactly. Raw section names,
addresses, bytes and diagnostic details are not published.

This is more specific than PE SizeOfImage merely being larger. Phase 21 proves
that an oversized declaration can be safely accepted when every section remains
inside the actual owner. Here an actual declared section fails that condition.
The historical analysis view skipped an out-of-loaded section; its existence and
Phase 3 generation success do not establish compatibility with the stricter
nineteen-patch SDK. No older permissive CLI is substituted.

One production CLI dispatch is attempted with a **180-second external budget**.
A checksum-identical private XEX copy isolates the CLI's writable default user
root from the original intake. The derived configuration retains the original
hints and ABI flags, adds twenty production before-hooks and an explicit approved
pre-call boundary, and uses a new private output/configuration identity. It does
not replace the frozen namespace. The CLI rejects the image at PE loading before
analysis, emitting only a support `rexglue.cmake`, **zero guest C++ files**.
No second/extended run is justified by progress. There is no full-title emission
cost measurement or verified bounded-region output isolation result.

Allocations are not enlarged from PE metadata. No section is silently dropped,
no memory protection bypass is added, and no proprietary instruction stream is
rewritten to force acceptance. The actual private image/module lease and stack
binding remain unavailable. Resolving the nonresident-section representation
requires independent format/ownership evidence and a separately audited loader
contract, not an exception added solely to make this image pass.

## Independent instrumentation and nonexecuting build evidence

[The new read-only verifier](../../tools/audit_entry_instrumentation.py) reuses
the frozen decoder and manifest authority. On original production artifacts it
checks exact canonical identity, instruction words, actual function/callee/link
anchors, per-PC hooks, generated callback indices, checked macro expansions,
store widths and a bounded set of recognized source constructs. Missing hooks,
unknown memory/native constructs, base aliases or authority mismatches refuse.
No generated C++ is manually edited. Mutations exist only as in-memory negative
candidates and are never compiled/executed as accepted programs.

This is a conservative syntactic verifier, not exhaustive C++ alias analysis or
a native sandbox. Its offline result explicitly says runtime-observed=false.
Actual original compiled hook observations come separately from the unchanged
Phase 21 runtime suite. No original synthetic count certifies a private title
hook or unchecked-path exclusion.

The immutable private twenty-site policy is serialized by original tooling into
C++ **data**, compiled to a native object and LLVM IR in all three profiles, and
its exact compiler-emitted constant payload is compared with the authority.
No object is executed or dynamically loaded. Sanitized objects may contain
compiler-generated ASan metadata/initializers; no guest function is present.
This verifies policy-data identity only. It is **not** a compile/link of the two
guest functions, their checked-store paths, callbacks or import dependencies.

Production emission fails before new instrumented guest source is available
for compilation. Therefore
private syntax/ABI/link validation, actual compiled hooks, complete mediation
and compiled continuation refusal remain **BLOCKED**. Neither legacy C++ body
extraction/editing nor a permissive full-title binary is used as a substitute.

## Startup-state provenance: source classes and missing contract

The private matrix records every required field, exact source contract, source
hash/line evidence, original observations and whether the current SDK actually
provides a corresponding private instance. Public findings are sanitized below.

| Requirement | Source of truth and evidence class | Current provision / admission implication |
| --- | --- | --- |
| Entry identity and declared stack requirements | VERIFIED title-derived XEX/PE metadata and frozen manifest | Static metadata known; private module load fails before a launch thread exists |
| r1 origin, bounds, alignment | VERIFIED SDK `AllocateStack`, `ThreadState`, original pre-guest observer | Original supported modules get aligned stack/PCR; actual private ownership/window is not bound |
| LR and saved linkage | VERIFIED title-derived use and direct-call link/return; SDK original observations | Required initial title LR provenance UNKNOWN; an observed default is not a console launch contract |
| r2 | VERIFIED SDK construction/observation; excluded from normal operand dereference | Preservation mechanics tested; no verified title TOC/startup value is inferred |
| r13/PCR | VERIFIED SDK thread-state/PCR construction and original ownership/lifecycle tests | Actual private PCR/TLS/image instance unavailable because loading fails |
| Required input and saved GPRs | VERIFIED title-derived consumption; SDK source plus byte diagnostics | Title values UNKNOWN; full-width zero-default hypothesis disproved, not filled with constants |
| Stack loan/owner tokens | VERIFIED checked-copy requirements and SDK allocation source | Normal XThread allocation does not itself register the harness's checked owner loans; an explicit legitimate binding remains necessary |
| Module/TLS context | VERIFIED title metadata and SDK `CreateWithModule` lease/TLS machinery | Proven on original fixtures only; no valid private module instance is available |
| Initialization order | VERIFIED SDK prepare/argument/APC dispatch source; original observation before those stages | Actual Xbox startup order/inputs and relevant platform conformance UNKNOWN |
| Phase 21 explicitly seeded values | ASSUMED original synthetic fixture inputs | Not evidence for any title input; never copied into a private startup state |

The native observer uses a fresh original XThread and samples **before guest
argument/APC dispatch**, then requests ordinary owned exit. It invokes no
generated function. Passing zero observations on fresh Debug/Release allocations
do not prove deterministic startup initialization.

A strict full-width-zero challenge fails in Sanitized. Independently, an original
placement test prefills host storage, calls the **actual SDK ThreadState
constructor**, and inspects object representation without fetching instructions.
Of 240 examined bytes in thirty GPRs other than r1/r13, only thirty are zeroed
while 210 retain the known prefill. Supplied original r1/PCR parameters are
applied correctly. A direct aggregate-construction control overwrites those
bytes, so that control alone would miss the real SDK constructor behavior.
The strict failing oracle is retained; the representation diagnostic reports
OBSERVED, never PASS or a title-value contract.

This establishes incomplete full-width initialization in the observed constructor
path, not that Xbox requires all registers to start at zero. The SDK's
`PPCRegister` union/member initialization and actual compiled constructor need an
explicit, regression-tested contract. No arbitrary zeroing or guessed platform
value is applied to the game. Actual required title startup provenance therefore
remains **UNKNOWN**, independently of the image-loader failure.

## CFG, exceptions and failure containment

The original synthetic production path still validates helper call/return,
invalid return/entry/context, unknown/exceptional targets, checked-memory failure,
owned stop, image leases and teardown. New static candidates test omission,
duplicate/conflicting PC, wrong instruction/image/namespace, wrong helper/link,
stack owner/register mismatch, out-of-image write, oversized ownership claim,
overlap, unverified exception, missing stop, compiled-table mismatch, raw alias and explicit pointer cast.
An oversized PE declaration alone is not treated as ownership or as necessarily
invalid; an enlarged ownership **claim** is refused.

For the private region, normal CFG facts remain static and the intended exit
precedes the initializer. Actual CFG/stop/exception/refusal integration is absent:
zero emitted private hooks and no new guest object. The intersecting exception
metadata remains a gate. No guest handler, unwind or fallback continuation is
implemented or invoked. Bounded external watchdogs and existing fatal join
containment are prepared on original fixtures, but not exercised on private code.

A failed strict context observer is not relabeled as an orderly successful
lifecycle. Its explicit failed result and diagnostics remain separate from
passing resource-reclamation scenarios and expected fatal fail-stops.

## Formal private admission checklist

Detailed PASS entries link to specific existing local evidence in the private
checklist; no local private path or exact title identity is published here.

| Gate | Status | Direct evidence / remaining requirement |
| --- | --- | --- |
| G1 exact frozen CFG | PASS | Immutable hashes, independent re-decoding, graph rebuild and original production mappings reconcile |
| G2 eight stores instrumented | BLOCKED | Eight identified; zero newly emitted checked stores |
| G3 no unchecked reachable operation | BLOCKED | Legacy output unmediated; no new private source/object to certify |
| G4 image/code/section binding | BLOCKED | Actual loader failure and exact out-of-owner production span; no live private lease/stack binding |
| G5 actual initial-state provenance | UNKNOWN | Title inputs unresolved; SDK full-width-zero counterevidence; no invented values |
| G6 helper and return admission | PARTIAL | Frozen static call/return verified; no actual new compiled enforcement |
| G7 stop/site integration | BLOCKED | Twenty requests/data records; zero emitted private hooks |
| G8 exceptional/unsupported denial | BLOCKED | Original negatives pass in their scope; private exception enforcement absent |
| G9 no unverified import/fallback | PARTIAL | Zero normal imports in frozen graph; no newly built private continuation policy |
| G10 bounded containment prepared | PASS | Monotonic process-group watchdogs, bounded original tests and retained fail-stop controls |
| G11 independent build/manifest audit | BLOCKED | Manifest-data serialization verifies; private guest emission/build/link does not occur |
| G12 artifacts/inputs preserved | PASS | Original fingerprints, reused immutable inputs, installed runtime and private-only artifact audit |

No completion percentage is calculated. A compiled policy constant is insufficient
for a private entry experiment.

## Regression and resource evidence

**Selected current-source evidence: 817 core PASS, one core FAIL.** The unchanged
Phase 5–21 suites contribute 752 PASS/zero FAIL. New audits contribute 65 PASS
and one retained strict context FAIL in Sanitized. No failed observer is
counted as guest success, graceful cleanup or a passing startup contract.

| Profile | Phase 5–21 PASS | New PASS | New FAIL | Total PASS | Core child seconds | Peak core-child RSS bytes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Debug | 251 | 22 | 0 | 273 | 153.589801 | 375,357,440 |
| Release | 251 | 22 | 0 | 273 | 131.067956 | 412,368,896 |
| Sanitized | 250 | 21 | 1 | 271 | 170.689257 | 512,950,272 |

| New audit only | Compile seconds | Runtime child seconds | Runtime peak RSS bytes | Positive static source/table/IR audit seconds, including reads |
| --- | ---: | ---: | ---: | ---: |
| Debug | 4.555586 | 1.316966 | 27,852,800 | 0.008635 |
| Release | 4.271638 | 0.546568 | 22,380,544 | 0.006879 |
| Sanitized | 4.289817 | 1.461422 | 62,980,096 | 0.007641 |

The static timings are a separate final read-only measurement, excluding the
negative suite. Core seconds sum supervised children, not calendar project time.
RSS is the maximum individual child wait4 usage, not process-tree/game/Switch
memory. Host: macOS ARM64 / Apple Clang 17, C++23; original clients O0/O2/
O2+ASan+UBSan, matching unchanged Sanitized SDK O1/no recovery.

The real private loader rejects in **1.096532 / 0.957780 / 1.816051 seconds**
(Debug/Release/Sanitized), with peak RSS **58,671,104 / 53,444,608 /
98,516,992 bytes**. The actual production CLI refusal takes **0.179716 seconds**,
peak RSS **54,558,720 bytes**, exit 1. These real admission failures remain
separate from passing original fixtures. Metadata-policy compilation is
successful in all profiles; no private guest function or link is produced.

All **51** final static adversarial instances refuse (seventeen per profile).
Original production-source census agrees on 24 static/emitted sites, ten scalar
stores and 21 normal edges. Separate unchanged runtime tests observe all 24 sites/
21 edges and validate the eight corresponding startup store classes. Private
new instrumentation/runtime counts remain zero.

There are **zero unexpected native signals, zero unexpected timeouts and zero
reported ASan/UBSan findings**. A nonzero ordinary contract-test exit remains a
FAIL even without a sanitizer diagnostic. Expected historical outcomes retain
44 SIGBUS, three SIGSEGV and nine SIGABRT exits, twelve capture/recovery results
and nine separately expected join fail-stop exits 70. Their fatal paths have no
graceful-cleanup claim. The old ASan-owned plain default-signal exclusion is
unchanged and explains the historical one-count profile difference. Watchdog
self-tests intentionally induce timeouts as containment controls.

Separately pass 97 Python units (eighteen new), 27 component/source-control
groups, 33-import audits, the Debug reference trace `[2,2,2,2,1]`/one destructor
and three VMX characterizations of 212,068 invocations each. Known VMX special
value/hardware limits and incomplete typed-vector/full Xbox ABI/guest unwind
remain unchanged. System frameworks/assembly are not fully instrumented;
sanitizer cleanliness is neither race freedom nor a private native-code proof.

Resource evidence is scoped: the unchanged Phase 21 startup suite certifies
38 XThread/stack/TLS/PCR lifetimes and 38 thread-backed image unloads per profile,
plus 76 facade allocations/releases. Existing image/pointer/scratch lease balance
oracles and prior lifetime regressions pass. The new original context observer
certifies one additional reclaimed lifecycle in Debug and Release; its failed
Sanitized lifecycle is **not certified**. New original loader probes and all
private load refusals restore the measured image/virtual page counts. The
constructor representation diagnostic allocates no guest memory.

Verification composition is explicit: one complete, unchanged Phase 5–21 run
is retained; after the new raw-pointer scanner correction, every new audit is
rerun in all three profiles against those exact source/hash-verified production
artifacts. Current source receipts, object hashes, policy payloads and unit
results are checked. Obsolete development audits are not substituted as final
evidence. The retained strict Sanitized context FAIL is still present.

All private loader refusals and the production CLI failure are separate actual
admission blockers, not passing synthetic guest executions. No private function
or compiled policy object executes. Static private coverage is twenty reconciled
sites/eight identified stores, with zero new instrumented stores/hooks; runtime
private coverage is zero. No sanitizer-clean original test certifies unexecuted
private objects.

Retained development diagnostics include a rejected 180-second request to the
runtime watchdog before any CLI launch; the driver uses the existing static
supervisor instead, without changing its 30-second fault/runtime cap. The first
ASan policy audit exposed an unsupported compiler redzone wrapper; the verifier
now validates its exact lengths and payload, with positive/corrupt/layout units.
No sanitizer instrumentation is disabled. Review also found an explicit pointer-
cast mutation accepted by the initial scalar scanner; it was never compiled or
executed. Pointer constructs are now rejected conservatively and a seventeenth
production-source negative is required. Subsequent original context failures
remain failures and are independently characterized, not suppressed.

## Reproduction, local-only publication review and decisions

```sh
python3 tests/entry-audit/test_audit.py
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$ALL_TESTS"
python3 tests/entry-audit/run.py --sdk "$SDK" --input-suite "$ENTRY_RESULTS" --workdir "$AUDIT_RESULTS"
# Matching Sanitized inputs require --sanitize.
python3 tools/check_public_tree.py
```

Inputs to the new audit are freshly verified **original** Phase 21 fixture
results. Its loader/state probes link only SDK/original host code. Every such
compiler/runtime child has a 30-second external watchdog; native join uses a
15-second wait and original suites have existing finite outer ceilings. The one
private production CLI dispatch has its own 180-second static-analysis ceiling.
Detailed private commands, exact manifests, records and useful artifacts are
retained outside Git. No source containing private instructions/addresses, game
C++, objects, dumps, absolute private paths or sensitive logs is staged.

**Phase 23 A — GO for further original synthetic integration/recovery.** Focus on
real constructor initialization, checked-owner registration and the loader's
nonresident-section contract. Keep the current failed oracles and all original
regressions. Any isolated SDK correction needs its own evidence, new lock/hash
identity and audit; do not silently change the established nineteen-patch SDK.

**Phase 23 B — NO-GO / BLOCKED for private instrumented-build readiness.** First
resolve the actual image-layout contract without enlarging ownership, dropping
metadata or bypassing checks just to pass. Then obtain fresh production output,
independently verify all eight checked stores/twenty hooks and isolate a genuine
nonexecuting compile/link audit. The present policy-data object is insufficient.

**Phase 23 C — NO-GO for private entry execution.** G2/G3/G4/G5/G6/G7/G8/G9/G11
remain unsatisfied. Obtain real startup provenance, actual owner/lease bindings,
complete generated/call/stop/refusal enforcement and independent build audit
before even proposing a separately authorized bounded experiment. No Phase 23
invocation protocol or automatic execution is activated here.

The working tree is preserved for review/recovery. `VERSION` and published
milestone remain Phase 21. No Phase 22 commit, tag, push or Release is made when
these critical gates fail. No claim of boot, gameplay or Switch compatibility.
