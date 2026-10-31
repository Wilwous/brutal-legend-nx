# T8 — pre-entry stack permission and context admission

2026-10-31; baseline `ebc3d3600d903c37a32886b5bd649448dcbbd1f2`.
**Generic fix VERIFIED in the selected synthetic scope. Original title NO-GO.**
No title instructions or game-derived C++ were compiled/linked/executed. No title
plan admission, unknown range, historical pointer, Xbox loader, SDK lock or NRO
was changed. There is no Switch build/deployment in this task.

## Root cause and call chain

GuestThread created a writable owned stack, but the old Execution predicate only
validated register/PCR/TLS metadata and pinned allocations. Later protection changes
could leave a structurally valid read-only stack. Scope dispatch admitted the call;
checked-memory rejected the eventual store, after admission. Public allocation
fields also needed comparison against the context's original private registrations.

The fixed chain is:

`context creation → Scope/Execution admission → invoke/direct/indirect dispatch
→ production generated prologue → generated instruction → checked-memory access`.

Constructor admission, every dispatch and the generated prologue now use the same
Execution validation. The prologue rechecks after its observer/stop poll and before
incrementing guest_entries. A direct C++ call to a registered generated function
also passes that prologue guard; a call without the matching Scope/context refuses.
Generated C++ and production templates were not manually edited.

## One authoritative range/permission check

[Space](../../runtime/horizon/space.h) adds validate_owned_window: exact owner/id/
address/size token, nonempty nonwrapping range wholly inside that one allocation,
current commitment and permissions. It delegates to existing checked Memory view
translation/validation. For stack admission, read and write are checked over the
entire required span under one Space lock. There is no separate rights table,
guessed permission or global protection bypass.

[GuestThread](../../runtime/horizon/guest.h) compares stack/PCR/TLS handles with
their private creation registrations. Missing, substituted, foreign or retired
tokens cannot become valid by pointing at another resident allocation. Thread and
ModuleLife instance identity are part of the explicit stack contract. Existing
PCR/TLS/register defaults and slot registration checks remain synthetic-only.

The inherited ABI metadata bounds still require 16-byte r1 alignment, at least
64 bytes below r1 and 128 above it in the owned allocation. High register bits,
address/size overflow, reversed/empty windows and signed offset extremes refuse.
No new title startup values or complete Xbox ABI claim is introduced.

## Explicit required window

- Default: the complete registered stack allocation, currently 16,384 bytes.
- Explicit relative contract: `[current r1 + begin, current r1 + end)` at each
  applicable function entry in that Scope. The tested leaf contract is `[-32,+32)`,
  exactly 64 bytes, not an implicit whole-allocation requirement.
- The caller must review the mandatory stack footprint of every selected entry
  using that relative contract. No PPC/ABI footprint is inferred from the two
  offsets, and the synthetic leaf contract is not a Brütal Legend entry contract.

The validated address/length is available for audit. One writable byte never
stands in for the requested span. Cross-page accesses within one owned RW region
work; a span crossing into a separate RO/inaccessible allocation refuses. A narrow
span ending before that boundary succeeds.

The present backend has uniform permissions per logical allocation, not independently
protected subpages inside one allocation. Partial writable/nonwritable boundaries
are tested with separate logical allocations; spanning them is unsupported and
refused. No physical guard-page or complete host/guest isolation proof is claimed.

## Lifetime, revocation and cleanup

Execution pins original stack/PCR/TLS registrations before marking the context
active. Failure unwinds pins/module leases without setting active. Leases prevent
release/decommit throughout the Scope, including narrow-window execution. They do
not freeze protect(): rights are rechecked at each admission and every actual
checked load/store. Revocation after an admission can still cause a later access
failure; the fix does not promise atomic permission stability throughout a body.

Module admission is rechecked before each entry while the retained module lease
protects lifetime. Context cleanup still refuses active/pending/unjoined work.
For a host-retired original stack, admission fails, then a context-only survivor
cleanup reclaims remaining private registrations. Every surviving token/lease is
checked before any survivor is freed; foreign/unissued/duplicate registrations
refuse. A retired registration is never accepted for entry. Strict release_all,
NativeImagePlan/Image ownership policy and the Xbox loader are unchanged.

## Synthetic regression evidence

The new [window fixture](../../tests/pre-entry-stack/window.cpp) covers complete
RW and cross-page spans, narrow spans next to RO memory, inaccessible/uncommitted
memory, foreign/missing/stale/overflowing tokens, invalid access, lease protection
and transactional survivor cleanup. It uses no SDK or title input.

