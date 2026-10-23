# Reproducible original XEX binding

The public encoder constructs two independently authored, unencrypted XEX2/PE
images at fixed bases. Their PDATA supplies all three function extents; no manual
function hints or raw-test rebasing is used. Production ReXGlue `codegen` emits
C++ and its actual function/import tables. Those sources are regenerated for
identity checking, compiled unchanged and linked with the matching patched SDK.

One command runs the new pipeline and Phase 5–15 regressions in three profiles:

```sh
python3 tests/xex-binding/workflow.py --sdk-workspace "$SDK_WORK" --workdir "$PHASE16_TESTS"
```

SDK_WORK must contain the isolated source/build/staged outputs of
`tools/sdk/xex-binding-lock.json`. To construct it in the same workflow, add
`--build --cmake "$CMAKE" --ninja "$NINJA"` and optionally
`--source-cache "$PINNED_SOURCE_CACHE"`. The builder verifies pinned tools,
dependencies and patches; no global installation is used. All output directories
must be private and outside this checkout; PHASE16_TESTS must be new.

The workflow revalidates all SDK patches, staged headers/runtime hashes and
submodule revisions. Historical leaf controls are reconstructed from pristine
pinned Git objects, avoiding hidden local source snapshots. Every dangerous
child has a 30-second external process-group watchdog; suites/builds also have
finite outer budgets. No game input is consumed.

For one matching profile:

```sh
python3 tests/xex-binding/run.py --sdk "$SDK_DEBUG" --rexglue "$REXGLUE_DEBUG" --workdir "$PRIVATE_OUTPUT"
python3 tests/xex-binding/test_fixture.py
```

SDK patch 0019 adds separate checked image copying through retained ImageLease:
owner/section/extent/commit/protection/MMIO checks precede copying. Ordinary
checked memory still rejects XEX; fetch remains unsupported. Generated image
RO/RW references use this API. Automatic binding consumes the generated mapping
and actual module import metadata while retaining Phase 13–15 admission.

Two fixed-base products coexist through compiler-only namespacing of global
image-info/map/entry/registration symbols. All flags and digests are recorded.
There is no runtime relocation or handwritten replacement for generated PPC.
Wrong image/code/entry/layout/TLS/import contracts fail before entry admission.

Tests cover both images, distinct native XThreads, SDK results, checked image and
capability data, repeated loading, concurrency and bounded failure cleanup. An
intentional join failure exits the dedicated child with code 70 before destroying
resources still used by active threads; it is expected fail-stop, not orderly
thread reclamation. Unknown dynamic targets and unsupported signatures retain
explicit refusal. Typed vectors, guest unwind, private game mediation and Switch
runtime remain unverified or blocked.

[Full scope, commands, results and limitations](../../docs/research/synthetic-xex-binding.md).
