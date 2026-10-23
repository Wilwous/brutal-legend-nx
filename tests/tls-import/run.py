#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original XEX TLS/import fixtures against one pinned SDK profile."""
import argparse,hashlib,json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for key in ['sdk','workdir','source']:ap.add_argument('--'+key,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();work=a.workdir.resolve()
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 meta=json.loads((sdk/'fixture-sdk.json').read_text())
 if meta['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' or (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('profile/revision mismatch')
 if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:raise RuntimeError('runtime mismatch')
 for key in list(os.environ):
  if key.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[key]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';work.mkdir(parents=True,exist_ok=False)
 from audit import audit
 census=audit(a.source.resolve());(work/'admission-audit.json').write_text(json.dumps(census,indent=2)+'\n')
 records=[];checks=[];failures=[]
 def execute(args,label,required=False):
  rec=run(args,work,label,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:
   failures.append(label)
   if required:raise RuntimeError(label+' failed; private diagnostics retained')
  return rec
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT)]
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')];binary=work/'contracts'
 execute(['clang++',*flags,str(ROOT/'tests/tls-import/contracts.cpp'),*libs,'-o',str(binary)],'build-contracts',True)
 inject=work/'allocation-failure.dylib';native=work/'create-failure.dylib'
 execute(['clang++',*flags,'-dynamiclib',str(ROOT/'tests/tls-import/allocation-failure.cpp'),*libs,'-o',str(inject)],'build-allocation-failure',True)
 execute(['clang++',*flags,'-dynamiclib',str(ROOT/'tests/synthetic/create-failure.cpp'),'-o',str(native)],'build-create-failure',True)
 for mode in ['denied-matrix','malformed','lifetime','repeat','failure','admission','tls-zero','tls-throw','pcr-zero','pcr-throw','scratch-zero','native-failure']:
  cmd=[str(binary),str(work/('runtime-'+mode)),mode]
  if mode in ['tls-zero','tls-throw','pcr-zero','pcr-throw','scratch-zero']:cmd=['/usr/bin/env','DYLD_INSERT_LIBRARIES='+str(inject),*cmd]
  if mode=='native-failure':cmd=['/usr/bin/env','DYLD_INSERT_LIBRARIES='+str(native)+':'+str(inject),*cmd]
  rec=execute(cmd,mode)
  rows=[json.loads(line) for line in (work/(mode+'.stdout')).read_text().splitlines() if line.startswith('{')];rec['checks']=rows
  passed=[row for row in rows if row.get('result')=='PASS']
  if not rec['exit_code'] and not rec['timeout']:
   if len(passed)!=1:failures.append(mode+':missing-oracle')
   else:checks+=passed
 title=work/'title-tls'
 execute(['clang++',*flags,str(ROOT/'tests/tls-import/title-tls.cpp'),*libs,'-o',str(title)],'build-title-tls',True)
 rec=execute([str(title),str(work/'runtime-title-tls')],'title-tls')
 rows=[json.loads(line) for line in (work/'title-tls.stdout').read_text().splitlines() if line.startswith('{')];rec['checks']=rows
 passed=[row for row in rows if row.get('result')=='PASS']
 if not rec['exit_code'] and not rec['timeout']:
  if len(passed)!=1:failures.append('title-tls:missing-oracle')
  else:checks+=passed
 sources=[p for p in (ROOT/'tests/tls-import').glob('*') if p.is_file()]+[ROOT/'runtime/import_admission.h',ROOT/'runtime/pointer_imports.h',ROOT/'runtime/sdk_checked_memory.h',ROOT/'tests/image-runtime/xex-fixture.h',ROOT/'tests/synthetic/create-failure.cpp']
 result={'sdk':meta,'records':records,'checks':checks,'passed_groups':len(checks),'failed_groups':failures,'game_loaded':False,'fixture_sources_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources}}
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','fixture_sources_sha256']},indent=2));return bool(failures)
if __name__=='__main__':sys.exit(main())
