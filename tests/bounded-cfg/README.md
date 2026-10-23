# Original bounded CFG fixtures

The original encoder builds ten functions in an unencrypted XEX/PE container.
PDATA drives actual production discovery; there are no function hints or edited
emitted bodies. Nine bounded function ranges admit 74 before-instruction sites;
the unrelated helper and suffixes outside selected ranges remain unadmitted.
The prefix is structurally analogous to stack preparation/data initialization,
with different instructions, registers and data. No game content is copied.

```sh
python3 tests/bounded-cfg/test_manifest.py
python3 tests/bounded-cfg/run.py --sdk "$SDK" --rexglue "$CLI" --workdir "$OUTPUT"
# --sanitize only for a matching Sanitized SDK; OUTPUT must be new and private.
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$ALL_TESTS"
```

The workflow verifies the unchanged nineteen-patch SDK and runs Phase 5–20
against Debug/O0, Release/O2 and ASan+UBSan clients (SDK Sanitized O1). Every
compiler/generator/runtime child has an external monotonic 30-second process-group
watchdog; the complete suite has a 300-second outer ceiling. Core dumps are
suppressed; expected refusals have explicit result fields and are not guest
success. Native join and observers have shorter independent bounds.

The manifest comes from independently decoded bytes/ranges, not execution results.
It binds image/code/configuration/revision/patch hashes. A second construction,
forced production regeneration, instruction/comments/link/callee/label/memory
anchors, emitted hooks and actual PCH macros are reconciled before compilation.
The trusted host verifies canonical manifest and image file hashes before SDK
loading. Runtime tables are immutable compiled products of that authority; this
is not an untrusted JSON policy interpreter or native C++ sandbox.

Seventeen children test nonzero/zero joins, calls/returns, known and unknown
indirect calls, callback/return refusal, prefix exit, denied boundary escape,
an attempted exceptional callback (refused before dispatch), invalid tables/hash, straight paths, checked failure, loops and post-import stop.
All 74 admitted instructions have stop hooks. The two direct loop backedges must
be observed; actual edges must belong to the declared graph. Return admission
also requires the exact shadow caller continuation, rather than a union of
possible callers. Unknown computed tail transfers remain unsupported/refused.

The scoped checked header changes only this original fixture's call seam. There
is no import-to-native fallback: a dedicated import-only direct API rejects even a native symbol known to the lower checked scope; each site selects an exact import or explicitly
admitted original native target. Existing generated import policy is unchanged.
Image/code validation, real XThread/TLS/PCR, checked stack/image/opaque memory,
actual SDK critical-section initialization, active unload refusal, native join,
final-owner reclamation and page-count restoration all remain required.

[Phase 20 scope and results](../../docs/research/bounded-entry-region-cfg.md).

An exceptional callback request is tested through the same exact control gate before a store. This is an explicit host refusal, not Xbox exception dispatch, unwinding or integration with a title handler. The canonical policy denies exceptional/unlisted continuations; the private entry owner's exception metadata remains a Phase 21 dependency.
