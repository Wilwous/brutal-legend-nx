#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original PPC loops and audited production midasm_hook configuration."""
import importlib.util,struct,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('xex_original',ROOT/'tests/xex-binding/fixture.py');original=importlib.util.module_from_spec(spec);spec.loader.exec_module(original)
d=original.caller.d

def create(path,base=0x82000000,instrument=True):
 p=Path(path);layout=original.create(p,base);code=bytearray(layout['code']);functions=[(0,46),(184,3),(196,14)];named={};labels={};pending=[]
 def emit(w):code.extend(struct.pack('>I',w));return len(code)-4
 def label(name):labels[name]=len(code)
 def branch(name,conditional=False):pending.append((emit(0),name,conditional))
 def start(name):named[name]=len(code)
 def end():functions.append((list(named.values())[-1],(len(code)-list(named.values())[-1])//4))
 def cmp(r):emit(d(11,0,r,0))
 def counter():emit(original.caller.x(444,3,11,3));emit(d(14,3,0,0));cmp(11)
 start('finite');counter();branch('finite_end',True)
 label('finite_body');emit(d(14,3,3,1));emit(d(14,11,11,-1));cmp(11);pending.append((emit(0),'finite_body','ne'));label('finite_end');emit(0x4e800020);end()
 start('infinite');label('forever');branch('forever');end()
 start('nested');counter();branch('nested_end',True)
 label('outer');emit(d(14,10,0,3));label('inner');emit(d(14,3,3,1));emit(d(14,10,10,-1));cmp(10);pending.append((emit(0),'inner','ne'));emit(d(14,11,11,-1));cmp(11);pending.append((emit(0),'outer','ne'));label('nested_end');emit(0x4e800020);end()
 start('memory');counter();branch('memory_end',True)
 label('memory_body');emit(d(32,3,4,0));emit(d(14,3,3,1));emit(d(36,3,4,0));emit(d(14,11,11,-1));cmp(11);pending.append((emit(0),'memory_body','ne'));label('memory_end');emit(0x4e800020);end()
 start('imports');emit(d(37,1,1,-64));emit(original.caller.spr(0,8));emit(d(36,0,1,68));emit(d(62,14,1,16));emit(d(62,15,1,24));emit(original.caller.x(444,3,14,3));emit(d(32,15,14,0xb0));cmp(15);branch('imports_end',True)
 label('imports_body');emit(original.caller.x(444,14,3,14));at=emit(0);struct.pack_into('>I',code,at,0x48000001|((-at)&0x3fffffc));emit(d(14,15,15,-1));cmp(15);pending.append((emit(0),'imports_body','ne'))
 label('imports_end');emit(d(58,15,1,24));emit(d(58,14,1,16));emit(d(32,0,1,68));emit(original.caller.spr(0,8,True));emit(d(14,1,1,64));emit(0x4e800020);end()
 start('long');
 for _ in range(64):emit(d(14,3,3,1))
 emit(0x4e800020);end()
 for at,name,kind in pending:
  delta=labels[name]-at;w=(0x48000000|(delta&0x3fffffc)) if not kind else ((0x40820000 if kind=='ne' else 0x41820000)|(delta&0xfffc));struct.pack_into('>I',code,at,w)
 # Decode independently of hook choices. Reject indirect non-call transfers.
 start_pc=base+0x10000;sites={};backedges=[];calls=[];memory=[]
 def site(off,why):sites.setdefault(start_pc+off,[]).append(why)
 for off,length in functions:
  site(off,'entry')
  for at in range(off,off+length*4,4):
   w=struct.unpack_from('>I',code,at)[0];op=w>>26
   if op in [18,16]:
    bits=26 if op==18 else 16;mask=0x3fffffc if op==18 else 0xfffc;delta=w&mask;delta-=1<<bits if delta&(1<<(bits-1)) else 0
    target=delta if w&2 else start_pc+at+delta
    if w&1:calls.append(start_pc+at);site(at,'pre-import-or-call');site(at+4,'post-import-or-call')
    elif target<=start_pc+at:
     if not start_pc+off<=target<start_pc+off+length*4:raise ValueError('cross-function backedge')
     backedges.append(start_pc+at);site(at,'backedge')
   elif op==19 and w!=0x4e800020:
    if w!=0x4e800421:raise ValueError('unsupported indirect transfer')
    calls.append(start_pc+at);site(at,'pre-import-or-call');site(at+4,'post-import-or-call')
   if op in [32,36,37,58,62] or op==31 and ((w>>1)&1023) in [103,231]:memory.append(start_pc+at);site(at,'pre-memory')
   if (at-off)%64==0:site(at,'long-block')
 if len(backedges)!=6:raise ValueError('loop census drift')
 image=bytearray((p/'fixture.xex').read_bytes());image[0x11000:0x11000+len(code)]=code
 def be(off,v):struct.pack_into('>I',image,off,v)
 def le(off,v):struct.pack_into('<I',image,0x1000+off,v)
 # Entry is now the original finite-loop function; all extents still use PDATA.
 entry=start_pc+named['finite'];be(44,entry);le(0x98+16,entry-base);le(0x98+100+3*8,len(functions)*8)
 for i,(off,length) in enumerate(functions):be(0x21100+i*8,start_pc+off);be(0x21104+i*8,(length<<8)|0x40000000)
 (p/'fixture.xex').write_bytes(image)
 manifest=(p/'fixture_manifest.toml').read_text()
 for pc in sorted(sites) if instrument else []:
  # No register args or early C++ return: callback throws only at a whole
  # instruction boundary; outer typed catch performs controlled cancellation.
  manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="brutal_point_{pc:X}"\n'
 (p/'fixture_manifest.toml').write_text(manifest)
 meta={'base':base,'entry':entry,'code_base':start_pc,'code':list(code),'functions':[start_pc+off for off,n in functions],'extents':{str(start_pc+off):length for off,length in functions},'named':{n:start_pc+off for n,off in named.items()},'sites':{str(pc):why for pc,why in sorted(sites.items())},'backedges':backedges,'calls':calls,'memory_sites':memory,'instructions':len(code)//4,'instrumented':instrument}
 (p/'loop.json').write_text(json.dumps(meta,indent=2)+'\n');return meta
