# Minimal Horizon adapter for an original generated PPC leaf

2026-10-25. **BUILD VERIFIED / HARDWARE EXECUTION UNKNOWN.**
The user reports the previous native probe running on a real Switch with
`PASS=23 FAIL=0`: **VERIFIED for that probe only**, based on external observation.
It contains no generated PPC execution. The new PPC Runtime Probe has not yet
been observed on hardware. Game-code execution remains **NO-GO**.

## Preserved baseline and boundaries

Development uses an isolated worktree from `95e9199c14673b970a8293463ba7073b6f0c00ad`
on the independent Switch research line. Main and `research/phase-22-blocked`
remain at their prior commits; Xbox Phase 22 is still **BLOCKED**, with original
XEX residency **UNKNOWN/BLOCKED**. No loader/binder rule is changed. No Phase 23,
completion tag, merge or GitHub Release is created.

The original XEX was hashed, not loaded or executed:
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
Private checkpoints, game configuration and generated title output remain in
place. Checkpoint manifest/report sentinel hashes are rechecked separately;
the entire large private tree is not claimed to have been rehashed in this task.
The native probe and its ZIP retain their prior hashes, and an additional local
byte-verified NRO backup is retained. No SD writes are needed to build/package
this experiment.

The unchanged [toolchain lock](../../tests/switch-horizon/toolchain.json) pins
Linux/arm64 image digest `1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`,
devkitA64 r29.2-1 / GCC 15.2.0, libnx 4.12.0-1, switch-tools 1.13.1-1 and
rules 1.1.1-1. The installed libnx archive matches
`12a59cdfdceab83e39ee129b0275dd803e393e83f19de3922a3b6cc95c767edd`.
ReXGlue is v0.10.0 at `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2` with the
unchanged nineteen-patch [SDK lock](../../tools/sdk/xex-binding-lock.json).

## What actually changed

| Prior blocker | Scoped resolution | Remaining scope |
| --- | --- | --- |
| Unsupported target OS | Separate BSD-licensed `horizon-leaf.patch` admits `__SWITCH__` for context headers. | Full SDK CMake/runtime still does not support Horizon. No Linux/macOS spoofing. |
| `tm_gmtoff`, dynamic library platform errors | Explicit generator templates include the actual SDK `ppc/context.h` instead of the desktop umbrella/logging/PCH dependency closure. | spdlog portability and desktop dynamic loading are not ported or required by this leaf. |
| GCC alias attribute placement | Leaf template places alias attributes after the extern-C declarator; ELF verifies the weak public symbol and strong implementation share one address. | Darwin uses a separate weak forwarding function in host tests. |
| Checked guest memory | Dedicated native-thread binding delegates both generated memory sites to the existing original bounded owner-checked arena. | No raw 4-GiB guest base, aliases, MMIO, hardware page isolation or XEX memory backend. |
| Thread/context dependency | libnx native worker, bounded wait, join/close; real SDK PPCContext, per-native-thread scope and explicit foreign/unbound-context refusal. | No Xbox XThread, PCR, guest TLS, guest stack or exception implementation. |

No Xbox exports or fake successful SDK APIs are introduced. No dependency on
`librexruntime` is linked into the NRO. Required SDK components are the real
`ppc/context.h`, `ppc/func.h`, `platform/fpscr.h`, `types.h`, `assert.h`, patched
`platform.h` and their pinned SIMDe header closure. Context definitions are not
copied/reinvented in this repository. libnx/newlib/libstdc++ supply native mutex,
thread-local storage, C++ exceptions and ordinary allocation. A guest condition
variable, APC, cancellation or guest atomic contract is not required or supplied.

## Actual generation and independent identity check

The independently authored 16-byte fixture is the same original leaf:
`lwz r3,0(r4); addi r3,r3,1; stw r3,0(r4); blr`. Input SHA-256:
`165a81367d48577ba244f980d7e8f521965d8815b2dfdee75c4502b1ac406a24`.
It has one function, no imports, calls, relocations, vectors or stack/TLS accesses.
It is not an XEX image or a test of Xbox module loading.

The production `rexglue recompile-tests` decoder/discovery/emitter generates it.
The small optional CLI patch exposes the existing production
`TemplateRegistry::loadOverrides` API through `--templates`; without that option
default output remains identical to its pinned prior SHA-256. The explicit
original templates change includes/ABI helper definitions, not the generated
instruction body. No generated C++ is manually edited. Independent PPC field
decoding, exact ordered statement comparison and whole-function comparison to
the default production output all pass. Generated function TU SHA-256:
`88777beb155234650d2e463470757cf3e7d7e5c11b33e295c9ead470cc89909c`.
The [leaf lock](../../tests/horizon-runtime/lock.json) pins headers, patch,
original CLI, default command source and all generated products.

