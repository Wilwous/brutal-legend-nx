# Stage 3 — original synthetic XEX runtime integration

2026-10-26. **HOST VERIFIED / BUILD VERIFIED for the admitted fixture profile;
overall PARTIAL; HARDWARE EXECUTION UNKNOWN.** Stage 2 host/build acceptance,
including its separate stack-window correction, passed before final Stage 3 work.
No new console result is claimed and no game XEX is loaded. The prior PASS38
four-instruction probe remains HARDWARE VERIFIED only for that externally observed
scope, with prior launch mode UNKNOWN. Xbox Phase 22 remains BLOCKED.

## Genuine pipeline and deliberately excluded parts

The original fixture builder produces a complete **unencrypted/uncompressed
XEX2 containing a four-section PPC PE image**, TLS, PDATA and one xboxkrnl import.
The production ReXGlue `codegen` command parses this XEX through the actual pinned
SDK loader/analyzer and emits six functions / 65 instructions, an import reference
and both requested production midasm stop hooks. Original template overrides emit
minimal Horizon PCH and the **actual generator-derived image/function table**;
configuration flags preserving registers/LR are compile-time checked. The SDK's
normal desktop registration/runtime initialization units are excluded. The native
PPC function translation unit is byte-identical to default production generation.
Repeat generation matches all emitted C++/header hashes. No generated C++ is edited.

The NRO embeds that original XEX and its fixed identity bytes, then runs the same
original `XexImage` parser/loader, ownership, guest setup, generated functions,
checked import wrapper, SDK leaf and cleanup implementation as host tests.
Image allocation and parsing are real runtime operations, not a preloaded PE
sidecar pretending to be an XEX load. Image **construction/code generation remain
host-side**; no console execution has yet been observed. Therefore the overall
Stage 3 result remains PARTIAL, not a complete SDK/XEX platform port.

The Horizon loader is a deliberately strict original-fixture loader, not the
full SDK UserModule implementation. It accepts only the audited flat image
family. No encryption, compressed input, general relocation, title resource
loading, instruction fetch or game entry is implemented. Unknown residency is
denied. No fake successful Xbox API, desktop platform spoof or RWX storage is used.

Input XEX SHA-256:
`906a552244b03a2ad16e75a51e3087625b2f296850afeebec6eef5cb593496c0`.
ReXGlue remains v0.10.0 / `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`, with the
preserved nineteen-patch lock and existing isolated Horizon header/template patch.
The devkitA64/libnx digest and versions remain [unchanged](../../tests/switch-horizon/toolchain.json).

## Loader, code binding and lifetime contracts

Byte-wise bounded readers validate XEX/PE signatures, header identities, exact
base/entry, security image extent, contiguous PDATA ranges and all four section
names, raw/virtual extents and flags. A larger PE SizeOfImage is rejected rather
than adopted as resident bytes. Every accepted section is physically present in
the flat input. Executable guest sections receive software read-only storage;
the original compiled AArch64 functions live separately in normal NRO text.

TLS requires 64 slots, 32 static bytes, 16 initialized bytes and the template in
the owned read-only section. Import parsing requires the exact xboxkrnl table,
one ordinal/type pair and matching thunk reference. All other PE directories,
relocations, unknown imports and unreviewed image forms refuse. The complete
input is compared to its fixed identity before any allocation. Actual generated
image base/size/code extent and seven generator-produced mappings (six PPC
functions plus the import) must agree with independently parsed image metadata.

The module retains private section ownership tokens. Public section views expose
only address/extent, not release/protection authority. Call admission also checks
the **actual module object and Space object**, not just an equal numeric identity.
A foreign object with the same numeric module ID is rejected. GuestThread pins
module lifetime; Scope pins stack/PCR/TLS and lent image regions. Imported IN
memory has an additional retained read-only capability. Closing stops new
admission, preserves active calls, and returns busy until native exit/join and
guest cleanup. Multi-section release prevalidates the entire group under one
lock. Concurrent final unloads are serialized and idempotent. No active object
is force-freed and no whole-arena Reset is used.

Every generated memory macro passes through the scoped checked layer: static
census **6 load32 + 8 store32 + 3 load64 + 3 store64 = 20 sites**. There are five
direct call sites and one checked indirect call. Direct and indirect targets
come only from the bound table. These are software ownership/permission checks,
not a claim of complete hardware guest isolation or arbitrary title coverage.

## Admitted SDK contract

