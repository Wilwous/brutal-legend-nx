# Switch roadmap Stage 21 — synthetic XEX differential observation

**HOST VERIFIED** the finite corpus and actual strict SDK checks.
**PARTIAL** reference comparison: unchanged Xenia source slices run with an
explicit bounded observation-memory adapter; full Xenia loader/heap/CPU lifecycle
is **UNOBSERVED**. No Xbox hardware was used. No proprietary XEX was loaded.
This is not the preserved Xbox Phase22 milestone.

[Source pin and scope](xex-loader-reference-audit.md),
[reproduction commands and adapter limitations](../../tests/loader-observation/README.md),
[original fixture SHA256 manifest](../../tests/loader-observation/fixture-manifest.json),
[sanitized metadata observations](../../tests/loader-observation/expected-observations.json).

## Experimental design and trace authority

A = actual22-patch ReXGlue SDK UserModule.LoadFromFile in tool mode, exact
preexisting locked runtime per profile. It reports load status, per-page query
state/protect/allocation base/size, checked-copy results, active lease denial and
final heap balance. No XThread or guest entry is invoked; the real SDK still contains processor
and kernel import machinery.
B = exact pinned Xenia ReadImageUncompressed/BasicCompressed/ReadPEHeaders and
descriptor protection loop, connected to an original small observation adapter.
The adapter supplies backing and records actual method allocation/protection
requests. Its rounding/commitment/cleanup is a model, not observed Xenia Memory.
Full reference accept/reject, owner identity and unload remain UNKNOWN.
C = no second independent consumer executed; RXDK is a producer, ReXGlue shares
Xenia ancestry. D = no Xbox hardware observation. Agreement is not console proof.

Each14-case corpus is emitted twice identically in all profiles. A synthetic
entry writes0x6e to a dedicated data marker then traps; neither probe invokes
a guest executor. For accepted images with readable marker, it remains zero. This is a
load-only guard, not a validation of the entry's CPU semantics. Invalid entry
metadata is observed without attempting to dispatch it.

Traces show mapping requests and final project guest-page state, known synthetic
content, descriptor-protection requests and outside-adapter failures. They do not
show real reference PTEs, live console owners, every intermediate project mapping
event, or reference module rollback. Project failure may already have rolled back
before its first public probe snapshot; source evidence locates the stage.

## Differential results

Page unit64KiB; P=NONE payload bytes, D=descriptor extent. All image/PE results
in B are local method results, not overall LoadContinue success.

| Case | A: actual strict SDK | B: Xenia slice / allocation request | Supported conclusion |
| --- | --- | --- | --- |
| plain | load0,3 committed pages,R/R/RW | image0,PE0;3 pages | Scoped agreement; not original-title owner evidence |
| raw-zero | load0; data first byte0xa5 | PE0;0xa5 retained | PE raw_size0 does not zero an already RVA-laid-out NONE payload |
| raw-short | load0; virtual page fully backed | PE0; payload retained | VirtualSize>raw_size can be valid **inside proven allocation** |
| pe-overhang | C0000001; pages reclaimed | PE0;3 pages only | Recording VirtualSize does not create a fourth page/owner |
| owned-tail | load0;4 committed pages | PE0;4 pages requested | Explicit payload/descriptors cover tail; zero extension is owned, not guessed |
| basic-zero | load0; data zero | image/PE0;3 pages zeroed | Explicit BASIC zero span within D valid |
| basic-gap | load0; data zero | image/PE0;3 pages zeroed, only2 output pages described | Entire allocated D was memset; this is observed initialization inside owner, not outside it |
| none-short | C0000001 before NONE load | image/PE0;2 pages; protection request for third fails in adapter and original loop ignores result | Path-dependent allocation; no safe full-reference acceptance claim |
| security-larger | load0;3 pages | image/PE0;3 pages | Stored security size alone does not enlarge these paths; this mismatch is not an authenticity/integrity pass |
| no-read | C0000001; rollback | PE0; protections still requested by descriptor kind | Project PE permission admission is stricter; do not copy reference permissiveness |
| overlap | C0000001; rollback | PE0; no new allocation | Project enforces section nonoverlap; PE parser alone does not |
| invalid-entry | load0; marker unchanged | image/PE0 | Successful mapping is not entry admission; load-only SDK is not a complete call binder |
| bad-pe | C0000001; rollback | image0,PE1; allocation exists until **adapter** cleanup | Real SDK rollback observed; full reference rollback not measured |
| rounded-owned | C0000001 (P≠D) | image/PE0;P=3pages+1, adapter rounds4; extra known byte0x7b | Strict NONE extent equality is intentional; rounding is not title residency evidence |

