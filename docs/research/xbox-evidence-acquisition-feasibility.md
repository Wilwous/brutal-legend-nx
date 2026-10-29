# Switch Stage 23 — Xbox evidence acquisition feasibility

2026-10-29 · baseline `785834efa98661f3779de9c4f0bdc9456d62b5cb`.
**Decision: BLOCKED PENDING EXTERNAL EVIDENCE — EXTERNAL EVIDENCE REQUIRED.**
G4 BLOCKED; G5 UNKNOWN; G8 BLOCKED; original game execution NO-GO.
This is Switch roadmap Stage23, not an original-title experiment or resumption
of preserved Xbox Phase22. Stage15 remains HARDWARE VERIFIED for its finite
synthetic workload only. Four Horizon SDK imports and all runtime policies stay
unchanged. No new experiment, NRO, SDK change, connection or console action.

## Exact missing proof

**G4:** independent authority for the disputed original image tail's owner/load
instance, reserved extent, commitment, initial contents or valid zero-fill,
permissions and their establishment, neighbor preservation, active-reference
lifetime, failed-load rollback and unload. The observations must be after complete
mapping/fixups/protection and before *any* candidate TLS/initializer/entry code.
One of14 PE section spans exceeds the proven original backing; PE declarations,
page rounding, security-size fields and visible bytes cannot create an owner.
The synthetic/source results in [Stages20–22](xex-gate-evidence-review.md) do not
establish that original-image-class rule or its title-specific applicability.

**G8:** exact fault/refusal and context-preservation obligations for the frozen
19-instruction/8-store region and matched helper/return: implicit memory faults,
unsupported transfers, fault PC/register state, error propagation, supported host
cleanup, deterministic termination and no unverified guest continuation. Missing
live hook/binding evidence remains missing. Refusing a map before any guest PC
can avoid guest dispatch for that refusal, but does not admit the original region.
A debugger notification, register structure, skip button or C++ catch is neither
Xbox kernel dispatch nor guest unwind. No universal dispatcher is assumed needed
or implemented. G5 consumed initial-state provenance remains a separate UNKNOWN.

## New candidates inspected and pinned

Source availability is distinct from access to an authorized working target.
Pinned source copies and file SHA256 receipts are retained privately; clients
were read only, never installed, built, discovered, connected or run.

