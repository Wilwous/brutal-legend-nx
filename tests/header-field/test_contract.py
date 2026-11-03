#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original XEX-like bytes; absent field is not malformed memory or a provider."""
import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
from xex_header_contract import lookup_selected, selected_field
KEY = 0x20401


def make(records):
    return struct.pack('>6I', 0x58455832, 1, 24+8*len(records), 0, 0, len(records))+b''.join(struct.pack('>II', *r) for r in records)


def oracle(blob):
    """Separate literal record scan; expected offset/value, no runtime semantics."""
    magic, _, size, _, _, count = struct.unpack('>6I', blob[:24])
    if magic != 0x58455832 or size != len(blob) or size != 24+count*8: raise ValueError('fixture shape')
    found = []
    for offset in range(24, size, 8):
        key, payload = struct.unpack('>II', blob[offset:offset+8])
        if key == KEY: found.append((offset+4, payload))
    if len(found) > 1: raise ValueError('ambiguous')
    return found[0] if found else None


class HeaderContract(unittest.TestCase):
    def test_five_distinct_valid_record_cases(self):
        cases = [([], None), ([(0x10000, 7)], None),
                 ([(KEY, 0x12345678), (0x10000, 7)], (28, 0x12345678)),
                 ([(0x10000, 7), (KEY, 0x12345678)], (36, 0x12345678)),
                 ([(KEY, 0)], (28, 0))]
        for records, expected in cases:
            with self.subTest(records=records):
                b = make(records); self.assertEqual(oracle(b), expected)
                x = lookup_selected(b)
                self.assertFalse(x['execution_admitted'])
                self.assertEqual(x['state'], 'ABSENT' if expected is None else 'PRESENT')
                self.assertEqual((x['offset'], x['value']), (None, None) if expected is None else expected)

    def test_absent_is_not_guest_null_allocation(self):
        x = lookup_selected(make([]))
        self.assertEqual((x['width'], x['kind']), (0, 'absent'))
        self.assertIsNone(x['offset']); self.assertIsNone(x['value'])

    def test_t14_provider_policy_still_refuses_absence(self):
        with self.assertRaisesRegex(ValueError, 'missing or duplicate'): selected_field(make([]))

    def test_duplicate_policy_refuses_both_oracles(self):
        b = make([(KEY, 9), (KEY, 10)])
        for scan in (oracle, lookup_selected):
            with self.assertRaises(ValueError): scan(b)

    def test_same_prefix_wrong_representation_is_different_full_key(self):
        x = lookup_selected(make([(KEY & ~255, 0x12345678)]))
        self.assertEqual(x['state'], 'ABSENT')
        with self.assertRaisesRegex(ValueError, 'unsupported selector'): lookup_selected(make([]), KEY & ~255)

    def test_wrong_endian_magic_refuses(self):
        b = struct.pack('<6I', 0x58455832, 1, 32, 0, 0, 1)+struct.pack('<II', KEY, 4)
        with self.assertRaisesRegex(ValueError, 'header magic'): lookup_selected(b)

    def test_wrong_endian_record_is_not_matching_key(self):
        b = make([]); b = b[:8]+struct.pack('>I', 32)+b[12:20]+struct.pack('>I', 1)+struct.pack('<II', KEY, 4)
        self.assertEqual(lookup_selected(b)['state'], 'ABSENT')

    def test_all_truncated_lengths_refuse(self):
        b = make([(KEY, 1)])
        for n in range(len(b)):
            with self.subTest(n=n), self.assertRaises(ValueError): lookup_selected(b[:n])

    def test_count_overflow_refuses(self):
        b = make([])[:20]+b'\xff'*4
        with self.assertRaisesRegex(ValueError, 'directory extent'): lookup_selected(b)

    def test_length_overflow_refuses(self):
        b = bytearray(make([])); b[8:12] = b'\xff'*4
        with self.assertRaisesRegex(ValueError, 'header extent'): lookup_selected(bytes(b))

    def test_count_crosses_header_even_with_backing_padding(self):
        b = make([])[:20]+struct.pack('>I', 1)+bytes(8)
        with self.assertRaisesRegex(ValueError, 'directory extent'): lookup_selected(b)

    def test_no_module_claim_or_status_code_is_returned(self):
        for records in ([], [(KEY, 0)]):
            x = lookup_selected(make(records))
            self.assertFalse(x['execution_admitted'])
            self.assertNotIn('module_admitted', x); self.assertNotIn('Xbox_status', x)


if __name__ == '__main__': unittest.main()
