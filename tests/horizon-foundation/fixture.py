# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original multicall PPC. No proprietary input and no assembler dependency."""
import json,struct
BASE=0x82010000
def d(op,rt,ra,imm=0):return op<<26|rt<<21|ra<<16|(imm&65535)
def spr(rt,n,write=False):return 31<<26|rt<<21|(n&31)<<16|(n>>5)<<11|(467 if write else 339)<<1
def create(path):
 path.mkdir();words=[];listing=[];labels={};pending=[];functions={};rows=[]
 def emit(word,text):words.append(word);listing.append('    '+text)
 def label(name):labels[name]=len(words)*4;listing.append(name+':')
 def start(name):functions[name]=len(words)*4;label('test_'+name)
 def branch(name,kind='b'):pending.append((len(words)*4,name,kind));emit(0,kind+' '+name)
 start('entry');emit(d(37,1,1,-64),'stwu r1,-64(r1)');emit(spr(0,8),'mflr r0');emit(d(36,0,1,68),'stw r0,68(r1)')
 emit(d(62,14,1,16),'std r14,16(r1)');emit(31<<26|3<<21|14<<16|3<<11|444<<1,'mr r14,r3')
 emit(d(32,5,13),'lwz r5,0(r13)');emit(d(32,3,5,32),'lwz r3,32(r5)');branch('test_step','bl')
 emit(d(36,3,5,32),'stw r3,32(r5)');emit(d(36,3,1,24),'stw r3,24(r1)');emit(d(32,6,1,24),'lwz r6,24(r1)')
 emit(d(11,0,6),'cmpwi r6,0');branch('error','beq');emit(d(11,0,14),'cmpwi r14,0');branch('end','beq')
 label('loop');branch('test_step','bl');emit(d(14,14,14,-1),'addi r14,r14,-1');emit(d(11,0,14),'cmpwi r14,0');branch('loop','bne');branch('end')
 label('error');emit(d(14,3,0,-1),'li r3,-1')
 label('end');emit(d(58,14,1,16),'ld r14,16(r1)');emit(d(32,0,1,68),'lwz r0,68(r1)');emit(spr(0,8,True),'mtlr r0');emit(d(14,1,1,64),'addi r1,r1,64');emit(0x4e800020,'blr')
 start('step');emit(d(14,3,3,1),'addi r3,r3,1');emit(0x4e800020,'blr')
 start('forever');label('again');branch('test_step','bl');branch('again')
 for at,name,kind in pending:
  delta=labels[name]-at;words[at//4]=(0x48000001 if kind=='bl' else 0x48000000)|(delta&0x3fffffc) if kind in ('bl','b') else (0x41820000 if kind=='beq' else 0x40820000)|(delta&0xfffc)
 # Independent decoder census: every backward edge targets a call-safe point.
 backedges=[];calls=[]
 for i,w in enumerate(words):
  op=w>>26
  if op in (16,18):
   bits=16 if op==16 else 26;delta=w&(0xfffc if op==16 else 0x3fffffc);delta-=1<<bits if delta&(1<<(bits-1)) else 0
   if w&1:calls.append(BASE+i*4)
   elif delta<=0:
    target=i*4+delta
    if words[target//4]>>26!=18 or not words[target//4]&1:raise ValueError('unpolled loop backedge')
    backedges.append(BASE+i*4)
 (path/'foundation.bin').write_bytes(b''.join(struct.pack('>I',w) for w in words))
 (path/'foundation.map').write_text(''.join(f'{off:08x} T test_{name}\n' for name,off in functions.items()))
 (path/'foundation.s').write_text('\n'.join(listing)+'\n')
 meta={'base':BASE,'functions':{n:BASE+off for n,off in functions.items()},'instructions':len(words),'words':words,'backedges':backedges,'calls':calls}
 (path/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n');return meta
