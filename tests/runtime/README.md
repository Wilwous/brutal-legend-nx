# Runtime component harness

Original standalone C++20 memory/TLS components and diagnostics; no game files, generated game code, graphics/audio initialization, Xbox keys or guest entry point.

Use Python 3.9+ and Clang with C++20/threads. Output must remain outside the checkout:

```sh
python3 tests/runtime/run.py --workdir "$PRIVATE_TEST_OUTPUT/O0"
python3 tests/runtime/run.py --workdir "$PRIVATE_TEST_OUTPUT/O2" --optimization 2
python3 tests/runtime/run.py --workdir "$PRIVATE_TEST_OUTPUT/sanitized" --sanitize
```

`run.py` emits JSON results and retains compiler/process stdout/stderr privately. Compiler/child subprocesses have 45-second timeouts. `-iquote` is intentional: adding this checkout with `-I` lets `VERSION` shadow the C++ `<version>` header on a case-insensitive filesystem.

The six original test groups cover bounds, logical protections, endian helpers, allocation failures/alignment, stale mapping epochs/ownership, 4,000 seeded memory operations against a byte-array model, TLS layout/exhaustion/free/reuse, synchronized host-thread isolation/cleanup and explicit failure at the diagnostic execution boundary. API misuse returns typed `Error` failures rather than an unchecked pointer or null-success value. Arena and TLS manager lifetimes must outlive their callers/scopes, and thread scopes must be destroyed on their creating host thread.

The memory component is a bounded component-test foundation: 4 KiB logical guest pages, checked copies, no raw host pointer export, no physical aliases/MMIO, no execute permission, no subrange reserve/commit API and no native `mprotect`. Freed guest addresses are quarantined until arena destruction, so this is not a recycling production heap. TLS follows the inspected static-prefix/dynamic-slot layout, but does not implement PCR/r13, fibers/APCs or guest XThread binding. [Contract details](../../docs/research/runtime-foundation.md).

Optional exact SDK leaf checks, requiring public open-source source files only:

```sh
python3 tests/runtime/run.py --workdir "$PRIVATE_TEST_OUTPUT/sdk-leaf" \
  --sdk-source-root "$ORIGINAL_SDK_SOURCE"
```

The runner verifies four original source hashes, checks/applies the published patch to a private copy, then compiles the exact Get/Set TLS, STFS and protection-conversion functions using original guarded adapters. This validates leaf behavior, not the full SDK ABI, guest scheduling or arbitrary pointer safety. Extracted/generated SDK artifacts remain outside Git and retain their upstream license obligations.

Optional real memory-subsystem probe against the existing host SDK:

```sh
python3 tests/runtime/run.py --workdir "$PRIVATE_TEST_OUTPUT/sdk-native" \
  --sdk-binary-root "$SDK_BINARY_ROOT"
```

The probe calls `Memory::Initialize` directly and checks allocation/alignment, actual mappings and pointer round-trips, endian access, protection queries/changes, reserve/commit/decommit/release. It never creates a Runtime, loads an XEX or starts guest code. The prebuilt library itself is not sanitizer-instrumented. Fault handling is separate: an opt-in `--probe-protection-fault` child deliberately writes to read-only memory, with core dumps disabled and a 45-second timeout. In Phase 5 that single child timed out and was killed/reaped; enforcement/fault recovery is **UNKNOWN**, not PASS. Do not confuse the normal probe's protection-query success with that unverified behavior. The fault probe is excluded from ordinary runs and CI.

CI builds/runs only the original dependency-free components at O0/O2 and with ASan/UBSan. It neither downloads the SDK nor runs optional SDK probes or game compilation.
