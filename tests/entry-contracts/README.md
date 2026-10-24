# Original bounded startup contracts

The original encoder builds an unencrypted XEX with a save helper, startup prefix,
unadmitted initializer and two scalar probes. It reads no game file or private
operand inventory. Registers, frame offsets, data values, image addresses and
instruction ordering are independently authored. The startup/save path has 19
admitted instructions and one pre-call exit boundary; four more sites belong to
the scalar probes. The canonical policy has 24 sites and 21 normal edges.

```sh
python3 tests/entry-contracts/test_fixture.py
python3 tests/entry-contracts/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# Use --sanitize only with matching Sanitized SDK; OUTPUT must be new and private.
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$ALL_TESTS"
```

Production `codegen`, forced repeat generation and a second fixture construction
must agree. All instruction comments, actual mappings, link/return/branch anchors,
24 emitted hooks, checked PCH macros and the immutable compiled policy are
independently reconciled. No generated C++ is edited. The actual SDK loads the
XEX, creates XThread/PCR/TLS/stack state and supplies image leases. Trusted host
orchestration supplies explicit original initial values and roots; normal Xbox
title initialization and private startup provenance remain UNKNOWN.

The eight startup contracts are four pre-frame U64 saves, one pre-frame U32 link
save, one U32 frame/backchain update and two signed-displacement U32 image stores.
Each has a full successful byte/endian oracle and an independently forced
read-only refusal with unchanged destination bytes. The frame-update failure
must preserve r1. Separate generated scalar probes test writable and denied
cross-page spans, unaligned accesses, uncommitted/freed/foreign backing, overflow,
read-only image data and image-end escape. Successful unaligned host copies do
not prove Xbox hardware alignment/exception semantics.

Forty-one bounded children include metadata, exact code/image/entry binding,
manifest hash/stale namespace, actual allocator failure injection, malformed
startup context, eight store refusals, repeated loading, exception/continuation
refusal and an externally requested stop inside generated execution. The
exception cases attempt denied host control transfers; they implement neither
Xbox exception dispatch nor unwinding. Metadata-only cases never execute PPC.
A larger PE SizeOfImage is accepted only with the original loaded ownership;
overlapping/nonexistent/out-of-owner sections must never gain execution admission.

Each child has a 30-second external monotonic/process-group watchdog; runtime
join and observer gates have 15/10-second bounds. The enclosing workflow bounds
this suite to 300 seconds. No global protection bypass is introduced. The
uncommitted-stack test uses the actual 64-KiB stack heap granularity and restores
only its owned page before ordinary teardown. All orderly paths require native
join, legitimate final-owner destruction and image/virtual page-count balance.
Expected historical fatal join exits remain separately accounted for.

See [Phase 21 evidence and remaining gaps](../../docs/research/bounded-entry-runtime-contracts.md).
