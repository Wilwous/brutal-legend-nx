# Stage 17 — XEX residency evidence gate

**HOST VERIFIED** offline refusal and synthetic owner-policy tests.
**G4 BLOCKED / G5 UNKNOWN / G8 BLOCKED. Original execution NO-GO.**
No SDK/loader change was justified; no game PPC was executed or newly compiled.

## Reproduction and exact predicate

Original XEX SHA256 was reverified as
`636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456`.
The adjacent title update is absent. The actual isolated pinned SDK22-patch
loader, Runtime tool mode, returned `0xC0000001` again. Guest functions invoked0,
guest threads created0; image and virtual heap page counts restored. Child exit4
is the expected refusal, not a successful load. External monotonic limit30s;
observed0.939s, peak RSS58,638,336 bytes, CPU33.28%; no fault or timeout.

Exact section index13 (zero based,14th declaration). Private report retains the
section name, addresses, raw sizes, security/descriptor limits, uncovered span,
reference receipts and exact input hashes; none of its bytes/addresses are copied
into this public report. The newly computed metadata census equals the preserved
report exactly.215 descriptors verify14,090,240 decoded bytes. Security size,
descriptor extent and decoded length agree;13/14 PE spans fit, one does not.
A SHA1 content chain is a consistency check, not RSA signature verification.

The actual `ReadPEHeaders` rejection predicate in the pinned patched source is:

```cpp
uint64_t(base_address_) + sechdr->VirtualAddress > UINT32_MAX ||
!ImageContains(base_address_ + sechdr->VirtualAddress, sechdr->Misc.VirtualSize)
```

`ImageContains` checks nonempty full-span containment in `owned_images_`; it
does not itself query commitment or permission. AllocateImage establishes the
owned reservation/commit transaction; checked image copies separately recheck
owner, commitment and access permission. PE READ flags and nonoverlap are separate
preflight requirements.
The section's full span is not contained in the allocated image. The metadata
comparison identifies this rule without weakening it or running a guest PC.
UserModule maps failed load to `STATUS_UNSUCCESSFUL`. No extra backing is
inferred from PE SizeOfImage, a section label or a successful host translation.

## Evidence ledger

| Evidence | Kind / established fact | Does not establish |
| --- | --- | --- |
| Original XEX + independent decoded image | Verified metadata/content: D=descriptor bytes=S=security size=L=decoded length; one PE tail beyond D | Extra reservation, committed page, initial contents, owner, permissions, lifetime |
| Actual SDK load above | Observed source and host behavior: refuses full-section ownership claim; rollback balances | Xbox hardware loader semantics |
| Pinned Xenia normal loader | Observed source: allocates/decompresses image_size; BASIC sums descriptor pages and zeroes its allocation | A second title-tail allocation; shared SDK ancestry is not independent console evidence |
| Xenia PE section recording | Records VirtualSize spans after decoding | Recording a span does not allocate it |
| Xenia patch grow path | Can extend memory after new security metadata changes image_size | Authorization to grow an unpatched original based only on VirtualSize; no title update was used |
| PE/COFF specification | Documents Windows PE virtual/raw sizing and zero-fill semantics | An Xbox XEX-specific extra allocation/owner policy |
| Independent RXDK producer | Builds security/descriptors to cover its authored image/sections | Consumer behavior for this overhang case |
| Independent xsdk hardware notes | Author reports image query refusal versus protection-only query | Owner/commit/content of this title's tail; a protection flag alone is insufficient |

