# Isolated unified SDK build

This original builder targets **macOS ARM64** and ReXGlue v0.10.0 at the revision in [lock.json](lock.json). It consumes no game data. [Phase 7 evidence and limitations](../../docs/research/unified-sdk.md).

Prerequisites: Python 3 with venv/pip, Git, Xcode/Apple Clang and its macOS SDK, network access to the pinned upstream dependencies. The measured host versions are in the report. Choose an empty private directory outside the checkout with adequate disk space; all source/dependency/build/log/staging outputs stay there. No root access, global SDK install or CMake user-package registration is used.

Run from the project root; assign absolute paths appropriate to your machine:

```sh
export PROJECT="$PWD"
export PRIVATE_WORK="$(mktemp -d "${TMPDIR:-/tmp}/brutal-sdk.XXXXXX")"
python3 -m venv "$PRIVATE_WORK/tools"
"$PRIVATE_WORK/tools/bin/python" -m pip install --no-cache-dir --only-binary=:all: --require-hashes -r "$PROJECT/tools/sdk/requirements-macos.txt"
python3 "$PROJECT/tools/sdk/build.py" --workspace "$PRIVATE_WORK/unified" --cmake "$PRIVATE_WORK/tools/bin/cmake" --ninja "$PRIVATE_WORK/tools/bin/ninja"
```

`PRIVATE_WORK` must be outside the project even if the shell's temporary directory is customized. The wheel requirements include SHA-256 values and fixed versions; [official CMake distribution](https://pypi.org/project/cmake/3.31.6/) and [Ninja distribution](https://pypi.org/project/ninja/1.11.1.4/). Compiler/Xcode are host prerequisites, not downloaded or silently substituted. The lock file records every dependency source commit; their own upstream licenses apply.

Repeat a clean build, retaining only source/download caches:

```sh
python3 "$PROJECT/tools/sdk/build.py" --workspace "$PRIVATE_WORK/unified" --cmake "$PRIVATE_WORK/tools/bin/cmake" --ninja "$PRIVATE_WORK/tools/bin/ninja" --clean
```

Default profiles are Debug, Release and Sanitized; `--profiles Debug` selects one. `--jobs` accepts 1 or 2. An optional `--source-cache "$PINNED_SDK_CHECKOUT"` uses committed Git objects from an existing explicit clone and its submodules. It does not copy working-tree edits. The measured run used that option to avoid repeated downloads. Patch verification compares the actual source with pristine revision plus ordered patches on every run; failures stop immediately. All seven patch hashes and the CPM bootstrap hash must match.

The private result is `unified/staged/{Debug,Release,Sanitized}` with matching headers/libraries and a `fixture-sdk.json` fingerprint. Debug retains `librexruntimed.dylib` and supplies a conventional linker-name copy. `build-results.json` / `clean-build-results.json` record versions, source/patch revisions, elapsed time and peak RSS; compact logs and individual process records are adjacent. No claim of byte-identical SDK binaries across different host paths/toolchains is made.

Each acquisition/configuration/build command has an external process-group watchdog: 180 seconds for clone/configure, 300 for submodules and 1,200 per profile build. Test children have a separate 30-second limit. A timeout or nonzero exit fails the operation; build logs retain only bounded beginning/end sections. RSS is wait4 process/descendant accounting, not total concurrent-tree memory.

The build configures upstream Vulkan dependencies because the pinned CMake project requires a backend. It builds only `rexruntime`, `rexcodegen`, `rexglue`; it does not build the GPU rendering plugin or execute graphics/audio. Sanitized instruments supported C-family SDK sources with ASan/UBSan at O1, without recovery; system frameworks and assembly remain uninstrumented. Do not infer Switch compatibility from a Mac build.

Run the [unified fixtures](../../tests/unified/README.md) against these outputs. Do not run old SDK/hybrid adapter commands and call them unified integration.

Phase 8 selects `--lock tools/sdk/pre-entry-lock.json` in a new private workspace, retaining the original seven-patch default for Phase 7. Its eleven-patch series adds bounded XDBF decoding, stricter reservation, explicit unsupported failures, checked allocation arithmetic and subsystem ownership. [Phase 8 reproduction](../../tests/pre-entry/README.md) and [safety limits](../../docs/research/pre-entry-safety.md). All headers and libraries must be rebuilt together; the extended SystemHeapAlloc signature and ownership metadata are not binary-compatible with old SDK clients.

## Phase 9 lock

Use `--lock tools/sdk/entry-readiness-lock.json` in a new private workspace for the thirteen-patch checked-copy/closed-handle series. Rebuild matching headers and all clients; older locks remain historical. [New fixture recipe](../../tests/guest-memory/README.md) · [Coverage and NO-GO gate](../../docs/research/guest-memory-entry-readiness.md).

## Phase 10 lock

Use `--lock tools/sdk/instrumentation-lock.json` for the fourteen-patch series in a private workspace. Run only one builder against that workspace, wait for all profiles to finish, and rebuild matching clients. [Synthetic instrumentation recipe](../../tests/instrumentation/README.md) · [Coverage and limits](../../docs/research/generated-memory-instrumentation.md). Existing locks remain unchanged.

## Phase 11 lock

Use `--lock tools/sdk/synthetic-lock.json` in a fresh private workspace. The sixteen-patch series adds checked ordinary virtual 64-KiB stack copies and a Mac owned thread-exit boundary with balanced creation-failure ownership. Foreign asynchronous termination explicitly fails before state mutation. Physical/XEX heaps and fetch remain refused. Run one builder, wait for all profiles, then use each matching `sdk/out/mac-arm64/$PROFILE/rexglue` (`rexglued` for Debug) with its staged runtime. [Synthetic module recipe](../../tests/synthetic/README.md). No earlier SDK workspace or global installation is changed.
