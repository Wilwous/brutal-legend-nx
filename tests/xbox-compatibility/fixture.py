# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original scalar SDK-call ABI workload; no title PPC or visual demonstration."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CODE=0x82010000

def create(directory):
 spec=importlib.util.spec_from_file_location('original_container',ROOT/'tests/module-startup/fixture.py');base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base);base.create(directory)
 image=bytearray((directory/'fixture.xex').read_bytes());words=[];sites=[];functions={};sizes={};calls=[]
 def d(op,rt,ra,imm=0):return op<<26|rt<<21|ra<<16|(imm&65535)
 def emit(word,kind='branch',width=0,target=0):
  pc=CODE+len(words)*4;words.append(word);sites.append(dict(pc=pc,owner=list(functions.values())[-1],kind=kind,width=width,target=target))
 def spr(r,n,write=False):return 31<<26|r<<21|(n&31)<<16|(n>>5)<<11|(467 if write else 339)<<1
 def li(r,v):
  if -32768<=v<=32767:emit(d(14,r,0,v))
  else:emit(d(15,r,0,v>>16));emit(d(24,r,r,v&65535))
 def load(r,off):emit(d(32,r,31,off),'read',4)
 def store(r,off):emit(d(36,r,31,off),'write',4)
 def call(ordinal):li(11,ordinal);calls.append(len(words));emit(0,'direct')
 def begin(name):
  functions[name]=CODE+len(words)*4;emit(d(37,1,1,-96),'write',4);emit(spr(0,8));emit(d(36,0,1,100),'write',4);emit(d(62,31,1,80),'write',8);emit(31<<26|3<<21|31<<16|3<<11|444<<1)
 def end(name,result):
  load(3,result);emit(d(58,31,1,80),'read',8);emit(d(32,0,1,100),'read',4);emit(spr(0,8,True));emit(d(14,1,1,96));emit(0x4e800020,'ret');sizes[name]=(CODE+len(words)*4-functions[name])//4
 begin('initialize');call(338);store(3,0);end('initialize',0)
 begin('update');load(3,0);li(4,0x89abcdef);call(341);store(3,4);load(3,0);call(340);store(3,8)
 emit(d(14,3,31,1024));li(4,16);li(5,0x12345678);call(294);store(3,12)
 load(3,0);call(339);store(3,16)
 emit(d(14,3,31,2048));li(4,12);call(0x10000);store(3,20)
 emit(d(14,3,31,8192));li(4,6144);call(0x10001);store(3,24);end('update',8)
 functions['gate']=CODE+len(words)*4;emit(0x4e800020,'ret');sizes['gate']=1
 for index in calls:
  target=functions['gate'];words[index]=0x48000001|((target-(CODE+4*index))&0x3fffffc);sites[index]['target']=target
 image[0x11000:0x21000]=bytes(65536);image[0x11000:0x11000+4*len(words)]=b''.join(struct.pack('>I',w) for w in words);struct.pack_into('>I',image,44,functions['initialize']);struct.pack_into('<I',image,0x1000+0x98+100+3*8,len(functions)*8);image[0x21100:0x21200]=bytes(256)
 for i,(name,pc) in enumerate(functions.items()):struct.pack_into('>II',image,0x21100+i*8,pc,(sizes[name]<<8)|0x40000000)
 (directory/'fixture.xex').write_bytes(image)
 text='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(ROOT/'tests/xbox-compatibility/templates'))+'\n'
 for s in sites:text+=f'\n[[entrypoint.midasm_hook]]\naddress={s["pc"]}\nname="platform_point_{s["pc"]:X}"\n'
 (directory/'fixture_manifest.toml').write_text(text);meta=dict(functions=functions,sizes=sizes,words=words,sites=sites,hooks=[s['pc'] for s in sites],code_base=CODE,command_bytes=0);(directory/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n');return meta
