#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original PPC words and explicit call-gate metadata; never generated C++."""
import json,struct
from pathlib import Path
BASE=0x82010000

def d(op,t,a,i=0):return op<<26|t<<21|a<<16|(i&65535)
def x(xo,t,a,b):return 31<<26|t<<21|a<<16|b<<11|xo<<1
def spr(t,n,write=False):return 31<<26|t<<21|(n&31)<<16|(n>>5)<<11|(467 if write else 339)<<1

def create(directory):
 p=Path(directory);p.mkdir(exist_ok=False,parents=True)
 # The two blr gate identities are intercepted BEFORE their generated bodies
 # by explicit import metadata. They are never admitted as synthetic functions.
 functions={};labels={};words=[];asm=[]
 def start(name):functions[name]=len(words)*4;asm.append('test_'+name+':')
 def emit(w,s):words.append(w);asm.append('    '+s)
 def load(t,off):emit(d(32,t,14,off),f'lwz r{t},{off}(r14)')
 def store(t,off):emit(d(36,t,14,off),f'stw r{t},{off}(r14)')
 start('entry')
 emit(d(37,1,1,-256),'stwu r1,-256(r1)');emit(spr(0,8),'mflr r0')
 emit(d(36,0,1,260),'stw r0,260(r1)');emit(d(62,14,1,16),'std r14,16(r1)')
 emit(x(444,3,14,3),'mr r14,r3')
 load(5,0x60);store(5,0x64)
 emit(d(14,4,14,0x80),'addi r4,r14,128');emit(x(103,3,0,4),'lvx v3,0,r4')
 emit(4<<26|4<<21|3<<16|3<<11,'vaddubm v4,v3,v3')
 emit(d(14,4,14,0x90),'addi r4,r14,144');emit(x(231,4,0,4),'stvx v4,0,r4')
 emit(d(32,6,13),'lwz r6,0(r13)');load(5,0x6c)
 emit(d(36,5,6,16),'stw r5,16(r6)');emit(d(32,5,6,16),'lwz r5,16(r6)');store(5,0x68)
 emit(d(58,3,14,0x70),'ld r3,112(r14)');load(11,0x78)
 emit(d(36,11,1,84),'stw r11,84(r1)');emit(('call','echo'),'bl test_echo')
 emit(d(62,3,14,0xa0),'std r3,160(r14)')
 load(3,0);load(4,4);load(5,8);emit(('call','compare'),'bl test_compare');store(3,0x24)
 load(3,0xc);load(4,0x10);load(5,0x14);load(12,0x18)
 emit(spr(12,9,True),'mtctr r12');emit(0x4e800421,'bctrl');store(3,0x28)
 emit(d(11,0,3,0),'cmpwi r3,0');emit(('bne','epilogue'),'bne epilogue')
 load(3,0x1c);emit(('call','critical'),'bl test_critical')
 emit(d(14,5,0,0x55aa),'li r5,21930');store(5,0x2c);emit(d(14,3,0,0),'li r3,0')
 labels['epilogue']=len(words)*4
 emit(d(58,14,1,16),'ld r14,16(r1)');emit(d(32,0,1,260),'lwz r0,260(r1)')
 emit(spr(0,8,True),'mtlr r0');emit(d(14,1,1,256),'addi r1,r1,256');emit(0x4e800020,'blr')
 asm.append('');start('echo')
 emit(d(32,11,1,84),'lwz r11,84(r1)');emit(x(316,3,3,11),'xor r3,r3,r11');emit(0x4e800020,'blr')
 for name in ['compare','critical']:
  asm.append('');start(name);emit(0x4e800020,'blr')
 for name,ops in [('atomic',[(x(20,3,0,4),'lwarx r3,0,r4')]),('partial',[(x(519,3,0,4),'lvlx v3,0,r4')]),('load',[(d(32,3,4),'lwz r3,0(r4)')]),('store',[(d(36,3,4),'stw r3,0(r4)')])]:
  asm.append('');start(name)
  for w,s in ops:emit(w,s)
  emit(0x4e800020,'blr')
 for i,w in enumerate(words):
  if isinstance(w,tuple):
   kind,name=w;target=functions[name] if kind=='call' else labels[name];delta=target-i*4
   words[i]=(0x48000001|(delta&0x3fffffc)) if kind=='call' else (0x40820000|(delta&0xfffc))
 (p/'caller.bin').write_bytes(b''.join(struct.pack('>I',w) for w in words))
 (p/'caller.map').write_text(''.join(f'{off:08x} T test_{name}\n' for name,off in functions.items()))
 (p/'caller.s').write_text('# Independently authored PPC test program.\n'+'\n'.join(asm)+'\n')
 metadata={'analysis_base':BASE,'functions':functions,'instructions':len(words),'entry':0,'synthetic_reachable':['entry','echo'],'import_gate_bodies_never_executed':['compare','critical'],'stack_argument_offset':84,'stack_argument_bits':32,'gpr_argument_return_bits':64}
 (p/'caller.json').write_text(json.dumps(metadata,indent=2)+'\n')
 h='// Original address metadata, not a caller implementation.\n#pragma once\n'
 for name,off in functions.items():h+=f'DECLARE_REX_FUNC(caller_{BASE+off:X});\ninline constexpr auto fn_{name}=caller_{BASE+off:X};\ninline constexpr unsigned pc_{name}=0x{BASE+off:X};\n'
 (p/'layout.h').write_text(h);return metadata
