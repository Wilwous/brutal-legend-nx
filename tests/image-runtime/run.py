#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original image ownership fixtures, including non-suppressed SDK regressions."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
sys.path.insert(0,str(ROOT/'tests/synthetic'))
from watchdog import run
from fixture import create

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ['sdk','rexglue','workdir']:ap.add_argument('--'+name,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args()
 sdk=a.sdk.resolve();work=a.workdir.resolve();cli=a.rexglue.resolve()
 if ROOT==work or ROOT in work.parents:ap.error('private output required')
 meta=json.loads((sdk/'fixture-sdk.json').read_text())
 if meta['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2':raise RuntimeError('revision mismatch')
 if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('runtime mismatch')
 if (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('profile mismatch')
 for key in list(os.environ):
  if key.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[key]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
 work.mkdir(parents=True,exist_ok=False);create(work/'input');out=work/'generated';out.mkdir()
 records=[];failures=[];checks=[]
 def execute(cmd,label,required=True):
  rec=run(cmd,work,label,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:
   failures.append(label)
   if required:raise RuntimeError(label+' failed; inspect private diagnostics')
  return rec
 execute([str(cli),'recompile-tests','--bin-dir',str(work/'input'),'--asm-dir',str(work/'input'),'--output',str(out)],'generation')
 generated=out/'ppc_test_functions.cpp';digest=hashlib.sha256(generated.read_bytes()).hexdigest()
 if digest!='7ccd9893b5e2e898e16080e2740a70ec8dc6a0b870cf95aed8641f2936f88d9e':raise RuntimeError('generated fixture drift')
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-I'+str(out),'-I'+str(work/'input'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/generated_access.h"']
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 libraries=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
 injector=work/'allocation-failure.dylib'
 execute(['clang++',*flags,'-dynamiclib',str(ROOT/'tests/image-runtime/allocation-failure.cpp'),*libraries,'-o',str(injector)],'build-allocation-failure')
 for name,modes in [('contracts',['ownership','rollback','concurrent','tls','tls-rollback']),('imports',['selected']),('ownership',['collision','rollback','basic-compression','known-function-import','reload','metadata','active-references','concurrent-images','header-failure-zero','header-failure-throw']),('xex',['load-unload','foreign-allocation','failed-load-rollback','user-module-teardown'])]:
  binary=work/name;extra=[str(generated)] if name=='contracts' else []
  execute(['clang++',*flags,str(ROOT/'tests/image-runtime'/(name+'.cpp')),*extra,*libraries,'-o',str(binary)],'build-'+name)
  for mode in modes:
   label=name+'-'+mode;args=[str(binary),str(work/('runtime-'+label))]
   if name=='contracts':args+=[str(work/'input'),mode]
   elif name in ['xex','ownership']:args+=[mode]
   if name=='ownership' and mode.startswith('header-failure-'):args=['/usr/bin/env','DYLD_INSERT_LIBRARIES='+str(injector),*args]
   rec=execute(args,label,False)
   rows=[json.loads(line) for line in (work/(label+'.stdout')).read_text().splitlines() if line.startswith('{')];rec['checks']=rows
   passed=[row for row in rows if row.get('result')=='PASS']
   if not rec['exit_code'] and not rec['timeout']:
    if len(passed)!=1:failures.append(label+':missing-oracle')
    else:checks+=passed
 if hashlib.sha256(generated.read_bytes()).hexdigest()!=digest:raise RuntimeError('generated source changed')
 inputs=list((ROOT/'tests/image-runtime').glob('*'))+[ROOT/'tests/synthetic/module.h',ROOT/'runtime/generated_access.h',ROOT/'runtime/sdk_checked_memory.h']
 result={'sdk':meta,'records':records,'checks':checks,'passed_groups':len(checks),'failed_groups':failures,'generated_sha256':digest,'game_loaded':False,
 'fixture_sources_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs if p.is_file()}}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
 print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','fixture_sources_sha256']},indent=2))
 return 1 if failures else 0
if __name__=='__main__':sys.exit(main())
