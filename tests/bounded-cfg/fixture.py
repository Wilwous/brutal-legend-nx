#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independently authored code; reuse only the original XEX container writer."""
import importlib.util,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
import bounded_cfg as cfg
spec=importlib.util.spec_from_file_location('container',ROOT/'tests/xex-binding/fixture.py');container=importlib.util.module_from_spec(spec);spec.loader.exec_module(container)
d=container.caller.d;spr=container.caller.spr;x=container.caller.x

def create(path,base=0x82000000):
 p=Path(path);container.create(p,base);code=[];named={};extents={};labels={};patches=[]
 def emit(word):code.append(word)
 def start(name):named[name]=len(code)*4
 def end():off=list(named.values())[-1];extents[base+0x10000+off]=len(code)*4-off
 def label(name):labels[name]=len(code)*4
 def branch(name,kind='b'):patches.append((len(code)*4,name,kind));emit(0)
 def prologue():emit(d(37,1,1,-80));emit(spr(0,8));emit(d(36,0,1,84))
 def epilogue():emit(d(32,0,1,84));emit(spr(0,8,True));emit(d(14,1,1,80));emit(0x4e800020)
 start('prefix');emit(spr(7,8));emit(d(37,1,1,-112));emit(d(36,7,1,116));branch('save','bl');emit(d(14,6,0,0x3a));emit(d(36,6,4,0));emit(d(14,6,6,9));emit(d(36,6,4,4));boundary=len(code)*4;branch('unadmitted','bl');emit(d(14,3,0,0x7777));emit(d(32,7,1,116));emit(spr(7,8,True));emit(d(14,1,1,112));emit(0x4e800020);end()
 start('save');emit(d(62,20,1,24));emit(d(62,21,1,32));emit(0x4e800020);end()
 start('unadmitted');emit(0x4e800020);end()
 start('flow');prologue();emit(d(62,14,1,16));emit(x(444,3,14,3));emit(d(11,0,14,0));branch('flow_end','beq');emit(d(14,3,0,0));label('loop');emit(d(14,3,3,3));emit(d(11,0,3,6));branch('join','blt');emit(d(14,3,3,1));label('join');branch('helper','bl');emit(d(14,14,14,-1));emit(d(11,0,14,0));branch('loop','bne');label('flow_end');emit(spr(5,9,True));indirect=len(code)*4;emit(0x4e800421);emit(d(58,14,1,16));epilogue();end()
 start('helper');emit(d(14,3,3,5));emit(0x4e800020);end()
 start('leaf');emit(d(14,3,3,2));emit(0x4e800020);end()
 start('imports');prologue();import_site=len(code)*4;patches.append((import_site,base+0x30014,'absolute-call'));emit(0);epilogue();end()
 start('spin');branch('spin');end()
 start('escape');escape=len(code)*4;branch('escaped');label('escaped');emit(d(14,3,0,0x1234));emit(0x4e800020);end()
 start('long');
 for i in range(24):emit(d(14,3,3,1))
 emit(0x4e800020);end()
 labels.update(named)
 for at,name,kind in patches:
  target=(name-(base+0x10000)) if kind=='absolute-call' else labels[name];delta=target-at
  code[at//4]=((0x48000001 if kind in ['bl','absolute-call'] else 0x48000000)|(delta&0x3fffffc)) if kind in ['b','bl','absolute-call'] else ({'beq':0x41820000,'bne':0x40820000,'blt':0x41800000}[kind]|(delta&0xfffc))
 raw=b''.join(struct.pack('>I',w) for w in code);origin=base+0x10000;named={k:origin+v for k,v in named.items()};boundary+=origin;indirect+=origin;import_site+=origin;escape+=origin
 image=bytearray((p/'fixture.xex').read_bytes());image[0x11000:0x21000]=bytes(0x10000);image[0x11000:0x11000+len(raw)]=raw
 def be(off,v):struct.pack_into('>I',image,off,v)
 def le(off,v):struct.pack_into('<I',image,0x1000+off,v)
 be(44,named['flow']);le(0x98+16,named['flow']-base);le(0x98+100+3*8,len(extents)*8)
 for i,(pc,size) in enumerate(extents.items()):be(0x21100+i*8,pc);be(0x21104+i*8,0x40000000|((size//4)<<8))
 (p/'fixture.xex').write_bytes(image)
 ranges=[]
 for name,pc in named.items():
  if name=='unadmitted':continue
  end=boundary+4 if name=='prefix' else pc+4 if name=='escape' else pc+extents[pc]
  ranges.append({'start':pc,'end':end,'owner_start':pc,'owner_end':pc+extents[pc]})
 roots=[named[n] for n in ['prefix','flow','imports','spin','escape','long']]
 hooks=sorted(pc for r in ranges for pc in range(r['start'],r['end'],4))
 manifest=(p/'fixture_manifest.toml').read_text()
 for pc in hooks:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="brutal_cfg_{pc:X}"\n'
 (p/'fixture_manifest.toml').write_text(manifest)
 bindings={'xex_sha256':cfg.sha(image),'image_code_sha256':cfg.sha(raw),'cli_revision':cfg.REVISION,'patch_lock_sha256':cfg.sha((ROOT/'tools/sdk/xex-binding-lock.json').read_bytes()),'codegen_config_sha256':cfg.sha(manifest.encode())}
 m=cfg.build(raw,origin,ranges,roots,[base+0x30014],{indirect:[named['leaf']]},{boundary:'exit',escape:'refuse'},bindings);cfg.validate(m,m);proof=cfg.stop_proof(m)
 (p/'region.json').write_bytes(cfg.canonical(m));metadata={'base':base,'entry':named['flow'],'code_base':origin,'code':list(raw),'functions':list(extents),'extents':extents,'named':named,'boundary':boundary,'indirect':indirect,'import_site':import_site,'escape':escape,'ranges':ranges,'proof':proof}
 (p/'cfg.json').write_text(json.dumps(metadata,indent=2)+'\n');return metadata,m
