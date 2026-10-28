# Original residency evidence fixtures

No proprietary input is used by the test suites. `audit_xex_residency.py`, the20
metadata unit fixtures, loader probe and15 original SDK cases were individually
reviewed and reused from public commit
`f3c6affb913fa4fb4da1031fdbf6a9b4fca73416`; this is not an Xbox branch merge.
Eight new cases plus exact fencepost/cross-section assertions strengthen the
existing owner rule. No loader patch is applied by these tools.

The isolated **22-patch** desktop SDK is distinct from the **19-patch** Switch
codegen/leaf SDK. Its revision, patch source commit/lock hash and staged macOS
runtime hashes are in `sdk-lock.json`. Reproduce its build with the SDK tools and
`tools/sdk/residency-lock.json` from that pinned public commit, in a separate
private SDK checkout. Never overwrite the Switch SDK or a global installation.
Receipts alone do not replace source verification: the recorded research also
reversed all22 patches in a private copy of the31 modified files and compared
the result byte-for-byte to pristine pinned source. Original SDK was untouched.

Use new output directories outside the checkout:

```sh
python3 tests/residency-evidence/test_residency.py
python3 tests/residency-evidence/run.py --sdk "$SDK_DEBUG" --workdir "$OUT_DEBUG"
python3 tests/residency-evidence/run.py --sdk "$SDK_RELEASE" --workdir "$OUT_RELEASE"
python3 tests/residency-evidence/run.py --sdk "$SDK_SANITIZED" --workdir "$OUT_SANITIZED" --sanitize
```

Each compile and child fixture is watched externally for30 seconds using
`tests/ppc/watchdog.py`; timeout kills its whole process group. Full logs, exact
commands, source hashes, CPU time, peak RSS and exit statuses stay private.
The SDK tests use Runtime tool mode and UserModule, no XThread/PPC dispatch.
Positive cases test actual loaded image leases, not a transcription of loader
policy. Missing/invalid descriptors, one-byte tail, foreign neighbor, malformed
base/permissions/overlap, BASIC expansion overflow/overrun/truncation must refuse.
Decommit/noaccess after a valid synthetic load must refuse checked copies without
changing the host output sentinel. An active lease blocks unload; releasing it
permits complete cleanup. Raw uncommitted pointers are never dereferenced.
BASIC compression bounds are tested; this is not an LZX decoder fuzz campaign.

## Optional private offline reproduction

This separate operation requires an explicitly authorized input. It does not
execute guest code. Set XEX_FILE, EXPECTED_XEX_SHA256, SDK_DEBUG and PRIVATE_OUT
in your environment. PRIVATE_OUT must name a new private directory outside the
checkout. The source XEX is read-only, no adjacent title update is allowed.
The load probe source below is the exact reviewed source used in this research.
A real loader refusal exits4 and must not be reported as successful loading.

```sh
python3 - <<'PYCODE'
from pathlib import Path
import hashlib, json, os, sys
repo=Path.cwd().resolve(); x=Path(os.environ['XEX_FILE']).resolve()
sdk=Path(os.environ['SDK_DEBUG']).resolve(); out=Path(os.environ['PRIVATE_OUT']).resolve()
assert out != repo and repo not in out.parents
assert not (x.parent/'default.xexp').exists()
assert hashlib.sha256(x.read_bytes()).hexdigest()==os.environ['EXPECTED_XEX_SHA256']
out.mkdir(parents=True,exist_ok=False)
meta=json.loads((sdk/'fixture-sdk.json').read_text())
lock=json.loads((repo/'tests/residency-evidence/sdk-lock.json').read_text())
assert meta==lock['profiles']['Debug']
assert hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()==meta['runtime_sha256']
for k in list(os.environ):
    if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')): del os.environ[k]
sys.path.insert(0,str(repo/'tests/ppc')); from watchdog import run
binary=out/'loader'
cmd=['clang++','-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0','-g',
     '-I'+str(sdk/'include'),str(repo/'tests/residency-evidence/load_probe.cpp'),
     '-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)]
r=run(cmd,out,'build-loader',30,True); assert not r['timeout'] and r['exit_code']==0
r=run([str(binary),str(x.parent),x.name,str(out/'runtime')],out,'loader',30,True)
assert not r['timeout'] and r['exit_code']==4
rows=[json.loads(s) for s in (out/'loader.stdout').read_text().splitlines() if s.startswith('{')]
assert len(rows)==1 and rows[0]['load_status']==0xc0000001
assert rows[0]['image_virtual_pages_reclaimed'] and rows[0]['guest_functions_invoked']==0
PYCODE
```

For a preserved independently decoded image, the read-only census additionally is:

```sh
python3 tools/audit_xex_residency.py --xex "$XEX_FILE" --image "$DECODED_IMAGE" --report "$PRIVATE_REPORT"
```

It checks descriptor/content consistency and bounds, not RSA authentication,
physical memory, live ownership, permission establishment or executable safety.
Never publish its title-specific report or turn metadata coverage into admission.
