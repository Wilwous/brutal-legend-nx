# Minimal startup trace envelope — T65, schema 1

This is a conservative small **analysis-only** profile and capture proposal.
It is not an implemented Xbox collector, runtime admission mechanism or universal
trace completeness proof. See the [feasibility decision](xbox-startup-tracing-feasibility.md).
The original [fixture](../../tests/trace-evidence/fixture.py) is executable schema
example data; all its values, IDs and addresses are independently authored.

## Collection contract

Start before the earliest applicable executable-image materialization and related
initializers, including preexisting provider activity that can access the image.
Identify the entry/startup transfer and both target stores separately. End at a
justified observation or stop after the second store, before unproved continuation.
A collector that attaches after startup fails the early-observer question.

Record tool/build revision and collector/configuration hashes, module/source hash,
launch configuration, environment/boot identity, evidence class, target interval,
store anchors and observation boundary. Keep real addresses, trace data, values,
symbols and detailed environment private. Persist raw capture plus transport/loss
receipts and a separately retained digest. Capture the initial bytes as values or
commitments; later value hashes alone cannot reconstruct or verify those bytes.

Each event needs sequence, module, thread/generation where applicable, guest PC
or identified host/device origin, kind, relevant address/width/direction, byte
sequence or commitment, and explicit happens-before predecessors. Memory events
need logical allocation, owner, generation, commitment and current rights via an
identified prior transition. Record allocation/reservation, commit, protect,
materialize, imports, TLS/PCR initialization, thread start/exit, APC enqueue/delivery,
callback enter/exit, startup, accesses, exception, stop, rollback and release.
Distinguish guest addresses from host pointers and resolve physical/virtual aliases.
Record callback invocation identity/arguments and thread/PCR/TLS associations in the
full private capture; schema 1 only models a single known callback invocation.

Causal edges require evidence: program order on a known thread, an actual release/
acquire or other established synchronization, enqueue/delivery and collector barrier
receipts. Timestamps and collector append order alone do not order guest threads.
Record exception type/address/context, dispatch/handler/unwind outcome and stop
reason without inventing guest exception semantics. A notification is not a proven
continuation contract. Observer instrumentation must report its scheduling effects.

Coverage must explicitly account for scalar, vector/partial-vector and atomic paths,
indirect/aliased accesses, host loader copies/zeroing, HLE/native translations,
APC/callbacks, TLS/PCR/import work, other threads, exception paths, DMA/MMIO/device
agents, and reads as well as writes. An excluded agent needs independent proof of
irrelevance, not an empty log. Retain per-producer counts, sequence continuity,
overflow/drop flags and barriers covering start/end and flush. Audit filters and
optimizations that remove callbacks or accesses; range overlap includes accesses
starting before the target. Validate against independently known synthetic effects
before any proposed title capture.

**OBSERVED NO EVENT** requires a qualified collector covering that path/window.
Otherwise the result is **EVENT COULD HAVE BEEN MISSED**. Even a qualified single
run supports that observed execution only; repetition does not prove universal safety.

## Small implemented parser profile

[tools/trace_evidence.py](../../tools/trace_evidence.py) accepts JSON with `schema=1`,
`capture` metadata and `events`. It admits at most 256 events, 32-bit nonzero logical
allocation spans, whole-allocation commitment/protection, BE byte commitments and
explicit causal predecessors. It requires nine named complete-coverage declarations:
`guest_scalar`, `guest_vector_atomic`, `indirect_alias`, `loader_host`, `hle_host`,
`apc_callbacks`, `tls_pcr`, `threads_exceptions`, `other_agents`.
Declarations are checked for presence; their truth requires external review.

The profile checks contiguous sequence/module identity, matching producer/record
counts and zero declared drops; early window start, causal initialization/startup/
stop; known threads/callbacks and enqueue/delivery/entry/exit; nonoverlapping logical
owners, generations, commitment, rights, access bounds and release ordering. Two
ordered four-byte writes must cover the declared eight-byte target, followed by a
causally justified read or stop anchor before window end. Conflicting overlapping
accesses without causal order are refused. This conservatively refuses racing
traces rather than inferring a serialization from timestamps. It lists recorded
early/unordered target readers without turning a final value into absence of observers.

This deliberately limited parser does **not** model per-page split transitions,
physical alias resolution, reused callback invocations, owner transfer, decommit,
all thread lifetimes, complete exception dispatch, arbitrary large captures or
rollback. Full collection needs those fields/capabilities and independent review;
unknown semantics are not silently supplied. It checks hash syntax/integrity,
not correctness of memory values against execution. An external digest catches
alteration against that digest; it does not authenticate the collector or prevent
a producer from omitting and renumbering events and recomputing all counts/hashes.
The causal graph and ownership labels are also asserted input, not measured truth.

A consistent result always includes `EXTERNAL_REVIEW_REQUIRED` and `gate_impact=NONE`.
Evidence class is preserved as a declaration, never certified by the parser. A
hardware label cannot promote a gate. Malformed input exits nonzero; no runtime
loader, title plan, permission or SDK lock is changed.

```sh
python3 tests/trace-evidence/test_trace.py
# For an independently retained canonical JSON digest:
python3 tools/trace_evidence.py capture.json --sha256 DIGEST
```

Canonical digest: SHA-256 of UTF-8 `json.dumps(trace, sort_keys=True,
separators=(',', ':'))`. Archive raw-file digest separately. The unit fixture
contains 21 records, an indirect callback read at record 12 before startup, two
ordered BE32 writes and a later second-thread read. It is **SYNTHETIC ONLY data**,
not an observed CPU run, independent oracle for collector coverage, or framework PoC.
