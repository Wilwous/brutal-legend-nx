# Stage 18 — targeted compatibility service audit

**HOST VERIFIED** selected existing refusal/ownership contracts; broader title
compatibility **PARTIAL**. No new PPC instruction, SDK import, ABI signature,
loader rule, privileged Horizon operation or runtime feature was added.
Stage16 R01/R05 remain the P0 decision; adding unrequired services would not
reduce those blockers. The Stage15 artifact and its genuine hardware result stand.

## What is required and admitted

The frozen title region has19 instructions plus one unexecuted pre-call boundary,
one save-helper call with matched return, and zero ordinary SDK imports. This
is a preserved offline classification, not new execution or complete startup
coverage.274 import records and261 function shims are identity inventory only.
Later title-specific call contracts remain unestablished; no generic no-op table
was added. The Horizon runtime continues to admit exactly:

| Export / xboxkrnl ordinal | Explicit synthetic contract | Remaining limit |
| --- | --- | --- |
| RtlCompareMemoryUlong /0x11b | Bounded aligned IN words, scalar result, checked retained memory | No arbitrary guest pointer or full export-domain claim |
| RtlInitializeCriticalSection /0x12e | Checked INOUT28-byte structure using pinned helper/entry | Does not implement Xbox Enter/Leave, contention or scheduler |
| NtQueryVirtualMemory /0xee | Three arguments; selector0; checked OUT28 bytes, synthetic uniform whole-page metadata | No XEX tail ownership inference, no general query selector support |
| RtlFillMemoryUlong /0x126 | Aligned checked OUT4–64 bytes; transaction publishes only on success | No unbounded fill, no silent OUT publication on refusal |

Their bodies/signatures/structures remain pinned by the existing SDK locks in
`tests/horizon-xex`, `tests/horizon-expansion`, `tests/horizon-runtime-ii` and
`tests/horizon-acceptance`. Compare/init/query/fill source bodies are extracted
from the real pinned SDK by the production test driver, not recreated as stubs.
Unknown imports and typed-vector signatures remain denied. Four admitted
synthetic contracts cannot be reported as4/274 verified title requirements.

## PPC / ABI decision

No newly required integer, floating-point, vector, CR/XER/LR/CTR behavior was
established for the frozen scope. The unchanged production fixture covers selected
scalar memory, scalar/FPR calls, integer carry/extension, selected rotate/cntlzw,
full16-byte VMX transfers and selected vector arithmetic, with strict CFG and
r1/r2/r13 admission. Its21 functions/203 words are not a complete ISA or ABI.

Reservation atomics remain **BLOCKED**: no verified Xenon reservation granularity,
per-thread conflict/invalidation and ordering oracle for a required title path
was obtained. No host CAS substitute was introduced. Partial vectors, typed-vector
ABI, MMIO/instruction fetch fallbacks, unsupported exceptional transfers and
other unadmitted constructs remain refused. General Xbox scheduler and full
register/exception conventions remain unverified.

## G8 diagnostic obligations and new original witnesses

`tests/compatibility-services/contracts.cpp` exercises the existing portable
`execution_boundary.h`, `Space`, `ModuleLife`, `NativeThread` and diagnostic
bindings.30 assertions per Debug/Release/ASan+UBSan profile PASS, zero FAIL.
It intentionally links no PPC-generated code and is not a console execution test.

Verified: checked read-only write refusal leaves the value unchanged; action code
after throw does not run; two supported host RAII frames unwind; active module
close refuses; module/memory leases balance; diagnostic PC/module/thread/phase
and allocation identities survive the refusal; native TLS bindings restore;
a copied numeric context survives subsequent caller mutation; callback failure
publishes no partial INOUT value; stale/decommitted views refuse; recommit zeros
owned memory; foreign release refuses; worker callback failure is visible after
join; resources end at zero. Explicit unsupported_exception produces failure,
never a successful guest return.

Important limits from actual source inspection:

- DiagnosticContext is a host-supplied32-bit numeric model (32 GPR values, PC,
  LR, CTR). Its copy is made after host unwinding. This test neither captures a
  real fault-time PPCContext nor an Xbox CONTEXT record; it does not preserve
  CR/XER/FPR/vector state or prove arbitrary destructors leave guest state intact.