| Import | Profile status | Evidence and limitation |
| --- | --- | --- |
| RtlCompareMemoryUlong, ordinal 0x11b | HOST/BUILD VERIFIED | Exact pinned `u32(mapped_void,u32,u32)` SDK body, real SDK MappedPtr/be types; aligned nonempty IN span <=64 bytes within a retained .rdata capability. |
| NtQueryVirtualMemory, ordinal 0xee | BLOCKED for this profile | No admitted SDK Memory/Heap LookupHeap/QueryRegionInfo backend or verified OUT structure/status/protection translation on Horizon. No success substitute. |
| RtlInitializeCriticalSection, ordinal 0x12e | BLOCKED for this profile | Its INOUT28/type/trace adapter has not been implemented and validated here. Initialization itself is small; broader critical-section ownership/waits are separate work. This is not a claim of fundamental Horizon incompatibility. |

The [SDK extraction lock](../../tests/horizon-xex/sdk-lock.json) pins the complete
open-source source file and exact function body. The body stays in a private build
include with no changes or replacement implementation. Body SHA-256:
`3c1578f638f7e47343f2150791ee6115f6927289e37abac5341e586ddc426217`.

The original import wrapper validates context/module association and signature,
copies one full checked span atomically into private typed big-endian scratch,
then calls that real SDK function. No raw guest pointer is passed to it. Only
r3 is published after validation and successful return; failures retain it.
Three/one/zero matching-word controls establish the inspected SDK word-count
behavior, not an unmeasured assertion of complete Xbox equivalence. Invalid,
unaligned, cross-boundary, null or non-admitted source pointers refuse before
the SDK call count increases. General SDK ExportResolver/KernelState, import
callbacks, OUT/INOUT contracts, dynamic IAT rebinding and other signatures are
not implemented. Typed-vector ABI is explicitly denied.

## Generated execution, stack and stop

The generated entry calls its generated multicall helper, reads PCR/dynamic TLS,
uses nested guest stack frames, invokes the admitted SDK import and calls a
generated helper through CTR. The primary result is **r3=17**. A separate dynamic
stack-argument control returns 11 from 2+9, using an explicit fixture-defined
32-bit argument at r1+84. This does not prove arbitrary Xbox stack signatures.
Selected r1/r2/r13/LR and r14/r15 preservation, two concurrent guest contexts,
independent image-derived static TLS and independent dynamic values are tested.

The expanded stack-boundary negative initially returned **109 PASS / 1 FAIL**
because an invalid r1 could direct an argument read into adjacent admitted PCR
memory. Stage 3 finalization stopped. The separate Stage 2 correction now checks
the selected owned r1 window/alignment at every generated prologue; both suites
verify refusal before the generated helper accesses the wrong region. The failed
receipt is retained privately and is not counted as a final pass.

Production codegen emits actual callbacks at **both independently decoded
backward branch sites**. Dynamic primary finite-loop hook count is 4 (including
the final not-taken condition); the continuing-loop stop case records 1. These
are hook invocations, not counts of taken edges. A real event coordinates stop,
active unload refusal and a zero-budget join refusal. Stop follows request →
whole-instruction hook → typed C++ unwind → native exit → join → reclaim → unload.
No longjmp, forced cancellation or unsupported guest exception continuation is
used. Observer-error containment is tested separately; Xbox exception dispatch
and unwind remain BLOCKED. The discarded cancelled guest context is not resumed.

## Final tests and artifacts

Debug, Release and ASan+UBSan each pass **118 / 0** runtime checks. Five expected
compiler refusals cover raw address/vector access, longjmp, unimplemented traps
and typed-vector entry calls. Eight independent public XEX/encoding units pass.
Final unexpected faults, timeouts and sanitizer findings are **zero**. Explicit
creation/initialization/observer failure injection and controlled join refusals
are intentional tests, not observed platform failures.

The corpus covers wrong image/code/generator identity, section overlap and W+X,
PE-residency inflation, invalid TLS, wrong ordinal/signature, unknown indirect
and hook targets, read-only destinations, forged release tokens, stale images,
all four allocation rollback stages, actual small-arena allocation failure,
active unload/cleanup refusal, concurrent execution, serialized concurrent unload,
repeated load/run/unload and failed callback cleanup. Race freedom is not inferred
solely from clean sanitizers; lock ordering, admission and retained ownership are
part of the design. Native kernel resource-exhaustion and hardware scheduler
behavior remain unverified.

