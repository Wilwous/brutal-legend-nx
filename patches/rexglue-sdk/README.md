# Focused SDK safety patches

Target: ReXGlue **0.10.0**, commit `f5337cdc947ff6d4c4196737e2c807a48f2a1fc2`. This is an open-source SDK patch, not a game patch. It contains no proprietary code.

`0001-runtime-safety.patch`:

- Snapshots the complete source vector before float16_4 output writes. The scope prevents temporary-name collisions. It changes aliasing only; numeric conversion, mask warnings and rounding remain unchanged.
- Bounds XThread TLS access against dynamic slot bytes, checks null output/invalid metadata and computes the address without 32-bit multiplication/addition wrap.
- Returns `X_STATUS_NOT_IMPLEMENTED` from unsupported STFS device/control operations. It does not dereference potentially invalid output pointers or fabricate a device. Callers must check failure status; outputs remain unchanged on failure.

Apply only to a separate private source copy:

```sh
git -C "$PRIVATE_SDK_COPY" apply --check "$PROJECT/patches/rexglue-sdk/0001-runtime-safety.patch"
git -C "$PRIVATE_SDK_COPY" apply "$PROJECT/patches/rexglue-sdk/0001-runtime-safety.patch"
```

The runtime test runner verifies source fingerprints, copies the four relevant source files to its private output directory and checks/applies the patch there. The VMX runner recognizes the exact alias-patched builder hash with `--alias-safe`. Whole SDK integration, linking and game behavior remain unvalidated; the installed prebuilt SDK and previous game output are unchanged. This patch has not been submitted upstream.

Patch contributions are **BSD-3-Clause**, following the affected SDK source. Original SDK/Xenia notices and the full conditions are retained in [LICENSE.BSD-3-Clause](LICENSE.BSD-3-Clause). Original standalone foundation/harness code elsewhere in this repository remains GPL-3.0-only. No reference-port implementation was copied.

[Evidence, tests and limitations](../../docs/research/runtime-foundation.md).

`0002-host-fault-and-protect.patch` (apply after 0001) makes an unclaimed POSIX fault fail-stop via its original signal instead of retrying an unchanged instruction, and converts NtProtectVirtualMemory old protection through the existing ToXdkProtectFlags helper.

```sh
git -C "$PRIVATE_SDK_COPY" apply --check "$PROJECT/patches/rexglue-sdk/0002-host-fault-and-protect.patch"
git -C "$PRIVATE_SDK_COPY" apply "$PROJECT/patches/rexglue-sdk/0002-host-fault-and-protect.patch"
```

[Phase 6](../../docs/research/ppc-execution.md) validates selected real PPC exports, real-class TLS methods and isolated dispatcher behavior with original fixtures. A hybrid test executable does not establish complete SDK library integration or guest unwinding. The installed SDK remains unchanged; neither patch has been submitted upstream. Phase 6 contributions retain the same BSD-3-Clause boundary.
