# Original image-pointer and cooperative-stop fixtures

This extends the Phase 16 production workflow with the same original XEX,
unchanged generated C++ and isolated nineteen-patch SDK. No new SDK patch or
proprietary execution is involved.

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$TESTS"
```

Both variables must name private directories outside this checkout. `SDK_WORK`
is the verified Phase 16 build workspace; `TESTS` must not exist. The workflow
verifies the patch lock, source, dependencies and staged headers, then runs Phase
5–17 suites across Debug, Release and Sanitized. It also supports the documented
[private SDK build options](../xex-binding/README.md). Never use a global SDK.

To run only the new integration suite, with explicit private paths:

```sh
python3 tests/image-pointer-shutdown/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# For a Sanitized SDK add --sanitize.
```

The fourteen child groups cover original generated calls into all three reviewed
pointer contracts; RO IN, RW OUT/INOUT, section and wide-integer bounds;
protection preflight and final-copy rollback; same-image query ownership;
callback errors and reentrancy; stale/closing admission; concurrent callers;
eight start/stop cycles; initialization failure; and expected join fail-stop.
Each child has an external monotonic 30-second process-group watchdog.

Cooperative requests are observed after a complete generated entry or supported
host error boundary. Deterministic observers pause inside the synchronous
adapter, including after a real SDK return. They do not replace SDK imports or
simulate their results. The import then finishes and unwinds before thread exit,
join and final reclamation. Host entry-loop scheduling is explicitly PARTIAL;
this does not provide in-instruction cancellation or guest exception unwind.
An expected exit 70 on failed join proves containment, not orderly reclamation.

[Contracts, evidence, ownership and limits](../../docs/research/image-backed-pointer-shutdown.md).
