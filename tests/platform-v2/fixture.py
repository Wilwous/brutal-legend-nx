# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original PPC integer scene loop, camera/animation state and indexed draw records."""
import importlib.util
import json
import struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
CODE=0x82010000
COMMANDS=1024

def create(directory):
    spec=importlib.util.spec_from_file_location('original_container',ROOT/'tests/module-startup/fixture.py');base=importlib.util.module_from_spec(spec);spec.loader.exec_module(base);base.create(directory)
    image=bytearray((directory/'fixture.xex').read_bytes());words=[];sites=[];functions={};sizes={};calls=[];branches=[];labels={}
    def d(op,rt,ra,imm=0):return op<<26|rt<<21|ra<<16|(imm&65535)
    def spr(r,n,write=False):return 31<<26|r<<21|(n&31)<<16|(n>>5)<<11|(467 if write else 339)<<1
    def emit(word,kind='branch',width=0,target=0):
        pc=CODE+len(words)*4;words.append(word);sites.append(dict(pc=pc,owner=list(functions.values())[-1],kind=kind,width=width,target=target))
    def li(r,v):
        if -32768<=v<=32767:emit(d(14,r,0,v))
        else:emit(d(15,r,0,v>>16));emit(d(24,r,r,v&65535))
    def store(r,offset):emit(d(36,r,31,offset),'write',4)
    def load(r,offset):emit(d(32,r,31,offset),'read',4)
    def value(offset,v):li(8,v);store(8,offset)
    def service(op):li(3,op);calls.append(len(words));emit(0,'direct')
    def begin(name):
        functions[name]=CODE+4*len(words);emit(d(37,1,1,-96),'write',4);emit(spr(0,8));emit(d(36,0,1,100),'write',4);emit(d(62,31,1,80),'write',8);emit(31<<26|3<<21|31<<16|3<<11|444<<1)
    def end(name,result):
        load(3,result);emit(d(58,31,1,80),'read',8);emit(d(32,0,1,100),'read',4);emit(spr(0,8,True));emit(d(14,1,1,96));emit(0x4e800020,'ret');sizes[name]=(CODE+4*len(words)-functions[name])//4
    begin('initialize');service(2);store(3,92);service(5);store(3,96);end('initialize',92)
    begin('update');service(1)
    for offset,delta,mask in [(64,0,22),(68,4,22),(72,8,24),(76,12,None)]:
        load(6,offset);load(7,delta);emit(31<<26|6<<21|6<<16|7<<11|266<<1)
        if mask is not None:emit(21<<26|6<<21|6<<16|mask<<6|31<<1)
        store(6,offset)
    load(6,80);emit(d(14,6,6,1));emit(21<<26|6<<21|6<<16|24<<6|31<<1);store(6,80)
    load(6,84);load(7,16);emit(31<<26|6<<21|6<<16|7<<11|266<<1);store(6,84)
    load(6,88);emit(d(14,6,6,1));store(6,88)
    service(3);store(3,28);emit(d(11,0,3,0));branches.append((len(words),'after_draw'));emit(0)
    # The guest emits the entire command ABI, including real owner/generation tokens.
    commands=[[1,0,0,0x142030ff,0,0,0,0],[3,0,1,0,0,0,0,0],[2,0,0,0,0,0,0,0],[5,0,0,0,0,0,0,0]]
    for model in range(4):commands.extend([[4,model+2,0,0,0,0,0,0],[6,0,36,0,0,0,0,0]])
    commands.append([7,0,0,0,0,0,0,0]);dynamic={(0,2):88}
    for j in range(4):dynamic[2,j+1]=112+j*4;dynamic[3,j+1]=128+j*4
    for i,c in enumerate(commands):
        for j,word in enumerate(c):
            if (i,j) in dynamic:load(8,dynamic[i,j]);store(8,COMMANDS+i*32+j*4)
            else:value(COMMANDS+i*32+j*4,word)
    emit(d(14,4,31,COMMANDS));li(5,len(commands)*32);service(4)
    labels['after_draw']=CODE+4*len(words);service(6);end('update',88)
    functions['gate']=CODE+4*len(words);emit(0x4e800020,'ret');sizes['gate']=1
    for index in calls:
        target=functions['gate'];words[index]=0x48000001|((target-(CODE+4*index))&0x3fffffc);sites[index]['target']=target
    for index,label in branches:
        pc=CODE+4*index;target=labels[label];words[index]=0x41820000|((target-pc)&0xfffc);sites[index]['target']=target
    assert 4*len(words)<65536
    image[0x11000:0x21000]=bytes(65536);image[0x11000:0x11000+4*len(words)]=b''.join(struct.pack('>I',w) for w in words);struct.pack_into('>I',image,44,functions['initialize'])
    struct.pack_into('<I',image,0x1000+0x98+100+3*8,len(functions)*8);image[0x21100:0x21200]=bytes(256)
    for i,(name,pc) in enumerate(functions.items()):struct.pack_into('>II',image,0x21100+i*8,pc,(sizes[name]<<8)|0x40000000)
    (directory/'fixture.xex').write_bytes(image)
    manifest='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(ROOT/'tests/platform-v2/templates'))+'\n'
    for s in sites:manifest+=f'\n[[entrypoint.midasm_hook]]\naddress={s["pc"]}\nname="platform_point_{s["pc"]:X}"\n'
    (directory/'fixture_manifest.toml').write_text(manifest)
    meta=dict(functions=functions,sizes=sizes,words=words,sites=sites,hooks=[s['pc'] for s in sites],code_base=CODE,command_bytes=len(commands)*32);(directory/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n');return meta
