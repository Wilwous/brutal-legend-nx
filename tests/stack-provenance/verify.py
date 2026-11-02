#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent observation oracle: no encoder, decoder or generated C++ imports."""
import argparse
import hashlib
import itertools
import json
import struct
from pathlib import Path
from model import Object, Relation, normalize, nested_fields, frame_access


def verify(path):
    raw = path.read_bytes()
    if len(raw) > 1024*1024 or raw[:8] != b'SPV1\0\0\0\x24':
        raise ValueError('snapshot header/budget')
    at, byte_checks = 8, 0
    rows = {}
    for _ in range(36):
        if at+80 > len(raw):
            raise ValueError('truncated record')
        values = struct.unpack_from('>20I', raw, at);at += 80
        mode, seed, pages, base, sp, pcr, tls, result, cell, size = values[:10]
        registers = values[10:]
        key = mode, seed, pages
        if (key in rows or mode not in range(6) or seed not in (0, 1)
                or pages not in (0, 1, 3) or size != 16384 or at+size > len(raw)):
            raise ValueError('invalid record identity/extent')
        data = raw[at:at+size];at += size
        assert base == 0x10001000+pages*4096 and sp == base+0x3f00
        assert pcr == base+16384 and tls == pcr+4096
        marker = 0x10203040 ^ seed
        expected = bytearray(b'\xa5'*16384)
        # Different serializer and literal specification; no fixture metadata.
        for offset, value in ((-64, sp), (-160, sp-64), (-192, sp-160),
                              (-188, sp-160), (-152, marker)):
            expected[sp-base+offset:sp-base+offset+4] = value.to_bytes(4, 'big')
        assert data == expected;byte_checks += size
        assert registers == (sp-160, sp-64, sp, sp-160, sp-160, marker,
                             sp, sp-64, sp-160, sp)
        assert result == ((0x55667788 if seed else 0x11223344) if mode == 1
                          else sp-185 if mode == 3 else sp-192 if mode == 4
                          else int(sp-192 == 0x10004e40) if mode == 5 else marker)
        assert cell == (sp-192 if mode == 2 else 0xdeadbeef)
        # The returned pointer names a dead logical frame even while the backing
        # allocation remains live. Numeric observation does not admit a load.
        if mode == 4:
            try:
                frame_access(result, sp-192, sp-160, False)
            except ValueError:
                pass
            else:
                raise AssertionError('dead frame admitted')
        rows[key] = {'raw': data, 'normalized': normalize(data, base, nested_fields(sp-base)),
                     'result': result, 'cell': cell, 'sp': sp, 'base': base,
                     'pointers': registers[:5]+registers[6:]}
    if at != len(raw):
        raise ValueError('snapshot trailer')
    equal = bad = raw_diff = graph_pairs = nonrefuting = 0
    witnesses = []
    for mode, seed in itertools.product(range(6), range(2)):
        for a, b in itertools.combinations((0, 1, 3), 2):
            x, y = rows[mode, seed, a], rows[mode, seed, b]
            relation = Relation({'stack': Object(x['base'], 16384)},
                                {'stack': Object(y['base'], 16384)})
            for px, py in zip(x['pointers'], y['pointers']):
                assert relation.pointer('stack', px-x['base']) == (px, py)
            assert x['pointers'][0] == x['pointers'][3] == x['pointers'][4]
            assert y['pointers'][0] == y['pointers'][3] == y['pointers'][4]
            graph_pairs += 1
            assert x['raw'] != y['raw'];raw_diff += 1
            same = all(x[k] == y[k] for k in ('normalized', 'result', 'cell'))
            if mode < 2:
                assert same;equal += 1
            elif mode != 5 or a == 0:
                assert not same;bad += 1
                witnesses.append({'mode': mode, 'seed': seed, 'sp': [x['sp'], y['sp']],
                                  'result': [x['result'], y['result']], 'cell': [x['cell'], y['cell']]})
            else:
                assert same;nonrefuting += 1
    assert (equal, bad, raw_diff, graph_pairs, nonrefuting) == (12, 22, 36, 36, 2)
    return {'records': 36, 'independent_oracle_bytes': byte_checks,
            'equivalent_pairs': equal, 'counterexample_pairs': bad,
            'raw_stack_differences': raw_diff, 'pointer_graph_pairs': graph_pairs,
            'nonrefuting_absolute_pairs': nonrefuting, 'witnesses': witnesses,
            'snapshot_sha256': hashlib.sha256(raw).hexdigest(), 'game_input': False}


if __name__ == '__main__':
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('snapshot', type=Path)
    print(json.dumps(verify(ap.parse_args().snapshot)))
