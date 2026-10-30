#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Two isolated pinned AArch64 builds of an already host-verified original fixture."""
import argparse,hashlib,json,subprocess,sys,uuid,shutil,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser()
for n in ['workdir','host-output','toolchain','icon']:ap.add_argument('--'+n,required=True,type=Path)
a=ap.parse_args();w=a.workdir.resolve();host=a.host_output.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False);results=json.loads((host/'results.json').read_text());assert len(results['tests'])==9 and all(not r['failed'] and not r['remaining'] and not r['leases'] for r in results['tests'])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
generated=host/'input/generated';actual_generated={p.name:sha(p) for p in sorted([*generated.glob('*.h'),*generated.glob('*.cpp')])};assert hashlib.sha256(json.dumps(actual_generated,sort_keys=True,separators=(',',':')).encode()).hexdigest()==results['generated']
assert sha(host/'sdk-compare.inc')==json.loads((ROOT/'tests/horizon-xex/sdk-lock.json').read_text())['function_sha256']
actual_runtime=hashlib.sha256(b''.join((ROOT/n).read_bytes() for n in ['runtime/horizon/native_image_plan.h','runtime/horizon/native_image_binding.h','runtime/horizon/generated.h','runtime/horizon/guest.h','runtime/horizon/space.h','tests/native-image-binding/probe.cpp'])).hexdigest();assert actual_runtime==results['runtime'], 'host/runtime source drift'
prepared=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==prepared['cli_sha256'];lock=json.loads((ROOT/'tools/sdk/xex-binding-lock.json').read_text())
for p in lock['patches']:assert sha(ROOT/p['path'])==p['sha256']
tool=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());info=json.loads(subprocess.check_output(['docker','image','inspect',tool['image']],text=True,timeout=20))[0];assert tool['image'] in info['RepoDigests']
records=[];builds=[]
for i in range(2):
 out=w/f'switch-{i}';out.mkdir()
 def container(command,label):
  name='native-'+uuid.uuid4().hex[:12];cmd=['docker','run','--rm','--pull','never','--platform',tool['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
  for src,dst in [(ROOT,'/src'),(tc,'/toolchain'),(host,'/data'),(host/'input/generated','/generated'),(a.icon.resolve(),'/icon.jpg')]:cmd+=['--mount',f'type=bind,src={src},dst={dst},readonly']
  cmd+=['--mount',f'type=bind,src={out},dst=/build','-w','/build',tool['image'],*command]
  try:
   r=run(list(map(str,cmd)),w,f'{label}-{i}',30);records.append(r)
   if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; inspect bounded diagnostics')
  finally:
   p=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
   if p.returncode and 'No such container' not in p.stderr:raise RuntimeError('owned container cleanup failed')
 for target in ['main.o','probe.o','leaf.o','mapping.o','hooks.o','probe.nro']:container(['make','-f','/src/tests/native-image-horizon/Makefile',target],'build-'+target)
 container(['python3','/src/tests/native-image-horizon/inspect_nro.py'],'inspect')
 builds.append({n:sha(out/n) for n in ['probe.elf','probe.nro','leaf.o','mapping.o','probe.o','main.o','hooks.o']})
assert builds[0]==builds[1], 'clean build identity mismatch'
ready=w/'ready';app=ready/'package/switch/brutal-legend-nx-native-image';app.mkdir(parents=True)
name='brutal-legend-nx-native-image.nro';shutil.copy2(w/'switch-0/probe.nro',app/name);shutil.copy2(ROOT/'LICENSE',app/'LICENSE.txt');shutil.copy2(ROOT/'THIRD_PARTY_NOTICES.md',app/'THIRD_PARTY.txt')
(app/'README.txt').write_text('Brutal Legend NX - Native Image Probe / 0.1.0-n3\nOriginal synthetic PPC only, no game code.\nBUILD VERIFIED; HARDWARE UNKNOWN until actual screen evidence.\nExpected finite workload: result32, SDK2/2, allocations0, leases0.\nSuccessful tests print actual measured counters; expected summary PASS36 FAIL0.\nUse hbmenu, photograph full screen, press PLUS and verify normal return.\nThis separate app preserves Stage15; no SD deployment was performed.\n')
zip_path=ready/'brutal-legend-nx-native-image.zip'
with zipfile.ZipFile(zip_path,'x',compression=zipfile.ZIP_DEFLATED) as z:
 for f in sorted((ready/'package').rglob('*')):
  if f.is_file():z.write(f,f.relative_to(ready/'package'))
source_files={p.relative_to(ROOT).as_posix():sha(p) for base in ['runtime','tests/native-image-binding','tests/native-image-horizon'] for p in (ROOT/base).rglob('*') if p.is_file() and '__pycache__' not in str(p)}
report={'source_files_sha256':source_files,'generated_files_sha256':actual_generated,'builds':builds,'deterministic_clean_builds':2,'inspection':json.loads((w/'switch-0/inspection.json').read_text()),'host_results':results['tests'],'source_image':results['source'],'ppc':results['ppc'],'generated':results['generated'],'runtime_source_contract':results['runtime'],'toolchain':tool,'SDK19_lock_sha256':sha(ROOT/'tools/sdk/xex-binding-lock.json'),'records':records,'hardware_execution':'UNKNOWN','proprietary_execution':False}
(w/'results.json').write_text(json.dumps(report,indent=2)+'\n')
(ready/'BUILD-MANIFEST.json').write_text(json.dumps({k:v for k,v in report.items() if k!='records'},indent=2)+'\n');(ready/'SHA256SUMS').write_text(sha(app/name)+'  '+str((app/name).relative_to(ready))+'\n'+sha(zip_path)+'  '+zip_path.name+'\n');print(json.dumps({'sha256':builds[0],'ready':str(ready),'hardware':'UNKNOWN'}))
