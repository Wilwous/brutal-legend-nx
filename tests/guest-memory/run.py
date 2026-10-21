#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded checked guest-memory fixtures against the unified pinned SDK."""
import argparse
import hashlib
import json
import os
from pathlib import Path
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
    if (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('sanitizer profile mismatch')
    for name in list(os.environ):
        if name.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[name]
    os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT)]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    records=[];rows=[]
    for name in ['contracts','imports']:
        binary=work/name
        commands=[(['clang++',*flags,str(ROOT/'tests/guest-memory'/(name+'.cpp')),'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-'+name),([str(binary),str(work/(name+'-runtime'))],name)]
        for cmd,label in commands:
            rec=run(cmd,work,label,30,True);records.append(rec)
            if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; inspect private diagnostics')
        observed=[json.loads(line) for line in (work/(name+'.stdout')).read_text().splitlines() if line.startswith('{')]
        if len(observed)!=1 or observed[0].get('result')!='PASS':raise RuntimeError('missing result')
        rows.extend(observed)
    result={'sdk':meta,'records':records,'passed_groups':len(rows),'checks':rows,'unexpected_failures':0}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({'passed_groups':len(rows),'checks':rows,'seconds':sum(r['seconds'] for r in records if not r['label'].startswith('build-')),'peak_rss_bytes':max(r['peak_rss_bytes'] for r in records if not r['label'].startswith('build-'))}))
if __name__=='__main__':
    try:main()
    except (RuntimeError,OSError) as e:print(str(e),file=sys.stderr);sys.exit(1)
