#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded three-profile Phase 5-16 workflow; all products stay private."""
import argparse,hashlib,json,os,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/sdk'))
from supervise import command

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['sdk-workspace','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--build',action='store_true')
 for n in ['cmake','ninja','source-cache']:ap.add_argument('--'+n,type=Path)
 a=ap.parse_args();work=a.workdir.resolve();sdkwork=a.sdk_workspace.resolve()
 if any(p==ROOT or ROOT in p.parents for p in [work,sdkwork]):ap.error('private output required')
 work.mkdir(parents=True,exist_ok=False);env={k:v for k,v in os.environ.items() if not k.startswith(('DYLD_','REXSDK','VK_','VULKAN_'))};records=[];failures=[]
 def execute(cmd,label,limit=300,cwd=ROOT):
  rec=command(cmd,work,label,limit,env,cwd=cwd);records.append(rec);return rec
 if a.build:
  if not a.cmake or not a.ninja:ap.error('--build requires --cmake and --ninja')
  extra=['--source-cache',str(a.source_cache.resolve())] if a.source_cache else []
  execute(['python3',str(ROOT/'tools/sdk/build.py'),'--lock',str(ROOT/'tools/sdk/xex-binding-lock.json'),'--workspace',str(sdkwork),'--cmake',str(a.cmake),'--ninja',str(a.ninja),'--interposable-runtime',*extra],'sdk-build',4500)
 lock=json.loads((ROOT/'tools/sdk/xex-binding-lock.json').read_text());build=json.loads((sdkwork/'build-results.json').read_text())
 if build['revision']!=lock['sdk_revision'] or build['patches']!=lock['patches'] or not build['interposable_test_linkage']:raise RuntimeError('SDK build/lock mismatch')
 source=sdkwork/'sdk'
 def git(*args,cwd=source):return subprocess.check_output(['git',*args],cwd=cwd,text=True,timeout=30).strip()
 if git('rev-parse','HEAD')!=lock['sdk_revision'] or git('ls-files','--others','--exclude-standard'):raise RuntimeError('SDK source mismatch')
 for sub in lock['submodules']:
  if git('rev-parse','HEAD',cwd=source/sub['path'])!=sub['commit'] or git('status','--porcelain',cwd=source/sub['path']):raise RuntimeError('dependency mismatch')
 verify=work/'source-verification';verify.mkdir();changed=git('diff','--name-only').splitlines()
 for name in changed:
  f=verify/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(subprocess.check_output(['git','show','HEAD:'+name],cwd=source,timeout=30))
 for patch in lock['patches']:
  if hashlib.sha256((ROOT/patch['path']).read_bytes()).hexdigest()!=patch['sha256']:raise RuntimeError('patch digest mismatch')
  execute(['git','apply',str(ROOT/patch['path'])],'verify-'+Path(patch['path']).stem,30,cwd=verify)
 for name in changed:
  if (verify/name).read_bytes()!=(source/name).read_bytes():raise RuntimeError('unexpected SDK source change')
 # Historical Phase 5 controls come from pristine pinned Git objects, never
 # from an undocumented local snapshot or old linked runtime.
 sys.path.insert(0,str(ROOT/'tests/runtime'));from run import PINS
 controls=work/'historical-controls'
 for name,digest in PINS.items():
  data=subprocess.check_output(['git','show','HEAD:'+name],cwd=source,timeout=30)
  if hashlib.sha256(data).hexdigest()!=digest:raise RuntimeError('historical control mismatch')
  f=controls/name;f.parent.mkdir(parents=True,exist_ok=True);f.write_bytes(data)
 profiles={}
 for profile in ['Debug','Release','Sanitized']:
  sdk=sdkwork/'staged'/profile;meta=json.loads((sdk/'fixture-sdk.json').read_text());san=['--sanitize'] if profile=='Sanitized' else [];cli=source/'out/mac-arm64'/profile/('rexglued' if profile=='Debug' else 'rexglue')
  for name in changed:
   if name.startswith('include/') and (source/name).read_bytes()!=(sdk/name).read_bytes():raise RuntimeError('staged header mismatch')
  suites=[('binding','tests/xex-binding/run.py',['--rexglue',str(cli)]),('generated','tests/generated-dispatch/run.py',['--rexglue',str(cli)]),('pointer','tests/pointer-import/run.py',[]),('tls-import','tests/tls-import/run.py',['--source',str(source)]),('image','tests/image-runtime/run.py',['--rexglue',str(cli)]),('synthetic','tests/synthetic/run.py',['--rexglue',str(cli)]),('instrument','tests/instrumentation/run.py',['--source',str(source)]),('guest','tests/guest-memory/run.py',[]),('unified','tests/unified/run.py',['--source',str(source),'--pre-entry']),('pre-entry','tests/pre-entry/run.py',[])]
  ordinary=faults=0
  for name,script,extra in suites:
   output=work/(name+'-'+profile)
   try:
    execute(['python3',str(ROOT/script),'--sdk',str(sdk),'--workdir',str(output),*extra,*san],name+'-'+profile)
    result=json.loads((output/'results.json').read_text())
    if result['sdk']!=meta or result.get('failed_groups') or result.get('unexpected_failures',0):raise RuntimeError('suite oracle failed')
    for f,digest in result.get('fixture_sources_sha256',{}).items():
     if hashlib.sha256((ROOT/f).read_bytes()).hexdigest()!=digest:raise RuntimeError('test sources changed')
    ordinary+=result['passed_groups'];faults+=result.get('fault_checks',0);print(profile,name,'PASS',flush=True)
   except (RuntimeError,OSError):failures.append([profile,name]);print(profile,name,'FAIL; private diagnostics retained',flush=True)
  opt='0' if profile=='Debug' else '2'
  for cmd,label in [(['python3',str(ROOT/'tests/runtime/run.py'),'--sdk-source-root',str(controls),'--workdir',str(work/('components-'+profile)),'--optimization',opt,*san],'components'),(['python3',str(ROOT/'tests/vmx128/run.py'),'--sdk-source',str(source/'src/codegen/builders/vector.cpp'),'--sdk-include',str(sdk/'include'),'--workdir',str(work/('vmx-'+profile)),'--alias-safe','--optimization',opt,*san],'vmx')]:
   try:execute(cmd,label+'-'+profile,180)
   except RuntimeError:failures.append([profile,label])
  profiles[profile]={'ordinary_pass':ordinary,'expected_fault_recovery_pass':faults}
 for label,cmd,budget in [('watchdog',['python3',str(ROOT/'tests/ppc/test_watchdog.py')],60),('static-entry',['python3',str(ROOT/'tests/guest-memory/test_audit.py')],60),('static-instrumentation',['python3',str(ROOT/'tests/instrumentation/test_audit.py')],60),('encoding-old',['python3',str(ROOT/'tests/synthetic/test_fixture.py')],60),('encoding-caller',['python3',str(ROOT/'tests/generated-dispatch/test_fixture.py')],60),('encoding-xex',['python3',str(ROOT/'tests/xex-binding/test_fixture.py')],60),('source-audit',['python3',str(ROOT/'tests/image-runtime/audit.py'),'--source',str(source),'--output',str(work/'import-source-audit.json')],30),('signature-audit',['python3',str(ROOT/'tests/tls-import/audit.py'),'--source',str(source)],30),('reference',['python3',str(ROOT/'tests/synthetic/reference-trace.py'),'--sdk',str(sdkwork/'staged/Debug'),'--workdir',str(work/'reference')],120)]:
  try:execute(cmd,label,budget)
  except RuntimeError:failures.append(['controls',label])
 result={'profiles':profiles,'failed_suites':failures,'records':records,'sdk_build_versions':build['versions'],'patch_lock_sha256':hashlib.sha256((ROOT/'tools/sdk/xex-binding-lock.json').read_bytes()).hexdigest(),'game_code_executed':False}
 (work/'workflow-results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2));return bool(failures)
if __name__=='__main__':
 try:sys.exit(main())
 except (RuntimeError,OSError,ValueError,subprocess.SubprocessError) as e:print(str(e),file=sys.stderr);sys.exit(1)