Stage 2 recovery independently passed 108/0 in all three profiles, five compile
refusals and two clean Horizon builds. Portable adapter controls pass 20/0 in all
three profiles. Earlier leaf/native/foundation regressions, 74 existing packaging/
deployment/ELF units and the new encoding controls pass. The final Stage 3 runner
records exact per-process wall time/RSS in private receipts; measurements below
refer to host children only, never Switch performance.

The corrected Foundation package is separate from Synthetic XEX Runtime, and both
preserve the prior successful PASS38 NRO. The new XEX NRO title is **Brutal Legend
NX - Synthetic XEX Runtime**, NACP 0.4.0, SD path
`sdmc:/switch/brutal-legend-nx-xex/brutal-legend-nx-xex.nro`.
Expected successful screen: **PASS=119 FAIL=0**, primary r3=17, primary SDK calls=1,
hook hits 4/1 and zero live guest allocations. These are computed execution
checks, not hardcoded successes. Both new applications wait for PLUS.

## Reproduction and manual hardware gate

Use the same isolated pinned CLI/header environment as Stage 2. SDK_SOURCE points
to the preserved nineteen-patch SDK, used read-only to fingerprint/extract the
one real SDK leaf. Outputs must remain outside the checkout.

```sh
python3 tests/horizon-xex/test_fixture.py
python3 tests/horizon-xex/run.py --toolchain "$HORIZON_TC" \
  --sdk-source "$SDK_SOURCE" --workdir "$XEX_WORK/build" \
  --switch-build --icon "$ORIGINAL_ICON"
python3 tests/horizon-foundation/run.py --toolchain "$HORIZON_TC" \
  --workdir "$XEX_WORK/foundation-regression"
python3 tests/horizon-foundation/core.py --workdir "$XEX_WORK/core"
python3 tools/switch/stage_package.py --application xex \
  --build "$XEX_WORK/build" --workdir "$XEX_WORK/ready"
```

CI runs public encoding/portable component/deployment controls; the private SDK
production generation, full local runtime corpus and AArch64 builds are not
misrepresented as CI/device execution. ZIPs contain six intended files under
separate SD directories, with exact source hashes, NRO checksums and license texts.
ULTRASWITCH was not mounted at the deployment check, so neither new NRO was copied
and no SD/firmware/configuration file changed. Existing tested USB tools remain
unchanged. Keep earlier probes and use only verified volume identity, temporary
writes, backup, flush/hash verification and safe non-forced unmount when deploying.

First manually test the **corrected Foundation (expected PASS109)** and normal
PLUS exit. Only after its hardware evidence, test **Synthetic XEX Runtime
(expected PASS119)**. Record complete screens, exact hashes, Applet/Title Override
mode and platform/loader versions. Hardware results remain UNKNOWN until then.
This work authorizes no proprietary execution, game compatibility or Phase 23.

## Final artifact measurements

NRO **626,082 bytes**, SHA-256 `8ce7c1f9018a474d1746788115158f38e4b18d21b20b4ff02f474580ce46d205`.
ELF SHA-256 `c53fa2246db1091d2ff2ee96874a3536bb592dd17fc303109b3a791ab8c54072`.
Two clean ELF/NRO/object builds match; AArch64 PIE, RX/R/RW segments, no W+X,
zero DT_NEEDED and zero unresolved strong symbols. Original icon/NACP present,
no RomFS or private user/build paths. The unchanged pinned read-only newlib
diagnostic sanitizer changes no executable bytes or layout.
Loaded file bytes: RX 241144, R 323700, RW 17456; writable memory extent 36448.
Heap request 16 MiB; main arena 4 MiB, each isolated repeat arena 4 MiB;
guest stack 16 KiB/thread, PCR 728 bytes and TLS 288 bytes (page-rounded backing).
Native worker stack request 64 KiB; main stack remains loader-owned/UNKNOWN.
Compiler reports a 24,016-byte static frame for xex_probe (not total call-chain
usage). Actual console peak memory, SVC permissions and timing are UNKNOWN.
Final runner: 18 bounded child commands, 41.626 summed child seconds.
run-Debug: 0.986 s, peak RSS 19,333,120 bytes.
run-Release: 0.182 s, peak RSS 10,354,688 bytes.
run-Sanitized: 0.456 s, peak RSS 52,969,472 bytes.
Docker-client RSS does not measure compiler/container peak RSS. No game FPS
or stock-clock performance conclusion follows from these host measurements.
