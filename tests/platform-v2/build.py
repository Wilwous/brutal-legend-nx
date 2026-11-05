#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Two clean offline pinned A64 builds; separate synthetic NRO/ZIP, no deployment."""
import argparse
import hashlib
import json
import shutil
import subprocess
import sys
import uuid
import zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
p=argparse.ArgumentParser()
for name in ['workdir','host-output','toolchain','icon','notices']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();w=a.workdir.resolve();host=a.host_output.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:p.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False)
sha=lambda path:hashlib.sha256(path.read_bytes()).hexdigest()
results=json.loads((host/'results.json').read_text());assert len(results['tests'])==3 and all(not row['failures'] for row in results['tests'])
for name,digest in results['source_files'].items():assert sha(ROOT/name)==digest,'host source drift: '+name
for name,digest in results['generated_files'].items():assert sha(host/'input/generated'/name)==digest
receipt=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==receipt['cli_sha256']
lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());image=json.loads(subprocess.check_output(['docker','image','inspect',lock['image']],text=True,timeout=20))[0];assert lock['image'] in image['RepoDigests']
romfs=w/'romfs/resources';romfs.mkdir(parents=True);
for name in ['scene.bin','checker.rgba','ambient.pcm','action.pcm']:shutil.copy2(host/'assets'/name,romfs/name)
graphics_lock=json.loads((ROOT/'tests/platform-v2/graphics-lock.json').read_text());assert sha(a.notices)==graphics_lock['attribution_sha256']
records=[];builds=[]
for iteration in range(2):
 out=w/('switch-'+str(iteration));out.mkdir()
 def container(command,label):
  name='platform-'+uuid.uuid4().hex[:12];cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
  for src,dst in [(ROOT,'/src'),(tc,'/toolchain'),(host,'/data'),(host/'input/generated','/generated'),(a.icon.resolve(),'/icon.jpg'),(w/'romfs','/romfs')]:cmd+=['--mount',f'type=bind,src={src},dst={dst},readonly']
  cmd+=['--mount',f'type=bind,src={out},dst=/build','-w','/build',lock['image'],*command]
  try:
   row=run(list(map(str,cmd)),w,f'{label}-{iteration}',30);records.append(row)
   if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; inspect private bounded diagnostics')
  finally:
   cleanup=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
   if cleanup.returncode and 'No such container' not in cleanup.stderr:raise RuntimeError('owned container cleanup failed')
 container(['python3','/src/tests/platform-v2/dependencies.py'],'dependencies')
 for target in ['main.o','demo.o','leaf.o','mapping.o','hooks.o','probe.nro']:container(['make','-f','/src/tests/platform-v2/Makefile',target],'build-'+target)
 container(['python3','/src/tests/platform-v2/inspect_nro.py'],'inspect')
 builds.append({name:sha(out/name) for name in ['probe.nro','probe.elf','main.o','demo.o','leaf.o','mapping.o','hooks.o']})
assert builds[0]==builds[1],'two clean builds disagree'
ready=w/'ready';app=ready/'package/switch/brutal-nx-platform-demo-v2';app.mkdir(parents=True);nro=app/'brutal-nx-platform-demo-v2.nro';shutil.copy2(w/'switch-0/probe.nro',nro)
shutil.copy2(ROOT/'LICENSE',app/'LICENSE.txt');shutil.copy2(ROOT/'THIRD_PARTY_NOTICES.md',app/'THIRD_PARTY.txt')
shutil.copy2(ROOT/'tests/horizon-runtime/NOTICE.txt',app/'RUNTIME-NOTICES.txt');shutil.copy2(a.notices,app/'OPEN-SOURCE-NOTICES.txt')
(app/'README.txt').write_text('Brutal NX - Synthetic 3D Demo V2 / 0.105.0\nOriginal independently authored PPC and assets only. NOT THE GAME.\nBUILD VERIFIED; HARDWARE EXECUTION UNKNOWN until actual observation.\nLaunch using hbmenu (Title Override preferred). Four textured animated cubes; left stick moves first object, right stick rotates camera.\nA changes scale and plays original PCM; B ambient toggle; X/Y volume.\nDisconnect/reconnect is shown; first PLUS stops and cleans up.\nPhotograph complete final counters; release PLUS then press PLUS to return to hbmenu.\nNo fixed PASS count: frame/input/audio outcomes depend on actual device use.\nAudio UNAVAILABLE or any cleanup failure must be reported, not counted as PASS.\nNo automatic deployment; older application directories preserved.\n')
zip_path=ready/'brutal-nx-platform-demo-v2-t105.zip'
with zipfile.ZipFile(zip_path,'x',compression=zipfile.ZIP_DEFLATED) as z:
 for f in sorted((ready/'package').rglob('*')):
  if f.is_file():
   info=zipfile.ZipInfo(str(f.relative_to(ready/'package')),(2026,11,5,0,0,0));info.compress_type=zipfile.ZIP_DEFLATED;info.external_attr=0o100644<<16;z.writestr(info,f.read_bytes())
report={'builds':builds,'clean_builds':2,'inspection':json.loads((w/'switch-0/inspection.json').read_text()),'host_tests':results['tests'],'generation':{k:results[k] for k in ['functions','instructions','hooks','memory_sites','source','ppc','generated','runtime','sdk']},'toolchain':lock,'graphics_lock':graphics_lock,'records':records,'native_sources':{n:sha(ROOT/'tests/platform-v2'/n) for n in ['main.cpp','demo.cpp','demo.h','Makefile','inspect_nro.py','build.py','sanitize_elf.py','ui.h']},'hardware_execution':'UNKNOWN','game_execution':False}
(w/'results.json').write_text(json.dumps(report,indent=2)+'\n');(ready/'BUILD-MANIFEST.json').write_text(json.dumps({k:v for k,v in report.items() if k!='records'},indent=2)+'\n');(ready/'SHA256SUMS').write_text(sha(nro)+'  '+str(nro.relative_to(ready))+'\n'+sha(zip_path)+'  '+zip_path.name+'\n');print(json.dumps({'ready':str(ready),'nro_sha256':sha(nro),'zip_sha256':sha(zip_path),'hardware':'UNKNOWN'}))
