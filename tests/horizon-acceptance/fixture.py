# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original appended acceptance corpus; preserves every earlier instruction PC."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('runtime_ii_original',ROOT/'tests/horizon-runtime-ii/fixture.py');prior=importlib.util.module_from_spec(s);s.loader.exec_module(prior)
x=prior.x;d=prior.d;spr=prior.spr
def create(path,templates,query=True):
 m=prior.create(path,templates,query=True);words=m['words'];functions=m['functions'];sizes=m['sizes'];code=m['code_base']
 def function(name,body):functions[name]=code+4*len(words);sizes[name]=len(body);words.extend(body)
 # cntlzw r6,r3; rlwinm r3,r4,8,8,23; blr (non-record forms).
 function('word_logic',[x(3,6,0,26),21<<26|4<<21|3<<16|8<<11|8<<6|23<<1,0x4e800020])
 function('vector_blocks',[x(0,0,3,103),x(1,0,4,103),4<<26|2<<21|0<<16|1<<11|128,4<<26|3<<21|0<<16|1<<11|1220,x(2,0,5,231),d(14,6,5,16),x(3,0,6,231),0x4e800020])
 pc=code+4*len(words);body=[d(37,1,1,-64),spr(0,8),d(36,0,1,68),d(62,14,1,16)]
 def bl(target):return 0x48000001|((target-(pc+4*len(body)))&0x3fffffc)
 body+=[d(14,3,0,4)];body+=[bl(functions['stage8_entry'])];body+=[x(3,14,3,444)];body+=[bl(functions['float_mul'])];body+=[d(14,3,0,0),d(15,4,0,0x0102),d(24,4,4,0x0304)];body+=[bl(functions['word_logic'])];body+=[x(14,3,14,266)]
 for reg,offset in [(3,0xc00),(4,0xc10),(5,0xc20)]:body+=[d(15,reg,0,0x8203),d(24,reg,reg,offset)]
 body+=[bl(functions['vector_blocks']),x(14,3,14,444),d(58,14,1,16),d(32,0,1,68),spr(0,8,True),d(14,1,1,64),0x4e800020]
 function('acceptance_multicall',body)
 image=bytearray((path/'fixture.xex').read_bytes());be=lambda at,value:struct.pack_into('>I',image,at,value)
 codebytes=b''.join(struct.pack('>I',v) for v in words);image[0x11000:0x11000+len(codebytes)]=codebytes
 struct.pack_into('<I',image,0x1000+0x98+100+24,len(functions)*8)
 for i,(name,addr) in enumerate(functions.items()):be(0x21100+i*8,addr);be(0x21104+i*8,(sizes[name]<<8)|0x40000000)
 for i,value in enumerate([0xffffffff,0x10203040,0x80000000,0x01020304,1,0x11223344,0x80000000,0x04030201]):be(0x31c00+i*4,value)
 (path/'fixture.xex').write_bytes(image)
 sites=[];memory=[];back=[];returns=[]
 for i,w in enumerate(words):
  pc=code+4*i;owner=max(v for v in functions.values() if v<=pc);op=w>>26;xo=(w>>1)&1023;kind=None;width=0;target=0
  if op in (32,36,37,58,62) or op==31 and xo in (103,231):
   kind='read' if op in (32,58) or op==31 and xo==103 else 'write';width=16 if op==31 else 8 if op in (58,62) else 4;memory.append(pc)
  elif w==0x4e800020:kind='ret';returns.append(pc)
  elif w==0x4e800421:kind='indirect';target=functions['combine']
  elif op in (16,18):
   mask,bits=(0xfffc,16) if op==16 else (0x3fffffc,26);delta=w&mask
   if delta&(1<<(bits-1)):delta-=1<<bits
   target=pc+delta;kind='direct' if op==18 and w&1 else 'branch'
   if kind=='branch' and target<=pc:back.append(pc)
  if kind:sites.append(dict(pc=pc,owner=owner,kind=kind,width=width,target=target))
 m.update(words=words,code=list(codebytes),instructions=len(words),sites=sites,hooks=[s['pc'] for s in sites],memory_sites=memory,backedges=back,return_sites=returns)
 manifest=(path/'fixture_manifest.toml').read_text().split('\n[[entrypoint.midasm_hook]]')[0]
 for pc in m['hooks']:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="horizon_point_{pc:X}"\n'
 (path/'fixture_manifest.toml').write_text(manifest);(path/'xex-layout.json').write_text(json.dumps(m,indent=2)+'\n');return m
