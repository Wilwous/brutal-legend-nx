#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independently encoded PPC, relative nm symbols, assembly listing, sidecar."""
import json
import struct
from pathlib import Path
BASE = 0x82010000  # Pinned recompile-tests analysis coordinate, not runtime VA.
def d(op, rt, ra, imm=0): return (op << 26) | (rt << 21) | (ra << 16) | (imm & 65535)
def x(xo, rt, ra, rb): return (31 << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (xo << 1)
def spr(rt, n, write=False): return (31 << 26) | (rt << 21) | ((n & 31) << 16) | ((n >> 5) << 11) | ((467 if write else 339) << 1)
def create(directory):
    directory=Path(directory); directory.mkdir(parents=True,exist_ok=False)
    functions=[]
    def add(name, instructions): functions.append((name,instructions))
    add('entry',[(d(37,1,1,-64),'stwu r1,-64(r1)'),(spr(0,8),'mflr r0'),
        (d(36,0,1,68),'stw r0,68(r1)'),(d(62,14,1,8),'std r14,8(r1)'),
        (d(32,5,4),'lwz r5,0(r4)'),(d(36,5,3),'stw r5,0(r3)'),
        (None,'bl test_direct'),(spr(12,9,True),'mtctr r12'),(0x4e800421,'bctrl'),
        (d(58,14,1,8),'ld r14,8(r1)'),(d(32,0,1,68),'lwz r0,68(r1)'),
        (spr(0,8,True),'mtlr r0'),(d(14,1,1,64),'addi r1,r1,64'),(0x4e800020,'blr')])
    add('direct',[(d(14,5,5,1),'addi r5,r5,1'),(d(36,5,3,4),'stw r5,4(r3)'),(0x4e800020,'blr')])
    add('indirect',[(d(14,5,5,2),'addi r5,r5,2'),(d(36,5,3,8),'stw r5,8(r3)'),(0x4e800020,'blr')])
    add('tls',[(d(32,6,13),'lwz r6,0(r13)'),(d(36,3,6),'stw r3,0(r6)'),(d(32,3,6),'lwz r3,0(r6)'),(0x4e800020,'blr')])
    add('vector',[(x(103,3,0,4),'lvx v3,0,r4'),((4<<26)|(4<<21)|(3<<16)|(3<<11),'vaddubm v4,v3,v3'),(x(231,4,0,3),'stvx v4,0,r3'),(0x4e800020,'blr')])
    add('load',[(d(32,3,4),'lwz r3,0(r4)'),(0x4e800020,'blr')])
    add('store',[(d(36,3,4),'stw r3,0(r4)'),(0x4e800020,'blr')])
    add('atomic',[(x(20,3,0,4),'lwarx r3,0,r4'),(0x4e800020,'blr')])
    add('partial',[(x(519,3,0,4),'lvlx v3,0,r4'),(0x4e800020,'blr')])
    offsets={}; offset=0
    for name,insns in functions: offsets[name]=offset; offset+=len(insns)*4
    words=[]; listing=[]; nm=[]
    for name,insns in functions:
        nm.append(f'{offsets[name]:08x} T test_{name}')
        listing.append(f'test_{name}:')
        for word,asm in insns:
            if word is None: word=0x48000001|((offsets['direct']-len(words)*4)&0x3fffffc)
            words.append(word);listing.append('    '+asm)
        listing.append('')
    (directory/'module.bin').write_bytes(b''.join(struct.pack('>I',w) for w in words))
    (directory/'module.map').write_text('\n'.join(nm)+'\n')
    (directory/'module.s').write_text('# Original PPC encoding fixture; no assembler dependency.\n'+'\n'.join(listing)+'\n')
    metadata={'format':'rexglue-raw-test-with-original-sidecar','analysis_base':BASE,'entry':0,'image_size':0xc000,
        'functions':offsets,'instruction_count':len(words),
        'sections':[{'offset':0,'size':8192,'permission':'code-read','file':'module.bin'},
                    {'offset':0x4000,'size':8192,'permission':'read','bytes':list(range(16))},
                    {'offset':0x8000,'size':8192,'permission':'read-write','bytes':[]}],
        'relocations':[{'offset':0x8010,'target':0x4000,'type':'abs32be'}]}
    (directory/'module.json').write_text(json.dumps(metadata,indent=2)+'\n')
    # Sidecar descriptors are consumed by the C++ fixture through this metadata
    # header; the PPC implementation itself is emitted exclusively by ReXGlue.
    loaded=json.loads((directory/'module.json').read_text())
    header='// Original sidecar metadata, not a PPC implementation.\n#pragma once\n'
    header+='\n'.join(f'inline constexpr unsigned pc_{name}={off};' for name,off in loaded['functions'].items())+'\n'
    header+='struct FixtureSection { unsigned offset,size; bool write,code; };\n'
    header+='inline constexpr FixtureSection fixture_sections[]{'+','.join('{%d,%d,%s,%s}'%(s['offset'],s['size'],'true' if s['permission']=='read-write' else 'false','true' if s['permission']=='code-read' else 'false') for s in loaded['sections'])+'};\n'
    header+='inline constexpr unsigned fixture_relocation_offset=%d,fixture_relocation_target=%d;\n'%(loaded['relocations'][0]['offset'],loaded['relocations'][0]['target'])
    (directory/'layout.h').write_text(header)
    return metadata
