# Synthetic Platform Demo — hardware acceptance

Candidate 0.85.0, T66–T85. **BUILD VERIFIED / HARDWARE EXECUTION UNKNOWN.**
Original synthetic PPC and original demo assets only. This is not Brütal Legend.

No automatic deployment is part of this cycle. After separately choosing a safe
installation method, copy only the new `switch/brutal-nx-platform-demo/` directory.
Preserve all previous probes, especially Stage 15 and Native Image Probe.
Verify the NRO against the package's SHA256SUMS before copying.

1. Boot Atmosphère and launch **Brutal NX - Synthetic Platform Demo** from hbmenu,
   preferably with Title Override. Record launch mode and console/system version.
2. Photograph the live scene and complete overlay. Expect a moving colored
   rectangle and checker texture, increasing FRAME/PPC CALLS, input connection
   state and resource count 36 bytes. Actual frame totals depend on elapsed use.
3. Move the left stick in both axes. Verify direction, deadzone and scene wrap.
   Press/release A: verify a color change per press and a short original tone.
   A held continuously must not repeatedly act as newly pressed.
4. Record whether audio is READY and audible. SENT and DONE are cumulative
   submission/completion counts, not evidence by themselves of audible output.
   UNAVAILABLE/native errors, silence or distorted output must be reported.
5. Where possible test Joy-Con and Pro Controller, including observed disconnect/
   reconnect. The adapter represents logical player 1, not physical serial IDs.
6. Press PLUS. Expect the final result screen after ordered teardown. Photograph
   it completely: actual frames/result, service entries/completions, audio state/
   counters/native error, allocations, leases, handles and resource bytes. Normal
   five-frame host numbers are not fixed device acceptance targets.
7. Require zero outstanding allocations, leases, handles and resource bytes and
   no reported execution/cleanup failure. COMPLETED means the observed loop and
   cleanup returned; it does not automatically certify visuals, input or sound.
8. Release PLUS, then press PLUS again. Confirm normal return to hbmenu. Record
   hangs, fatal screens, missing final screen or abnormal exit as failures.

Do not claim hardware PASS before actual observation. Submit live/final photos,
controller/audio observations and PLUS return result. If a native service aborts,
retain the observed error; do not repeat indefinitely or alter firmware/settings
to force acceptance. No original title execution or gate change is authorized by
synthetic success.
