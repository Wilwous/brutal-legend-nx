# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Author-created XEX with explicit descriptor-covered zero tail for codegen."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def load(n,p):
 s=importlib.util.spec_from_file_location(n,p);m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
old=load('own_original',ROOT/'tests/xex-binding/fixture.py');d=old.caller.d;spr=old.caller.spr
BASE=0x82000000;CODE=BASE+0x10000

def create(p):
 old.create(p);b=bytearray((p/'fixture.xex').read_bytes());b.extend(bytes(65536));words=[];sites=[];functions={};sizes={}
 def start(name):functions[name]=CODE+len(words)*4
 def emit(w,kind=None,width=0,target=0):
  pc=CODE+len(words)*4;words.append(w)
  if kind:sites.append({'pc':pc,'owner':list(functions.values())[-1],'kind':kind,'width':width,'target':target})
 def addr(r,v):emit(d(15,r,0,v>>16));emit(d(24,r,r,v&65535))
 def call(target):emit(0x48000001|((target-(CODE+len(words)*4))&0x3fffffc),'direct',target=target)
 start('entry');emit(d(37,1,1,-96),'write',4);emit(spr(0,8));emit(d(36,0,1,100),'write',4)
 addr(4,BASE+0x20220);emit(d(32,3,4),'read',4);emit(d(32,5,13),'read',4);emit(d(32,6,5),'read',4);emit(31<<26|3<<21|3<<16|6<<11|266<<1)
 helper_call=len(words);emit(0)
 indirect_hi=len(words);addr(12,0);emit(spr(12,9,True));emit(0x4e800421,'indirect')
 addr(4,BASE+0x40000);emit(d(36,3,4),'write',4)
 addr(3,BASE+0x20200);emit(d(14,4,0,16));addr(5,0x10203040);call(BASE+0x30004)
 addr(4,BASE+0x40000);emit(d(32,6,4),'read',4);emit(31<<26|3<<21|3<<16|6<<11|266<<1);emit(d(36,3,4,4),'write',4)
 emit(d(32,0,1,100),'read',4);emit(spr(0,8,True));emit(d(14,1,1,96));emit(0x4e800020,'ret');sizes['entry']=len(words)
 start('helper');emit(d(14,3,3,7));emit(0x4e800020,'ret');sizes['helper']=2
 start('forever');pc=CODE+len(words)*4;emit(0x48000000,'branch',target=pc);sizes['forever']=1
 words[indirect_hi]=d(15,12,0,functions['helper']>>16);words[indirect_hi+1]=d(24,12,12,functions['helper']&65535)
 for site in sites:
  if site['kind']=='indirect':site['target']=functions['helper']
 words[helper_call]=0x48000001|((functions['helper']-CODE-helper_call*4)&0x3fffffc);sites.append({'pc':CODE+helper_call*4,'owner':functions['entry'],'kind':'direct','width':0,'target':functions['helper']});sites.sort(key=lambda s:s['pc'])
 b[0x11000:0x21000]=bytes(65536);code=b''.join(struct.pack('>I',x) for x in words);b[0x11000:0x11000+len(code)]=code
 def be(o,v):struct.pack_into('>I',b,o,v)
 def le(o,v):struct.pack_into('<I',b,0x1000+o,v)
 be(44,functions['entry']);be(0x104,0x50000);be(0x284+3*24,0x22);le(0x98+56,0x50000);le(0x98+224+3*40+8,0x20000);le(0x98+224+3*40+16,0x20000);le(0x98+16,0x10000)
 be(0x41c,48);b[0x41c+38:0x41c+40]=(2).to_bytes(2,'big');be(0x400,0x4c)
 le(0x98+100+3*8,len(functions)*8)
 for i,(name,pc) in enumerate(functions.items()):be(0x21100+8*i,pc);be(0x21104+8*i,(sizes[name]<<8)|0x40000000)
 be(0x21200,0x10203040);be(0x21204,0);be(0x21208,0x10203040);be(0x2120c,0x10203040);be(0x21220,10);be(0x21000,5) # static TLS template seed
 (p/'fixture.xex').write_bytes(b)
 hooks=[s['pc'] for s in sites];manifest='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(ROOT/'tests/native-image-binding/templates'))+'\n'
 for pc in hooks:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="native_point_{pc:X}"\n'
 (p/'fixture_manifest.toml').write_text(manifest);meta={'base':BASE,'code_base':CODE,'functions':functions,'sizes':sizes,'words':words,'sites':sites,'hooks':hooks,'expected_result':32,'sdk_result':3,'constant':10,'tls_seed':5,'helper_addend':7,'native_zero_range':[BASE+0x30400,BASE+0x50000]};(p/'fixture.json').write_text(json.dumps(meta,indent=2));return meta
