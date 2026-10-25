# Switch/Horizon synthetic runtime feasibility

Audit date: 2026-10-25. **Independent Switch track; not Phase 23.**

**VERIFIED:** an original component NRO cross-compiles and links with the
official devkitPro toolchain. Two fresh builds produce identical ELF/NRO
bytes. **UNKNOWN:** execution on Horizon. **BLOCKED:** unchanged pinned
ReXGlue-generated C++ compilation for Horizon. No game code was executed.

## Decisions and preservation

| Question | Decision | Evidence boundary |
| --- | --- | --- |
| A. Switch homebrew toolchain | **GO — VERIFIED** | Isolated official arm64 container, digest pinned, successful static AArch64 link and elf2nro. |
| B. Minimal original NRO build | **GO — BUILD VERIFIED / RUNTIME UNKNOWN** | Original checked component sources, libnx console/native-thread wrapper; validated NRO0. |
| C. Portable synthetic core | **CONDITIONAL GO — PARTIAL** | Unchanged memory/TLS/stop headers compile and link for Horizon; 63 host assertions pass. Hardware behavior is unknown. |
| D. ReXGlue synthetic PPC integration | **NO-GO for the unchanged pinned pipeline — BLOCKED** | Actual CLI emits the original leaf, but the real SDK headers reject Horizon. A focused adapter/emitter investigation is justified; integration is not achieved. |
| E. Actual hardware execution | **UNKNOWN** | The user confirms access to an existing homebrew Switch and will test the NRO later. No device was accessed or execution observed in this audit. Docker runs Linux build tools, not Horizon. |
| F. Brütal Legend Switch boot | **NO-GO** | No title execution, graphics/audio integration or playable-performance evidence. |

