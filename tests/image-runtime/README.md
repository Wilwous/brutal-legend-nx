# Phase 12 image/runtime contracts

Independently authored fixtures execute original generated PPC and selected real SDK exports. They never accept a game path. The original unencrypted XEX2/minimal PE32 fixture exercises the actual SDK loader without dispatching image instructions. Custom OwnedModule/ThreadTls tests remain separate from real XexModule/UserModule ownership.

Build an isolated fixture SDK with `tools/sdk/image-runtime-lock.json` (seventeen ordered patches). The first sixteen historical patches/locks are unchanged. Never install globally:

```sh
python3 tools/sdk/build.py --lock tools/sdk/image-runtime-lock.json --workspace "$PRIVATE/sdk-repro" --cmake "$CMAKE" --ninja "$NINJA" --source-cache "$SDK_SOURCE" --interposable-runtime
python3 tests/image-runtime/run.py --sdk "$SDK" --rexglue "$REXGLUE" --workdir "$PRIVATE/phase12-Debug"
python3 tests/image-runtime/run.py --sdk "$SDK_RELEASE" --rexglue "$REXGLUE_RELEASE" --workdir "$PRIVATE/phase12-Release"
python3 tests/image-runtime/run.py --sdk "$SDK_SANITIZED" --rexglue "$REXGLUE_SANITIZED" --workdir "$PRIVATE/phase12-Sanitized" --sanitize
python3 tests/image-runtime/audit.py --source "$SDK_SOURCE" --output "$PRIVATE/import-audit.json"
```

`--interposable-runtime` is a macOS test-only linkage option, default off. It enables ld `-interposable` so two bounded children can actually inject a return-zero or bad_alloc at the SDK SystemHeapAlloc entry for the header request. Normal direct internal binding made the first injection attempt ineffective; its failed oracle is retained privately. Each final injection must prove one interception and then disarm/retry the real loader successfully. This is failure injection into the actual runtime path, not a replacement success implementation. The child-only dylib is never loaded into ordinary regression children, and no memory protection is bypassed. All required suites use the same matching staged profile SDK. Normal-linkage initial ownership results and the unmodified Phase 11 SDK remain separate evidence.

Use fresh private directories. Every generation/build/execution child has an external monotonic 30-second process-group watchdog, explicit exit codes, disabled core dumps, capped private diagnostics and wait4 accounting. Fixtures are O0 / O2 / O2+ASan+UBSan, SDK Debug / Release / Sanitized. No global SDK is used.

Acceptance requires **all twenty groups** to pass. The three original failure assertions in `xex.cpp` remain byte-identical. The ten additional actual-SDK ownership groups cover independent images/collision/foreign reservations, ten failure stages, basic compression, recognized function-import metadata, sixteen same-XexModule reloads, sixteen UserModule metadata lifetimes, active leased owner references, forty-eight scheduled concurrent image cycles, and two header allocation failure paths. All independent groups run; any signal, timeout or failed assertion makes the suite exit 1 and is retained in results.json.

A real UserModule ImageLease retains the legitimate module reference and pins the image. Unload with a lease returns the existing UNSUCCESSFUL status and closes new admission. Final lease release does not force unload while other owners exist. Header metadata remains UserModule-owned through explicit image unload and is reclaimed at final destruction; failed file-load metadata rolls back immediately. Raw borrowed headers/section spans require a lease and owner lifetime. Legacy title dispatch is not automatically admitted through this API.

The custom OwnedModule/ThreadTls types implement a separate narrow protocol, not SDK HMODULE or XEX TLS. SDK/memory outlive all handles and leases. Checked scopes and TLS blocks end before their lease/XThread exit. Reset/Restore/MMIO changes require quiescence. The import matrix is source-contract documentation, not a production admission policy or Xbox ABI certification.

See [the Phase 12 report](../../docs/research/image-ownership-runtime-contracts.md) for exact final results and the continuing private-execution NO-GO gate.