The [entry fixture](../../tests/pre-entry-stack/entry.cpp) compiles the real
production-generated original Foundation PPC bodies. It tests writable valid
entry; RO, write-only, inaccessible and uncommitted stacks; missing/substituted/
retired stack registrations; wrong thread/module identity; alignment and offset
overflow; cross-region windows; revocation after valid Scope and in a pre-body
observer; absent/foreign Scope; partial initialization failures; and repeated
worker start/stop/join with cleanup.

Root dispatch, direct API, indirect API and raw generated-function calls each
succeed with a valid stack and refuse after protection revocation. Rejection
keeps guest_entries unchanged and the leaf result register unchanged. The private
records include 28 zero-entry-delta checks per profile; this is a case count,
not 28 distinct dispatch mechanisms. The four mechanisms above are the tested
bypass audit. It is not a global proof for arbitrary host callbacks or unreviewed
future templates/functions.

| Per Debug / Release / ASan+UBSan profile | Positive assertions | Expected refusals | Byte comparisons |
| --- | ---: | ---: | ---: |
| New owned-window fixture | 5 | 16 | 0 |
| New production-generated entry fixture | 39 | 28 | 0 |
| T7 inert data contracts | 29 | 38 | 32 |
| T7 synthetic context, updated to require early refusal | 26 | 23 | 32 |
| Existing N1 ownership/rollback | 26 | 49 | 16,384 |
| Existing tail model | 28 | 17 | 327,680 |

Each new entry profile has seven valid leaf entries and three deterministic
stop/join cycles with no entry after the pre-requested stop. Existing Foundation
replay passes 108 combined checks per profile. Original NativeImageBinding
passes smoke (19 positive/19 refusal), eight-cycle replay and two-worker concurrency
in all profiles, with zero remaining allocations/leases. Its bytewise image
comparisons remain separately counted. Legacy Horizon core passes 20 combined
checks; 77 static metadata unit methods also pass.

Foundation uses actual production recompile-tests, 30 independently encoded
instructions/three functions, with baseline-body comparison and deterministic
regeneration. NativeImageBinding uses actual codegen from an original synthetic
XEX, 35 instructions/three generated functions plus one import. The pinned SDK
CLI/headers are unchanged. Final frozen-source builds and generated-file hashes
are retained privately; no proprietary source is an input to either pipeline.

Final selected 81 compile/test children total 62.437 seconds of measured child
wall time. Peak RSS including builds: 447,414,272 bytes; peak noncompile child RSS:
96,763,904 bytes. Unexpected failures, faults, timeouts and sanitizer findings: 0.
Nine nonzero malformed/control-reader exits are expected. All potentially running
generation/compile/test children have external monotonic 30-second process-group
watchdogs. Earlier successful development runs remain separate from final counts.

## Reproduction and CI boundary

```sh
python3 tests/pre-entry-stack/run.py --workdir "$FRESH_WINDOW_OUTPUT"
python3 tests/horizon-foundation/run.py --workdir "$FRESH_FOUNDATION_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN"
python3 tests/pre-entry-stack/run.py --workdir "$FRESH_ENTRY_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN" --foundation "$FRESH_FOUNDATION_OUTPUT"
python3 tests/native-provenance/run_data.py --workdir "$FRESH_T7_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN"
python3 tests/native-image-binding/run.py --workdir "$FRESH_BINDING_OUTPUT" --toolchain "$VERIFIED_TOOLCHAIN" --sdk-source "$VERIFIED_ISOLATED_SDK"
python3 tests/native-image-plan/run.py --workdir "$FRESH_PLAN_OUTPUT"
python3 tests/native-image-contract/run.py --workdir "$FRESH_TAIL_OUTPUT"
python3 tests/horizon-foundation/core.py --workdir "$FRESH_CORE_OUTPUT"
```

Use fresh external output directories. Do not pass --switch-build. Public CI runs
SDK-independent window/data/ownership/core regressions. The pinned production
generator/real-context matrices ran locally in all three profiles; without those
verified inputs, the new runner explicitly reports generated entry NOT RUN,
never substitutes a mock and never claims that the SDK-independent CI is equivalent.

## Title boundary and next proof

The generic admission gap is fixed in this scope. Title-specific provenance,
the 392-byte gap, unknown XEX tails, 0/13 variable providers, actual stack/PCR/TLS
startup values, code/stop identity and exception semantics remain unresolved.
Historical G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED and native N-G4 BLOCKED /
N-G5 PARTIAL / N-G8 BLOCKED are unchanged. Original execution remains NO-GO.

**Next most useful title proof:** establish and bind the exact nonexecuting
entry/startup contract for the previously frozen title slice: independently
owned stack/window and context identity, mandatory memory footprint and PC/stop
obligations. Do not borrow the synthetic 16-KiB stack or leaf window as title data,
and do not execute the slice while any applicable obligation remains unresolved.
