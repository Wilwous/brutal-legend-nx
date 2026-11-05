# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Production generation with independent instruction/census and plan identities."""
import hashlib
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path
from fixture import ROOT,create
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def sources():
    names=[p for p in (ROOT/'runtime').rglob('*.h')]
    names += [p for p in (ROOT/'tests/platform-v2').rglob('*') if p.is_file() and p.suffix in {'.cpp','.h','.py','.inja'} and '__pycache__' not in str(p)]
    names += [p for p in (ROOT/'tests/platform-services').rglob('*') if p.is_file() and '__pycache__' not in str(p)]
    names += [ROOT/'tests/switch-horizon/toolchain.json',ROOT/'tools/sdk/xex-binding-lock.json',ROOT/'tests/platform-v2/graphics-lock.json']
    return {p.relative_to(ROOT).as_posix():sha(p) for p in sorted(names)}

def prepare(w,tc):
    records=[]
    def execute(cmd,label):
        r=run(list(map(str,cmd)),w,label,30);records.append(r)
        if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; inspect private diagnostics')
    receipt=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==receipt['cli_sha256']
    meta=create(w/'input');manifest=w/'input/fixture_manifest.toml';execute([tc/'rexglue','codegen',manifest],'codegen')
    generated=w/'input/generated';files=sorted([*generated.glob('*.h'),*generated.glob('*.cpp')]);fingerprints={p.name:sha(p) for p in files}
    execute([tc/'rexglue','codegen',manifest,'--ignore-stamp'],'codegen-repeat');assert fingerprints=={p.name:sha(p) for p in files}
    spec=importlib.util.spec_from_file_location('independent_decoder',ROOT/'tests/horizon-acceptance/run.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
    def instruction(word,pc):
        # clrlwi is rlwinm with SH=0, ME=31 and Rc=0; verify fields independently.
        if word>>26==21 and (word>>11)&31==0 and (word>>1)&31==31 and word&1==0:
            return f'clrlwi r{(word>>16)&31},r{(word>>21)&31},{(word>>6)&31}'
        return decoder.decode(word,pc)
    cpp=(generated/'fixture_recomp.0.cpp').read_text();assert [x.strip() for x in re.findall(r'^\s*// (.*)$',cpp,re.M)]==[instruction(word,meta['code_base']+4*i) for i,word in enumerate(meta['words'])]
    assert cpp.count('DEFINE_REX_FUNC(')==len(meta['functions'])==3
    assert [int(v,16) for v in re.findall(r'^\tplatform_point_([0-9A-F]+)\(\);$',cpp,re.M)]==meta['hooks']
    for token in ['REX_RAW_ADDR(','REX_UNIMPLEMENTED(','REX_FATAL(','ppc_trap(','base +']:assert token not in cpp
    memory_sites=sum(cpp.count(k+'(') for k in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64']);assert memory_sites==sum(s['kind'] in ['read','write'] for s in meta['sites'])
    image=(w/'input/fixture.xex').read_bytes();source=hashlib.sha256(image).hexdigest();ppc=hashlib.sha256(b''.join(struct.pack('>I',n) for n in meta['words'])).hexdigest();code=hashlib.sha256(json.dumps(fingerprints,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    sdk=sha(ROOT/'tools/sdk/xex-binding-lock.json');runtime=hashlib.sha256(json.dumps(sources(),sort_keys=True,separators=(',',':')).encode()).hexdigest()
    roots=[meta['functions']['initialize'],meta['functions']['update']];terminal=0x8201feed
    bounds=[(pc,meta['sizes'][name]*4) for name,pc in meta['functions'].items()]
    nums=[len(bounds),*sum(([pc,size] for pc,size in bounds),[]),len(meta['sites'])];kinds={'read':0,'write':1,'direct':2,'indirect':3,'ret':4,'branch':5}
    for s in meta['sites']:nums += [s['pc'],s['owner'],kinds[s['kind']],s['width'],s['target']]
    nums += [len(roots),*roots,terminal]
    enc=lambda ns:b''.join(struct.pack('>Q',n) for n in ns)
    contracts=hashlib.sha256(enc(nums)).hexdigest()
    regions=[]
    for i in range(4):
        size=65536 if i<3 else 131072;length=65536 if i<3 else 1024
        regions.append([i+1,0x82000000+i*65536,size,4096+i*65536,length,size-length,4096,[1,5,1,3][i],1,1,1])
    assert not any(image[4096+0x30400:4096+0x50000])
    values=[1,1,0x504c4154,1,0x82000000,0x82060000,roots[0],0x82020000,16,32,64,0]
    plan=hashlib.sha256(enc(values)+b''.join(bytes.fromhex(v) for v in [source,code,sdk,runtime])+enc([len(regions)])+enc(sum(regions,[]))+enc([0])).hexdigest()
    dig=lambda s:'{'+','.join(map(str,bytes.fromhex(s)))+'}'
    text='#pragma once\n#include "runtime/horizon/native_image_binding.h"\nusing namespace brutal::horizon;namespace platform_image=brutal::horizon::native_image;\nextern const Function demo_functions[];extern const size_t demo_function_count;\n'
    text+='inline constexpr uint8_t platform_bytes[]{'+','.join(map(str,image))+'};\n'
    text+='inline constexpr platform_image::CodeIdentity platform_identity{'+','.join(dig(v) for v in [source,ppc,code,sdk,runtime,contracts,plan])+f',0x{roots[0]:x},0x82020000'+'};\n'
    text+='inline platform_image::Plan platform_plan(){using namespace platform_image;Plan p;p.provenance=Provenance::original_synthetic;p.module=0x504c4154;p.lifetime=1;p.base=0x82000000;p.limit=0x82060000;p.entry=platform_identity.entry;p.tls_address=0x82020000;p.tls_file_size=16;p.tls_total_size=32;p.tls_slots=64;p.source=platform_identity.source;p.code=platform_identity.generated;p.sdk=platform_identity.sdk;p.runtime=platform_identity.runtime;\n'
    for row in regions:
        owner,address,size,offset,length,zero,alignment,rights,*_=row
        text+=f'p.regions.push_back(Region{{{owner},{address},{size},{offset},{length},{zero},{alignment},Rights::{ {1:"read",3:"read_write",5:"read_execute"}[rights]},Init::file_then_authored_zero,true,true}});\n'
    text+='return p;}\ninline constexpr Site platform_sites[]{'+','.join('{'+f'{s["pc"]},{s["owner"]},SiteKind::{s["kind"]},{s["width"]},{s["target"]}'+'}' for s in meta['sites'])+'};\n'
    text+='inline constexpr uint32_t platform_roots[]{'+','.join(map(str,roots))+'};\ninline constexpr ControlPlan platform_control{platform_sites,platform_roots,'+str(terminal)+'};\n'
    text+='inline constexpr platform_image::Boundary platform_bounds[]{'+','.join('{'+str(pc)+','+str(size)+'}' for pc,size in bounds)+'};\n'
    for name,pc in meta['functions'].items():text+=f'inline constexpr uint32_t platform_{name}={pc};\n'
    text+=f'inline constexpr uint32_t platform_command_bytes={meta["command_bytes"]};\n'
    (w/'program.h').write_text(text)
    (w/'hooks.cpp').write_text('#include <cstdint>\nvoid platform_poll(uint32_t);\n'+''.join(f'void platform_point_{pc:X}(){{platform_poll({pc});}}\n' for pc in meta['hooks']))
    return dict(records=records,source=source,ppc=ppc,generated=code,runtime=runtime,sdk=sdk,contracts=contracts,plan=plan,functions=3,instructions=len(meta['words']),hooks=len(meta['hooks']),memory_sites=memory_sites,generated_files=fingerprints,source_files=sources(),fixture=meta)
