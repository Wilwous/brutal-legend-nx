# Nintendo Switch runtime assessment

Status: a proposal informed by inspected source and build files. **No Switch build, deployment or hardware test was performed.**

## Deployment model

Prefer a homebrew **NRO** launched from Homebrew Menu in full application/title takeover mode. Reference ports document this path and reject or discourage Album applet mode due to its smaller resource budget. This is the first deployment candidate, not a tested requirement for Brütal Legend. Atmosphère version, loader permissions and address-space setup must be recorded during tests.

An NSP forwarder is unnecessary for the first proof of concept and adds packaging/permission questions. Do not introduce console-key-dependent packaging or proprietary Nintendo SDK components. A 39-bit address space described by references is virtual address capacity, not available physical memory.

## Platform foundation

[libnx](https://github.com/switchbrew/libnx/tree/20cbcfd90567f7bf84df7725f27e345e2164fa6b) exposes homebrew environment, thread/SVC, controller, audio-output and SD filesystem APIs. These declarations establish API availability in source, not that every process has the required permission or mapping semantics.

The [Project 8 Horizon overlay](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/patches/rexglue-sdk/0004-switch-horizon-runtime.patch) contains platform adaptations for guest memory, exceptions/fibers, threading, audio, controllers and presentation. Follow-up patches change exception/SIMD handling and memory/GPU behavior. Audit the complete ordered series rather than copying one patch.

## Graphics

| Candidate | Evidence | Open cost |
| --- | --- | --- |
| Vulkan through Mesa NVK/NAK for Horizon | Used by inspected ReXGlue reference source/build configurations; driver patches and exact base commits documented | Reproduce driver and runtime ABI; game-specific Xenos semantics and resource/shader work |
| deko3d | Independent low-level Switch homebrew API; [upstream](https://github.com/devkitPro/deko3d/tree/373bfd43d8b2d805fa2eb7d406dbb823032df958) documents native shaders and offline compilation | New renderer backend and shader path; not compatible with Vulkan calls directly |
| SDK Xenos/EDRAM backend | Available as a semantic baseline in the reference architecture | Performance and write-watch costs unmeasured for this game; not an assumed viable shipping path |

Vulkan is not supplied by Atmosphère as a desktop system driver. The references build/link a specific homebrew Mesa driver. Validate device features, descriptors, buffer address support, image formats, MSAA, depth resolves, synchronization and pipeline caches against the exact driver. Do not assume desktop Vulkan extension support or that shader translation covers Brütal Legend.

## Memory and exceptions

Prove the guest virtual layout and aliases, alignment/page size, allocation/commit, guest protection calls, memory visibility, safe release and CPU/GPU cache invalidation. Map only required physical backing; a multi-gigabyte guest window is not a multi-gigabyte resident allocation. Record code/heap, guest backing, texture/render targets, shader/pipeline caches, staging, stacks and decoder memory separately, including peaks.

The [Project 8 memory audit](https://github.com/R-drg/project8-sw/blob/3393f2615deb7e8ac11ce51d2cd0e9defded5669/docs/SWITCH_MEMORY_REPROTECTION.md) explains that aliasing does not prove write-watch/protection correctness and that a no-op protection shim may silently break cache invalidation. Its historical status statements must be reconciled with later patches and hardware behavior. Another port's successful mapping is not proof that this game's required mappings work.

Test host fault entry, controlled recovery, SIMD registers and floating-point state, independently of guest C++ exception handling. A crash handler that only logs is not a recoverable fault mechanism. Do not stub memory-protection calls to make boot continue without proving semantic safety.

## Threads and synchronization

Treat CPU availability, priorities, affinity and SVC permissions as loader-dependent observations to measure. Do not assume all four cores are available or plan a fourth-core/overclock dependency. Multiple Xenon guest threads must share the available ARM64 resources with rendering, decoding and operating-system work. Busy waiting can starve progress; replace waits only after demonstrating preserved return values and timing requirements. Validate atomics, memory barriers, callback/APC behavior, thread-local data and shutdown.

## Audio and controllers

The inspected Project 8 patch adds a libnx `audout` audio adapter. The SDK has XMA decoding and FFmpeg integration; none proves this game's audio path. Begin with a known original audio fixture, then test the actual game's codec, mixing and movie timing privately. Log underruns/latency without publishing music or decoded samples.

Use libnx pad support for Joy-Con and Pro Controller, sticks, triggers and rumble as needed by demonstrated imports. Define letter versus position face-button mapping explicitly and test reconnection and handheld/docked changes. Existing controller adapters are likely reusable after review; game behavior is untested.

## Files, saves, modes and performance

Read owner's extracted data from SD via a constrained guest filesystem mount. Keep writes in a separate saves/config/cache root. Inventory case sensitivity, path normalization, seek sizes, async I/O and streaming. Preserve existing saves and prove crash-safe save updates before enabling persistence tests. No game archive or save format is known yet.

Record handheld/docked mode, console revision, launch mode, firmware/Atmosphère, driver/toolchain, stock clocks and thermals for each run. Measure CPU/GPU frame times, shader-compilation stalls, SD streaming and memory peaks. Begin without overclocking; reference performance reports that use overclocking are not baseline evidence. Suspend/resume, display resize, mode transitions and clean exit require separate tests.
