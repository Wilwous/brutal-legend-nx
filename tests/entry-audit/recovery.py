#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original SDK loader/context recovery cases; no game inputs."""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
MODES=['within','unaligned-section','overstated-pe','zero-fill','hole','short-payload','extra-payload',
 'beyond-owner','discardable-beyond','overlap','wrapping-rva','wrapping-size',
 'missing-read','missing-flags','allocation-collision','partial-rollback',
 'unbacked-extension','active-lease']
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ['sdk','workdir']:ap.add_argument('--'+name,type=Path,required=True)
 ap.add_argument('--sanitize',action='store_true');a=ap.parse_args();sdk=a.sdk.resolve();work=a.workdir.resolve()
 if work==ROOT or ROOT in work.parents:ap.error('private output required')
 work.mkdir(parents=True,exist_ok=False);meta=json.loads((sdk/'fixture-sdk.json').read_text())
 sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
 if sha(sdk/'lib'/meta['runtime_filename'])!=meta['runtime_sha256'] or (meta['config']=='Sanitized')!=a.sanitize:raise RuntimeError('SDK/profile mismatch')
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VULKAN_','VK_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
 flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if meta['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-iquote',str(ROOT)]
 if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
 records=[];checks=[];failures=[];findings=[]
 for name,modes in [('loader_contracts',MODES),('context_fields',['fields'])]:
  binary=work/name;rec=run(['clang++',*flags,str(ROOT/'tests/entry-audit'/(name+'.cpp')),*libs,'-o',str(binary)],work,'build-'+name,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:failures.append('build-'+name);continue
  for mode in modes:
   label=name+'-'+mode;cmd=[str(binary),str(work/('runtime-'+mode)),mode] if name=='loader_contracts' else [str(binary)]
   rec=run(cmd,work,label,30,True);records.append(rec)
   rows=[json.loads(s) for s in (work/(label+'.stdout')).read_text().splitlines() if s.startswith('{')]
   if rec['timeout'] or rec['exit_code'] or len(rows)!=1 or rows[0].get('result')!='PASS':failures.append(label)
   else:checks+=rows
 for rec in records:
  for suffix in ['.stdout','.stderr']:
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',(work/(rec['label']+suffix)).read_text(errors='replace')):findings.append(rec['label']+suffix)
 if findings:failures.append('sanitizer')
 files=['tests/entry-audit/recovery.py','tests/entry-audit/context_fields.cpp','tests/entry-audit/loader_contracts.cpp','tests/image-runtime/xex-fixture.h']
 result=dict(sdk=meta,records=records,checks=checks,passed_groups=len(checks),failed_groups=failures,sanitizer_findings=findings,fixture_sources_sha256={n:sha(ROOT/n) for n in files},game_code_executed=False)
 (work/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records','checks','fixture_sources_sha256']},indent=2));return bool(failures)
if __name__=='__main__':sys.exit(main())
