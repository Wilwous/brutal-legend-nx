# External Xbox startup evidence acquisition — T65

2026-11-04. Inspected branch `research/switch-horizon-feasibility`, starting HEAD
`0eb266e90113a9909b4c89767764ec5985bd1e35`; clean starting tree.
**Investigation COMPLETE; qualified title evidence collection BLOCKED.**
No original PPC execution, generation, compilation, image admission, production
runtime change, hardware connection, installation or NRO deployment occurred.

## Decision and exact remaining question

A current, locally operational, qualified Xbox observation framework was not
found. Public source access is AVAILABLE; selective emulator instrumentation is
FEASIBLE WITH SETUP, not a ready full-window collector. Real hardware access is
UNAVAILABLE in this environment (the user previously reported no debug hardware).
**EXTERNAL EVIDENCE REQUIRED.** No gate changes:

| Historical | Native |
|---|---|
| G4 BLOCKED | N-G4 BLOCKED |
| G5 UNKNOWN | N-G5 PARTIAL |
| G8 BLOCKED | N-G8 BLOCKED |

Original execution remains **NO-GO**. The unresolved question is an applicable
writer/observer record from before executable-image materialization through a
justified observation/stop after the two BE32 stores. It must account for early
APCs, callbacks, other threads, aliases, host loader/HLE effects and memory lifetime.
A saved source snapshot cannot supply that record. This extends the capture start
of the [T53–T64 proposal](title-writer-observer-contract.md) earlier to avoid
missing materialization-time effects; it does not assert such effects occurred.

G4 still lacks applicable live owner/reservation/commitment/permission/content/
lifetime/rollback evidence, including the unresolved virtual tail. G5 lacks complete
startup/import/context/call closure. G8 lacks applicable exception delivery,
dispatch, unwind and continuation proof. Emulator observations cannot by themselves
promote historical gates. Native image gaps, 13 providers, unknown continuations,
register context and exception boundaries remain unadmitted.

## Source inventory and practical access

Fresh remote branch resolution and source-only shallow snapshots were obtained for
three projects below. Exact commit identity was checked after fetch; selected source
bytes have private SHA-256 receipts. No third-party executable was run. Older
research references below are explicitly reused pins, not newly resolved HEADs.
GitHub API requests timed out; bounded Git transport succeeded. This network result
says nothing about emulator execution support.

| Candidate / exact revision | Available here | Evidence class and independence |
|---|---|---|
| Xenia `95a5c3ee250f80c3b9d139658649d9ffb6db3eec` | Source AVAILABLE NOW; executable not found in scoped inventory; compatible x86-64 environment/build needed | STATIC SOURCE CONTRACT now; future IMPLEMENTATION-SPECIFIC TRACE. SDK ancestry prevents treating it as independent hardware corroboration |
| Xenia Canary `455b9882004af40dbf68e4e8ef5acc14339a88d2` | Source AVAILABLE NOW; runtime WITH SETUP | Fork of Xenia; separate run/configuration is not independent implementation corroboration |
| xenon-emu/Xenon `0284bbe6c8125935d97bf54ab3132089c6c65c8b` | Source AVAILABLE NOW; executable not found; build and boot inputs NOT VERIFIED locally | Separate LLE candidate; future INDEPENDENT EMULATOR OBSERVATION subject to shared-source caveats; no observation made |
| EmDbg `da67b3085f2bc07572cef9bcc38fcd7a8b1414a6`; NeighborSharp `219edae557d19f73cfefa1b0a5fb3dded8de5f0d` (prior audit pins) | REQUIRES EXTERNAL HARDWARE and a legitimate supported debug environment | Transport/source only here; HARDWARE OBSERVATION only after qualified capture |
| X360DebuggerWV `f68b398f5889e48a1903cae7625d54fa1942eba5` (prior audit pin) | XBDM/JRPC2/Neighborhood prerequisites unavailable | Same hardware dependency; not a proven pre-TLS recorder |
| Microsoft current GDK public documentation | Public pages AVAILABLE; authorized legacy Xbox360 tool entitlement UNAVAILABLE/NOT VERIFIED | Authoritative only for its stated platform/version; no applicable Xbox360 loader contract obtained |
| XenonRecomp `ddd128bcca99fe8bfbb99bea583c972351fa6ace`; UnleashedRecomp `cf829a9eca8fb680fba4b0409ddeb6ca92f22e3c` (prior pins) | Existing source analysis; not rerun | STATIC SOURCE CONTRACT / native implementation; recompilation is not external Xbox observation |
| Pinned patched SDK `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` and existing ReXGlue fixtures | AVAILABLE NOW in isolated preserved toolchain | Existing STATIC SOURCE CONTRACT and SYNTHETIC ONLY results; not a new independent source |
| Local Apple LLDB | AVAILABLE NOW on arm64 macOS | Host debugger, UNQUALIFIED as an Xbox guest collector; no target/backend/causal recorder supplied |