The invalid-entry case is deliberately **not** called: the existing strict
runtime/binder remains responsible for a separately proven entry target. No
project code was changed to admit it. No arbitrary image growth, dropped section,
permission bypass, fabricated contents or guest continuation is introduced.

## Tests, sanitizer contradiction and regression correction

| Configuration | New observation assertions | Outcome |
| --- | --- | --- |
| Debug, original reference spans | 199 PASS /0 FAIL |14 original synthetic cases |
| Release, original reference spans | 199 PASS /0 FAIL |same fixture hashes and observations |
| ASan+UBSan, unchanged reference |13 process failures with UBSan;1 invalid-PE case returns before faulty macro |**BLOCKED**, not a passing profile; one shared source root cause |
| ASan+UBSan, explicit offset-only observation variant |199 PASS /0 FAIL |same14 observations as original Debug/Release; no findings in this variant |

The unmodified source's offsetof1 macro (pinned xex_module.cc833) accesses a member
through a null IMAGE_NT_HEADERS pointer. Full diagnostics are preserved. A one-line
standard offsetof replacement is supplied separately and requires an explicit
option; no sanitizer is disabled. A mechanical patch-apply check reproduces the
compiled variant exactly. Upstream reference source and project SDK remain intact.

The directly relevant image-runtime regression initially had19 passing groups
and one stale rollback oracle failure: descriptor growth made P≠D, now correctly
rejected at Load by existing strict preflight. Its test expected a later
LoadContinue rejection. Only that stage expectation was corrected; required
reclamation, empty metadata, reload and reuse checks remain. No SDK policy changed.
Initial adapter namespace/C-linkage build failures, CLI-directory invocation
setup failures and patch-format failure are retained privately, not hidden.

Final old residency tests:337/0 per Debug/Release/ASan+UBSan,23 isolated cases.
Host refusal/context/cleanup:30/0 per profile. Existing image ownership,
allocation-failure, import/TLS/rollback, collision, active references, header-failure
and teardown regressions are additionally reported in the final gate review.
Independent SDK-free corpus layout/identity tests:13 PASS. No unrelated large
Switch workload is rerun. All potentially faulting children have a30s external
monotonic whole-process-group watchdog; native unexpected signals/timeouts are
reported separately: all13 original-reference UBSan terminations were
SIGABRT (watchdog exit_code=-6), not ordinary exit1.

## Unsupported combinations and interpretation limits

NORMAL/LZX and encrypted input are source-audited only: no independently verified
new encoder/oracle was obtained, so no invented LZX fixture or keys are used.
Actual reference commitment gaps, owner collisions, active unload and rollback
are UNOBSERVED in the adapter. They are tested in the **real project SDK** by
existing regressions, including decommit/noaccess and neighboring-owner integrity;
those results must not be projected onto Xenia. Base misalignment, descriptor
malformation/overflow and BASIC expansion bounds are covered by those tests.
Runtime initialization failure is not modeled as success; actual SDK header
allocation zero/throw failure paths test partial initialization rollback.

G4/G8 remain BLOCKED, G5 UNKNOWN. The meaningful result is a stronger reason to
preserve strict ownership and keep mapping, PE acceptance, CPU entry and Xbox
exception semantics as separate proof obligations.

## Measured bounded process cost

| New observation configuration | Children incl compilers/emitter | Aggregate child wall seconds | Peak child RSS bytes (includes compiler) |
| --- | --- | --- | --- |
| Original Debug |34 |6.880 |286359552 |
| Original Release |34 |6.685 |294780928 |
| Explicit defined-offset Sanitized |34 |9.115 |309198848 |

These are host test costs, not game or console memory requirements. No timeout
or native signal occurred in these final configurations; the unmodified13 UBSan
failures above remain a distinct recorded result. Existing image-runtime tests
pass504 assertions/20 groups per profile, including both header allocation-failure
modes. Their aggregate child times are12.715/13.005/16.123s and peak RSS
340246528/338575360/364199936 bytes for Debug/Release/Sanitized, compilers included.
