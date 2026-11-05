#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Pinned offline AArch64 compile/link of the original workload, no NRO/deploy."""
import argparse,json,hashlib,subprocess,sys,uuid,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
p=argparse.ArgumentParser()
for name in ['workdir','acceptance-output','toolchain']:p.add_argument('--'+name,type=Path,required=True)
a=p.parse_args();w=a.workdir.resolve();host=a.acceptance_output.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:p.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);out=w/'aarch64';out.mkdir();sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest()
r=json.loads((host/'results.json').read_text());assert r['stage']==15 and len(r['host'])==3 and all(x['failed']==0 for x in r['host'])
for n,h in r['sources_sha256'].items():assert sha(ROOT/n)==h,'source drift: '+n
for n,h in r['generation_sha256'].items():assert sha(host/'input/generated'/n)==h
lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());receipt=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==receipt['cli_sha256']
image=json.loads(subprocess.check_output(['docker','image','inspect',lock['image']],text=True,timeout=20))[0];assert lock['image'] in image['RepoDigests'];records=[]
def container(command,label):
 name='compatibility-'+uuid.uuid4().hex[:12];cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
 for src,dst in [(ROOT,'/src'),(tc,'/toolchain'),(host,'/data'),(host/'input/generated','/generated')]:cmd+=['--mount',f'type=bind,src={src},dst={dst},readonly']
 cmd+=['--mount',f'type=bind,src={out},dst=/build','-w','/build',lock['image'],*command]
 try:
  row=run(list(map(str,cmd)),w,label,30);records.append(row)
  if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
 finally:
  cleanup=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
  if cleanup.returncode and 'No such container' not in cleanup.stderr:raise RuntimeError('owned container cleanup failed')
container(['/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++','--version'],'compiler')
container(['python3','-c','from pathlib import Path;assert Path("/opt/devkitpro/pacman/var/lib/pacman/local/libnx-4.12.0-1/desc").is_file();assert Path("/opt/devkitpro/pacman/var/lib/pacman/local/devkitA64-r29.2-1/desc").is_file()'],'packages')
for target in ['main.o','compare.o','initialize.o','query.o','fill.o','leaf.o','mapping.o','hooks.o','audit.elf']:container(['make','-f','/src/tests/sdk-contract-audit/Makefile',target],'build-'+target)
container(['/opt/devkitpro/devkitA64/bin/aarch64-none-elf-nm','-u','audit.elf'],'undefined');assert not (w/'undefined.stdout').read_text().strip()
b=(out/'audit.elf').read_bytes();assert b[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',b,18)[0]==183
(w/'results.json').write_text(json.dumps({'build':'VERIFIED','hardware_execution':'UNKNOWN','nro_created':False,'game_executed':False,'toolchain':lock,'sdk_revision':receipt['sdk_revision'],'original_audit_sources':{p.relative_to(ROOT).as_posix():sha(p) for p in (ROOT/'tests/sdk-contract-audit').iterdir() if p.is_file()},'elf_sha256':sha(out/'audit.elf'),'undefined_symbols':0,'records':records},indent=2)+'\n');print('AArch64 ELF compile/link VERIFIED; no NRO created; hardware UNKNOWN')
