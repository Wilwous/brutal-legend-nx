#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Production generator -> immutable native plan -> real SDK leaf -> checked PPC."""
import argparse,hashlib,json,re,sys,struct,subprocess
from pathlib import Path
from fixture import create,load,ROOT
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser()
for n in ['workdir','toolchain','sdk-source']:ap.add_argument('--'+n,required=True,type=Path)
a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve();sdk=a.sdk_source.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False);records=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def execute(cmd,label):
 v=run(list(map(str,cmd)),w,label,30);records.append(v)
 if v['timeout'] or v['exit_code']:raise RuntimeError(label+' failed; inspect diagnostics')
 return (w/(label+'.stdout')).read_text()
prepared=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==prepared['cli_sha256']
meta=create(w/'input');manifest=w/'input/fixture_manifest.toml';execute([tc/'rexglue','codegen',manifest],'generation');g=w/'input/generated';files=sorted([*g.glob('*.h'),*g.glob('*.cpp')]);fingerprints={p.name:sha(p) for p in files};execute([tc/'rexglue','codegen',manifest,'--ignore-stamp'],'regeneration');assert fingerprints=={p.name:sha(p) for p in files}
cpp=(g/'fixture_recomp.0.cpp').read_text();decoder=load('independent_decoder',ROOT/'tests/horizon-acceptance/run.py').decode
assert [s.strip() for s in re.findall(r'^\s*// (.*)$',cpp,re.M)]==[decoder(word,meta['code_base']+i*4) for i,word in enumerate(meta['words'])]
assert cpp.count('DEFINE_REX_FUNC(')==3
assert [int(v,16) for v in re.findall(r'^\tnative_point_([0-9A-F]+)\(\);$',cpp,re.M)]==meta['hooks']
for token in ['REX_RAW_ADDR(','REX_UNIMPLEMENTED(','REX_FATAL(','ppc_trap(','base +']:assert token not in cpp
memory_sites=sum(cpp.count(k+'(') for k in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64']);assert memory_sites==sum(s['kind'] in ['read','write'] for s in meta['sites'])
lock=json.loads((ROOT/'tests/horizon-xex/sdk-lock.json').read_text());assert sha(sdk/lock['file'])==lock['source_sha256'];text=(sdk/lock['file']).read_text();start=text.index('u32 RtlCompareMemoryUlong_entry(');end=text.index('{',start)+1;depth=1
while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
body=text[start:end]+'\n';assert hashlib.sha256(body.encode()).hexdigest()==lock['function_sha256'];(w/'sdk-compare.inc').write_text(body)
code_id=hashlib.sha256(json.dumps(fingerprints,sort_keys=True,separators=(',',':')).encode()).hexdigest();source_id=sha(w/'input/fixture.xex');ppc_id=hashlib.sha256(b''.join(v.to_bytes(4,'big') for v in meta['words'])).hexdigest();sdk_id=hashlib.sha256(json.dumps(lock,sort_keys=True).encode()).hexdigest();runtime_id=hashlib.sha256(b''.join((ROOT/n).read_bytes() for n in ['runtime/horizon/native_image_plan.h','runtime/horizon/native_image_binding.h','runtime/horizon/generated.h','runtime/horizon/guest.h','runtime/horizon/space.h','tests/native-image-binding/probe.cpp'])).hexdigest();contract_numbers=[4]
for name,pc in meta['functions'].items():contract_numbers += [pc,meta['sizes'][name]*4]
contract_numbers += [0x82030004,4,len(meta['sites'])]
kind_ids={'read':0,'write':1,'direct':2,'indirect':3,'ret':4,'branch':5}
for s in meta['sites']:contract_numbers += [s['pc'],s['owner'],kind_ids[s['kind']],s['width'],s['target']]
contract_numbers += [len(meta['functions']),*meta['functions'].values(),0x8201feed]
contracts_id=hashlib.sha256(b''.join(struct.pack('>Q',n) for n in contract_numbers)).hexdigest()
def dig(s):return '{'+','.join(map(str,bytes.fromhex(s)))+'}'
native_regions=[]
for i in range(4):
 size=65536 if i<3 else 131072;length=65536 if i<3 else 1024
 native_regions.append([i+1,0x82000000+i*65536,size,4096+i*65536,length,size-length,4096,[1,5,1,3][i],1,1,1])
plan_numbers=[1,1,0x4e324e33,1,0x82000000,0x82060000,0x82010000,0x82020000,16,32,64,0]
canonical=b''.join(struct.pack('>Q',n) for n in plan_numbers)+b''.join(bytes.fromhex(v) for v in [source_id,code_id,sdk_id,runtime_id])+struct.pack('>Q',4)+b''.join(struct.pack('>Q',n) for row in native_regions for n in row)+b''.join(struct.pack('>Q',n) for n in [1,0x11b,1])
plan_id=hashlib.sha256(canonical).hexdigest()
image=(w/'input/fixture.xex').read_bytes();assert not any(image[4096+0x30400:4096+0x50000]);data='#pragma once\n#include "runtime/horizon/native_image_binding.h"\nusing namespace brutal::horizon;using namespace brutal::horizon::native_image;\nextern const Function native_functions[];extern const size_t native_function_count;\n'
data+='inline constexpr uint8_t native_bytes[]{'+','.join(map(str,image))+'};\n'
data+='inline constexpr CodeIdentity native_identity{'+','.join(dig(v) for v in [source_id,ppc_id,code_id,sdk_id,runtime_id,contracts_id,plan_id])+',0x82010000,0x82020000};\n'
data+='inline Plan native_plan(){Plan p;p.provenance=Provenance::original_synthetic;p.module=0x4e324e33;p.lifetime=1;p.base=0x82000000;p.limit=0x82060000;p.entry=0x82010000;p.tls_address=0x82020000;p.tls_file_size=16;p.tls_total_size=32;p.tls_slots=64;p.source=native_identity.source;p.code=native_identity.generated;p.sdk=native_identity.sdk;p.runtime=native_identity.runtime;p.imports={{0x11b,1}};\n'
for i in range(4):
 address=0x82000000+i*65536;size=65536 if i<3 else 131072;length=65536 if i<3 else 1024;rights=['read','read_execute','read','read_write'][i];data+=f'p.regions.push_back(Region{{{i+1},{address},{size},{4096+i*65536},{length},{size-length},4096,Rights::{rights},Init::file_then_authored_zero,true,true}});\n'
data+='return p;}\n';data+='inline constexpr Site native_sites[]{'+','.join('{'+f"{s['pc']},{s['owner']},SiteKind::{s['kind']},{s['width']},{s['target']}"+'}' for s in meta['sites'])+'};\n'
data+='inline constexpr uint32_t native_roots[]{'+','.join(map(str,meta['functions'].values()))+'};\ninline constexpr ControlPlan native_control{native_sites,native_roots,0x8201feed};\n'
rows=[(pc,meta['sizes'][name]*4) for name,pc in meta['functions'].items()]+[(0x82030004,4)];data+='inline constexpr Boundary native_bounds[]{'+','.join('{'+str(pc)+','+str(size)+'}' for pc,size in rows)+'};\ninline constexpr uint32_t native_forever='+str(meta['functions']['forever'])+';\ninline constexpr uint32_t native_helper='+str(meta['functions']['helper'])+';\n'
actual_map=[int(x,16) for x in re.findall(r'\{ (0x[0-9A-Fa-f]+),', (g/'fixture_init.cpp').read_text())];assert actual_map==[pc for pc,size in rows]
(w/'native-data.h').write_text(data);(w/'hooks.cpp').write_text('#include <cstdint>\nvoid native_poll(uint32_t);\n'+''.join(f'void native_point_{pc:X}(){{native_poll({pc});}}\n' for pc in meta['hooks']))
tests=[];builds={}
compiler=subprocess.check_output(['clang++','--version'],text=True,timeout=10).splitlines()[0]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/name;execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',w,'-I',g,ROOT/'tests/native-image-binding/host.cpp',ROOT/'tests/native-image-binding/probe.cpp',g/'fixture_recomp.0.cpp',g/'fixture_init.cpp',w/'hooks.cpp','-o',binary],'compile-'+name);row=json.loads(execute([binary],'run-'+name));assert not row['failed'] and row['result']==32 and not row['remaining'] and not row['leases'];tests.append({'profile':name,'mode':'smoke',**row});builds[name]={'compiler':compiler,'flags':flags,'runtime_source_contract':runtime_id,'binary_sha256':sha(binary)}
 for option in ['--repeat','--concurrent']:
  row=json.loads(execute([binary,option],option[2:]+'-'+name));assert not row['failed'] and row['result']==32 and not row['remaining'] and not row['leases'];tests.append({'profile':name,'mode':option[2:],**row})
assert fingerprints=={p.name:sha(p) for p in files}
result={'runtime_builds':builds,'tests':tests,'records':records,'source':source_id,'ppc':ppc_id,'generated':code_id,'sdk':sdk_id,'runtime':runtime_id,'contracts':contracts_id,'plan':plan_id,'functions':3,'import_functions':1,'instructions':len(meta['words']),'memory_sites':memory_sites,'hooks':len(meta['hooks']),'game_executed':False};(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k!='records'}))