| Candidate / exact source | Accessibility and authority | Hardware, pre-TLS/entry and gate capability |
| --- | --- | --- |
| [Microsoft resource-access policy](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/development-downloads/access-resources?view=gdk-2604), [GDK scope](https://learn.microsoft.com/en-us/gaming/gdk/docs/gdk-dev/intro/introduction?view=gdk-2604), checked2026-10-29 | Public policy is accessible and vendor-authored. Secure resources require an onboarded organization/agreement and granted access. These pages concern current GDK, not a retrieved360 loader/exception contract. | No360 mapping/owner/exception rule supplied. Availability/entitlement to legacy360 XDK documentation is **UNKNOWN**, not promised by modern enrollment. No account/access request made. A matched authorized archival contract could matter; none obtained. |
| [EmDbg](https://github.com/InvoxiPlayGames/EmDbg/tree/da67b3085f2bc07572cef9bcc38fcd7a8b1414a6) + [NeighborSharp](https://github.com/InvoxiPlayGames/NeighborSharp/tree/219edae557d19f73cfefa1b0a5fb3dded8de5f0d) | Public source accessible; independent client implementation, one project family, not two independent observations or Microsoft kernel authority. | Requires an already configured360/XBDM target; user has no qualified Xbox debug access. No proved post-map/pre-TLS hold or title-tail ownership rule. Exception/context transport is partial, not guest semantics. Cannot currently change G4/G8. |
| [X360DebuggerWV](https://github.com/zeroKilo/X360DebuggerWV/tree/f68b398f5889e48a1903cae7625d54fa1942eba5) | Public client source accessible; README requires XBDM, JRPC2 and working Neighborhood. Source uses XDevkit/JRPC_Client; installed dependencies/target are not inferred. | Module notifications plus later Pause do not prove pre-TLS ordering. Region/section labels are address containment, not allocation owner. Exception UI is not a matched kernel contract. No qualified runtime access or gate-changing evidence. |
| [xbdm_gdb_bridge](https://github.com/abaire/xbdm_gdb_bridge/tree/237eceb4f29afc0dee84af90fe5b8ac1cb8ea4db) | Public source accessible; README targets **original Xbox**, default.xbe and nxdk. | Wrong platform for360/XEX/PPC; reject without installation, injection or experiment. No G4/G8 applicability. |
| Existing Xenia/ReXGlue/RXDK and xsdk reports; previously reviewed MemoryEngine360/Experiment5X/OXDK | Already examined implementation/synthetic evidence or external author's narrower hardware observations. | No new independent contract/trace identified; no experiments repeated. Shared ancestry or a new client UI does not improve provenance. |
| Authorized legacy documentation holder or qualified Xbox lab | No specific accessible provider/target/entitlement has been established. This is an **external dependency**, not a discovered available method. | Could change a gate only with matched authority, complete observations and applicability reconciliation. Devkit ownership alone is insufficient. No contact or acquisition undertaken. |

### Decisive source observations, not feature-list assumptions

EmDbg `EmDbg/XboxDebugger.cs`: GetMemoryMap150–166 parses `base/size/protect/phys`;
no owner, reservation generation or commit state is provided by that parsed
record. PulseNotify71–87 parses exception metadata, omits general parameter
handling and assigns stopped=true without checking the stop indication.
GetThreadContext345–364 requests control/integer registers; FPU/vector capture
is unfinished. `EmDbg.WinForms/ExceptOrBreakWindow.cs`83–94 changes PC and resumes
for skip; that UI action is not an exception correctness oracle and is not used.
`NeighborSharp/README.md` delegates debugging to EmDbg, not a separate backend.

X360DebuggerWV `X360DebuggerWV/Debugger.cs`60–65 calls Pause in response to a
module/thread notification; it does not prove loader ordering. GetMemoryRegionInfos
332–369 attaches module/section labels by address containment and reports region
flags. A displayed module name therefore cannot be used as an ownership token.
These are source-backed exclusions; actual target behavior remains unobserved.

## Property and independence assessment

| Evidence type | Owner / reservation / commitment | Permissions / contents | Lifetime / rollback | Relevant G8 semantics | Can resolve title-tail question now? |
| --- | --- | --- | --- | --- | --- |
| New public360 client code | Not proven; missing fields or unvalidated inferred labels | Transport/UI fields exist; no fresh hardware observation | Events/snapshots do not establish the required allocation history | Selected metadata/register transport only; no authoritative dispatch/unwind contract | No |
| Vendor public modern GDK pages | No360 contract | No360 contract | No360 contract | No360 contract | No |
| Direct real Xbox evidence from this task | None | None | None | None | No |
| Existing emulator/source/synthetic evidence | Scoped implementation/model facts only | Scoped known fixture values | Scoped project cleanup only | Host refusal/source stubs only | No new independent evidence |
| Hypothetical matched archival contract or qualified external trace | Must explicitly establish each missing property | Must establish initial byte validity and protection ordering | Must include owner generation, neighbor integrity and failure/unload | Must cover the actual proposed boundary/context/refusal | Potentially, after independent validation; currently unavailable |

The new clients are independent *source implementations of transport*, not
independent real-hardware traces. No new direct Xbox hardware evidence or
publicly accessible authoritative360 loader/exception contract satisfying the
requirements was found in this bounded candidate review. This is not a claim
that no such private documentation or capability exists anywhere.

## Minimum external dependency, experiment gate and stop

Either (a) legitimately accessible, version-matched Xbox360 loader/exception
contracts supplied by an authorized holder, with confirmation of applicability,
or (b) an already qualified authorized lab/debug environment that can demonstrate
a supported post-map/pre-TLS hold plus image-capable owner/reserve/commit/content/
permission/lifetime/rollback observation. For G8 it must additionally establish
the relevant fault-time context and refusal/termination contract. Both paths need
independent reconciliation with the preserved title metadata; neither is assured.

No inspected interface is qualified, so **no new hardware experiment is GO**.
The existing [conditional original-only protocol](xbox-hardware-observation-protocol.md)
remains design-only; it is not activated by finding a downloadable client.
Do not install plugins, modify firmware, bypass privileges, force faults or
substitute guessed values. A program reporting after entry cannot measure itself
before entry; the user's Switch cannot provide Xbox loader evidence.

**STOP RULE APPLIED:** candidate inspection is finished. No repeated Xenia/RXDK
runs, speculative runtime implementation or follow-up stage is proposed. Resume
only upon receipt of genuinely new qualified external evidence, not more test
counts. Documentation-only validation uses the publication/link checker and CI;
no runtime regressions or original-image loading are needed for these changes.
