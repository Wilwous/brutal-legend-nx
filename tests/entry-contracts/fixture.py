#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original startup analogue. No private bytes, addresses or operand tables."""
import importlib.util,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import bounded_cfg as cfg
spec=importlib.util.spec_from_file_location('original_container',ROOT/'tests/xex-binding/fixture.py');container=importlib.util.module_from_spec(spec);spec.loader.exec_module(container)
d=container.caller.d;spr=container.caller.spr;x=container.caller.x

def create(path,base=0x83000000):
 p=Path(path);container.create(p,base);origin=base+0x10000;code=[];named={};extents={};stores=[]
 def emit(w):code.append(w)
 def start(n):named[n]=origin+len(code)*4
 def end():pc=list(named.values())[-1];extents[pc]=origin+len(code)*4-pc
 def store(w):stores.append(origin+len(code)*4);emit(w)
 start('save')
 for r,off in zip(range(20,24),[-56,-48,-40,-32]):store(d(62,r,1,off))
 store(d(36,7,1,-16));emit(0x4e800020);end()
 start('startup');emit(spr(7,8));at=origin+len(code)*4;emit(0x48000001|((named['save']-at)&0x3fffffc))
 store(d(37,1,1,-320));emit(x(444,17,20,17));emit(d(15,6,0,(base+0x30000)>>16));emit(d(14,6,6,0x6000));emit(d(14,5,0,55));store(d(36,5,6,-0x5be0));emit(d(14,5,5,9));store(d(36,5,6,-0x5bdc));emit(x(444,17,21,17));emit(x(444,20,22,20));emit(x(444,21,23,21))
 boundary=origin+len(code)*4;emit(0);emit(0x4e800020);end()
 start('unadmitted');emit(0x4e800020);end()
 code[(boundary-origin)//4]=0x48000001|((named['unadmitted']-boundary)&0x3fffffc)
 start('probe32');emit(d(36,5,6,0));emit(0x4e800020);end()
 start('probe64');emit(d(62,23,6,0));emit(0x4e800020);end()
 raw=b''.join(struct.pack('>I',w) for w in code);image=bytearray((p/'fixture.xex').read_bytes());image[0x11000:0x21000]=bytes(0x10000);image[0x11000:0x11000+len(raw)]=raw
 def be(off,v):struct.pack_into('>I',image,off,v)
 def le(off,v):struct.pack_into('<I',image,0x1000+off,v)
 be(44,named['startup']);le(0x98+16,named['startup']-base);le(0x98+56,0x60000) # PE declaration exceeds actual 0x40000 loaded ownership.
 le(0x98+100+3*8,len(extents)*8)
 for i,(pc,size) in enumerate(extents.items()):be(0x21100+i*8,pc);be(0x21104+i*8,0x40000000|((size//4)<<8))
 (p/'fixture.xex').write_bytes(image)
 ranges=[dict(start=pc,end=boundary+4 if n=='startup' else pc+extents[pc],owner_start=pc,owner_end=pc+extents[pc]) for n,pc in named.items() if n!='unadmitted']
 hooks=sorted(pc for r in ranges for pc in range(r['start'],r['end'],4));config=(p/'fixture_manifest.toml').read_text()
 for pc in hooks:config+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="brutal_cfg_{pc:X}"\n'
 (p/'fixture_manifest.toml').write_text(config)
 bindings=dict(xex_sha256=cfg.sha(image),image_code_sha256=cfg.sha(raw),cli_revision=cfg.REVISION,patch_lock_sha256=cfg.sha((ROOT/'tools/sdk/xex-binding-lock.json').read_bytes()),codegen_config_sha256=cfg.sha(config.encode()))
 m=cfg.build(raw,origin,ranges,[named[n] for n in ['startup','probe32','probe64']],[],{},{boundary:'exit'},bindings)
 assert len(stores)==8 and sum(r['end']-r['start'] for r in ranges)//4==24 and len(m['policy']['edges'])==21
 metadata=dict(base=base,entry=named['startup'],code_base=origin,code=list(raw),functions=list(extents),extents=extents,named=named,boundary=boundary,stores=stores,ranges=ranges,proof=cfg.stop_proof(m))
 (p/'region.json').write_bytes(cfg.canonical(m));(p/'cfg.json').write_text(json.dumps(metadata,indent=2)+'\n');return metadata,m
