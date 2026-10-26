# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original extended PPC functions; construction happens on the build host."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('prior_xex',ROOT/'tests/horizon-xex/fixture.py');prior=importlib.util.module_from_spec(spec);spec.loader.exec_module(prior)
d=prior.d;spr=prior.spr

def create(path,templates,initialize=False):
 m=prior.create(path,templates);words=m['words'];functions=m['functions'];sizes=m['sizes'];code=m['code_base']
 def function(name,body):functions[name]=code+len(words)*4;sizes[name]=len(body);words.extend(body)
 function('add64',[31<<26|3<<21|3<<16|4<<11|266<<1,0x4e800020])
 function('float_add',[63<<26|1<<21|1<<16|2<<11|21<<1,0x4e800020])
 # Two finite nested loops, two independently audited backward conditional edges.
 function('nested',[d(14,3,0,0),d(14,4,0,2),d(14,5,0,3),d(14,3,3,1),d(14,5,5,-1),d(11,0,5,0),0x4082fff4,d(14,4,4,-1),d(11,0,4,0),0x4082ffe4,0x4e800020])
 function('bad_return',[d(14,0,0,0),spr(0,8,True),0x4e800020])
 if initialize:
  legacy=functions.pop('xex_entry');functions['legacy_entry']=legacy;sizes['legacy_entry']=sizes.pop('xex_entry')
  # Preserve code order after renaming the old entry.
  functions=dict(sorted(functions.items(),key=lambda kv:kv[1]));m['functions']=functions
  start=code+len(words)*4;body=[d(37,1,1,-64),spr(0,8),d(36,0,1,68),d(62,14,1,16)]
  def bl(target):return 0x48000001|((target-(start+4*len(body)))&0x3fffffc)
  body+=[bl(legacy),31<<26|3<<21|14<<16|3<<11|444<<1,d(15,3,0,0x8203),d(24,3,3,0x800)]
  body+=[bl(0x8203000c),31<<26|14<<21|3<<16|14<<11|444<<1,d(14,3,3,1),d(58,14,1,16),d(32,0,1,68),spr(0,8,True),d(14,1,1,64),0x4e800020]
  function('xex_entry',body);m['entry']=functions['xex_entry']
 image=bytearray((path/'fixture.xex').read_bytes())
 def be(at,v):struct.pack_into('>I',image,at,v)
 if initialize:
  be(44,m['entry']);struct.pack_into('<I',image,0x1000+0x98+16,m['entry']-m['base'])
  be(0x400,0x54);be(0x41c,56);struct.pack_into('>H',image,0x442,4);be(0x44c,0x82030008);be(0x450,0x8203000c);be(0x31008,0x12e);be(0x3100c,0x100012e)
 codebytes=b''.join(struct.pack('>I',w) for w in words);image[0x11000:0x11000+len(codebytes)]=codebytes
 struct.pack_into('<I',image,0x1000+0x98+100+3*8,len(functions)*8)
 for i,(name,pc) in enumerate(functions.items()):be(0x21100+i*8,pc);be(0x21104+i*8,(sizes[name]<<8)|0x40000000)
 (path/'fixture.xex').write_bytes(image)
 sites=[];backedges=[];memory=[];returns=[]
 for i,w in enumerate(words):
  pc=code+4*i;owner=max(v for v in functions.values() if v<=pc);op=w>>26;kind=None;width=0;target=0
  if op in [32,36,37,58,62]:kind='read' if op in [32,58] else 'write';width=8 if op in [58,62] else 4;memory.append(pc)
  elif w==0x4e800020:kind='ret';returns.append(pc)
  elif w==0x4e800421:kind='indirect';target=functions['combine']
  elif op in [18,16]:
   mask,bits=(0x3fffffc,26) if op==18 else (0xfffc,16);v=w&mask;delta=v-(1<<bits) if v&(1<<(bits-1)) else v;target=pc+delta
   kind='direct' if op==18 and w&1 else 'branch'
   if kind=='branch' and target<=pc:backedges.append(pc)
  if kind:sites.append({'pc':pc,'owner':owner,'kind':kind,'width':width,'target':target})
 m.update(initialize=initialize,words=words,code=list(codebytes),instructions=len(words),hooks=[s['pc'] for s in sites],sites=sites,backedges=backedges,memory_sites=memory,return_sites=returns)
 manifest=(path/'fixture_manifest.toml').read_text().split('\n[[entrypoint.midasm_hook]]')[0]
 for pc in m['hooks']:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="horizon_point_{pc:X}"\n'
 (path/'fixture_manifest.toml').write_text(manifest);(path/'xex-layout.json').write_text(json.dumps(m,indent=2)+'\n');return m
