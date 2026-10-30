# Non-admitting native image provenance audits

2026-10-30. This method separates byte provenance from guest image admission.
It does not map guest addresses, execute instructions or supply missing data.

## Representation first

Identify the original container, each derived artifact, producing tool/receipt,
transformation stage, whole-file hash, logical origin and byte-offset mapping.
Do not identify a representation from its filename. A decompressor output and a
post-import-binding loader snapshot can be valid products of different stages
without being interchangeable initializers. Record the SDK/source pin separately
from exact binary/creation receipts; source resemblance alone is not build proof.

Compare both full hashes and explicit half-open offset/address intervals. Record
byte equality, byte difference and semantic uncertainty separately. A differing
import word may be a legitimate resolver result, but its old pointer value is
not authority for a new runtime allocation. Replaying observed changed words can
prove a byte transformation while leaving provider/lifetime semantics unresolved.

Retain failed oversized-decode artifacts as diagnostics, never as evidence that
unfilled padding should initialize a guest image. Likewise, PE raw pointers,
virtual RVAs, compressed-container offsets and host addresses are distinct.
No automatic remapping or inferred zero fill is permitted.

## Interval ledger

[Generic interval accounting](../../tools/native_provenance.py) partitions an
explicit audit domain at source, descriptor, section and other evidence boundaries.
It rejects invalid/wrapping extents, overlapping content authorities, unknown
source kinds, missing evidence/zero rules and bad source/hash boundaries. Claims
without a source become UNBACKED; unclaimed gaps become UNKNOWN. A PE declaration
never creates source bytes or native ownership.

Classes are FILE-BACKED, DERIVED, EXPLICIT ZERO-FILL, UNBACKED and UNKNOWN.
Classification is relative to the chosen original input. For encrypted compressed
containers, verified decoded bytes are DERIVED, even if stored in a local file.
Observed zero-valued decoded bytes remain DERIVED unless an applicable explicit
zero-initialization rule is separately established. Each row retains identity,
offset, initialization, intended permissions, ownership/lifetime status and
relocation evidence. The helper verifies bookkeeping, not the truth of supplied
evidence; callers must independently validate hashes and transformations.

Cryptographic content checks are not proof of live ownership, RSA signatures,
console commitment or functional correctness. Shared decompressor implementations
must not be counted as independent decoders. A second header parser is independent
for header interpretation, not for the image bytes it consumes.

## Drafts and strict rejection

The audit draft is explicitly non-admitting. It carries all unresolved regions,
source/semantic identities, TLS/import/relocation/startup/code prerequisites and
never reports full title admission. Do not serialize a lossy projection as if it
were the full NativeImagePlan. Current schema-v1 synthetic origin, whole-page,
region-budget, selected-signature and static-TLS constraints remain unchanged.

[DATA-only validator reader](../../tests/native-provenance/native_refusal.cpp)
accepts bounded metadata and an ordinary source file. It uses checked integer/
boolean parsing, calls the existing N1 validator and records expected refusal.
It never constructs Image/Space or invokes a guest PC. Its second origin-isolation
control is diagnostic, not an admitted partial plan. Unknown code identity and
unsupported title provenance must not be replaced with synthetic defaults.

An optional materialization audit may copy only verified intervals into an
ordinary inert host buffer and compare with independent source identities. It
must allocate no unknown tail as an image, grant no executable permissions and
invoke no game functions. This is a data audit, not a successful loader run.

## Reproducible original tests

```sh
python3 tests/native-provenance/test_intervals.py
python3 tests/native-provenance/run_reader.py --workdir "$NEW_READER_OUTPUT"
python3 tests/native-image-plan/run.py --workdir "$NEW_PLAN_OUTPUT"
python3 tests/native-image-contract/run.py --workdir "$NEW_TAIL_OUTPUT"
python3 tests/bounded-cfg/test_manifest.py
python3 tests/residency-evidence/test_residency.py
```

Use fresh directories outside the repository. The reader fixtures require no
proprietary data. Debug, Release and ASan/UBSan are externally supervised with
30-second process-group watchdogs. Malformed protocol exits 1/2 are expected
refusals, not sanitizer findings. The tail fixture now reports 45 semantic checks
(28 positive/17 refusal) separately from 327680 byte comparisons; this separates
the historical327725 combined count without removing a single comparison.

Detailed title ledgers, source paths, disassembly, coordinates, native draft
projections and private source hashes remain outside Git. Public output contains
aggregate coverage, methodology, generic code/tests and explicit gate limits.