The new branch `research/switch-horizon-feasibility` starts at main commit
`1da0e11a54c3cd8b950d379c8045646bfc9e5051`
(`v0.0.21-entry-runtime-contracts`). Before creating its separate worktree,
the existing checkout was clean, including untracked-file inspection. Remote
main and `research/phase-22-blocked` were verified. Preservation commit
`f3c6affb913fa4fb4da1031fdbf6a9b4fca73416` has
[successful CI](https://github.com/Wilwous/brutal-legend-nx/actions/runs/40176913893).
No Phase 22 changes were merged into this worktree. Phase 22 remains
**BLOCKED** pending independent Xbox residency/ownership evidence; the user
has no Xbox 360 debugging-hardware access. Its private checkpoints, restore
procedure and diagnostics remain local. Checkpoint sentinels, the public Git
bundle and original XEX checksum were rechecked without repeating the full
private-tree scan. No private manifest or original XEX metadata is republished.

## Toolchain and reproducibility

No native devkitPro installation was found. The existing Docker/Colima daemon
supports Linux arm64, so no privileged host installation or unrelated
environment change was needed. The official
[devkitPro container recipe](https://github.com/devkitPro/docker/blob/master/devkita64/Dockerfile)
was inspected; the resolved image is locked by digest, rather than relying on
its mutable `latest` tag. Provisioning completed in 71.281 seconds within a
300-second download budget. No packages were installed into the existing SDK.

| Component | Verified version/identity |
| --- | --- |
| Image | `devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282`, linux/arm64 |
| devkitA64 | `r29.2-1`; GCC/G++ `15.2.0` |
| libnx | `4.12.0-1`; upstream tag commit `7644c9b26099aa2d2145bc72a21ee24190e92085` |
| switch-tools | `1.13.1-1` |
| devkita64-rules | `1.1.1-1` |
| C/C++ runtime | devkitA64 newlib/libstdc++ and libnx; selected exceptions, RTTI, atomics, mutexes, C++ TLS and joined threads compile/link |
| ReXGlue | v0.10.0, `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`; isolated nineteen-patch Phase 16 stage and matching host CLI |

The installed `svc.h`, `virtmem.h` and `thread.h` match the pinned libnx source
byte-for-byte. Its archive SHA-256 is
`12a59cdfdceab83e39ee129b0275dd803e393e83f19de3922a3b6cc95c767edd`.
All nineteen public SDK patch hashes match their existing lock. Exact local
tool locations, package inventory, source/header hashes and raw compiler output
are in the separate private audit directory, outside Git. The new
[toolchain lock](../../tests/switch-horizon/toolchain.json) contains public
identities only. GCC's standard library is not the Mac libc++ ABI; existing
Mach-O libraries cannot be linked into this NRO.

## Compatibility matrix

`SUPPORTED` below means API/build support within the identified scope. It
does not mean hardware execution was verified.

| Contract | Classification | Verified evidence / required work |
| --- | --- | --- |
| AArch64 compilation | SUPPORTED | GCC accepts Cortex-A57-targeted ARMv8-A sources; ELF machine 183 and PIE type 3 verified. |
| C++ ABI | REQUIRES ADAPTER | Rebuild every dependency with devkitA64; desktop dylibs and SDK platform selection are incompatible. Complete Xbox ABI remains unverified. |
| Exceptions and RTTI | SUPPORTED | Selected host throws/catches/type operations pass and NRO links with `-fexceptions -frtti`. Horizon unwind behavior is UNKNOWN. Guest unwind is not implemented. |
| Thread creation/join | SUPPORTED | `std::thread` and actual `threadCreate/Start/WaitForExit/Close` link. Native failure handling is explicit. Priority/core permissions need observation. |
| Native TLS | SUPPORTED | `thread_local`, libstdc++ TLS and libnx/newlib integration link; two-worker isolation verified on host only. |
| Guest TLS/PCR/r13/XThread | REQUIRES ADAPTER | Bounded original `Tls` is reusable; it is not the SDK XThread or guest PCR. No r13 state was exercised on Switch. |
| Mutex/synchronization | SUPPORTED | Selected `std::mutex`, joining and libnx wait calls link. Robust pthread mutexes, cancellation and full desktop waits are not implied. |
| Atomic stop | SUPPORTED | Unsigned atomics are compile-time lock-free; a finite native worker uses acquire/release stop. Hardware execution remains UNKNOWN. |
| Guest backing allocation | SUPPORTED | Original bounded 64-KiB arena and checked offsets compile/link without a fixed mapping. Host allocation/failure assertions pass. |
| Page alignment | SUPPORTED | Aligned allocator and 4-KiB guest-page contracts compile/link; host alignment test passes. No native guest mapping/protection fixture ran. |
| Guest read/write permissions | REQUIRES ADAPTER | Logical checked permissions pass on host. Native page transitions, loan lifetimes and alias effects must be tested independently. |
| Desktop fixed mappings/shared backing | UNSUPPORTED | Existing `mmap/mprotect/shm_open` backend cannot be treated as Horizon support. No 4-GiB fixed window is reserved. |
| Executable mappings/JIT | UNKNOWN | Not attempted or required for static output; ordinary permission changes cannot enable execute. No RWX/JIT shortcut. |
| File I/O / SD | REQUIRES ADAPTER | libnx exposes `fsdevMountSdmc`, newlib/devoptab and normal file operations. Mount/path/error/cleanup contracts and sandbox roots need original fixtures; no SD read/write was tested. |
| Stack/heap limits | UNKNOWN | 16-MiB heap requested with correct 8-byte `size_t`; a loader override can supersede it. Main stack/physical availability are loader-dependent. Native worker requests 64 KiB. |
| Module lifetime | REQUIRES ADAPTER | Original core is statically owned; thread leases, borrowed images, import admission and transactional SDK unload are not ported. |
| Timeouts/cooperative stop | REQUIRES ADAPTER | Host watchdog works. Native worker wait has a two-second limit and aborts on failure; no whole-process Switch watchdog or generated stop integration exists. |
| Logging/diagnostics | SUPPORTED | libnx framebuffer console builds without a GPU renderer, audio or network. Observation of startup failures/exit still requires hardware capture. |
| Desktop signal/fault integration | UNSUPPORTED | POSIX signal/ucontext/Mach handling is not a Horizon backend. No fault recovery is invented. |

API sources: pinned libnx
[thread declarations](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/include/switch/kernel/thread.h),
[newlib thread/TLS implementation](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/source/runtime/newlib.c),
[filesystem adapter](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/include/switch/runtime/devices/fs_dev.h).
The actual pinned desktop SDK has explicit
[OS admission](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/include/rex/platform.h)
and POSIX-specific
[memory](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/core/memory_posix.cpp),
[threading](https://github.com/rexglue/rexglue-sdk/blob/f5337cdc947ff6d4c4196737e2c807a48f2a1fc2/src/core/threading_posix.cpp)
and fault paths. libnx API presence is not a promise of all POSIX semantics.

## Memory model findings

**VERIFIED from source, not hardware:** libnx discovers separate ASLR, stack,
heap and alias ranges. Their actual addresses and sizes depend on the process;
no particular 39-bit layout or free physical budget is assumed. Its
[`virtmem` manager](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/source/kernel/virtmem.c)
queries mappings, tracks its own reservations and uses 4-KiB alignment. A
reservation object is bookkeeping, not a kernel allocation or committed page.
Find-and-reserve/map must share its mutex. A failed find or mapping must roll
back ownership; callers must preserve handles until unmap/release succeeds.
The
[reservation API](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/include/switch/kernel/virtmem.h)
does not establish arbitrary fixed-address placement.

[`svc.h`](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/include/switch/kernel/svc.h)
documents heap resizing in 2-MiB multiples. `svcSetMemoryPermission` permits
none/read/read-write transitions, not write-only or execute. `svcMapMemory`
borrows the source and removes source access; it is not a writable duplicate
alias. Other mapping operations have distinct state/capability requirements.
SVC declarations alone do not prove ordinary homebrew permission. No such
operation was invoked from this audit. No memory protection was disabled.

**PARTIAL feasibility:** the existing original `Memory` component can represent
a small guest arena as owned byte backing plus numeric guest offsets. Its
mutex covers validation and copies; stale epochs/owners/permissions are checked
at each access. This avoids requiring the guest address to equal a native
pointer. It does not provide host page isolation, sparse 4-GiB aliases, MMIO,
GPU sharing, SDK memory integration or a general allocator (released addresses
remain quarantined). Host alignment does not turn its `std::vector` backing
into independently protectable host pages. A native adapter must serialize
mapping changes with live copies/loans and reject unsupported transitions.

The
[libnx thread implementation](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/source/kernel/thread.c)
allocates stack/TLS/reentrancy backing and maps a guarded stack mirror under
the virtmem lock. This is concrete reference behavior, not verification of
Xbox stack aliases. Physical memory availability must be queried and measured
on the target, rather than inferred from virtual address capacity. The
[heap initializer](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/source/runtime/init.c)
also accepts loader-provided backing, so the compiled heap request is not a
guaranteed runtime allocation.

Static recompilation compiles original generated C++ into the NRO text section
before launch. Guest code bytes need no host execute permission. Runtime JIT
and libnx code-memory APIs are separate capability-sensitive designs, outside
this audit. Static code does not remove the need to validate guest memory and
function-call admission.

## Minimal NRO and artifact audit

[Original sources and commands](../../tests/switch-horizon/README.md).
`main.cpp` adds no SDK stubs. It calls the same portable fixture as the host,
then creates a finite libnx worker and sets its atomic stop flag. Creation,
start, wait and close errors are reported. A native wait failure aborts the
process without freeing live stack-owned state. Successful cleanup/exit on
Switch remains UNKNOWN. `std::thread::join` and libnx startup do not have a
whole-process hardware timeout here; a finite source loop is not a substitute
for one. The console is framebuffer text via libnx, not a GPU backend.

| Measured property | Result |
| --- | --- |
| Clean independent builds | 2; ELF and NRO byte-identical |
| ELF | 3,720,048 bytes including debug information; AArch64 ELF64 little-endian PIE |
| NRO | 258,048 bytes |
| NRO text / RO / data segments | 188,416 / 49,152 / 20,480 bytes; ordered, nonoverlapping and 4-KiB aligned |
| NRO BSS | 16,384 bytes; runtime allocator/loader overhead is additional |
| ELF load permissions | RX / R / RW; no writable-executable load segment |
| Dynamic libraries / undefined strong symbols | 0 / 0 |
| NRO SHA-256 | `f3f3944f48f1bb93299f2de730ca82318f08e663781bb271b74e002dec2b2135` |
| ELF SHA-256 | `92b73eec3888ef945756ffaf3d2a6fa6542f7d375edbbc606698d8c1d0612ce0` |
| GCC stack-usage report | Maximum individual reported frame in `probe.cpp`: 784 bytes; not a full call-chain/exception/thread high-water bound |

No `mmap`, `mprotect`, `shm_open`, `dlopen`, `pthread_cancel`, socket/connect or
JIT code-memory dependency is defined in the linked symbol census. This is a
symbol/disassembly audit, not a complete reachable-call proof. Twenty-eight
SVC wrapper immediates are present:

```text
01 02 03 04 05 06 07 08 09 0a 0b 0f 13 14 15 16
17 18 1a 1b 1c 1d 1f 21 26 28 29 52
```

**PARTIAL:** `0x52` (`svcUnmapTransferMemory`) is marked privileged by installed
libnx. It remains in the standard library's linked transfer-memory cleanup;
[`tmemClose`](https://github.com/switchbrew/libnx/blob/7644c9b26099aa2d2145bc72a21ee24190e92085/nx/source/kernel/tmem.c)
only calls unmap when `map_addr` is non-null. Presence is not proof that the
prototype invokes it. Its permission/reachability is not declared safe or
silently suppressed; retain it as a hardware/launch-environment audit item.
No application code invokes privileged mapping calls. Other wrapper permissions
also remain unobserved. NRO format/link checks do not establish loader admission.

## Actual ReXGlue compile-only experiment

The smallest probe used here is an independently encoded 16-byte leaf:
`lwz`, `addi`, `stw`, `blr`. The actual pinned CLI's raw `recompile-tests`
path generated one function containing four instruction comments, one
`REX_LOAD_U32` and one `REX_STORE_U32`. The raw path is not XEX/ELF loading.
No codegen output or PCH was edited. The checked-memory header was explicitly
selected; preprocessing/compilation failed before complete mediation could be
verified. No generated code was linked or executed.

The exact primary diagnostics, using stable container paths, were:

```text
/probe/sdk-include/rex/platform.h:42:2: error: #error Unsupported target OS.
/probe/sdk-include/rex/platform/dynlib.h:75:2: error: #error No library names provided for the target platform.
/probe/sdk-include/spdlog/details/os-inl.h:266:32: error: 'const struct tm' has no member named 'tm_gmtoff'
/probe/generated/ppc_config.h:70:3: warning: attributes are not permitted in this position [-Wattributes]
```

Compilation exits 1 without timeout. The alias/weak warning means GCC wrapper
semantics also require inspection; no link failure is invented because linking
was not attempted. The unchanged PCH imports SDK kernel/thread/MMIO/performance/
logging machinery, even for this leaf. The real checked adapter in turn needs
SDK Memory/UserModule ownership. Missing portable startup, context/TLS,
function registry, checked copies, error propagation and generated stop hooks
cannot be replaced with successful no-ops. No `__linux__` spoofing, unchecked
access fallback, generic trap recovery or broad SDK port was attempted.

The selected native fixture's C++ TLS is not guest TLS/PCR initialization.
The core stop state machine does not yet instrument a generated loop on
Horizon. Typed-vector signatures remain unsupported, complete Xbox ABI
unverified and guest exception dispatch/unwind incomplete. The scalar leaf
does not test vectors, direct/indirect dispatch or imports. Earlier host
Phase 11–21 results are retained as historical evidence, not reclassified
as Switch execution.

## Tests and retained failures

| Final check | PASS | FAIL / blocker |
| --- | ---: | --- |
| Original host Debug assertions | 21 | 0 |
| Original host Release assertions | 21 | 0 |
| Original host ASan+UBSan assertions | 21 | 0; no sanitizer findings |
| Fresh NRO build/link | 2 | 0 |
| NRO/ELF inspection | 2 | 0; explicit privileged-wrapper finding retained |
| Independent determinism comparison | 1 | 0; both ELF and NRO hashes match |
| Actual original PPC CLI generation/census | 1 | 0 |
| Unchanged generated Horizon TU compilation | 0 | **1 reproduced portability blocker**, not a passing compilation |

Host execution took 0.679 / 0.143 / 0.972 seconds (Debug / Release /
Sanitized); peak child RSS was 1,327,104 / 1,245,184 / 17,203,200 bytes.
Host compilation peaked at 182,091,776 bytes RSS. NRO builds took 2.267 and
2.056 seconds. Docker-client RSS for these builds was about 26 MiB; this
**does not measure compiler/container peak memory or Switch consumption**.
Every final process record has an explicit exit code. There were zero
unexpected process faults, zero expected deliberate faults and zero timeouts.
The two artifact inspections were repeated after adding explicit reporting of
the privileged wrapper; no native regression was repeated for that metadata edit.

Two exploratory build failures remain in private logs: treating libnx C-style
header initializers as application `-Werror` warnings, and an include-search
collision between the repository `VERSION` file and libc++ `<version>` on a
case-insensitive host. Correct system-header classification (`-isystem`) and
quote-only repository includes (`-iquote`) resolve those build-driver issues
without suppressing application warnings or changing runtime contracts.
The heap override was independently checked against libnx's actual `size_t`
definition and is now validated in the ELF. No prior native fault was retried.

All new host-supported checks run in CI. No shared runtime headers were changed;
unrelated historical Phase 22 regressions were not rerun. The preservation
branch and its diagnostics remain independent. Publication allows original
sources, build/test recipes, public lock identities and sanitized Markdown;
NRO/ELF/objects, generated C++, local logs and private manifests stay outside Git.

## Platform separation and next milestone

Keep three boundaries:

1. **Host tools:** the pinned generator, build orchestration, external process
   watchdog and analysis stay on the host. Do not cross-compile the analyzer
   into the NRO.
2. **Portable component contracts:** retain checked owner/range/epoch copies,
   numeric guest addresses, admission policy and cooperative lifecycle order.
   Reuse interfaces; changing historical Phase 11–21 assertions is unnecessary.
3. **Horizon adapters:** explicit allocation/mapping ownership and rollback,
   native thread/TLS/join/error handling, monotonic waits and failure capture.
   Each unsupported operation must refuse rather than silently succeed.

The three highest-priority blockers are:

1. **SDK/emitter portability:** platform admission, transitive desktop imports,
   logging/newlib differences and GCC alias semantics block even one generated
   checked scalar leaf. A narrow isolated header/runtime dependency audit is
   needed before any integrated PPC NRO claim.
2. **Faithful native memory/thread ownership:** mapping, page permission,
   alias/rollback and guest stack/TLS lifetimes require actual Horizon contracts.
   The bounded logical arena proves none of the full SDK's fixed guest layout.
3. **Hardware observability and bounded execution:** establish suitable existing
   homebrew access, capture launch failures/results and verify a whole-process
   timeout strategy before running deeper original fixtures. No device access
   or firmware modification is authorized or attempted here.

**Recommended next Switch milestone:** validate this exact original component
NRO on an already available, authorized homebrew setup, with recorded loader
limits, stable diagnostics and a bounded test/recovery procedure. If hardware
is unavailable, the independent compile-only subtask is to identify the
minimum SDK header/context/checked-copy dependencies and propose a narrow
Horizon seam; do not begin a broad port or claim end-to-end execution.

The user confirmed existing homebrew Switch access after the local audit and
will perform the hardware check later. This removes the hardware-availability
question, not the execution/permission/timeout evidence gap. The expected
successful screen is `PASS=23 FAIL=0` (21 portable assertions plus native
wait and close). Record the screen/video and launch environment, including any
startup abort or error code; a missing result is not a pass. The program
automatically exits after displaying the result for two seconds. No hardware
test result is available yet.

Peak actual target heap, native stack high-water, thread/resource limits,
startup/teardown reliability, SD behavior and SVC permissions remain UNKNOWN.
Two portable workers plus one native worker run sequentially; at most the main
thread and two test workers are intended to overlap. The NRO has no generated
PPC code, so its size cannot predict game-derived code growth. Checked copies
currently allocate small byte vectors, lock and scan metadata; cost and CPU
bottlenecks must be measured on target. No FPS estimate is made. Future GPU,
Xenos/shader, audio/codec, file streaming and Switch driver work remains
separate and unassessed by these tests. Synthetic build success does not
establish game compatibility or unblock Xbox Phase 22.

## Reproduction

The commands in the [fixture README](../../tests/switch-horizon/README.md) are
the exact public entry points. Use a fresh private output path and the pinned
image; the runners do not pull images implicitly or connect to a console.
For branch isolation, after verifying a clean existing checkout and refs:

```sh
git worktree add -b research/switch-horizon-feasibility "$SWITCH_WORKTREE" main
```

Run `run.py --switch-build` for the original host/NRO scope and separately
`integration.py` with the matching private isolated SDK/CLI paths for the
classified codegen compile-only result. All individual children are bounded
to 30 seconds; containers are explicitly cleaned on timeout. Reusing a completed
output directory is refused. No full SDK rebuild, game build, XEX loader retry
or proprietary execution belongs in this procedure. No completion tag,
main merge or GitHub Release is created for this audit.
