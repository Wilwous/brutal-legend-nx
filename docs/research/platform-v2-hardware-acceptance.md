# Synthetic 3D Demo V2 — hardware acceptance

**2026-11-05 evidence update:** the T106–T125 request explicitly confirms T105
execution on the user's real Switch. Status is USER-CONFIRMED HARDWARE VERIFIED
for that synthetic scope. No new screenshot counters, FPS or resource measurements
were independently supplied/inspected in this cycle. The protocol below is retained
as the original acceptance checklist, not a newly observed numeric result.


T105 / version0.105.0. BUILD VERIFIED; new hardware execution UNKNOWN.
Preserved T85 remains USER-CONFIRMED HARDWARE VERIFIED. No fixed device PASS count
or FPS is predicted for V2. No proprietary title code/assets are included.

Deploy only the new `switch/brutal-nx-platform-demo-v2/` package directory after a
separate deployment instruction. Preserve T85 and older probes. Verify SHA256SUMS.
All four original resource files are embedded in the NRO RomFS; external notices
and README accompany the application. No NSP or firmware changes are required.

1. Launch **Brutal NX - Synthetic 3D Demo V2** using hbmenu, preferably Title
   Override. Record actual launch mode, controller and audio output device.
2. Observe the initial resource counters, then four textured 3D cubes with depth,
   rotation and an orbiting object. Wait until assets ready4/4 and delivered39240.
   Initialization/driver timing is unknown; record a persistent loading/failure
   screen rather than treating it as success.
3. Left stick moves the first object; right stick rotates camera. Test both axes
   and deadzones. A changes object scale and plays an action tone; holding A must
   not repeatedly become a new press. Verify normal animation continues.
4. Verify quieter ambient and action PCM can overlap. B toggles ambient; X/Y
   select louder/quieter ambient. Record distortion, silence, UNAVAILABLE, native
   error, underflow and queue counters. Counter increments alone do not prove sound.
5. Where available, test Joy-Con/Pro and observed disconnect/reconnect. The adapter
   uses a logical player1 aggregate, not physical serial identification.
6. Photograph the complete live HUD. Frame/update/render values are actual local
   measurements; missed steps are scheduler counters, not GPU drops or guaranteed
   FPS. No performance threshold is predeclared.
7. Press PLUS, photograph the complete exit summary after GL teardown. Require
   no reported failure, GPU cleanup errors0, voices/sources/queued0, allocations0,
   leases0, handles0 and resource_bytes0. Queued audio may be cancelled at exit;
   submitted/completed need not be equal.
8. Release PLUS, then press PLUS again. Confirm normal return to hbmenu. Report
   fatal screens, missing summary, crashes or hangs as failures. Do not repeat
   indefinitely or change firmware to force acceptance.

Only actual device observation can change V2 hardware status. Successful synthetic
acceptance does not change original-title gates or authorize T106 automatically.
