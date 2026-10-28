#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original corpus: actual SDK versus unchanged Xenia method slices.

B is NOT a full Xenia loader or its actual heap. UNKNOWN fields stay unknown.
"""
import argparse,hashlib,json,os,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from extract import extract
from verify import verify
NAMES=['plain','raw-zero','raw-short','pe-overhang','owned-tail','basic-zero','basic-gap','none-short','security-larger','no-read','overlap','invalid-entry','bad-pe','rounded-owned']
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['reference','sdk','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--defined-offset',action='store_true');ap.add_argument('--profile',choices=['Debug','Release','Sanitized'],required=True);a=ap.parse_args();w=a.workdir.resolve();ref=a.reference.resolve();sdk=a.sdk.resolve()
 if w==ROOT or ROOT in w.parents:ap.error('private output required')
 w.mkdir(parents=True,exist_ok=False);meta=json.loads((sdk/'fixture-sdk.json').read_text());pin=json.loads((ROOT/'tests/residency-evidence/sdk-lock.json').read_text())
 if meta!=pin['profiles'][a.profile] or sha(sdk/'lib'/meta['runtime_filename'])!=meta['runtime_sha256']:raise ValueError('SDK mismatch')
 for k in list(os.environ):
  if k.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[k]
 os.environ['UBSAN_OPTIONS']='print_stacktrace=1';records=[];failures=[];observations=[]
 def execute(cmd,label):
  rec=run(list(map(str,cmd)),w,label,30,True);records.append(rec)
  if rec['timeout'] or rec['exit_code']:failures.append(label);return None
  return (w/(label+'.stdout')).read_text()
 flags=['-O0' if a.profile=='Debug' else '-O2','-g']
 if a.profile=='Sanitized':flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 if execute(['clang++','-std=c++23',*flags,'-iquote',ROOT,ROOT/'tests/loader-observation/corpus.cpp','-o',w/'corpus'],'build-corpus') is None:return 1
 for i in [0,1]:
  if execute([w/'corpus',w/f'input-{i}'],'corpus-'+str(i)) is None:return 1
 manifests=[{n:sha(w/f'input-{i}'/(n+'.xex')) for n in NAMES} for i in [0,1]]
 if manifests[0]!=manifests[1]:raise ValueError('nondeterministic corpus')
 (w/'fixture-manifest.json').write_text(json.dumps({'original':True,'guest_entry':'write marker0x6e then trap; never invoked','sha256':manifests[0]},indent=2)+'\n')
 extract(ref,w,a.defined_offset)
 commands=[(['clang++','-std=c++23',*flags,'-DSPDLOG_FMT_EXTERNAL','-I'+str(sdk/'include'),ROOT/'tests/loader-observation/project.cpp','-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',w/'project'],'build-project'),(['clang',*flags,'-c',ref/'third_party/crypto/rijndael-alg-fst.c','-o',w/'crypto.o'],'build-reference-crypto'),(['clang++','-std=c++23',*flags,'-I'+str(ref/'src'),'-I'+str(ref),'-I'+str(w),ROOT/'tests/loader-observation/reference.cpp',w/'crypto.o','-o',w/'reference'],'build-reference')]
 for cmd,label in commands:
  if execute(cmd,label) is None:
   (w/'build-failure.json').write_text(json.dumps({'records':records,'failures':failures},indent=2)+'\n');return 1
 for name in NAMES:
  rows={}
  for model,cmd in [('project',[w/'project',w/'input-0',name+'.xex',w/('runtime-'+name)]),('reference_slice',[w/'reference',w/'input-0'/(name+'.xex')])]:
   label=model+'-'+name;out=execute(cmd,label)
   if out is None:continue
   lines=[json.loads(s) for s in out.splitlines() if s.startswith('{')]
   if len(lines)!=1:failures.append(label+'-json');continue
   rows[model]=lines[0]
  observations.append({'case':name,**rows})
 findings=[]
 for rec in records:
  for ext in ['stdout','stderr']:
   p=w/(rec['label']+'.'+ext)
   if re.search(r'ERROR: AddressSanitizer|SUMMARY:.*Sanitizer|runtime error:',p.read_text(errors='replace')):findings.append(p.name)
 passed,contract_failures=verify(observations)
 failures+=contract_failures
 result={'checks_passed':passed,'checks_failed':len(contract_failures),'profile':a.profile,'observations':observations,'records':records,'failures':failures,'sanitizer_findings':findings,'fixture_manifest':manifests[0],'reference_revision':'95a5c3ee250f80c3b9d139658649d9ffb6db3eec','reference_variant':'defined-offset' if a.defined_offset else 'original','reference_scope':'original image/PE/protection source slices; bounded observation memory adapter; full Xenia load UNOBSERVED','source_sha256':{p.name:sha(p) for p in (ROOT/'tests/loader-observation').glob('*') if p.is_file()},'original_game_loaded':False,'guest_instructions_executed':0}
 (w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:result[k] for k in ['profile','failures','sanitizer_findings']}));return bool(failures or findings)
if __name__=='__main__':sys.exit(main())