The preparation helper reads a previously built coherent macOS Release SDK,
reconstructs all nineteen patches against pristine tracked source and compares
all affected files, verifies critical staged headers, then compiles a **private
copy** of one command object and relinks a separate CLI. Ninja is used only to
print commands; no existing build rule is run. Existing SDK source, objects,
CLI and installed libraries are not modified. This incremental preparation
method is verified on macOS ARM64 only. A clean isolated SDK build can instead
apply the extra patch after the nineteen locked patches. Generated fixture
output and all SDK/build products stay outside Git.

## Contracts and limits

Both actual generated memory operations use `LeafScope`, with no raw-pointer
fallback. A scope binds one arena and one exact PPCContext on the current native
thread. Missing/foreign bindings, non-null raw bases and nested scope admission
fail explicitly. Memory retains allocation owner/id/epoch, full-span bounds,
software read/write permissions and address quarantine after release.

The fixtures verify the result 41→42 in r3 and big-endian storage, a valid
unaligned cross-page span, read-only store refusal without memory mutation,
null/out-of-range/MMIO-shaped addresses, decommitted/released memory,
foreign owner/double release, exhausted/invalid arenas and final zero live
allocations. The UINT32_MAX input also verifies the actual 64-bit GPR addition
and 32-bit store truncation. Permission metadata mediates these explicit
accesses; it does not provide complete guest memory isolation.

All PPCContext bytes except the modified r3 are compared, including selected
GPR/FPR/vector sentinels. r1/r2 sentinel values and zero r13 are preserved.
This proves preservation for this leaf only: it does not initialize an Xbox
stack/PCR/TLS, validate their ABI conventions, execute vector instructions or
prove complete Xbox ABI correctness. Host tests additionally demonstrate that
a second native thread cannot borrow the first thread's bound context.
Horizon thread-local behavior is linked but still requires console observation.

Eight compiler refusal probes cover absent raw-address, indirect/direct-call,
vector, timebase, trap and 64-bit memory helpers, plus a typed-vector argument
passed to the context/base entry signature. They must produce the specific
compiler diagnostic and exit 1, never crash or time out. Unknown imports,
unsupported signatures, guest exception handling/unwind and atomics have no
permissive implementation. Broader instruction coverage is not claimed.

## Final local regression evidence

All execution children use an external monotonic 30-second process-group
watchdog. Containers are digest locked, offline, read-only for source/headers,
without host devices or privileges; timeout cleanup explicitly removes them.
No new NRO is executed on the host.

| Suite/configuration | PASS | FAIL | Boundary |
| --- | ---: | ---: | --- |
| Generated leaf / real context — Debug | 59 | 0 | Two 29-check probes plus native-thread isolation |
| Generated leaf / real context — Release | 59 | 0 | Same contracts |
| Generated leaf / real context — ASan+UBSan | 59 | 0 | No sanitizer diagnostics |
| Unsupported compile probes | 8 | 0 | Expected compiler refusals, separate from runtime assertions |
| Previous native probe + result UI, three host profiles | 81 | 0 | Original source/output preserved |
| Existing foundation, three host profiles | 18 | 0 | Six groups/profile, including 4,000 modeled operations/group run |
| Existing USB deployment/package mock units | 45 | 0 | Original destination/API still pass |
| Existing synthetic ELF sanitizer units | 8 | 0 | Original sanitization policy unchanged |
| New encoding, scoped deployment and package units | 21 | 0 | Mock filesystems only |
| Clean Horizon links + ELF/NRO inspections | 2 + 2 | 0 | Identical ELF/NRO/object bytes |

There are zero unexpected faults, timeouts or sanitizer findings in these final
runs. Eight expected compiler errors are retained as successful refusal checks;
there are no deliberate host memory faults. Earlier preparation/verification
failures (overlapping patch verification, missing relative command headers and
blank-line correspondence parsing) and an early packaging attempt before its
receipt existed remain private diagnostics. They were harness issues, not
proprietary execution failures, and final checks retain the original safety
assertions.

The final generation/host/cross-build runner used 20 bounded processes and
17.869 seconds of summed child wall time in this local run. Individual host
execution measurements were Debug 0.192 s / 1,245,184 bytes peak RSS, Release
0.163 s / 1,196,032 bytes, Sanitized 0.290 s / 19,070,976 bytes.
The two Horizon build commands took 3.805 s and 3.512 s; Docker-client RSS is
not a measurement of compiler/container peak RSS. These timings are not Switch
performance evidence.

## Linked artifact and hardware handoff

