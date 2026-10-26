#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Stage 2: original raw PPC -> real production codegen -> host/native builds."""
import argparse,hashlib,json,re,subprocess,sys,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
from fixture import create

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def sources():
 names=['runtime/checked_memory.h','runtime/cooperative_stop.h','tests/switch-horizon/result_ui.h','tests/switch-horizon/toolchain.json','tools/switch/sanitize_elf.py']
 names+=['runtime/horizon/'+n for n in ['native.h','space.h','guest.h','generated.h']]
 names+=['tests/horizon-foundation/'+n for n in ['fixture.py','probe.h','probe.cpp','host.cpp','main.cpp','run.py','Makefile','inspect_nro.py','templates/test/ppc_config_h.inja','templates/test/ppc_test_functions_cpp.inja']]
 return {n:sha(ROOT/n) for n in names}
def bindings(meta):
 text='#pragma once\n#include "runtime/horizon/generated.h"\n'
 for name,pc in meta['functions'].items():text+=f'REX_EXTERN(foundation_{pc:X});\ninline constexpr uint32_t foundation_{name}={pc};\n'
 text+='inline constexpr uint64_t foundation_module_id=0x48524632;\ninline const brutal::horizon::Function foundation_functions[]{'+','.join(f'{{{pc},foundation_{pc:X},"foundation_{pc:X}"}}' for pc in meta['functions'].values())+'};\n';return text

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for name in ['workdir','toolchain']:ap.add_argument('--'+name,type=Path,required=True)
 ap.add_argument('--switch-build',action='store_true');ap.add_argument('--icon',type=Path)
 a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
 if w==ROOT or ROOT in w.parents:ap.error('private output required')
 w.mkdir(parents=True,exist_ok=False);records=[]
 def execute(cmd,label,expected=0):
  row=run(list(map(str,cmd)),w,label,30);row['expected_exit_code']=expected;records.append(row)
  if row['timeout'] or row['exit_code']!=expected:raise RuntimeError(label+' failed; private logs preserved')
  return (w/(label+'.stdout')).read_text()
 receipt=json.loads((tc/'prepared.json').read_text())
 if sha(tc/'rexglue')!=receipt['cli_sha256']:raise ValueError('CLI receipt mismatch')
 meta=create(w/'input');generated=w/'generated';baseline=w/'baseline';generated.mkdir();baseline.mkdir()
 cli=[tc/'rexglue','recompile-tests','--bin-dir',w/'input','--asm-dir',w/'input']
 execute([*cli,'--output',generated,'--templates',ROOT/'tests/horizon-foundation/templates'],'generation')
 execute([*cli,'--output',baseline],'baseline')
 first={p.name:sha(p) for p in generated.iterdir()}
 execute([*cli,'--output',generated,'--templates',ROOT/'tests/horizon-foundation/templates'],'regeneration')
 if first!={p.name:sha(p) for p in generated.iterdir()}:raise ValueError('non-deterministic generation')
 cpp=(generated/'ppc_test_functions.cpp').read_text();original=(baseline/'ppc_test_functions.cpp').read_text()
 if cpp[cpp.index('DEFINE_REX_FUNC('):]!=original[original.index('DEFINE_REX_FUNC('):]:raise ValueError('production bodies differ')
 comments=re.findall(r'^\s*// (.+)$',cpp,re.M);comments=[s.strip() for s in comments if not s.startswith(('Generated','DO NOT'))]
 expected=[s.strip() for s in (w/'input/foundation.s').read_text().splitlines() if s.startswith('    ')]
 # Resolve symbolic branch operands independently against the encoded words.
 for i,word in enumerate(meta['words']):
  if word>>26 in (16,18):
   bits=16 if word>>26==16 else 26;delta=word&(0xfffc if bits==16 else 0x3fffffc);delta-=1<<bits if delta&(1<<(bits-1)) else 0
   expected[i]=expected[i].split()[0]+' '+hex(meta['base']+4*i+delta)
 if comments!=expected or len(comments)!=30 or len(meta['backedges'])!=2:raise ValueError('independent instruction/CFG census mismatch')
 counts={key:cpp.count(key+'(') for key in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64']}
 if list(counts.values())!=[4,4,1,1]:raise ValueError('memory site census drift')
 (w/'foundation-data.h').write_text(bindings(meta));hosts=[]
 for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
  binary=w/('host-'+name)
  execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',w,'-I',generated,ROOT/'tests/horizon-foundation/host.cpp',ROOT/'tests/horizon-foundation/probe.cpp',generated/'ppc_test_functions.cpp','-o',binary],'compile-'+name)
  row=json.loads(execute([binary],'run-'+name))
  if row['failed'] or row['passed']!=108 or [g['passed'] for g in row['groups']]!=[19,26,15,18,13,17]:raise ValueError('host runtime failure/count drift')
  hosts.append({'profile':name,**row})
 negatives=['REX_RAW_ADDR(0)','REX_LOAD_VECTOR(ctx.v0,0)','REX_QUERY_TIMEBASE()','ppc_longjmp(0,0)','foundation_82010000(ctx.v0,base)']
 for i,expression in enumerate(negatives):
  f=w/f'negative-{i}.cpp';f.write_text('#include "ppc_config.h"\nREX_EXTERN(foundation_82010000);\nvoid test(PPCContext& ctx,uint8_t* base){(void)ctx;(void)base;'+expression+';}\n')
  execute(['clang++','-std=c++23','-isystem',tc/'include','-iquote',ROOT,'-I',generated,'-c',f,'-o',w/'negative.o'],f'negative-{i}',1)
  if expression.split('(')[0] not in (w/f'negative-{i}.stderr').read_text():raise ValueError('unclassified compile refusal')
 result={'host':hosts,'static':{'functions':3,'instructions':30,'memory_sites':counts,'backedges':2,'backedge_policy':'each loop targets an admitted helper call; no per-branch hook is claimed'},'generation_sha256':first,'input_sha256':sha(w/'input/foundation.bin'),'negative_compile_refusals':len(negatives),'sources_sha256':sources(),'hardware_execution':'UNKNOWN','game_executed':False}
 if a.switch_build:
  lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());outputs=[]
  for i in range(2):
   out=w/f'switch-{i}';out.mkdir()
   def container(command,label):
    name='foundation-'+uuid.uuid4().hex[:12]
    cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
    for src,dst in [(ROOT,'/src'),(tc,'/toolchain'),(generated,'/generated'),(w,'/data')]:cmd+=['--mount',f'type=bind,src={src},dst={dst},readonly']
    cmd+=['--mount',f'type=bind,src={out},dst=/build']
    if a.icon:cmd+=['--mount',f'type=bind,src={a.icon.resolve()},dst=/icon.jpg,readonly']
    cmd+=['-w','/build',lock['image'],*command]
    try:execute(cmd,f'{label}-{i}')
    finally:
     clean=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
     if clean.returncode and 'No such container' not in clean.stderr:raise RuntimeError('container cleanup failed')
   container(['make','-f','/src/tests/horizon-foundation/Makefile','REPO=/src',*(['ICON=/icon.jpg'] if a.icon else [])],'link')
   container(['python3','/src/tests/horizon-foundation/inspect_nro.py'],'inspect')
   outputs.append({n:sha(out/n) for n in ['probe.nro','probe.elf','leaf.o']})
  if outputs[0]!=outputs[1]:raise ValueError('clean build mismatch')
  result['switch']={'sha256':outputs[0],'deterministic_builds':2,'inspection':json.loads((w/'switch-0/inspection.json').read_text())}
 if first!={p.name:sha(p) for p in generated.iterdir()}:raise ValueError('generated output mutated')
 result['records']=records;(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('records','sources_sha256','generation_sha256')},indent=2))
if __name__=='__main__':main()