Sources inspected locally and verified against their preserved hash receipts:
[Xenia loader](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.cc)
(BASIC587–610, normal741–759, PE840–860, patch304–317);
[RXDK packing](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/docs/xex-packing.md),
[producer](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/tools/elf2xex.py);
[xsdk hardware observations](https://github.com/xenosworks/xsdk/blob/2c4599b31066efcf67be099673287fbec786ef14/docs/HARDWARE.md).
The authoritative [Microsoft PE format](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format)
was also checked; its scope is Windows PE, not a proof of XEX residency.

## Competing explanations

| Hypothesis | Support / contrary evidence | Falsifier and smallest missing observation | Without hardware? |
| --- | --- | --- | --- |
| H1 tail is metadata only or discarded | Section classification can suggest it; raw data/read flags/resource intersections do not prove it is unused | Independent loader trace identifies absent commitment and demonstrates relevant consumers never access that span; mere discardable bit is insufficient | Exact trusted loader implementation/contract might suffice; static title headers cannot |
| H2 Xbox establishes additional backing | PE virtual extent is a plausible request; no descriptor or security extent authorizes it in available evidence | Image-capable allocation/PTE/owner observation shows backing or its absence, initial contents, permissions and lifetime | Only trusted documentation/implementation equivalent to that observation; unavailable here |
| H3 representation/decoder mismatch | XEX and PE are different representations | Independently decoded content/metadata reveals a corrected extent or proven transformation | Partly testable: independent decode, all215 links and repeated census agree; live policy remains UNKNOWN |

All three remain hypotheses; none permits changing allocation, dropping a
section or fabricating zero-fill. Existing nonzero backed data/resource overlap
also prevents assuming the whole section is irrelevant. Correct descriptor bounds
and explicit BASIC zero-fill are tested independently, not inferred for the title.

## Synthetic experiments with actual SDK

[Original fixtures, pin and reproducible commands](../../tests/residency-evidence/README.md).
23 isolated cases per profile,337 assertions. All accepted cases retain leases,
check code write refusal, cross-section copy refusal/output sentinel, unload
refusal while leased and final reclamation. One-byte-over-tail and exact last
section byte distinguish section authority from a rounded image allocation.

| Boundary family | Observed result / scope |
| --- | --- |
| Valid owned image, explicit BASIC zero-fill, neighbor, repeated8 loads | Accept; explicit committed zero tail readable through final owned byte; neighbor preserved |
| Declared virtual tail beyond allocation, including1 byte | Refuse; a foreign adjacent allocation cannot supply missing image ownership |
| Missing payload or incomplete/zero/invalid/wrapping descriptors | Refuse; transaction rolls back |
| Base misalignment, overlapping sections, absent READ permission | Refuse; no fabricated access flags |
| BASIC expansion size UINT32 overflow, one-byte overrun, truncated raw input | Refuse before successful load; no LZX fuzz coverage claimed |
| Decommitted/noaccess data after valid load | Checked copy returns uncommitted/protection; output sentinel unchanged; no host fault |
| Active image lease during unload | Refuse new leases/unload; release lease then unload succeeds and pages balance |

| Profile | Groups | Assertions PASS / FAIL | Total child wall seconds incl compile | Max child RSS bytes incl compiler | Sanitizer findings / faults / timeouts |
| --- | --- | --- | --- | --- | --- |
| Debug | 23 | 337 / 0 | 3.935 | 288391168 | 0 / 0 / 0 |
| Release | 23 | 337 / 0 | 2.897 | 298450944 | 0 / 0 / 0 |
| ASan+UBSan | 23 | 337 / 0 | 4.460 | 316555264 | 0 / 0 / 0 |

20 Python metadata/hash-chain tests also PASS. Their reverse-built original
fixture is checked by a forward verifier; static coverage never sets live_owner.
An initial reporting-path bug after the first suite execution was corrected;
failed reporting receipts and the intermediate valid runs remain private. Final
counts above are from the final pinned-source run, not silently merged attempts.

## Required proof and escalation

G4 closes only after independent authority establishes the relevant reservation,
commitment, initial content, permissions, owner, neighbor preservation, lifetime
and rollback/unload behavior for this image class. A RAM dump, PE section name,
query protection, emulator acceptance or synthetic policy alone is insufficient.
G5 additionally needs provenance for consumed initial state; it remains UNKNOWN.
G8 requires the actual proposed exceptional transitions/context/refusal and
unwind contracts; typed host Error and C++ cleanup alone leave it BLOCKED.

Smallest useful external evidence: a nonproprietary control XEX plus an authored
overhang XEX with identical surrounding layout. Record input hashes, kernel and
patch identity, image-capable allocator/PTE/owner observations before and after
load, contents and permissions, neighboring allocation integrity, successful and
failed unload/rollback. Do not invoke title entry. A failing image query must be
reported, not converted to a default committed range. Obtain a trusted loader
contract/trace for the original image class before applying a general correction.
No Xbox debug hardware is available to the user. This is an external evidence
blocker, not a request to repeat Switch tests or write a permissive loader.
