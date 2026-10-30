# N2 — production PPC bound to an original native image

2026-10-30. Host synthetic only; historical G4/G8 BLOCKED, G5 UNKNOWN, game NO-GO.

The exact earlier uncompressed XEX with a PE tail beyond its descriptor extent
was submitted to the unchanged pinned production CLI. It refused PE headers with
0xC0000001 in0.112s. This failure remains private. No strict loader was relaxed.

A **different original synthetic codegen fixture** explicitly supplies a fifth
zero page and extends its descriptor/security/PE declarations consistently. This
is not an equivalent rewritten original title, nor a silent retry of its failed
load. Its source identity is distinct. NativeImagePlan independently materializes
three64-KiB regions plus one128-KiB RW region. The RW prefix1024 bytes is file-backed;
the rest is explicitly authored zero-fill, including the fifth page. Codegen does
not consume NativeImagePlan: the author supplies both this supported XEX and the
native initialization contract. Their byte agreement remains a finite fixture
proof rather than a generic projection theorem.

## Binding and execution

The production v0.10.0 CLI/header overlay and nineteen-patch SDK are reused without
modification. The runner verifies CLI and SDK body hashes, generates twice, checks
unchanged output hashes, compares all instruction comments against the independent
public decoder, verifies every hook, and matches the generated function table to
producer boundaries. Generated C++ is never edited.

The original workload has3 PPC functions /35 instruction words, one real SDK
import,9 checked memory sites and15 hooks. A direct and an indirect helper call
are both admitted. The native map includes4 function records (three generated,
one imported). Runtime dispatch count is4 per successful entry, including the
root, two helper invocations and SDK call. The supported self-loop has a polled
backward branch. No proprietary fixture or execution is involved.

Native Binding checks source/plan/code/SDK/runtime metadata; entry/TLS identity;
actual function maps, owned boundaries, duplicates; exact import signature; and
SHA256 of canonical boundaries/control sites/roots/exit. Scope validates actual
call edges, indirect targets, memory widths, current function and return LR.
Unknown targets and exceptional transfers refuse. Image handles are lent as
leases to each execution scope; active unload refuses before freeing anything.
The immutable generated-code digest is checked by the build driver immediately
before compilation. Compiled-in hashes are build attestations, not runtime
measurement of native machine-code bytes or a defense against a malicious compiler.

Image, semantic plan, source PPC, generated text, SDK contract and runtime source
contract have separate fingerprints. Per-profile compiler flags, resulting
artifacts and watchdog records establish build provenance separately. The runner
records runtime source-contract identity; it does not claim binary equivalence
between O0/O2/sanitized builds.

GuestThread initializes the selected existing stack/PCR/static and dynamic TLS
layout. Native Binding checks static TLS bytes against admitted RO image data.
Scope additionally checks r1 alignment/bounds, r2, r13/PCR links and MSR. The test
observes initial FPR1/CTR/LR, selected register preservation and independent PCR/TLS.
This is not full Xbox ABI verification or title startup inference.

Only RtlCompareMemoryUlong is exercised in this new workload. Its exact pinned
SDK body is extracted and hashed, receives a bounded copied IN snapshot through
mapped_void, and publishes r3 only after result checks. One call per entry;
two successful entries (direct host and actual joined GuestTask) give SDK2/2.
The other three existing imports retain their Stage15 tests; they are not claimed
exercised by N2. Unsupported signatures/exports remain denied.

## Independent oracle and tests

Constants10 and static TLS5 feed the generated helper twice (+7 each). Three of
four independently initialized words match the SDK pattern, giving3. Thus the
expected result is10+5+7+7+3=32. BSS records29 before the SDK addition and32 after.
These values are checked independently of generated C++.

Debug/O0, Release/O2 and ASan+UBSan each pass16 positive and15 negative semantic
checks (31 total); bytewise bulk count is0. Each returns32, SDK2/2, zero live
allocations/leases. Coverage includes wrong source/stale code, changed boundary/
site metadata, static TLS mismatch, bad PCR, invalid entry/indirect target,
exceptional-transfer denial, RO/unknown memory, corrupted return, active unload,
cooperative loop stop, thread-creation injection, native join and cleanup.

Initial diagnostics remain preserved: a CTR union access caused a compile error;
the first executable exposed a fixture initialization error (only one matching
word instead of the authored three, result23). The fixture bytes were corrected;
the independent expected result was not relaxed. Final generator outputs are
unchanged throughout compilation/execution.

```sh
python3 tests/native-image-binding/run.py --workdir "$NEW_N2_OUTPUT" \
  --toolchain "$PREPARED_HORIZON_CLI" --sdk-source "$PINNED_SDK19_SOURCE"
```

Every child has a30-second external group watchdog. Raw paths, tool output,
source copies and generated artifacts stay outside Git. No claim is made for
unbounded/arbitrary guest programs, physical Horizon isolation, all exception
contexts, complete rollback under arbitrary native crashes or title correctness.