NRO: **290,210 bytes**, SHA-256
`06565f0e8e0e1fb30d4d1656ba9981375b0263efcf9649fa89c246955e9383df`.
ELF SHA-256:
`fb2a76a7fbd78d636e2bec8dad63d317b6ecbda913e0086d72ce6d6a5e1b3d2b`.
ELF inspection verifies AArch64 PIE, RX/R/RW mappings, no W+X mapping, zero
DT_NEEDED and zero unresolved strong symbols. The generated weak/strong alias
is present at one address. The NRO contains NACP 0.2.0, an original JPEG icon,
no RomFS and no private build/user paths. The unchanged bounded newlib diagnostic
sanitizer modifies only its two pinned read-only prefixes, never instructions
or layout. A libnx SVC 0x52 wrapper remains present as in the native probe;
its presence is not evidence of granted permissions or execution.

The separate SD destination is
`sdmc:/switch/brutal-legend-nx-ppc/brutal-legend-nx-ppc.nro`.
The old native destination and package remain intact. The new deploy wrapper
reuses verified USB discovery, identity checks, no-follow directory traversal,
exclusive writes, byte verification, sync and backup behavior. It cannot select
the old application; replacement of a different existing PPC NRO requires an
explicit `--replace-selected-ppc`. No actual SD deployment is performed in this
adapter task. The ZIP has only six intended files under the new application
folder. Firmware, Hekate, Atmosphère and unrelated SD data are not changed.

The new screen reports actual console/platform initialization, worker setup,
guest-memory initialization, generated entry invocation, expected result,
cleanup and PASS/FAIL. Success is **expected** to read `PASS=38 FAIL=0`:
29 leaf checks plus nine actual native/stage checks. The diagnostic counts are
`entries=10`, completed checked `loads=4`, `stores=3`; refused operations are
separate checks. It waits for PLUS. A failed two-second native join wait aborts
the process without destroying a live worker's state. PLUS waiting itself has
no timeout. **None of those console results have been observed yet.**

## Reproduction

Run from the public worktree. Variables name private directories supplied by
the developer; no proprietary input is required. Use the existing coherent
nineteen-patch SDK source/build/stage and pinned Ninja/Pillow environments.
The SDK source path must be the prior macOS Release build layout expected by
`prepare.py`; do not point it at a globally installed SDK for mutation.

```sh
python3 tests/horizon-runtime/prepare.py \
  --sdk-source "$SDK_SOURCE" --sdk-build "$SDK_BUILD" \
  --sdk-stage "$SDK_STAGE" --ninja "$PINNED_NINJA" \
  --workdir "$HORIZON_WORK/toolchain"
# In the pinned Pillow 12.3.0 environment, create the original existing icon:
python3 -c 'import sys; from pathlib import Path; sys.path.insert(0,"tools/switch"); from icon import create; create(Path(sys.argv[1]))' "$HORIZON_WORK/icon.jpg"
python3 tests/horizon-runtime/run.py \
  --toolchain "$HORIZON_WORK/toolchain" --workdir "$HORIZON_WORK/build" \
  --switch-build --icon "$HORIZON_WORK/icon.jpg"
python3 tests/horizon-runtime/test_contract.py
python3 tests/switch-horizon/test_deploy.py
python3 tests/switch-horizon/test_sanitize.py
python3 tests/switch-horizon/run.py --workdir "$HORIZON_WORK/native-host"
python3 tests/runtime/run.py --workdir "$HORIZON_WORK/foundation-Debug"
python3 tests/runtime/run.py --workdir "$HORIZON_WORK/foundation-Release" --optimization 2
python3 tests/runtime/run.py --workdir "$HORIZON_WORK/foundation-Sanitized" --sanitize
python3 tests/horizon-runtime/package.py \
  --from-build "$HORIZON_WORK/build" --workdir "$HORIZON_WORK/ready"
# Optional, deliberate deployment to a freshly verified Hekate USB mount:
python3 tests/horizon-runtime/deploy.py --package "$HORIZON_WORK/ready/package"
```

Local generation, builds, watchdog receipts, disassembly, raw/sanitized ELF,
NRO/ZIP and exact restoration/backup paths are recorded in the private report.
Public CI runs the 21 new encoding/package/mock units plus existing component
jobs; it does **not** recreate the private pinned-CLI generation or run a Switch.

## Next gate

**GO for external observation of this exact original-synthetic NRO only.**
Record its complete screen and counts, NRO hash, hbmenu launch mode and
Horizon/Atmosphère/loader versions, including native wait/cleanup and PLUS exit.
Until then: **BUILD VERIFIED / HARDWARE EXECUTION UNKNOWN**. A later separate
synthetic milestone can examine a real guest stack and PCR/TLS contract after
hardware evidence. Complete XThread/import/exception integration, broad memory
instrumentation and game compatibility remain **BLOCKED/UNKNOWN**.
**Game-code execution is NO-GO; Xbox residency evidence is still missing.**
