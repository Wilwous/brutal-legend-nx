#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Eight distinct original cases for actual SDK and optional host-parser comparison."""
import struct
KEY = 0x20401


def cases():
    records = [('empty', []), ('missing', [(0x10000, 7)]),
               ('first', [(KEY, 0x12345678), (0x10000, 7)]),
               ('last', [(0x10000, 7), (KEY, 0x12345678)]),
               ('zero', [(KEY, 0)]), ('different-kind', [(KEY & ~255, 0x12345678)]),
               ('duplicate', [(KEY, 9), (KEY, 10)]), ('record-little-endian', [(KEY, 4)])]
    result = []
    for name, rows in records:
        blob = struct.pack('>6I', 0x58455832, 1, 24+8*len(rows), 0, 0, len(rows))
        blob += b''.join(struct.pack('<II' if name == 'record-little-endian' else '>II', *x) for x in rows)
        result.append((name, blob))
    return result


def first_match_oracle(blob):
    """Literal separate scan: SDK-like first record, source offset; not Xbox proof."""
    count = struct.unpack_from('>I', blob, 20)[0]
    for index in range(count):
        off = 24+8*index
        key, value = struct.unpack_from('>II', blob, off)
        if key == KEY: return off+4, value
    return None
