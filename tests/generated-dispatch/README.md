# Original generated PPC import caller

The independently encoded 236-byte program has eight functions / 59 instructions.
The pinned ReXGlue `recompile-tests` decoder/emitter generates its actual caller;
that unedited translation unit is compiled separately and executed on real SDK
XThreads. No game files or generated game code are needed by these tests.

Use the isolated eighteen-patch SDK from `tools/sdk/tls-import-lock.json`, with
matching headers, runtime and CLI. Set the variables to private directories
outside the checkout; each work directory must be new:

```sh
python3 tests/generated-dispatch/test_fixture.py
python3 tests/generated-dispatch/run.py --sdk "$SDK_DEBUG" --rexglue "$REXGLUE_DEBUG" --workdir "$PHASE15/generated-Debug"
python3 tests/generated-dispatch/run.py --sdk "$SDK_RELEASE" --rexglue "$REXGLUE_RELEASE" --workdir "$PHASE15/generated-Release"
python3 tests/generated-dispatch/run.py --sdk "$SDK_SANITIZED" --rexglue "$REXGLUE_SANITIZED" --workdir "$PHASE15/generated-Sanitized" --sanitize
```

Every generator/compiler/test child has an external 30-second process-group
watchdog. Ordinary children scrub inherited runtime/loader overrides. Results,
input words, generated C++, binaries, logs and memory diagnostics stay private.
The runner verifies SDK revision/runtime digest, reviewed generated-body digest,
function/instruction census, call graph and preprocessed checked macro selection.

The original XEX2 fixture embeds the same PPC bytes in its code section and uses
actual SDK loading, import records, module leases and header TLS. Raw-test symbols
use the generator's fixed analysis coordinate. Explicit host metadata maps them
to the loaded image and imports; the two generated import-gate bodies are never
executed or admitted as synthetic functions. This is **PARTIAL automatic XEX to
codegen integration**, not production XEX generation or title startup.

Thirteen groups cover generated success/error branches, pointer/permission and
allocation errors, checked scalar/vector access, denied operations, dispatch
identity/nesting, active-call unload/release, four concurrent XThreads and eight
reload cycles. Main entry and its generated scalar callee contain 30 static
memory operations, all checked; a successful invocation completes all 30.
Generated indirect import sites reject even registered local entry/callee and direct-only gate targets. Separate negative roots deliberately reject raw atomics/partial vectors, MMIO,
unregistered fetch/exception paths and unsupported signatures. SDK scratch,
loader operations and host control are outside the generated-memory census.

GPR64 register arguments/returns and one explicitly encoded 32-bit ninth stack
argument are tested, with r1/r2/r13, r14/r15, f14 and v20 preservation. Host setup
provides the caller linkage area and observes errors. C++ failure containment
restores the fixture context; it does not implement guest dispatch/unwind or
prove full ABI semantics. Typed-vector signatures remain denied. Full Xbox ABI,
DLL TLS indexing, private game mediation and Switch execution remain unverified.

[Full evidence, reproduction, measurements and readiness](../../docs/research/generated-ppc-end-to-end-dispatch.md).
