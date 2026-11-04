#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Production codegen of original synthetic XEX -> checked module/variable startup."""
import argparse,hashlib,json,re,struct,sys,importlib.util
from pathlib import Path
from fixture import create,ROOT
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser();ap.add_argument('--workdir',required=True,type=Path);ap.add_argument('--toolchain',required=True,type=Path);a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('new external output required')
w.mkdir(parents=True,exist_ok=False);records=[];commands=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def execute(cmd,label):
 cmd=list(map(str,cmd));commands.append(cmd);r=run(cmd,w,label,30);records.append(r)
 if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; logs retained')
 return (w/(label+'.stdout')).read_text()
pin=json.loads((tc/'prepared.json').read_text());assert pin['sdk_revision']=='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' and sha(tc/'rexglue')==pin['cli_sha256']=='6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c'
meta=create(w/'input');manifest=w/'input/fixture_manifest.toml';execute([tc/'rexglue','codegen',manifest],'generation');g=w/'input/generated';files=sorted([*g.glob('*.h'),*g.glob('*.cpp')]);fingerprints={p.name:sha(p) for p in files};execute([tc/'rexglue','codegen',manifest,'--ignore-stamp'],'regeneration');assert fingerprints=={p.name:sha(p) for p in files}
cpp=(g/'fixture_recomp.0.cpp').read_text();spec=importlib.util.spec_from_file_location('oracle',ROOT/'tests/horizon-acceptance/run.py');oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
assert [s.strip() for s in re.findall(r'^\s*// (.*)$',cpp,re.M)]==[oracle.decode(word,meta['code_base']+i*4) for i,word in enumerate(meta['words'])]
assert cpp.count('DEFINE_REX_FUNC(')==5 and [int(x,16) for x in re.findall(r'^\tmodule_point_([0-9A-F]+)\(\);$',cpp,re.M)]==meta['hooks']
for forbidden in ['REX_RAW_ADDR(','REX_UNIMPLEMENTED(','REX_FATAL(','ppc_trap(','base +','RtlCompareMemoryUlong']:assert forbidden not in cpp
memory_sites=sum(cpp.count(k+'(') for k in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64']);assert memory_sites==sum(s['kind'] in ['read','write'] for s in meta['sites'])==13
source_id=sha(w/'input/fixture.xex');ppc_id=hashlib.sha256(b''.join(struct.pack('>I',x) for x in meta['words'])).hexdigest();generated_id=hashlib.sha256(json.dumps(fingerprints,sort_keys=True,separators=(',',':')).encode()).hexdigest();sdk_id=hashlib.sha256(pin['sdk_revision'].encode()).hexdigest();runtime_id=hashlib.sha256(b''.join((ROOT/n).read_bytes() for n in ['runtime/horizon/writable_image.h','runtime/horizon/sealed_object.h','runtime/horizon/variable_imports.h','runtime/horizon/native_image_binding.h','runtime/horizon/header_buffer.h','runtime/horizon/space.h','runtime/checked_memory.h'])).hexdigest()
bounds=[(pc,meta['sizes'][name]*4) for name,pc in meta['functions'].items()];roots=[meta['functions'][n] for n in ['entry','forever','indirect']];kind={'read':0,'write':1,'direct':2,'indirect':3,'ret':4,'branch':5};nums=[len(bounds),*[v for b in bounds for v in b],len(meta['sites'])]
for s in meta['sites']:nums += [s['pc'],s['owner'],kind[s['kind']],s['width'],s['target']]
nums += [len(roots),*roots,0x8201feed];contracts_id=hashlib.sha256(b''.join(struct.pack('>Q',n) for n in nums)).hexdigest()
module=0x4d4f44554c45;plan_nums=[1,1,module,1,0x82000000,0x82030000,meta['functions']['entry'],0x82020000,16,32,64,0];regions=[[1,0x82010000,65536,0x11000,65536,0,4096,5,1,1,1],[2,0x82020000,4096,0x21000,4096,0,4096,1,1,1,1]]
canonical=b''.join(struct.pack('>Q',n) for n in plan_nums)+b''.join(bytes.fromhex(x) for x in [source_id,generated_id,sdk_id,runtime_id])+struct.pack('>Q',2)+b''.join(struct.pack('>Q',n) for row in regions for n in row)+struct.pack('>Q',0);plan_id=hashlib.sha256(canonical).hexdigest()
def dig(s):return '{'+','.join(map(str,bytes.fromhex(s)))+'}'
text='#pragma once\n#include "runtime/horizon/native_image_binding.h"\nusing namespace brutal::horizon;\nextern const Function module_functions[];extern const size_t module_function_count;\n'
text+='inline const native_image::CodeIdentity program_identity{'+','.join(dig(s) for s in [source_id,ppc_id,generated_id,sdk_id,runtime_id,contracts_id,plan_id])+','+str(meta['functions']['entry'])+',0x82020000};\n'
text+=f'inline native_image::Plan program_plan(){{using namespace native_image;Plan p;p.provenance=Provenance::original_synthetic;p.module={module};p.lifetime=1;p.base=0x82000000;p.limit=0x82030000;p.entry={meta["functions"]["entry"]};p.tls_address=0x82020000;p.tls_file_size=16;p.tls_total_size=32;p.tls_slots=64;p.source=program_identity.source;p.code=program_identity.generated;p.sdk=program_identity.sdk;p.runtime=program_identity.runtime;'
text+='p.regions={{1,0x82010000,65536,0x11000,65536,0,4096,Rights::read_execute,Init::file_then_authored_zero,true,true},{2,0x82020000,4096,0x21000,4096,0,4096,Rights::read,Init::file_then_authored_zero,true,true}};return p;}\n'
text+='inline constexpr native_image::Boundary program_bounds[]{'+','.join('{'+str(pc)+','+str(size)+'}' for pc,size in bounds)+'};\n'
text+='inline constexpr Site program_sites[]{'+','.join('{'+f"{s['pc']},{s['owner']},SiteKind::{s['kind']},{s['width']},{s['target']}"+'}' for s in meta['sites'])+'};\ninline constexpr uint32_t program_roots[]{'+','.join(map(str,roots))+'};\ninline constexpr ControlPlan program_control{program_sites,program_roots,0x8201feed};\n'
for n in ['query','forever','indirect']:text+='inline constexpr uint32_t program_'+n+'='+str(meta['functions'][n])+';\n'
(w/'program.h').write_text(text);(w/'hooks.cpp').write_text('#include <cstdint>\nvoid module_poll(uint32_t);\n'+''.join(f'void module_point_{pc:X}(){{module_poll({pc});}}\n' for pc in meta['hooks']))
tests=[]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/name;execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',w,'-I',g,ROOT/'tests/writable-startup/host.cpp',g/'fixture_recomp.0.cpp',g/'fixture_init.cpp',w/'hooks.cpp','-o',binary],'compile-'+name)
 row=json.loads(execute([binary,w/'input/fixture.xex'],'run-'+name));assert row['result']==31 and row['completed_runs']==2 and row['native_queries']==4 and row['stops']==1 and row['allocations']==row['leases']==0;tests.append({'profile':name,**row})
assert fingerprints=={p.name:sha(p) for p in files}
(w/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n');(w/'results.json').write_text(json.dumps({'tests':tests,'records':records,'generation_sha256':fingerprints,'instructions':len(meta['words']),'hook_count':len(meta['hooks']),'function_count':5,'checked_memory_sites':memory_sites,'functions_imported':0,'native_query_kind':'explicit original synthetic before-blr hook, not an Xbox SDK export','source':source_id,'contracts':contracts_id,'plan':plan_id,'game_executed':False,'generated_unedited':True},indent=2)+'\n');print(json.dumps(tests))
