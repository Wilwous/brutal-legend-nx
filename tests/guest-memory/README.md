# Phase 9 checked guest memory and entry contracts

Original synthetic fixtures only. No XEX, generated game source or game execution.
Use the thirteen-patch [entry-readiness lock](../../tools/sdk/entry-readiness-lock.json)
and [isolated SDK builder](../../tools/sdk/README.md). The Phase 7 and Phase 8 locks
remain unchanged for historical reproduction.

```sh
python3 tools/sdk/build.py --lock tools/sdk/entry-readiness-lock.json \
  --workspace "$PRIVATE/phase9-sdk" --cmake "$CMAKE" --ninja "$NINJA" --clean
for profile in Debug Release Sanitized; do
  flags=()
  if [ "$profile" = Sanitized ]; then flags+=(--sanitize); fi
  python3 tests/guest-memory/run.py --sdk "$PRIVATE/phase9-sdk/staged/$profile" \
    --workdir "$PRIVATE/phase9-checks-$profile" "${flags[@]}"
done
python3 tests/guest-memory/test_audit.py
```

The loop uses bash/zsh arrays. Each compilation and execution gets a separate
30-second monotonic process-group watchdog, explicit exit status, core suppression,
compact private logs and wait4 timing/RSS. Sanitized fixtures require the Sanitized
library; no sanitizer suppression is added. Output directories must be new and
outside the checkout. SDK/header/library provenance follows the builder lock;
the runner also verifies the staged runtime hash.

`contracts.cpp` tests bounded owner-token access to the actual SDK 4-KiB virtual
heap: cross-page/endian copies, all-or-nothing permission rejection, host-page
slack, uncommitted pages, freed/foreign/replaced allocations, object bounds,
overflow, masked MMIO boundaries including host-page padding, unsupported fetch,
concurrent read/write/protect/release and a fixed-work timing sample.
`imports.cpp` calls eight real exported PPC shims with original data and no loaded
executable. Tests cover absent-module outputs, selected valid privilege indices,
null debug/string arguments, virtual-memory error outputs, event creation/close,
double/concurrent close and invalid waits. These are selected observed SDK contracts,
not complete Xbox behavior or mediation of those exports' raw pointer accesses.

The checked adapter exposes integer guest-address metadata and host-buffer copies,
never guest backing pointers. Trusted fixture setup still uses raw SDK operations
to configure pages; those operations are deliberately outside its protected API.
Its lifetime tokens are not serialized. Runtime reset/restore/teardown and MMIO
registration must be quiescent. Fetch, physical/XEX/large-page heaps, callbacks,
raw generated accesses and full ABI are unsupported here.

The timing compares checked 8-byte reads (including validation/oracle overhead)
to a host-local 8-byte copy loop with a volatile checksum. This is a noisy diagnostic
reference, not an equivalent guest workload, robust slowdown ratio or game benchmark.
MMIO detection conservatively scans each intersecting host page using the pinned
SDK's exact mask predicate; this deliberately favors correctness over throughput.

Static private audit, never compilation/execution:

```sh
python3 tools/audit_entry.py --generated "$PRIVATE_GENERATED" \
  --entry xstart --output "$PRIVATE/phase9-entry-audit.json"
```

The tool outputs counts/import names only; keep its report private. Literal direct
calls overestimate feasible branches and omit indirect targets, callbacks and
exception tables. [Contracts, evidence and NO-GO gate](../../docs/research/guest-memory-entry-readiness.md).
