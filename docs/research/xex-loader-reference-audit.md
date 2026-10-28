# Switch roadmap Stage 20 — independent XEX loader reference audit

This is Switch research Stage20, **not preserved Xbox Phase22**. Baseline
`bdd5b6d`; no original-title load/entry, SDK modification or new Switch build.
**PROVEN BY SOURCE** below means behavior of the pinned implementation only.
G4/G8 remain BLOCKED; G5 UNKNOWN. No emulator result establishes console policy.

## Pinned inventory and provenance

- Xenia `95a5c3ee250f80c3b9d139658649d9ffb6db3eec`:
  [loader](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.cc),
  [size calculation/header](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/xex_module.h),
  [guest heap](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/memory.cc),
  [POSIX host mapping](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/base/memory_posix.cc),
  [UserModule](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/user_module.cc).
- RXDK360 `7b631cc6fa9aec69b24a60390895ad10cb94a52a`:
  [packing documentation](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/docs/xex-packing.md),
  [elf2xex producer](https://github.com/Team-Resurgent/RXDK360/blob/7b631cc6fa9aec69b24a60390895ad10cb94a52a/tools/elf2xex.py).
- xsdk `2c4599b31066efcf67be099673287fbec786ef14`:
  [hardware notes](https://github.com/xenosworks/xsdk/blob/2c4599b31066efcf67be099673287fbec786ef14/docs/HARDWARE.md).
  External author's observations, not hardware observed in this assignment.
- [Microsoft PE specification](https://learn.microsoft.com/en-us/windows/win32/debug/pe-format):
  Windows format semantics; does not specify an Xbox XEX allocation extension.

The machine-readable Xenia file SHA256 inventory is
[reference-lock.json](../../tests/loader-observation/reference-lock.json).
RXDK/xsdk files and their pinned receipts are verified privately against the
preserved source copies. ReXGlue v0.10.0 is derived from Xenia; comparing those
implementations is not independent hardware corroboration. RXDK is an independent
**producer**, not a second independent loader. No copyrighted title bytes, private
keys or decrypted image are included in the source inventory or public fixtures.

## Compression-path reconstruction

Line numbers refer to pinned Xenia sources, not current upstream HEAD.
D = sum(page_count × selected guest heap page_size); P = file bytes after header;
B = sum BASIC data_size+zero_size; S = security image_size; V = PE VirtualSize.

| Path / exact symbol and location | Requested allocation / initialization | Reservation, commitment, permissions and limitations |
| --- | --- | --- |
| NONE: xex_module.cc522–562 ReadImageUncompressed | P; memset P, then copy P (or decrypt when encryption=NORMAL) | AllocFixed(base,P,4096,Reserve\|Commit,RW). Heap rounds to guest page/alignment; S and PE V are not allocation authority here. Metadata describes more than the payload only at its own risk. |
| BASIC: cc564–652 ReadImageBasicCompressed | D; memset entire D; advance output by data+zero for each block | Same reserve/commit request with D; explicit zero blocks and unused space inside D start zero. B is computed separately. Unpatched arithmetic/source-input/output checks are weaker than the project's hardened checks. This is not permission to run malformed unbounded copies. |
| NORMAL/LZX: cc654–774 ReadImageCompressed, header72–86 image_size | image_size() = D, **not stored S**; decrypt/deblock compressed bytes, memset D, lzx_decompress into D | Same reserve/commit RW request. Decompression result gates success. No producer/oracle for a new valid LZX fixture is established here; source evidence only, no compression-path execution claim. |
| PE: cc776–867 ReadPEHeaders | Validate DOS/PE/machine/optional header/subsystem; record declared spans | Called after image materialization, before final protection. It does not extend allocation to V or reject all span overlap/ownership conflicts. Recording a section is not reserving or committing it. |
| Protection: cc1064–1084 LoadContinue | Descriptor kind CODE/READONLY→R; DATA→RW | Uses heap page units; returned Protect booleans are ignored in this loop. This differs from the strict project check; unknown type has no matching switch branch. Host protection is not a software owner token. |
| Patch extension: cc304–317 ApplyPatch | New descriptor-derived image_size can explicitly allocate a size delta | Conditioned on patch/new security metadata; cannot justify growing an unpatched title from V alone. No title update is applied. |

Source applicability: XEX2, selected guest heap; page unit differs by heap (Xenia
memory.cc36–45 map, heap initialization). A fixed4096 argument is rounded by
BaseHeap; it does not prove every image uses4KiB pages. RXDK's producer selects
page_size_for(base), constructs a memory-layout image from ELF sections, rounds
its end and emits descriptor coverage (elf2xex.py build_pe_basefile~266–350,
build_page_descriptors485–550, build_security_info554–588). Its zero-initialized
bytearray establishes its **own** image contents, not an unknown consumer rule.
Its packer does not produce the disputed independent overhang-owner observation.

## Allocation and lifetime evidence matrix

| Claim / source symbol | Applicability and supporting implementation | Conflict / remaining uncertainty | Confidence |
| --- | --- | --- | --- |
| Reservation is distinct metadata from commitment | memory.cc839–920 BaseHeap::AllocFixed: aligned size, page-state scan, allocation request, state/base/region count updates | For combined Reserve\|Commit, the existing-reservation rejection tests equality with Reserve alone. Not an exclusive owner capability. No adversarial native heap execution claimed from this inspection. | PROVEN BY SOURCE, Xenia only |
| Decommit does not necessarily remove host readability | memory.cc1063–1092 BaseHeap::Decommit clears Commit bit; host decommit block is commented out | POSIX host accessibility cannot stand in for guest committed state. Source comments describe intent; active bit updates are evidence. | PROVEN BY SOURCE |
| Release clears guest metadata; host range can remain accessible | memory.cc1094–1151 BaseHeap::Release; protect_on_release default false at27–28 | Region-base check, no owner token; optional noaccess, not universal backing removal. Real Xbox cleanup UNKNOWN. | PROVEN BY SOURCE |
| Native reservation is not Xbox commitment | memory_posix.cc86–102 AllocFixed ignores allocation_type and mmap's host memory; Protect104–115 uses mprotect | This backend's host mechanics cannot establish Xbox guest ownership. QueryProtect returns false at117–119. | PROVEN BY SOURCE |
| Full lifecycle needs more than PE parsing | xex_module.cc958–975 LoadContinue sets finished_load before ReadPEHeaders; Unload1088 onward checks loaded and releases base | This observation harness will not execute complete Load/Continue/Unload or CPU/backend/import handling. Their rollback cannot be inferred from adapter cleanup. | PROVEN BY SOURCE; full reference lifecycle UNOBSERVED |
| Loader return is separate from entry | user_module.cc51–132 LoadFromFile,134–196 LoadFromMemory,198 onward LoadXexContinue | Source maps failed continuation to UNSUCCESSFUL; this does not imply guest exception dispatch. Other launch paths need their own review. | PROVEN BY SOURCE |
| Independent image queries can fail | xsdk HARDWARE.md4740–4760 NtQueryVirtualMemory follow-up: author reports image query refusal and protection-only result | Different hardware/kernel/patch provenance; no owner/extent of this title. A failed query cannot be replaced by zero or a default committed span. | External report; original-title ownership UNKNOWN |

No row proves original-title owner, reservation, commitment, initialized tail,
permission establishment, neighboring owner integrity, lifetime or rollback.
The project's real SDK is separately pinned at22 patches; it uses explicit
AllocFixedOwned, checked-copy owner/commit/protection and transactional cleanup.
No patch lock is changed to imitate Xenia's weaker validation.

## Exception and failure taxonomy

[Debug export source](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/kernel/xboxkrnl/xboxkrnl_debug.cc)
103–142: HandleCppException inspects selected structures then debugger-breaks;
RtlRaiseException special-cases naming/C++ codes, otherwise breaks; it is marked
Stub and has an unwind TODO. This is affirmative evidence **against** treating
this reference as a complete kernel/title exception oracle.

[Processor::OnUnhandledException](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/cpu/processor.cc#L705)
returns false without a listener/managed thread; otherwise pauses/suspends for a
debugger. [POSIX ExceptionHandlerCallback](https://github.com/xenia-project/xenia/blob/95a5c3ee250f80c3b9d139658649d9ffb6db3eec/src/xenia/base/exception_handler_posix.cc)
captures native signal context. Neither is an Xbox guest unwind contract.

| Event | Observable without proprietary execution? | What is not implied |
| --- | --- | --- |
| Header/descriptor rejection or allocation failure | Yes: original fixtures and actual SDK load status/heap state | No guest CPU exception or successful startup |
| Failed continuation/rollback | Yes in actual SDK original fixtures; full reference lifetime not run | Adapter byte-vector cleanup is not reference unload |
| Unsupported pointer/guest memory refusal | Yes via checked synthetic copies; no deliberate raw fault needed | Native signal handling is not guest dispatch |
| Guest CPU trap/exception | Requires a synthetic CPU execution experiment, outside this load-only harness | Source RtlRaiseException stub cannot validate continuation |
| Kernel exception dispatch/title handler/unwind | Requires exact independent contracts and context/ordering evidence | Not established by loader status, C++ catch or Switch hardware smoke |

Stage20 audit complete in source scope. Stage21 reports separately which code
spans were actually observed; no source-backed hypothesis is promoted to hardware.
