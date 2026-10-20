#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original Phase 8 fixtures; no game input or execution."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sdk',type=Path,required=True)
    ap.add_argument('--workdir',type=Path,required=True)
    ap.add_argument('--sanitize',action='store_true')
    a=ap.parse_args();sdk=a.sdk.resolve();work=a.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    work.mkdir(parents=True,exist_ok=False)
    meta=json.loads((sdk/'fixture-sdk.json').read_text())
    if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('SDK hash mismatch')
    for n in list(os.environ):
        if n.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[n]
    os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include')]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
    records=[];blocked=[];groups=0;faults=0
    def execute(cmd,label,status=0):
        rec=run(cmd,work,label,30,True);records.append(rec)
        if rec['timeout'] or rec['exit_code']!=status:raise RuntimeError(label+' unexpected failure; inspect private diagnostics')
        return rec
    for name in ['allocation-boundaries','xdbf','contracts','vector-context','memory-faults']:
        execute(['clang++',*flags,str(ROOT/'tests/pre-entry'/(name+'.cpp')),*libs,'-o',str(work/name)],'build-'+name)
        if name!='memory-faults':
            rec=execute([str(work/name),*([str(work/'runtime')] if name=='contracts' else [])],name)
            rows=[json.loads(line) for line in (work/(name+'.stdout')).read_text().splitlines() if line.startswith('{')]
            if len(rows)!=1 or rows[0].get('result')!='PASS':raise RuntimeError('missing contract result '+name)
            rec['checks']=rows;groups+=1
    for name in ['vector-negative-return','vector-negative-mixed']:
        rec=run(['clang++',*flags,'-c',str(ROOT/'tests/pre-entry'/(name+'.cpp')),'-o',str(work/(name+'.o'))],work,name,30);records.append(rec)
        diagnostic=(work/(name+'.stderr')).read_text()
        if not rec['exit_code'] or rec['timeout'] or 'VRegister' not in diagnostic:raise RuntimeError('unsupported vector contract changed')
        blocked.append({'finding':name,'status':'BLOCKED'})
    modes={'hole':-signal.SIGBUS,'physical-hole':-signal.SIGBUS,'reserved':-signal.SIGBUS,'decommit':-signal.SIGBUS,'release':-signal.SIGBUS,'physical-decommit':-signal.SIGBUS,'physical-release':-signal.SIGBUS,'physical-readonly':-signal.SIGBUS,'capture':-signal.SIGABRT,'unwind':-signal.SIGABRT,'handler':-signal.SIGABRT}
    for mode,status in modes.items():execute([str(work/'memory-faults'),mode],'fault-'+mode,status);faults+=1
    slack=79 if os.sysconf('SC_PAGE_SIZE')>4096 else -signal.SIGBUS
    execute([str(work/'memory-faults'),'host-slack'],'host-slack',slack)
    if slack==79:blocked.append({'finding':'host_page_slack_accessible','status':'BLOCKED'})
    else:faults+=1
    result={'sdk':meta,'records':records,'passed_groups':groups,'fault_checks':faults,'unexpected_failures':0,'blocked_findings':blocked}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['records','sdk']}))
if __name__=='__main__':
    try:main()
    except (RuntimeError,OSError) as e:print(str(e),file=sys.stderr);sys.exit(1)
