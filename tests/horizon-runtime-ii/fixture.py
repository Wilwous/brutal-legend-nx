# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original Runtime II PPC, appended to the preserved Stage 8 original corpus."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('stage8_original',ROOT/'tests/horizon-expansion/fixture.py');old=importlib.util.module_from_spec(s);s.loader.exec_module(old)
d=old.d;spr=old.spr

def x(rt,ra,rb,xo):return 31<<26|rt<<21|ra<<16|rb<<11|xo<<1

def create(path,templates,query=False):
 m=old.create(path,templates,initialize=True);words=m['words'];functions=m['functions'];sizes=m['sizes'];code=m['code_base']
 def function(name,body):functions[name]=code+4*len(words);sizes[name]=len(body);words.extend(body)
 function('integer_flags',[x(6,3,4,10),x(3,6,5,138),x(4,4,0,986),d(11,0,4,0),0x4e800020])
 function('ten_u32',[*[x(3,3,r,266) for r in range(4,11)],d(32,11,1,84),d(32,12,1,92),x(3,3,11,266),x(3,3,12,266),0x4e800020])
 function('record_pair',[d(32,4,3,0),d(32,5,3,4),x(3,4,5,266),0x4e800020])
 function('float_mul',[63<<26|1<<21|1<<16|2<<6|25<<1,0x4e800020])
 previous=functions.pop('xex_entry');functions['stage8_entry']=previous;sizes['stage8_entry']=sizes.pop('xex_entry');functions=dict(sorted(functions.items(),key=lambda item:item[1]));m['functions']=functions
 if query:
  pc=code+4*len(words);body=[d(37,1,1,-32),spr(0,8),d(36,0,1,36),0x48000001|((0x82030014-(pc+12))&0x3fffffc),d(32,0,1,36),spr(0,8,True),d(14,1,1,32),0x4e800020];function('query_only',body)
 root=code+4*len(words);body=[d(37,1,1,-64),spr(0,8),d(36,0,1,68),d(62,14,1,16)]
 def bl(target):return 0x48000001|((target-(root+4*len(body)))&0x3fffffc)
 body+=[bl(previous),x(3,14,3,444),d(14,3,0,1),d(14,4,0,2),d(14,5,0,3)]
 body+=[bl(functions['integer_flags']),x(3,14,3,266)]
 if query:
  body+=[x(3,14,3,444),d(15,3,0,0x8203),d(24,3,3,0),d(15,4,0,0x8203),d(24,4,4,0xa00),d(14,5,0,0)]
  body+=[bl(0x82030014),d(11,0,3,0),0x4082000c,x(14,3,14,444),0x48000008,d(14,3,0,-1)]
 body +=[d(58,14,1,16),d(32,0,1,68),spr(0,8,True),d(14,1,1,64),0x4e800020]
 function('xex_entry',body);m['entry']=functions['xex_entry']
 image=bytearray((path/'fixture.xex').read_bytes())
 def be(at,value):struct.pack_into('>I',image,at,value)
 be(44,m['entry']);struct.pack_into('<I',image,0x1000+0x98+16,m['entry']-m['base']);struct.pack_into('<I',image,0x1000+0x98+100+24,len(functions)*8)
 codebytes=b''.join(struct.pack('>I',w) for w in words);image[0x11000:0x11000+len(codebytes)]=codebytes
 for i,(name,pc) in enumerate(functions.items()):be(0x21100+i*8,pc);be(0x21104+i*8,(sizes[name]<<8)|0x40000000)
 if query:
  be(0x400,0x5c);be(0x41c,64);struct.pack_into('>H',image,0x442,6);be(0x454,0x82030010);be(0x458,0x82030014);be(0x31010,0xee);be(0x31014,0x10000ee)
 (path/'fixture.xex').write_bytes(image)
 sites=[];memory=[];backedges=[];returns=[]
 for i,w in enumerate(words):
  pc=code+4*i;owner=max(v for v in functions.values() if v<=pc);op=w>>26;kind=None;width=0;target=0
  if op in (32,36,37,58,62):kind='read' if op in (32,58) else 'write';width=8 if op in (58,62) else 4;memory.append(pc)
  elif w==0x4e800020:kind='ret';returns.append(pc)
  elif w==0x4e800421:kind='indirect';target=functions['combine']
  elif op in (16,18):
   bits,mask=(16,0xfffc) if op==16 else (26,0x3fffffc);delta=w&mask
   if delta&(1<<(bits-1)):delta-=1<<bits
   target=pc+delta;kind='direct' if op==18 and w&1 else 'branch'
   if kind=='branch' and target<=pc:backedges.append(pc)
  if kind:sites.append({'pc':pc,'owner':owner,'kind':kind,'width':width,'target':target})
 m.update(words=words,code=list(codebytes),instructions=len(words),sites=sites,hooks=[s['pc'] for s in sites],memory_sites=memory,backedges=backedges,return_sites=returns,query=query)
 manifest=(path/'fixture_manifest.toml').read_text().split('\n[[entrypoint.midasm_hook]]')[0]
 for pc in m['hooks']:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="horizon_point_{pc:X}"\n'
 (path/'fixture_manifest.toml').write_text(manifest);(path/'xex-layout.json').write_text(json.dumps(m,indent=2)+'\n');return m
