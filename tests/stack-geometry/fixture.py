# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original toy stack program, not a copied title prefix or startup context.

Store order, source registers and frame construction differ deliberately. The
production test generator fixes the base below. Geometry is an explicit test
parameter; the independent C++ byte oracle does not use these encoded words.
"""
import hashlib,json,struct,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
import bounded_cfg as cfg
import startup_dependencies as dep
BASE=0x82010000

def create(path):
    path.mkdir()
    def d(op,rt,ra,imm):return op<<26|rt<<21|ra<<16|(imm&65535)
    def spr(rt,write):return 31<<26|rt<<21|8<<16|(467 if write else 339)<<1
    # Split frame address calculation/store/base update. No title STWU is used.
    words=[d(14,10,1,-496),spr(7,False),0x48000019,d(36,1,10,0),
           31<<26|10<<21|1<<16|10<<11|444<<1,d(14,1,1,496),spr(7,True),0x4e800020,
           d(62,22,1,-24),d(62,20,1,-40),d(36,7,1,-8),
           d(62,23,1,-16),d(62,21,1,-32),0x4e800020]
    raw=b''.join(struct.pack('>I',w) for w in words)
    rows=[cfg.decode(BASE+4*i,w) for i,w in enumerate(words)]
    # Normal abstract walk ends before explicit root-LR restore/root return;
    # both remaining words are independently decoded and checked in codegen.
    pcs=[r['pc'] for i,r in enumerate(rows) if i not in (6,7)]
    proof=dep.analyze(raw,BASE,BASE,pcs,BASE+24,hashlib.sha256(raw).hexdigest())
    stores=[m for m in proof['memory'] if m['direction']=='write']
    if len(stores)!=6 or any(m['address']['base']!='r1' for m in stores):raise ValueError('unknown fixture footprint')
    if proof['stack_intervals']!=[[-496,-492],[-40,-4]] or proof['stack_distinct_bytes']!=40:raise ValueError('geometry drift')
    (path/'geometry.bin').write_bytes(raw)
    (path/'geometry.map').write_text('00000000 T test_entry\n00000020 T test_saves\n')
    (path/'geometry.s').write_text('test_entry:\n'+''.join('    '+r['text']+'\n'+('test_saves:\n' if i==7 else '') for i,r in enumerate(rows)))
    meta={'base':BASE,'entry':BASE,'helper':BASE+32,'instructions':14,'functions':2,'stores':6,
          'store_widths':[m['width'] for m in stores],
          'footprint':[{'offset':m['address']['amount'],'width':m['width']} for m in stores],
          'distinct_bytes':40,'envelope':[-496,-4],'raw_sha256':hashlib.sha256(raw).hexdigest(),
          'normal_static_sites':12,'normal_static_stop':BASE+24,'rows':rows,'source_kind':'independently authored original synthetic PPC'}
    (path/'fixture.json').write_text(json.dumps(meta,indent=2)+'\n')
    return meta