The scoped inventory checked PATH for LLDB, Wine/Wine64, Xenia, Xenon, QEMU PPC64/
x86-64 and GDB, plus standard system/user Applications roots for emulator/VM apps.
Only LLDB was found among those commands. Existing analysis artifacts and SDK tools
remain available. This is not an exhaustive filesystem or attached-device scan.
No device was probed. A source clone is not an installed working emulator.

## Verified capabilities and limits

Xenia's current [CPU flags](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/cpu_flags.cc)
select an x64 backend and expose function statistics/coverage options. Its
[FunctionTraceData](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/function_trace_data.h)
records counters, caller history and thread-use bits; that is not a chronological
memory event stream. The [x64 tracer](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/backend/x64/x64_tracers.cc#L25)
sets ITRACE and DTRACE to zero. Selected scalar/vector load/store emitters have
conditional trace calls in [x64_seq_memory.cc](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/backend/x64/x64_seq_memory.cc#L754).
Enabling those requires a source build and validation of address/value semantics,
optimized paths, atomics and omissions. It does not automatically instrument host
loader copies, host HLE writes or device agents.

The [XThread path](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xthread.cc#L287)
contains TLS/PCR setup, and Execute delivers APCs before startup selection
(lines 496–531). Those are candidate hook sites, not a ready qualified export.
[UserModule](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/user_module.cc#L134)
splits image loading from later load continuation. Capture must start before the
first stage. [Heap queries](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/memory.cc#L1238)
expose emulator allocation/state/protection data; they do not establish original
Xbox owners or a complete lifetime history. The [debug memory pane](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/debug/ui/debug_window.cc#L1112)
is a placeholder in the inspected revision.

Canary's [.xtr protocol](https://github.com/xenia-canary/xenia-canary/blob/455b9882004af40dbf68e4e8ef5acc14339a88d2/src/xenia/gpu/trace_protocol.h)
is GPU trace data, including GPU physical-memory commands. It is not a full CPU,
loader or APC read/write trace. CPU function-trace structures match the inspected
Xenia structure. No independent hardware loader/exception authority follows.

Xenon's [README](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/README.md)
describes an early LLE supporting small programs and credits Xenia/RPCS3 input.
Its [instruction profiler](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Xenon/Core/XCPU/Interpreter/InstructionProfiler.h)
counts instruction names; counts cannot recover target memory observers.
[Config](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Xenon/Base/Config.cpp#L116)
contains StartHalted, HaltOnRead/Write and a debug branch trace option.
[PPC_MMU](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Xenon/Core/XCPU/Interpreter/PPC_MMU.cpp#L1085)
checks an exact effective address for read/write halt. This point match is not a
proof of all accesses overlapping an eight-byte interval, aliases or host writes.
The [serial KD test](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Tools/WinDBG_Test/Main.cpp)
reads/acknowledges selected serial packets; it is not a demonstrated full-memory
recorder. It was not run. [XeMain](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Xenon/Core/XeMain.cpp)
has ELF and boot paths; applicable legitimate boot inputs and original-title loader
support remain unverified. The [macOS guide](https://github.com/xenon-emu/xenon/blob/0284bbe6c8125935d97bf54ab3132089c6c65c8b/Deps/Docs/Building/building-macos.md)
requires LLVM rather than AppleClang. No dependency installation was attempted.

The [prior hardware-client audit](xbox-evidence-acquisition-feasibility.md) already
found map/context and pre-TLS hold/ownership limitations. Current [EmDbg](https://github.com/InvoxiPlayGames/EmDbg)
describes an unfinished XBDM client, not an entitlement or hardware substitute.
The [Microsoft GDK access policy](https://learn.microsoft.com/en-us/xbox/gdk/docs/gdk-dev/development-downloads/access-resources?view=gdk-2604)
does not establish access to an archival Xbox360 debug environment. No applicable
public authoritative loader/exception contract was found in this bounded search.
This is a search result, not proof none exists anywhere.

## Evidence quality matrix

“Candidate hooks” means inspectable implementation sites, not validated capture.
Question marks remain unproved; no row is hardware-qualified here.

| Required property | Authorized hardware client | Xenia / Canary | Xenon LLE | Recomp/SDK/LLDB here |
|---|---|---|---|---|
| CPU execution fidelity | Actual CPU if qualified target exists | Emulated, revision-specific | Emulated, early-stage | Synthetic/native/host only |
| Original XEX loader | Potential real loader; no accessible early hold | HLE implementation | Applicable boot/kernel path UNKNOWN | Derived implementation/static |
| Initial contents / early window | Requires pre-materialization hold | Candidate loader hooks | Boot-level setup UNKNOWN | Saved snapshots only |
| Reserve/commit/rights | Map alone insufficient; needs qualified transitions | Emulator heap state, hook/export needed | MMU state not OS allocation-owner ledger | Native owned objects only |
| Owner/generation/lifetime/rollback | Not established by inspected clients | Additional identity/lifecycle instrumentation needed | Not established | No original runtime proof |
| Guest scalar/vector/atomic R/W | Full coverage NOT VERIFIED | Conditional selected emitter hooks; exhaustive coverage NOT VERIFIED | Point watch and interpreter sites; complete coverage NOT VERIFIED | Synthetic scope only |
| Indirect/alias/host/HLE/device effects | NOT VERIFIED | Must audit/add all paths | NOT VERIFIED | No applicable capture |
| Thread identity | Partial debug context | XThread IDs inspectable | CPU thread context; kernel identity UNKNOWN | Native synthetic identity |
| APC/callback ordering | Qualified early capture NOT VERIFIED | Explicit code sites; event recorder absent | Applicable guest-kernel visibility UNKNOWN | Known SDK source ordering only |
| TLS/PCR/imports | Applicable early trace NOT VERIFIED | Source hook sites, not hardware proof | Needs identified boot/kernel observations | Source/synthetic only |
| Guest exceptions | Debug notifications do not prove unwind/dispatch | Implementation behavior only | Implementation behavior; applicability UNKNOWN | Incomplete/unverified |
| Causality / dropped-event proof | NOT VERIFIED | No qualified full-window protocol located | No qualified protocol located | Proposed envelope only |
| Export / repeatability | Client-specific maps/logs, unavailable target | Counters/debug logs/GPU .xtr; scope-specific | Counts, branch log, serial packets | JSON synthetic fixtures |
| Overhead | UNKNOWN; halt changes scheduling | Memory logging potentially high, not measured | Branch trace potentially unbounded; not enabled | Parser-only measurement |
| Setup / independence | External qualified hardware + authorized tools | Compatible host/build + collector qualification; shared ancestry | New distinct candidate, build/boot/collector dependencies | Available but not external reference |

## Practical readiness A–H

| Question | Decision |
|---|---|
| A: Real Xbox hardware tracing here | UNAVAILABLE |
| B: Independent emulator tracing | PARTIAL; Xenon source accessible, qualified capture NOT VERIFIED |
| C: Startup/APC/TLS visibility | PARTIAL source hook sites; collection FEASIBLE WITH SETUP for Xenia scope |
| D: Complete guest/host memory events | UNKNOWN; qualified collector BLOCKED |
| E: Actual eight-byte writer/observer contract | BLOCKED |
| F: Applicable owner/commitment/rights evidence | BLOCKED for title; emulator heap metadata PARTIAL |
| G: Separate title behavior from implementation behavior | Classification explicit; hardware correlation UNKNOWN |
| H: Collect qualified evidence now | BLOCKED |

## Acquisition route and stop

The shortest possible emulator route is a compatible isolated x86-64 Windows/Linux
Xenia setup with pinned build, followed by **non-title** collector qualification:
known direct/indirect reads/writes, overlapping widths, two threads and explicit
synchronization, APC callback, loader/HLE writes, TLS/PCR/import order, permission/
owner teardown, and deliberate record loss. Selected existing hooks can reduce
work but do not replace coverage auditing. This yields implementation-specific
empirical evidence, not historical Xbox authority. No setup/build or execution was
performed; exact turnkey collector commands cannot honestly be provided.

Xenon adds an independently implemented LLE candidate, but its demonstrated program
scope and absent qualified recorder do not justify a title experiment. Its point
watchpoint/branch profiler is insufficient. Repeating prior Xenia/RXDK title
experiments now would add no qualified evidence.

The smallest missing external dependency is **access to a qualified reference
collection environment/operator** able to demonstrate the complete early capture
on an original authorized synthetic program first, export loss/causality/ownership
receipts, and state its hardware or emulator scope. For historical gates this must
be authorized real hardware or an applicable authoritative loader/exception
contract. Neither is available. Even that dependency would not authorize original
execution under T65: a separate scoped task would be needed.

Further title-specific static scanning is not justified for closing the dynamic
writer/observer obligation; a newly supplied contract or trace could justify a
bounded follow-up. Until then, preserve blockers and prefer independent synthetic
platform development in a separately authorized task. **Stop at T65; no T66.**

## Minimal protocol, validation and preservation

The [trace envelope specification](startup-trace-envelope.md) defines the capture
window, fields, coverage channels and causal requirements. The small validator
checks structural consistency only. It cannot certify that a collector saw every
access or that a claimed owner is true. It always reports `gate_impact=NONE` and
`EXTERNAL_REVIEW_REQUIRED`, including for hardware-labelled input.

No framework instrumentation PoC was performed because no suitable framework was
locally operational and qualified. The fallback is a specification, feasibility
report and independently authored **data fixtures**, not synthetic CPU execution.
Twelve unit cases pass: one consistent 21-event fixture and eleven expected
refusals, including removed record, producer mismatch and corrupted commitment.
The valid fixture contains two writes, an indirect pre-startup callback read,
thread identities and a post-pair read. Its independently asserted early-reader
index is 12; the parser must not hide that observer behind a final-state result.
Six existing partial-order cases and four overwrite-contract cases also pass
(22 unit cases total). The three isolated processes took 0.134 seconds combined;
peak RSS was 14,434,304 bytes (macOS time), with a 30-second watchdog each.
CLI export/parse checks also pass: consistent input exits 0 and corrupted input
exits 1 with an explicit refusal. No unexpected faults/timeouts;
Debug/Release/ASan/UBSan locally **N/A** because no
production C++ was changed or built in this task. Hosted portable regression jobs
are separate evidence, recorded in the private completion receipt.

```sh
python3 tests/trace-evidence/test_trace.py
python3 tests/observation-order/test_order.py
python3 tests/image-write-authority/test_contract.py
python3 tools/check_public_tree.py
```

The original XEX and both representations, sixteen prior checkpoint manifests,
103 external artifacts, 1118 SDK headers, 35 SDK locks, 32 runtime sources, eight
pinned SDK source files, protected branches and 21 tags are verified unchanged.
Private source snapshots, exact paths, inventory, test/CI receipts and restoration
instructions stay in the private T65 checkpoint. Public changes contain only
original envelope tooling/data tests, CI/allowlist integration and sanitized docs.
No proprietary trace, address map, generated title code or private path is published.
