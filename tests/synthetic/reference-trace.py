#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Read actual private SDK reference counts with bounded LLDB; no memory writes."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sdk',type=Path,required=True);ap.add_argument('--workdir',type=Path,required=True)
    a=ap.parse_args();sdk=a.sdk.resolve();work=a.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    meta=json.loads((sdk/'fixture-sdk.json').read_text())
    if meta['config']!='Debug' or hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('matching Debug SDK required')
    for name in list(os.environ):
        if name.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[name]
    work.mkdir(parents=True,exist_ok=False);records=[]
    cmd=['clang++','-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0','-g','-I'+str(sdk/'include'),str(ROOT/'tests/synthetic/reference-probe.cpp'),'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(work/'probe')]
    rec=run(cmd,work,'build-probe',30);records.append(rec)
    if rec['exit_code'] or rec['timeout']:raise RuntimeError('reference probe compilation failed')
    (work/'commands.txt').write_text('breakpoint set -n checkpoint\nbreakpoint command add 1\nframe variable phase\nframe variable thread->pointer_ref_count_\ncontinue\nDONE\nrun\n')
    rec=run(['/usr/bin/lldb','--batch','-s',str(work/'commands.txt'),'--',str(work/'probe'),str(work/'runtime')],work,'reference-trace',30);records.append(rec)
    output=(work/'reference-trace.stdout').read_text()
    counts=[int(n) for n in re.findall(r'Value = (\d+)',output)]
    phases=re.findall(r'phase = .*?"(constructed|created|executing|joined|handle-closed)"',output)
    expected=['constructed','created','executing','joined','handle-closed']
    passed=not rec['exit_code'] and not rec['timeout'] and counts==[2,2,2,2,1] and phases==expected and output.count('DESTROYED\n')==1 and 'exited with status = 0' in output
    result={'result':'PASS' if passed else 'FAIL','phases':phases,'pointer_reference_counts':counts,'destructors':output.count('DESTROYED\n'),'records':records,'sdk':meta}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['records','sdk']}))
    return 0 if passed else 1
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,RuntimeError,ValueError) as error:print(str(error),file=sys.stderr);sys.exit(1)
