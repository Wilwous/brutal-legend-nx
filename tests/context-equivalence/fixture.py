# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Five original synthetic PPC variants. Never accepts title input."""
import hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
import bounded_cfg as cfg
BASE=0x82010000
MODES=('relative','tls','fixed','escape','indirect')

def create(path):
    path.mkdir();result=[]
    def d(op,rt,ra,imm):return op<<26|rt<<21|ra<<16|(imm&65535)
    def spr(rt,write):return 31<<26|rt<<21|8<<16|(467 if write else 339)<<1
    for mode in MODES:
        words=[d(14,10,1,-496),spr(7,False),0]
        if mode=='fixed':
            # Absolute test: (S - fixed 0x10000000) == fixed 0x4f00.
            words += [d(15,9,1,-4096),d(11,0,9,0x4f00),d(14,3,0,0),0x40820008,d(14,3,0,1)]
        if mode=='escape':words += [d(36,1,4,0)]
        if mode=='indirect':words += [0x4e800421]
        if mode=='tls':words += [31<<26|6<<21|3<<16|6<<11|444<<1]
        words += [d(36,1,10,0),31<<26|10<<21|1<<16|10<<11|444<<1,d(14,1,1,496),spr(7,True),0x4e800020]
        helper=len(words)*4
        if mode=='tls':words += [d(32,5,13,0),d(32,6,5,32)]
        words += [d(62,22,1,-24),d(62,20,1,-40),d(36,6 if mode=='tls' else 7,1,-8),d(62,23,1,-16),d(62,21,1,-32),0x4e800020]
        words[2]=0x48000001|(helper-8)
        raw=b''.join(struct.pack('>I',x) for x in words);rows=[cfg.decode(BASE+4*i,x) for i,x in enumerate(words)]
        (path/(mode+'.bin')).write_bytes(raw)
        (path/(mode+'.map')).write_text(f'00000000 T test_entry\n{helper:08x} T test_helper\n')
        (path/(mode+'.s')).write_text('test_entry:\n'+''.join(('test_helper:\n' if i*4==helper else '')+'    '+r['text']+'\n' for i,r in enumerate(rows)))
        result.append({'mode':mode,'entry':BASE,'helper':BASE+helper,'rows':rows,'sha256':hashlib.sha256(raw).hexdigest(),'stores':sum(bool(r['memory'] and r['memory']['write']) for r in rows),'instructions':len(rows),'relocatable_contract':mode in ('relative','tls')})
    (path/'fixtures.json').write_text(json.dumps(result,indent=2)+'\n');return result