- `Scope::dispatch` clears pending/call bookkeeping and rethrows. It does not
  install a universal terminal thread/scope latch or a complete immutable PPC
  fault snapshot. Trusted harnesses must treat a failed action as failed; this
  is not proof that any subsequent guest continuation is safe.
- A C++ catch and two RAII frames are not Xbox guest stack unwind/dispatch.
  Hardware faults, guest handler lookup, stack restoration and valid guest
  continuation remain unsupported. No exception was converted to Xbox success.
- Checked software capabilities constrain supported APIs; raw host memory and
  all physical Horizon pages are not completely guest-isolated. SDK ImageContains
  is extent-only; actual checked copies separately validate owner/commit/access.

Thus **G8 BLOCKED**, despite safe deterministic refusal in these selected host
fixtures. Missing original context/transition evidence is stated explicitly.
No speculative recovery, context rollback or continuation implementation is added.

## Regressions and reproducibility

```sh
python3 tests/compatibility-services/run.py --workdir "$PRIVATE_SERVICES_OUT"
python3 tests/horizon-acceptance/run.py --toolchain "$PREPARED_TOOLCHAIN" --sdk-source "$SWITCH_SDK_SOURCE" --workdir "$PRIVATE_ACCEPTANCE_OUT" --stage 15
python3 tests/horizon-acceptance/lifecycle_core.py --workdir "$PRIVATE_LIFECYCLE_OUT"
python3 tests/horizon-acceptance/supervisor_core.py --workdir "$PRIVATE_SUPERVISOR_OUT"
python3 tests/horizon-runtime-ii/diagnostics_core.py --workdir "$PRIVATE_DIAGNOSTICS_OUT"
python3 tests/horizon-runtime-ii/query_core.py --workdir "$PRIVATE_QUERY_OUT"
python3 tests/horizon-foundation/core.py --workdir "$PRIVATE_FOUNDATION_OUT"
```

All directories must be new and outside the checkout. Relevant Stage15 production
codegen ran twice identically plus default-template comparison; no generated C++
was manually edited. Build fingerprint remains
`80407778989ce8796dce91d909fae5ebb9ce35434de2b5ea2f8ca08e0e23791d`,
codegen `c669e32fe3d79f8ec1fa3df64314a7910b908e8da227ac55af2bb1f4af43a016`,
SDK `753066c47cbc950881b175f20b6de55dfc07f4ef3645cb50577d1add26225f0a`.
Independent census:49/49 memory sites,97/97 hooks,4/4 backedges,17 calls,
20 returns,11 branch edges. These are original fixture counts only.

| Suite | Debug PASS/FAIL | Release PASS/FAIL | ASan+UBSan PASS/FAIL |
| --- | --- | --- | --- |
| New host refusal/cleanup witnesses | 30/0 | 30/0 | 30/0 |
| Existing generated acceptance smoke | 423/0 | 423/0 | 423/0 |
| Same smoke via native controller | 423/0 | 423/0 | 423/0 |
| Same smoke with tracing off | 423/0 | 423/0 | 423/0 |
| Existing bounded stress | 759/0 | 759/0 | 759/0 |
| Lifecycle | 35/0 | 35/0 | 35/0 |
| Supervisor | 20/0 | 20/0 | 20/0 |
| Diagnostics | 22/0 | 22/0 | 22/0 |
| Query metadata | 15/0 | 15/0 | 15/0 |
| Foundation | 20/0 | 20/0 | 20/0 |

Smoke result197650, SDK64/61,4 workers, allocations0/leases0; stress SDK232/229,
allocations0/leases0. Trace drops0. The three smoke variants are consistency
comparisons, not three independent capability inventories. Seven intentional
compiler refusals retain raw-address/partial-vector/trap/longjmp/typed-vector
and direct xstart boundary denials. No unexpected faults, timeouts or sanitizer
findings in final runs. Every child has an external30-second group watchdog.
New service tests:6 child processes including compilers,4.573s aggregate child
wall time, peak child RSS220,135,424 bytes including compiler. Acceptance:
25 child processes,45.262s, peak405,487,616 bytes including compiler/generator.

No cross-build was needed: production runtime, generated fixture, templates,
Makefiles and all pinned SDK/toolchain components remain unchanged. Historical
identical clean AArch64 builds retain BUILD VERIFIED status; no new hardware
behavior or new console result is claimed. This host-only stage deliberately
creates no Compatibility Probe NRO.
