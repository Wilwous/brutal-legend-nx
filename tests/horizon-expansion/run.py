#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Actual XEX production generation, checked native image/runtime, bounded tests."""
import argparse,hashlib,json,re,subprocess,sys,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
import importlib.util
def load(name,path):
 s=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
prior=load('old_xex',ROOT/'tests/horizon-xex/fixture.py')
expanded=load('new_xex',ROOT/'tests/horizon-expansion/fixture.py')

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def signed(v,bits):return v-(1<<bits) if v&(1<<(bits-1)) else v
def decode(word,pc):
 op=word>>26;rt=(word>>21)&31;ra=(word>>16)&31;rb=(word>>11)&31;imm=signed(word&65535,16)
 if op in [32,36,37,58,62]:return f'{dict(zip([32,36,37,58,62],["lwz","stw","stwu","ld","std"]))[op]} r{rt},{imm}(r{ra})'
 if op in [14,15]:return (('li' if op==14 else 'lis')+f' r{rt},{imm}') if ra==0 else f'addi r{rt},r{ra},{imm}'
 if op==24:return f'ori r{ra},r{rt},{word&65535}'
 if op==11:return f'cmpwi r{ra},{imm}'
 if op==18:return ('bl ' if word&1 else 'b ')+hex(pc+signed(word&0x3fffffc,26))
 if op==16:
  if ((word>>21)&31,(word>>16)&31) not in [(4,2),(12,2)]:raise ValueError('unsupported conditional encoding')
  return ('bne ' if (word>>21)&31==4 else 'beq ')+hex(pc+signed(word&0xfffc,16))
 if op==19 and word in [0x4e800020,0x4e800421]:return 'blr' if word==0x4e800020 else 'bctrl'
 if op==63 and (word&0x7ff)==42:return f'fadd f{rt},f{ra},f{rb}'
 if op==31:
  xo=(word>>1)&1023
  if xo==444 and rt==rb:return f'mr r{ra},r{rt}'
  if xo==266:return f'add r{rt},r{ra},r{rb}'
  spr=((word>>16)&31)|(((word>>11)&31)<<5)
  if xo==339 and spr==8:return f'mflr r{rt}'
  if xo==467 and spr in [8,9]:return ('mtlr' if spr==8 else 'mtctr')+f' r{rt}'
 raise ValueError('unreviewed original PPC word '+hex(word))
def sources():
 names=['runtime/checked_memory.h','runtime/cooperative_stop.h','tests/switch-horizon/result_ui.h','tests/switch-horizon/toolchain.json','tools/switch/sanitize_elf.py','tests/horizon-foundation/fixture.py','tests/xex-binding/fixture.py','tests/generated-dispatch/fixture.py']
 names+=['runtime/horizon/'+n for n in ['native.h','space.h','guest.h','generated.h','xex.h']]
 names+=['tests/horizon-xex/'+n for n in ['fixture.py','sdk-lock.json','probe.h','probe.cpp','host.cpp','import.cpp','main.cpp','Makefile','inspect_nro.py','run.py','templates/codegen/pch_h.inja','templates/codegen/init_cpp.inja']]
 return {n:sha(ROOT/n) for n in names}
def data_header(meta,image):
 text='#pragma once\n#include "runtime/horizon/xex.h"\nextern const brutal::horizon::GeneratedXex xex_generated;\n'
 for name,pc in meta['functions'].items():text+=f'inline constexpr uint32_t xex_{"entry" if name=="xex_entry" else "original_entry" if name=="entry" else name}={pc};\n'
 text+='inline constexpr uint32_t xex_hooks[]{'+','.join(map(str,meta['hooks']))+'};\n'
 text+=f'inline constexpr std::array<uint8_t,{len(image)}> xex_bytes{{'+','.join(map(str,image))+'};\n';return text

