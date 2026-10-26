# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original unencrypted XEX; production codegen consumes this exact image."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def module(name,path):
 spec=importlib.util.spec_from_file_location(name,path);m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m
foundation=module('foundation_original',ROOT/'tests/horizon-foundation/fixture.py')
original=module('xex_original',ROOT/'tests/xex-binding/fixture.py')
d=foundation.d;spr=foundation.spr
BASE=0x82000000;CODE=BASE+0x10000
def create(path,templates):
 original.create(path,BASE);meta=foundation.create(path/'multicall');words=list(meta['words']);functions=dict(meta['functions']);sizes={'entry':26,'step':2,'forever':2}
 wrapper=len(words)*4;functions['xex_entry']=CODE+wrapper;pending=[];labels={}
 def emit(w):words.append(w)
 def mr(dst,src):return 31<<26|src<<21|dst<<16|src<<11|444<<1
 def branch(name,conditional=False):pending.append((len(words)*4,name,conditional));emit(0)
 def addr(reg,value):emit(d(15,reg,0,value>>16));emit(d(24,reg,reg,value&65535))
 emit(d(37,1,1,-96));emit(spr(0,8));emit(d(36,0,1,100));emit(d(62,14,1,16));emit(d(62,15,1,24))
 at=len(words)*4;emit(0x48000001|((-at)&0x3fffffc));emit(mr(14,3))
 addr(3,BASE+0x20200);emit(d(14,4,0,16));addr(5,0x10203040)
 at=len(words)*4;emit(0x48000001|((BASE+0x30004-CODE-at)&0x3fffffc))
 emit(d(11,0,3,3));branch('error',True);emit(mr(3,14));emit(d(14,6,0,5));emit(d(36,6,1,84))
 indirect_hi=len(words);addr(12,0);emit(spr(12,9,True));emit(0x4e800421);branch('done')
 labels['error']=len(words)*4;emit(d(14,3,0,-1))
 labels['done']=len(words)*4;emit(d(58,15,1,24));emit(d(58,14,1,16));emit(d(32,0,1,100));emit(spr(0,8,True));emit(d(14,1,1,96));emit(0x4e800020)
 sizes['xex_entry']=(len(words)*4-wrapper)//4
 functions['combine']=CODE+len(words)*4;emit(d(32,6,1,84));emit(31<<26|3<<21|3<<16|6<<11|266<<1);emit(0x4e800020);sizes['combine']=3
 functions['store']=CODE+len(words)*4;emit(d(36,3,4));emit(0x4e800020);sizes['store']=2
 words[indirect_hi]=d(15,12,0,functions['combine']>>16);words[indirect_hi+1]=d(24,12,12,functions['combine']&65535)
 for at,name,conditional in pending:
  delta=labels[name]-at;words[at//4]=(0x40820000|(delta&0xfffc)) if conditional else 0x48000000|(delta&0x3fffffc)
 image=bytearray((path/'fixture.xex').read_bytes())
 def be(off,value):struct.pack_into('>I',image,off,value)
 def le(off,value):struct.pack_into('<I',image,0x1000+off,value)
 code=b''.join(struct.pack('>I',w) for w in words);image[0x11000:0x21000]=bytes(65536);image[0x11000:0x11000+len(code)]=code
 be(44,functions['xex_entry']);le(0x98+16,functions['xex_entry']-BASE);le(0x98+100+3*8,len(functions)*8)
 for i,(name,pc) in enumerate(functions.items()):be(0x21100+i*8,pc);be(0x21104+i*8,(sizes[name]<<8)|0x40000000)
 be(0x400,0x4c);be(0x41c,48);image[0x41c+38:0x41c+40]=(2).to_bytes(2,'big')
 image[0x41c+48:0x41c+64]=bytes(16)
 for i,v in enumerate([0x10203040,0,0x10203040,0x10203040]):be(0x21200+i*4,v)
 (path/'fixture.xex').write_bytes(image)
 sites=meta['backedges'];manifest='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(templates.resolve()))+'\n'
 for pc in sites:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={pc}\nname="horizon_point_{pc:X}"\n'
 (path/'fixture_manifest.toml').write_text(manifest)
 result={'base':BASE,'entry':functions['xex_entry'],'size':0x40000,'code_base':CODE,'code':list(code),'functions':functions,'sizes':sizes,'instructions':len(words),'hooks':sites,'ordinal':0x11b,'thunk':BASE+0x30004,'words':words}
 (path/'xex-layout.json').write_text(json.dumps(result,indent=2)+'\n');return result
