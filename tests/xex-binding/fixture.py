#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original unencrypted XEX builder; PPC originates in the public Phase 15 encoder."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('original_caller',ROOT/'tests/generated-dispatch/fixture.py');caller=importlib.util.module_from_spec(spec);spec.loader.exec_module(caller)
IMPORTS=[('RtlCompareMemoryUlong',0x11b,'u32_in_words_u32_u32'),('NtQueryVirtualMemory',0xee,'u32_address_out28_u32'),('RtlInitializeCriticalSection',0x12e,'void_inout28')]
def create(p,base=0x82000000):
 p=Path(p);p.mkdir(exist_ok=False,parents=True);original=caller.create(p/'original')
 code=bytearray((p/'original/caller.bin').read_bytes()[:original['functions']['compare']]);entry=base+0x10000
 targets={'compare':base+0x30004,'critical':base+0x30014}
 for i in range(0,len(code),4):
  w=struct.unpack_from('>I',code,i)[0]
  if w>>26==18:
   off=(w&0x3fffffc);off=off-(1<<26) if off&(1<<25) else off;name=next((n for n in targets if i+off==original['functions'][n]),None)
   if name:struct.pack_into('>I',code,i,0x48000001|((targets[name]-entry-i)&0x3fffffc))
 wrapper_offset=len(code)
 ops=[caller.d(37,1,1,-64),caller.spr(0,8),caller.d(36,0,1,68),
      caller.d(15,4,0,(base+0x20000)>>16),caller.d(24,4,4,0x200),caller.d(32,5,4),
      caller.d(15,4,0,(base+0x30000)>>16),caller.d(24,4,4,0x400),caller.d(36,5,4),
      0x48000001|((-wrapper_offset-9*4)&0x3fffffc),caller.d(32,0,1,68),caller.spr(0,8,True),caller.d(14,1,1,64),0x4e800020]
 code+=b''.join(struct.pack('>I',w) for w in ops)
 actual_entry=entry+wrapper_offset
 b=bytearray(0x41000)
 def be(p,v):struct.pack_into('>I',b,p,v)
 def le(p,v,n=4):b[0x1000+p:0x1000+p+n]=v.to_bytes(n,'little')
 be(0,0x58455832);be(8,0x1000);be(16,0x100);be(20,5)
 for i,(key,value) in enumerate([(0x3ff,0x300),(0x10201,base),(0x10100,actual_entry),(0x20104,0x380),(0x103ff,0x400)]):be(24+i*8,key);be(28+i*8,value)
 be(0x100,0x1cc);be(0x104,0x40000);be(0x210,base);be(0x280,4)
 for i,kind in enumerate([3,1,3,2]):be(0x284+i*24,0x10+kind)
 be(0x300,8);be(0x380,64);be(0x384,base+0x20000);be(0x388,32);be(0x38c,16)
 be(0x404,16);be(0x408,1);b[0x40c:0x419]=b'xboxkrnl.exe\0';pos=0x41c;be(pos,40+3*8);b[pos+38:pos+40]=(6).to_bytes(2,'big')
 for i,(name,ordinal,signature) in enumerate(IMPORTS):
  be(pos+40+i*8,base+0x30000+i*8);be(pos+44+i*8,base+0x30004+i*8)
  be(0x31000+i*8,ordinal);be(0x31004+i*8,0x1000000|ordinal)
 be(0x400,pos+64-0x400)
 le(0,0x905a4d);le(0x3c,0x80);le(0x80,0x4550);le(0x84,0x1f2,2);le(0x86,4,2);le(0x94,224,2);le(0x96,0x100,2)
 opt=0x98;le(opt,0x10b,2);le(opt+16,0x10000+wrapper_offset);le(opt+28,base);le(opt+32,0x10000);le(opt+36,0x200);le(opt+56,0x40000);le(opt+60,0x1000);le(opt+68,14,2)
 for i,(name,flags) in enumerate([('.header',0x40000040),('.text',0x60000020),('.rdata',0x40000040),('.data',0xc0000040)]):
  s=opt+224+i*40;b[0x1000+s:0x1000+s+len(name)]=name.encode();le(s+8,0x10000);le(s+12,i*0x10000);le(s+16,0x10000);le(s+20,i*0x10000);le(s+36,flags)
 le(opt+92,16);le(opt+96+3*8,0x20100);le(opt+100+3*8,24)
 for i,(off,length) in enumerate([(0,46),(184,3),(wrapper_offset,14)]):be(0x21100+i*8,entry+off);be(0x21104+i*8,(length<<8)|0x40000000)
 b[0x11000:0x11000+len(code)]=code;b[0x21000:0x21010]=bytes(range(0x41,0x51));be(0x21200,0x10203040);(p/'fixture.xex').write_bytes(b)
 manifest='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\n'
 (p/'fixture_manifest.toml').write_text(manifest)
 metadata={'base':base,'entry':actual_entry,'caller':entry,'code_base':entry,'size':0x40000,'code':list(code),'echo':entry+original['functions']['echo'],'imports':[{'name':n,'ordinal':o,'signature':s,'thunk':base+0x30004+i*8} for i,(n,o,s) in enumerate(IMPORTS)]}
 (p/'fixture.json').write_text(json.dumps(metadata,indent=2)+'\n');return metadata