def main():
 ap=argparse.ArgumentParser(description=__doc__)
 for n in ['toolchain','sdk-source','workdir']:ap.add_argument('--'+n,type=Path,required=True)
 ap.add_argument('--stage',type=int,choices=range(4,9),required=True);ap.add_argument('--switch-build',action='store_true');ap.add_argument('--icon',type=Path);a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
 if w==ROOT or ROOT in w.parents:ap.error('private output required')
 w.mkdir(parents=True,exist_ok=False);records=[]
 def execute(cmd,label,expected=0):
  row=run(list(map(str,cmd)),w,label,30);row['expected_exit_code']=expected;records.append(row)
  if row['timeout'] or row['exit_code']!=expected:raise RuntimeError(label+' failed; inspect private diagnostics')
  return (w/(label+'.stdout')).read_text()
 receipt=json.loads((tc/'prepared.json').read_text())
 if sha(tc/'rexglue')!=receipt['cli_sha256']:raise ValueError('pinned CLI changed')
 lock=json.loads((ROOT/'tests/horizon-xex/sdk-lock.json').read_text());source=a.sdk_source.resolve()/lock['file']
 if sha(source)!=lock['source_sha256']:raise ValueError('SDK source mismatch')
 code=source.read_text();start=code.index('u32 RtlCompareMemoryUlong_entry(');end=code.index('{',start)+1;depth=1
 while depth:depth+=(code[end]=='{')-(code[end]=='}');end+=1
 body=code[start:end]+'\n'
 if hashlib.sha256(body.encode()).hexdigest()!=lock['function_sha256']:raise ValueError('SDK leaf mismatch')
 (w/'sdk-compare.inc').write_text(body)
 if a.stage>=7:
  init_lock=json.loads((ROOT/'tests/horizon-expansion/sdk-lock.json').read_text())
  for name,digest in init_lock['files'].items():
   if sha(a.sdk_source/name)!=digest:raise ValueError('initializer SDK source changed')
  header=(a.sdk_source/'include/rex/system/xobject.h').read_text();header=header[header.index('typedef struct {'):header.index('static_assert_size(X_DISPATCH_HEADER, 0x10);')+len('static_assert_size(X_DISPATCH_HEADER, 0x10);')]+'\n'
  source=(a.sdk_source/'src/kernel/xboxkrnl/xboxkrnl_rtl.cpp').read_text();body=source[source.index('#pragma pack(push, 1)'):source.index('\nX_STATUS xeRtlInitializeCriticalSectionAndSpinCount')]+'\n'
  if hashlib.sha256(header.encode()).hexdigest()!=init_lock['header_sha256'] or hashlib.sha256(body.encode()).hexdigest()!=init_lock['body_sha256']:raise ValueError('initializer extraction fingerprint changed')
  if 'XE_EXPORT(xboxkrnl, 0x0000012E, RtlInitializeCriticalSection,' not in (a.sdk_source/'src/kernel/xboxkrnl/export_table.inc').read_text():raise ValueError('ordinal evidence changed')
  (w/'sdk-initialize.inc').write_text(header+body)
 meta=expanded.create(w/'input',ROOT/'tests/horizon-expansion/templates',initialize=a.stage>=7) if a.stage>=6 else prior.create(w/'input',ROOT/'tests/horizon-xex/templates');cli=[tc/'rexglue','codegen',w/'input/fixture_manifest.toml'];execute(cli,'generation')
 generated=w/'input/generated';emitted=sorted([*generated.glob('*.cpp'),*generated.glob('*.h')]);fingerprints={p.name:sha(p) for p in emitted}
 execute([*cli,'--ignore-stamp'],'regeneration')
 if fingerprints!={p.name:sha(p) for p in emitted}:raise ValueError('non-deterministic production codegen')
 baseline=(w/'input/fixture_manifest.toml').read_text();baseline=re.sub(r'^template_dir=.*\n','',baseline,flags=re.M).replace('out_directory_path="generated"','out_directory_path="baseline"')
 (w/'input/baseline_manifest.toml').write_text(baseline);execute([tc/'rexglue','codegen',w/'input/baseline_manifest.toml'],'baseline-codegen')
 cpp=(generated/'fixture_recomp.0.cpp').read_text()
 if cpp!=(w/'input/baseline/fixture_recomp.0.cpp').read_text():raise ValueError('generated instruction output differs from production default')
 comments=[s.strip() for s in re.findall(r'^\s*// (.*)$',cpp,re.M)]
 expected=[decode(word,meta['code_base']+4*i) for i,word in enumerate(meta['words'])]
 if comments!=expected or len(comments)!=meta['instructions'] or cpp.count('DEFINE_REX_FUNC(')!=len(meta['functions']):raise ValueError('independent exact instruction correspondence failed')
 actual_hooks=[int(s,16) for s in re.findall(r'^\thorizon_point_([0-9A-F]+)\(\);$',cpp,re.M)]
 if actual_hooks!=meta['hooks']:raise ValueError('production hook census mismatch')
 counts={key:cpp.count(key+'(') for key in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC']}
 for denied in ['REX_RAW_ADDR(','REX_UNIMPLEMENTED(','REX_FATAL(','ppc_trap(','base +']:
  if denied in cpp:raise ValueError('unmediated generated operation '+denied)
 (w/'xex-data.h').write_text(data_header(meta,(w/'input/fixture.xex').read_bytes())+'\n#include "expansion-plan.h"\n')
 plan='#pragma once\n#include "runtime/horizon/generated.h"\n'
 if a.stage>=6:
  plan+='REX_EXTERN(__imp__RtlInitializeCriticalSection);\ninline constexpr PPCFunc* expansion_initializer='+('__imp__RtlInitializeCriticalSection' if a.stage>=7 else 'nullptr')+';\n'
  plan+=f'inline constexpr uint32_t expansion_floating={meta["functions"]["float_add"]};\n'
  plan+='inline constexpr brutal::horizon::Site expansion_sites[]{'+','.join('{'+f"{s['pc']},{s['owner']},brutal::horizon::SiteKind::{s['kind']},{s['width']},{s['target']}"+'}' for s in meta['sites'])+'};\n'
  plan+='inline constexpr uint32_t expansion_roots[]{'+','.join(map(str,meta['functions'].values()))+'};\ninline constexpr brutal::horizon::ControlPlan expansion_plan{expansion_sites,expansion_roots,0x8201feed};\ninline constexpr auto expansion_plan_ptr=&expansion_plan;\n'
 else:plan+='inline constexpr const brutal::horizon::ControlPlan* expansion_plan_ptr=nullptr;\n'
 (w/'expansion-plan.h').write_text(plan)
 (w/'hooks.cpp').write_text('#include "runtime/horizon/xex.h"\n'+''.join(f'void horizon_point_{pc:X}(){{brutal::horizon::HookFrame::poll({pc});}}\n' for pc in meta['hooks']))
 hosts=[];stress=[]
 for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
  binary=w/('host-'+name)
  execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-DSTAGE_MAX='+str(a.stage),'-isystem',tc/'include','-iquote',ROOT,'-I',w,'-I',generated,ROOT/'tests/horizon-expansion/host.cpp',ROOT/'tests/horizon-expansion/probe.cpp',ROOT/'tests/horizon-xex/import.cpp',*([ROOT/'tests/horizon-expansion/import.cpp'] if a.stage>=7 else []),generated/'fixture_recomp.0.cpp',generated/'fixture_init.cpp',w/'hooks.cpp','-o',binary],'compile-'+name)
  row=json.loads(execute([binary],'run-'+name))
  if row['failed'] or not row['passed'] or row['remaining'] or row['leases']:raise ValueError('expansion runtime failed')
  if hosts and row!={k:v for k,v in hosts[0].items() if k!='profile'}:raise ValueError('profile result drift')
  hosts.append({'profile':name,**row})
  if a.stage==8:
   heavy=json.loads(execute([binary,'--stress'],'stress-'+name))
   if heavy['failed'] or heavy['remaining'] or heavy['leases']:raise ValueError('bounded stress failed')
   if stress and heavy!={k:v for k,v in stress[0].items() if k!='profile'}:raise ValueError('stress profile drift')
   stress.append({'profile':name,**heavy})
 census=load('independent_census',ROOT/'tests/horizon-expansion/audit.py').audit(meta,cpp) if a.stage>=6 else None
 result={'stress':stress,'census':census,'stage':a.stage,'host':hosts,'static':{'functions':len(meta['functions']),'instructions':meta['instructions'],'hooks':len(meta['hooks']),'counts':counts,'admitted_imports':2 if a.stage>=7 else 1},'sources_sha256':{**sources(),**{p.relative_to(ROOT).as_posix():sha(p) for p in (ROOT/'tests/horizon-expansion').rglob('*') if p.is_file() and '__pycache__' not in str(p)}},'generation_sha256':fingerprints,'xex_sha256':sha(w/'input/fixture.xex'),'sdk_function_sha256':lock['function_sha256'],'hardware_execution':'UNKNOWN','game_executed':False}
 negatives=['REX_RAW_ADDR(0)','REX_LOAD_VECTOR(ctx.v0,0)','ppc_longjmp(0,0)','REX_UNIMPLEMENTED(0,"trap")','xstart(ctx.v0,base)']
 for i,expression in enumerate(negatives):
  f=w/f'negative-{i}.cpp';f.write_text('#include "fixture_pch.h"\nvoid deny(PPCContext& ctx,uint8_t* base){(void)ctx;(void)base;'+expression+';}\n')
  execute(['clang++','-std=c++23','-isystem',tc/'include','-iquote',ROOT,'-I',generated,'-c',f,'-o',w/'negative.o'],f'negative-{i}',1)
  if expression.split('(')[0] not in (w/f'negative-{i}.stderr').read_text():raise ValueError('unclassified unsupported-operation refusal')
 result['negative_compile_refusals']=len(negatives)
 if a.switch_build:
  tool=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());outputs=[]
  for i in range(2):
   out=w/f'switch-{i}';out.mkdir()
   def container(command,label):
    name='xex-'+uuid.uuid4().hex[:12];cmd=['docker','run','--rm','--pull','never','--platform',tool['platform'],'--name',name,'--network','none','--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m']
    for src,dst in [(ROOT,'/src'),(tc,'/toolchain'),(w,'/data'),(generated,'/generated')]:cmd+=['--mount',f'type=bind,src={src},dst={dst},readonly']
    cmd+=['--mount',f'type=bind,src={out},dst=/build']
    if a.icon:cmd+=['--mount',f'type=bind,src={a.icon.resolve()},dst=/icon.jpg,readonly']
    cmd+=['-w','/build',tool['image'],*command]
    try:execute(cmd,f'{label}-{i}')
    finally:
     clean=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
     if clean.returncode and 'No such container' not in clean.stderr:raise RuntimeError('container cleanup failed')
   container(['make','-f','/src/tests/horizon-expansion/Makefile','REPO=/src','STAGE_MAX='+str(a.stage),*(['ICON=/icon.jpg'] if a.icon else [])],'link')
   container(['python3','/src/tests/horizon-expansion/inspect_nro.py'],'inspect')
   outputs.append({n:sha(out/n) for n in ['probe.nro','probe.elf','leaf.o','mapping.o','import.o']})
  if outputs[0]!=outputs[1]:raise ValueError('non-deterministic clean build')
  result['switch']={'sha256':outputs[0],'deterministic_builds':2,'inspection':json.loads((w/'switch-0/inspection.json').read_text())}
 if fingerprints!={p.name:sha(p) for p in emitted}:raise ValueError('generated output modified')
 result['records']=records;(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('records','sources_sha256','generation_sha256')},indent=2))
if __name__=='__main__':main()
