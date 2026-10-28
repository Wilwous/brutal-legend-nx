# Switch roadmap Stage21 — original loader observations

No original-title file is accepted by the driver: it generates and hashes its
own14-case corpus, twice, into private output. The entry contains a marker write
then trap, never invoked. The reference adapter has no CPU executor; the SDK
probe never invokes its runtime guest execution API.
The actual SDK is tool mode. `project.cpp` observes heap metadata and leased
checked copies, active-reference unload refusal and complete rollback.

**Reference B is a source-slice harness, NOT a complete Xenia binary.** The
pinned methods ReadImageUncompressed, ReadImageBasicCompressed and ReadPEHeaders
are extracted byte-identically, together with the descriptor-protection loop.
Their SHA256 and exact source lines are recorded by extract.py. Upstream headers
and crypto are unchanged. The original memory adapter in reference.cpp records
allocation/protection requests; its small vector/page model is not Xenia's native
Memory/BaseHeap, reservation ownership, OS commitment or module cleanup. Imports,
JIT/backend notifications, full LoadContinue, rollback and guest entry are not run.
Logging is a no-op adapter; unsupported encryption explicitly throws and is
rejected before invocation. No SDK or console success stub is substituted.

`reference-lock.json` pins upstream revision, URLs and full file hashes. Obtain
those public files in a separate reference directory preserving their relative
paths and LICENSE. Do not overwrite an existing SDK. Source is read-only and
checked before extraction; no keys or title content are required. The source
inventory includes other audited Xenia files, which are not all compiled here.

```sh
python3 tests/loader-observation/test_corpus.py
python3 tests/loader-observation/run.py --reference "$XENIA_REFERENCE" --sdk "$SDK_DEBUG" --profile Debug --workdir "$OBS_DEBUG"
python3 tests/loader-observation/run.py --reference "$XENIA_REFERENCE" --sdk "$SDK_RELEASE" --profile Release --workdir "$OBS_RELEASE"
python3 tests/loader-observation/run.py --reference "$XENIA_REFERENCE" --sdk "$SDK_SANITIZED" --profile Sanitized --defined-offset --workdir "$OBS_SANITIZED_OFFSET"
```

Output paths must be new and private, outside the repository. Each compile,
emission and probe child has an external30-second process-group watchdog. The
SDK binary/receipt hashes are checked against the existing22-patch SDK lock.
The driver specifies the compile/invocation commands; results retain process
labels, source/extraction/fixture hashes, observations,
checks, errors, CPU time and peak RSS. No large Switch workload or NRO is built.

## Unsuppressed original-reference UBSan limitation

Running profile Sanitized **without** --defined-offset reproduces upstream
ReadPEHeaders's null-member offsetof1 UB on valid PE paths. The preserved original
run had13 UBSan process failures and1 early invalid-PE refusal. Do not count those
13 as passes, disable UBSan, or claim the unchanged reference is sanitizer-clean.

`defined-offset.patch` changes only that macro to standard offsetof, in the
**private extracted copy**. Original source stays unchanged; it is not an
acceptance/ownership/protection policy patch. The driver requires the explicit
--defined-offset option and records the variant. The standard patch applies to
reference-methods-original.inc copied as reference-methods.inc, and produces
byte-for-byte the tested variant. That variant's14 observations match original
Debug/Release exactly. This is narrow comparative evidence, not proof that all
original undefined behavior is absent or that complete Xenia is verified.

The SDK compile also retains its existing ARM64 FPSCR operand-width warning;
it was not hidden or fixed by changing the locked SDK. Initial namespace/linkage
adapter build failures and the initial stale rollback oracle are saved privately.
The old rollback test now expects descriptor/payload mismatch to fail at Load,
while keeping all cleanup/reuse assertions; the loader itself is unchanged.

Original synthetic manifests and sanitized metadata observations are public.
Binary XEX files, compiled host programs and full native diagnostic logs remain
outside Git. These files contain no Brütal Legend-derived bytes or instructions.
