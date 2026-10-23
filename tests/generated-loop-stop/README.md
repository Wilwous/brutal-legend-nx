# Original generated PPC stop-boundary fixtures

The unchanged pinned nineteen-patch SDK supports production `midasm_hook`
configuration. Original PPC is encoded into XEX/PE with PDATA; the CLI emits the
configured checks before complete instructions. Generated C++ is regenerated,
hashed and compiled without manual editing. No new SDK patch is required.

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$TESTS"
python3 tests/generated-loop-stop/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# Use --sanitize only with the Sanitized SDK.
python3 tests/generated-loop-stop/test_fixture.py
```

All variables name private output/build directories outside this checkout.
`TESTS` and `OUTPUT` must not exist. The full workflow verifies the SDK lock,
source, dependencies and staged headers, and runs Phase 5–18 regressions in
Debug, Release and Sanitized. See the [SDK build options](../xex-binding/README.md).

The nine-function, 182-instruction original product contains six independently
censused backward branches. Entry, backedge, pre-memory, pre/post-call and bounded
straight-block hooks are audited against actual emitted instructions and gotos.
Missing or misplaced hooks fail before compilation/execution. A finite memory
loop without stop hooks is used only as the benchmark reference, with identical
PPC memory-loop words, checked memory and bounded inputs. Its other functions
are never admitted/executed. It is excluded from stop-coverage claims.

Nineteen isolated children test natural/early returns, finite/infinite/nested/
memory loops, long blocks, generated entry, imports/adapter work, active unload,
four XThreads, concurrent requesters, eight cycles, asynchronous observation,
checked access failures, callback errors, initialization, policy and benchmark.
The failed-join child expects process exit 70, not orderly resource reclamation.
Each child uses an external monotonic 30-second process-group watchdog; joins
and observer rendezvous have independent shorter limits.

A private typed host exception unwinds original generated C++ to its execution
scope. Active imports finish before the next emitted check. The trusted host
restores its PPC checkpoint before native thread exit; canceled guest frames
cannot resume. This does not implement guest exception dispatch/unwind or
interrupt SDK instructions. Production hook placement is configuration-driven
for the explicitly audited original scope, not automatic arbitrary-title CFG
instrumentation.

[Coverage, measurements, ownership and remaining gates](../../docs/research/generated-loop-stop-boundaries.md).
