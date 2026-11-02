# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original nested-frame fixtures; accepts no external PPC input."""
import hashlib
import json
import struct
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import bounded_cfg as cfg

BASE = 0x82010000
MODES = ('relative', 'tls', 'persistent', 'integer', 'returned', 'absolute', 'indirect')


def create(path):
    path.mkdir()
    result = []
    def d(op, rt, ra, imm):
        return op << 26 | rt << 21 | ra << 16 | (imm & 65535)
    def mr(dst, src):
        return 31 << 26 | src << 21 | dst << 16 | src << 11 | 444 << 1
    def lr(reg, write=False):
        return 31 << 26 | reg << 21 | 8 << 16 | (467 if write else 339) << 1
    for mode in MODES:
        root = [lr(15), mr(18, 1), d(37, 1, 1, -64), mr(20, 1), 0,
                d(32, 9, 1, 0), mr(1, 9), lr(15, True), 0x4e800020]
        helper = [lr(16), d(37, 1, 1, -96), mr(21, 1), 0,
                  d(32, 1, 1, 0), lr(16, True), 0x4e800020]
        leaf = [d(37, 1, 1, -32), d(32, 5, 1, 0), d(32, 6, 5, 0),
                d(32, 7, 6, 0), mr(8, 5), d(36, 22, 5, 8), d(32, 3, 8, 8),
                d(36, 5, 1, 4), d(32, 10, 1, 4), d(32, 11, 10, 8)]
        if mode == 'tls':
            leaf += [d(32, 12, 13, 0), d(32, 3, 12, 32)]
        elif mode == 'persistent':
            leaf += [d(36, 1, 4, 0)]
        elif mode == 'integer':
            leaf += [d(14, 3, 1, 7)]  # raw scalar observer, not a renamed pointer
        elif mode == 'returned':
            leaf += [mr(3, 1)]  # recorded, never dereferenced after frame exit
        elif mode == 'absolute':
            leaf += [d(15, 9, 1, -4096), d(11, 0, 9, 0x4e40),
                     d(14, 3, 0, 0), 0x40820008, d(14, 3, 0, 1)]
        elif mode == 'indirect':
            leaf += [0x4e800421]  # entire program refused before entry
        leaf += [d(32, 1, 1, 0), 0x4e800020]
        h = len(root)*4
        l = h + len(helper)*4
        root[4] = 0x48000001 | (h-16)
        helper[3] = 0x48000001 | (l-(h+12))
        words = root + helper + leaf
        raw = b''.join(struct.pack('>I', word) for word in words)
        rows = [cfg.decode(BASE+4*i, word) for i, word in enumerate(words)]
        (path/(mode+'.bin')).write_bytes(raw)
        (path/(mode+'.map')).write_text(f'00000000 T test_entry\n{h:08x} T test_helper\n{l:08x} T test_leaf\n')
        labels = {0: 'test_entry', h: 'test_helper', l: 'test_leaf'}
        (path/(mode+'.s')).write_text(''.join((labels[i*4]+':\n' if i*4 in labels else '')+'    '+row['text']+'\n' for i, row in enumerate(rows)))
        result.append({'mode': mode, 'entry': BASE, 'helper': BASE+h, 'leaf': BASE+l,
                       'root_return': BASE+20, 'helper_return': BASE+h+16,
                       'rows': rows, 'instructions': len(rows),
                       'sha256': hashlib.sha256(raw).hexdigest(),
                       'relocatable': mode in ('relative', 'tls')})
    (path/'fixtures.json').write_text(json.dumps(result, indent=2)+'\n')
    return result
