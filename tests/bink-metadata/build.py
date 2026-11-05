#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Pinned offline AArch64 ELF checks of metadata, module and existing V2 paths."""
import argparse,hashlib,json,struct,subprocess,sys,uuid
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'tests/ppc'));from watchdog import run
p=argparse.ArgumentParser()
for key in ['workdir','toolchain','v2-output','module-output']:p.add_argument('--'+key,type=Path,required=True)
a=p.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve();v2=a.v2_output.resolve();mod=a.module_output.resolve()
if w==R or R in w.parents:p.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();records=[];outputs={}
lock=json.loads((R/'tests/switch-horizon/toolchain.json').read_text());receipt=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==receipt['cli_sha256'] and receipt['sdk_revision']==lock['sdk_revision']
image=json.loads(subprocess.check_output(['docker','image','inspect',lock['image']],text=True,timeout=20))[0];assert lock['image'] in image['RepoDigests']
for name,source in [('v2',v2),('module',mod)]:
 result=json.loads((source/'results.json').read_text());assert len(result['tests'])==3 and all(not r.get('failures',0) and not r['allocations'] and not r['leases'] for r in result['tests'])
 for file,digest in result['generated_files' if name=='v2' else 'generation_sha256'].items():assert sha(source/'input/generated'/file)==digest
 if name=='v2':
  for file,digest in result['source_files'].items():assert sha(R/file)==digest
 else:assert sha(source/'input/fixture.xex')==result['source']

def container(target,source,command,label):
 name='readiness-'+uuid.uuid4().hex[:12];args=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
 mounts=[(R,'/src'),(tc,'/toolchain')]+([(source,'/data'),(source/'input/generated','/generated')] if source else [])
 for src,dst in mounts:args+=['--mount',f'type=bind,src={src},dst={dst},readonly']
 args+=['--mount',f'type=bind,src={w/target},dst=/build','-w','/build',lock['image'],*command]
 try:
  row=run(args,w,label,30);records.append(row)
  if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; bounded diagnostics retained')
 finally:
  clean=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
  if clean.returncode and 'No such container' not in clean.stderr:raise RuntimeError('owned container cleanup failed')
for target,source,make,objects,elf in [
 ('metadata',None,'/src/tests/bink-metadata/Makefile',['metadata.o'],'metadata.elf'),
 ('module',mod,'/src/tests/bink-metadata/Makefile',['module.o','leaf.o','mapping.o','hooks.o'],'module.elf'),
 ('v2',v2,'/src/tests/platform-v2/Makefile',['main.o','demo.o','leaf.o','mapping.o','hooks.o'],'probe.elf')]:
 (w/target).mkdir()
 if target=='metadata':
  container(target,source,['/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++','--version'],'gcc');assert lock['gcc'] in (w/'gcc.stdout').read_text()
  container(target,source,['python3','-c','from pathlib import Path;assert Path("/opt/devkitpro/pacman/var/lib/pacman/local/libnx-4.12.0-1/desc").is_file()'],'libnx')
 if target=='v2':container(target,source,['python3','/src/tests/platform-v2/dependencies.py'],'v2-dependencies')
 for obj in objects+[elf]:container(target,source,['make','-f',make,obj],target+'-'+obj)
 container(target,source,['/opt/devkitpro/devkitA64/bin/aarch64-none-elf-nm','-u',elf],target+'-undefined');assert not (w/(target+'-undefined.stdout')).read_text().strip()
 path=w/target/elf;b=path.read_bytes();assert b[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',b,18)[0]==183
 outputs[target]={'elf_sha256':sha(path),'undefined_symbols':0}
assert not list(w.rglob('*.nro'))
(w/'results.json').write_text(json.dumps({'outputs':outputs,'records':records,'toolchain':lock,'hardware':'UNKNOWN','nro_created':False,'title_input':False},indent=2)+'\n');print('Verified 3 AArch64 ELFs; 0 undefined symbols; no NRO')
