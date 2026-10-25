# Original Horizon feasibility fixtures

**BUILD VERIFIED / RUNTIME UNKNOWN.** These sources contain no game code and
do not provide a ReXGlue SDK port. [Measured audit and decisions](../../docs/research/switch-horizon-feasibility.md).

For the ready-to-copy NRO/ZIP, verified USB deployment and PLUS-to-exit screen,
use the [current USB packaging workflow](../../docs/research/switch-usb-deployment.md).

`probe.cpp` uses the unchanged public bounded checked-memory, host-thread TLS
and cooperative-stop components. Twenty-one assertions cover allocation,
alignment, endian/cross-page copies, uncommitted/RO/stale/freed/overflow refusal,
thread-local isolation, ownership cleanup, exceptions/RTTI and stop lifecycle.
`main.cpp` adds libnx startup, framebuffer console output (no GPU backend),
a finite native worker, atomic stop, timed native wait and resource cleanup.
No NRO is executed by these scripts. No network, audio, guest code or device
connection is required for building. Main stack size is provided by the loader;
the explicit native worker requests 64 KiB. Libnx can use a loader heap override
instead of the prototype's 16-MiB heap request.

From the repository root, choose **fresh directories outside Git**:

```sh
python3 tests/switch-horizon/run.py --workdir "$PRIVATE_WORK/horizon-host"
docker pull --platform linux/arm64 \
  devkitpro/devkita64@sha256:1fc388c3a0d34bd2045a6dadcb1020e069d5f876a187fd705de14b4440c00282
python3 tests/switch-horizon/run.py --workdir "$PRIVATE_WORK/horizon-build" --switch-build
```

The first command builds/executes Debug, Release and ASan+UBSan host children,
including six result-loop checks per profile in addition to the 21 components.
The second runner additionally builds the NRO twice in fresh directories and
compares ELF/NRO hashes. It validates ELF64/AArch64 PIE, segment bounds, absence
of RWX load segments, the real 8-byte `__nx_heap_size` contract, NRO0 layout,
static linkage, forbidden desktop/JIT dependencies and required libnx symbols.
The exact SVC-wrapper census is recorded; **presence does not establish that
a syscall is invoked or permitted**. Privileged `0x52` is explicitly reported.
Do not interpret that audit as a complete static call graph or permission proof.

Each build/run/inspection child has an external monotonic 30-second watchdog,
with process-group termination and no core dumps. Docker containers have no
network/devices/privileges, a read-only root/source mount, a private writable
output mount, 1-GiB memory cap and two CPUs. Containers are explicitly removed
even if the Docker client times out. Captured Docker RSS belongs to the client,
not the cross compiler or Switch. Logs/results/binaries remain private.

To reproduce the classified ReXGlue portability blocker, use an existing
isolated v0.10.0 SDK staged with the nineteen-patch
[Phase 16 lock](../../tools/sdk/xex-binding-lock.json) and its matching CLI:

```sh
python3 tests/switch-horizon/integration.py \
  --workdir "$PRIVATE_WORK/horizon-ppc" \
  --sdk "$SDK_WORK/staged/Release" \
  --rexglue "$SDK_WORK/sdk/out/mac-arm64/Release/rexglue"
```

The runner independently encodes a 16-byte / four-instruction PPC leaf, invokes
the actual `recompile-tests` CLI, checks the function/access census, and attempts
to compile its **unchanged** generated TU/PCH with the checked-memory opt-in.
It does not spoof Linux, patch headers, link a runtime or execute generated code.
Exit 0 means the **audit completed** with a classified result, not that the
generated object compiled: inspect `results.json` and `horizon-compile.stderr`.
Unexpected failures/timeouts return nonzero. Existing historical contracts and
Phase 22 artifacts are not rebuilt or modified.

For later authorized hardware validation, preserve `switch-0/probe.nro` and its
SHA-256. First establish suitable existing homebrew access and a way to observe
failures. Record console revision, firmware/homebrew environment, launch mode,
heap override, process limits and output/exit status. This task does not supply
a whole-process hardware watchdog: the portable `std::thread::join` and libnx
initialization paths still require hardware observation. A native wait timeout
aborts rather than freeing live worker state. Do not use this NRO to infer
Xbox XThread/PCR behavior, complete ABI support or game readiness.

The user has confirmed existing homebrew Switch access and will test later.
Expected success output is `PASS=23 FAIL=0`; the current result screen waits for
PLUS (+) or normal applet shutdown. Capture a video/screen and the launch mode,
including any abort/error code. No observed hardware result exists yet.
