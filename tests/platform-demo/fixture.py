# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original three-function service demo in an original uncompressed synthetic XEX."""
import importlib.util
import json
import struct
from pathlib import Path
ROOT = Path(__file__).resolve().parents[2]
CODE = 0x82010000
COMMANDS = 1024

def create(directory):
    spec = importlib.util.spec_from_file_location('original_container', ROOT/'tests/module-startup/fixture.py')
    base = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(base)
    base.create(directory)
    image = bytearray((directory/'fixture.xex').read_bytes())
    words, sites, functions, sizes, patches = [], [], {}, {}, []
    def d(op, rt, ra, imm=0): return op<<26 | rt<<21 | ra<<16 | (imm&65535)
    def spr(r, n, write=False): return 31<<26 | r<<21 | (n&31)<<16 | (n>>5)<<11 | (467 if write else 339)<<1
    def emit(word, kind='branch', width=0, target=0):
        pc = CODE+len(words)*4
        words.append(word)
        sites.append(dict(pc=pc, owner=list(functions.values())[-1], kind=kind, width=width, target=target))
    def start(name): functions[name] = CODE+len(words)*4
    def li(r, value):
        if -32768 <= value <= 32767: emit(d(14, r, 0, value))
        else: emit(d(15, r, 0, value>>16)); emit(d(24, r, r, value&65535))
    def store(r, offset): emit(d(36, r, 31, offset), 'write', 4)
    def load(r, offset): emit(d(32, r, 31, offset), 'read', 4)
    def value(offset, number): li(8, number); store(8, offset)
    def call(): patches.append(len(words)); emit(0, 'direct')
    def service(number): li(3, number); call()
    def begin(name):
        start(name); emit(d(37,1,1,-96),'write',4);emit(spr(0,8));emit(d(36,0,1,100),'write',4)
        emit(d(62,31,1,80),'write',8);emit(31<<26|3<<21|31<<16|3<<11|444<<1)
    def end(name, result):
        load(3,result);emit(d(58,31,1,80),'read',8);emit(d(32,0,1,100),'read',4)
        emit(spr(0,8,True));emit(d(14,1,1,96));emit(0x4e800020,'ret')
        sizes[name]=(CODE+len(words)*4-functions[name])//4
    begin('initialize')
    emit(d(14,4,31,256));li(5,len('demo:/demo.txt'));service(2);store(3,16)
    load(4,16);emit(d(14,5,31,512));li(6,64);service(3);store(3,20)
    load(7,512);store(7,72)
    load(4,16);service(4)
    service(5);store(3,24)
    end('initialize',20)
    begin('update')
    emit(d(14,4,31,0));service(1)
    load(6,64);load(7,0);emit(31<<26|6<<21|6<<16|7<<11|266<<1);emit(d(14,6,6,1))
    emit(21<<26|6<<21|6<<16|24<<6|31<<1);store(6,64)  # x & 255
    load(6,68);load(7,4);emit(31<<26|6<<21|6<<16|7<<11|266<<1)
    emit(21<<26|6<<21|6<<16|25<<6|31<<1);store(6,68)  # y & 127
    load(6,72);load(7,8);emit(31<<26|6<<21|6<<16|7<<11|266<<1);store(6,72)
    load(6,76);emit(d(14,6,6,1));store(6,76)
    # Every complete command word is emitted by actual generated PPC stores.
    commands = [[1,0,0,320,180,1,0,0], [2,0x102030ff,0,0,0,0,0,0],
                [3,0,0,0,0,0,0,0], [4,0,0,16,16,0,0,0],
                [5,0,4,4,16,0,0,0], [6,0,280,140,32,32,0,0], [7,0,0,0,0,0,0,0]]
    dynamic = {(0,2):76,(2,1):72,(3,1):64,(3,2):68}
    for i, command in enumerate(commands):
        for j, word in enumerate(command):
            if (i,j) in dynamic: load(8,dynamic[i,j]);store(8,COMMANDS+i*32+j*4)
            else: value(COMMANDS+i*32+j*4,word)
    emit(d(14,4,31,COMMANDS));li(5,len(commands)*32);service(7);store(3,80)
    load(6,32);li(4,0x61000000);li(5,19200);service(6);store(3,28)
    end('update',76)
    start('gate');emit(0x4e800020,'ret');sizes['gate']=1
    for index in patches:
        target=functions['gate'];words[index]=0x48000001|((target-(CODE+4*index))&0x3fffffc);sites[index]['target']=target
    image[0x11000:0x21000]=bytes(65536)
    image[0x11000:0x11000+4*len(words)]=b''.join(struct.pack('>I',word) for word in words)
    struct.pack_into('>I',image,44,functions['initialize'])
    struct.pack_into('<I',image,0x1000+0x98+100+3*8,len(functions)*8)
    image[0x21100:0x21200]=bytes(256)
    for i,(name,pc) in enumerate(functions.items()):struct.pack_into('>II',image,0x21100+i*8,pc,(sizes[name]<<8)|0x40000000)
    (directory/'fixture.xex').write_bytes(image)
    text='[project]\nname="fixture"\nsdk_version="0.10.0"\ngame_root="."\n[entrypoint]\nfile_path="fixture.xex"\nout_directory_path="generated"\ntemplate_dir='+json.dumps(str(ROOT/'tests/platform-demo/templates'))+'\n'
    for site in sites:text+=f'\n[[entrypoint.midasm_hook]]\naddress={site["pc"]}\nname="platform_point_{site["pc"]:X}"\n'
    (directory/'fixture_manifest.toml').write_text(text)
    meta=dict(functions=functions,sizes=sizes,words=words,sites=sites,hooks=[s['pc'] for s in sites],code_base=CODE,command_bytes=len(commands)*32)
    (directory/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta
