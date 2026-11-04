#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original module/header PPC workload in an author-built NONE XEX."""
import importlib.util,json,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];CODE=0x82010000

def create(p):
 spec=importlib.util.spec_from_file_location('own_base',ROOT/'tests/native-image-binding/fixture.py');old=importlib.util.module_from_spec(spec);spec.loader.exec_module(old);old.create(p)
 b=bytearray((p/'fixture.xex').read_bytes());words=[];sites=[];func={};sizes={};labels={};patch=[]
 def d(op,rt,ra,imm=0):return op<<26|rt<<21|ra<<16|(imm&65535)
 def spr(r,n,write=False):return 31<<26|r<<21|(n&31)<<16|(n>>5)<<11|(467 if write else 339)<<1
 def start(name):func[name]=CODE+len(words)*4
 def emit(w,kind='branch',width=0,target=0):
  pc=CODE+len(words)*4;words.append(w);sites.append({'pc':pc,'owner':list(func.values())[-1],'kind':kind,'width':width,'target':target})
 def branch(label,kind='b'):
  patch.append((len(words),label,kind));emit(0,'direct' if kind=='bl' else 'branch')
 start('entry');emit(d(37,1,1,-64),'write',4);emit(spr(0,8));emit(d(36,0,1,68),'write',4);emit(d(62,14,1,16),'write',8);emit(31<<26|4<<21|14<<16|4<<11|444<<1)
 emit(d(32,11,3),'read',4);emit(d(32,11,11),'read',4);emit(d(32,3,11,0x58),'read',4);emit(d(15,4,0,2));emit(d(24,4,4,0x401));branch('query','bl')
 emit(d(11,0,3,0));branch('missing','beq');emit(d(32,3,3),'read',4);branch('join');labels['missing']=CODE+len(words)*4;emit(d(14,3,0,5));labels['join']=CODE+len(words)*4;branch('helper','bl')
 emit(d(32,5,13),'read',4);emit(d(32,6,5,32),'read',4);emit(31<<26|3<<21|3<<16|6<<11|266<<1);emit(d(36,3,14),'write',4);emit(d(58,14,1,16),'read',8);emit(d(32,0,1,68),'read',4);emit(spr(0,8,True));emit(d(14,1,1,64));emit(0x4e800020,'ret');sizes['entry']=len(words)
 start('helper');emit(d(14,3,3,7));emit(0x4e800020,'ret');sizes['helper']=2
 start('query');emit(0x4e800020,'ret');sizes['query']=1
 start('forever');pc=CODE+len(words)*4;emit(0x48000000,'branch',target=pc);sizes['forever']=1
 start('indirect');emit(spr(3,9,True));emit(0x4e800421,'indirect',target=func['helper']);emit(0x4e800020,'ret');sizes['indirect']=3
 for index,label,kind in patch:
  target=func[label] if label in func else labels[label];pc=CODE+index*4
  words[index]=(0x41820000 if kind=='beq' else 0x48000001 if kind=='bl' else 0x48000000)|((target-pc)&(0xfffc if kind=='beq' else 0x3fffffc));sites[index]['target']=target
 # Remove the old synthetic function-import descriptor; author a heap field.
 count=struct.unpack_from('>I',b,20)[0];found=False
 for i in range(count):
  if struct.unpack_from('>I',b,24+8*i)[0]==0x103ff:struct.pack_into('>II',b,24+8*i,0x20401,21);found=True
 assert found
 b[0x11000:0x21000]=bytes(65536);code=b''.join(struct.pack('>I',w) for w in words);b[0x11000:0x11000+len(code)]=code
 struct.pack_into('>I',b,44,func['entry']);struct.pack_into('<I',b,0x1000+0x98+16,0x10000);struct.pack_into('<I',b,0x1000+0x98+100+3*8,len(func)*8)
 b[0x21100:0x21200]=bytes(256)
 for i,(name,pc) in enumerate(func.items()):struct.pack_into('>II',b,0x21100+8*i,pc,(sizes[name]<<8)|0x40000000)
 b[0x21000:0x21010]=bytes(16) # independently initialized static TLS, dynamic slot seed3
 (p/'fixture.xex').write_bytes(b)
 text='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(ROOT/'tests/module-startup/templates'))+'\n'
 for site in sites:text+=f'\n[[entrypoint.midasm_hook]]\naddress={site["pc"]}\nname="module_point_{site["pc"]:X}"\n'
 (p/'fixture_manifest.toml').write_text(text);meta={'code_base':CODE,'functions':func,'sizes':sizes,'words':words,'sites':sites,'hooks':[s['pc'] for s in sites],'expected_result':31,'field':21,'helper_addend':7,'tls_seed':3};(p/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n');return meta
