# Horizon threads, PCR and TLS — Stage 5

HOST VERIFIED: 67 new assertions per Debug/Release/ASan+UBSan profile; cumulative
117/0. Earlier Stage 2 and Stage 3 remain 108/0 and 118/0 per profile. Two clean
AArch64 builds match. No unexpected signals, timeouts or sanitizer diagnostics.
HARDWARE UNKNOWN for this expansion.

NativeThread exposes atomic empty/starting/running/exited/joined/failed state.
Only its control owner starts/joins; this is not a concurrent join interface.
A join timeout preserves ownership and resources. Actual termination is awaited
before close; an unjoined object's destructor fail-stops. No detach, forced
cancellation or speculative Xbox scheduler is supplied. Creation failure injection
is explicitly a fixture, not evidence of Horizon kernel exhaustion.

GuestThread clears the entire trivially copyable PPCContext representation,
including unions and padding, then restores the pinned SDK MSR default. A
placement construction over poisoned storage compares all context bytes.
Startup rejects wrong r1/r2/r13/MSR, PCR self or PCR TLS pointer. PCR is the
SDK-observed 0x2d8-byte subset, with TLS at +0 and self at +0x30. A synthetic
monotonic guest identity is not an Xbox handle. KTHREAD/APC/PRCB remain absent.

Four simultaneously active native workers retain distinct guest contexts,
stack/PCR/TLS allocations and initial TLS seeds. They execute the actual generated
wrapper and actual SDK compare concurrently, with distinct expected results
17/18/19/20. Guest cleanup and TLS slot recycling are refused before join/pin
release. Repeated construction, execution, join and teardown balance resources.

Horizon native TLS belongs to libnx; C++ thread_local binds a Scope/ImportFrame;
guest TLS is checked big-endian guest storage; r13 names the synthetic PCR. None
of these substitutes for another. Slot zero is the only fixture-selected slot.

Existing production-hook regressions preserve stop, join timeout and active-image
unload refusal while import/module/memory leases are held. Stop is cooperative;
an atomic checked copy or real SDK leaf completes before the next safe point.
Stage 6 extends exact site and backedge admission; no mid-copy preemption is
claimed. Clean ASan/UBSan is not proof of race freedom.

Reproduce the Stage 4 command with `--stage 5` and a fresh private output directory.
Failure logs from the first cross-build's libnx Event name collision remain
private. Qualified names and explicit loop braces fix the build without relaxing
warnings. SDK/global headers and original fixtures are unchanged.

Xbox Phase 22 BLOCKED; G4/G8 BLOCKED, G5 UNKNOWN. Proprietary game execution NO-GO.
