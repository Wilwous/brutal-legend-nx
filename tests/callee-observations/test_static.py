#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original static byte arrays only. No PPC generation, compilation or execution."""
import hashlib
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
import callee_observations as obs


def d(op, rt, ra, imm):
    return op << 26 | rt << 21 | ra << 16 | (imm & 65535)


def raw(words):
    return b''.join(w.to_bytes(4, 'big') for w in words)


def analyze(words, **kw):
    data = raw(words)
    args = dict(raw=data, base=0x1000, entry=0x1000, end=0x1000+len(data),
                expected_sha256=hashlib.sha256(data).hexdigest())
    args.update(kw)
    return obs.inspect(**args)


class StaticObservations(unittest.TestCase):
    def test_unsigned_immediate_and_CR_field(self):
        r = obs.decode(0x1000, d(10, 4 << 2, 9, 0xfffe))
        self.assertEqual(r['compare'], {'field': 4, 'reg': 9, 'unsigned': True, 'immediate': 65534})
        self.assertIn('XER.SO', r['reads'])
        with self.assertRaises(ValueError):
            obs.decode(0x1000, d(10, 1, 9, 0))

    def test_arbitrary_eq_field_branch(self):
        r = obs.decode(0x1000, d(16, 12, 18, 8))
        self.assertEqual((r['target'], r['bo'], r['bi']), (0x1008, 12, 18))
        self.assertEqual(r['reads'], ['CR4.2'])

    def test_update_store_has_old_payload(self):
        r = analyze([d(62, 1, 1, -31), 0x4e800020])
        m = r['frontiers'][0]['memory'][0]
        self.assertEqual(m['address'], {'symbol': 'r1', 'offset': -32, 'bits': 32})
        self.assertEqual(m['value']['symbol'], 'r1')
        self.assertEqual(r['frontiers'][0]['registers']['r1']['offset'], -32)
        with self.assertRaises(ValueError):
            obs.decode(0x1000, d(62, 7, 0, 9))

    def test_call_is_not_an_implicit_success_summary(self):
        r = analyze([0x48000101, d(14, 3, 0, 17), 0x4e800020])
        self.assertEqual(r['structural_site_count'], 3)
        self.assertEqual(r['call_free_prefix_sites'], [0x1000])
        self.assertEqual(r['frontiers'][0]['reason'], 'unproved call barrier')
        self.assertEqual(r['frontiers'][0]['call_continuation'], 0x1004)
        self.assertTrue(r['local_CFG'][0]['requires_unproved_call_return'])
        self.assertEqual(r['frontiers'][0]['registers']['r3']['symbol'], 'r3')

    def test_pointer_argument_is_an_obligation_not_a_provider_summary(self):
        r = analyze([d(37, 1, 1, -64), d(14, 8, 1, 24), 0x48000101, 0x4e800020])
        value = r['frontiers'][0]['registers']['r8']
        self.assertEqual((value['symbol'], value['offset']), ('r1', -40))
        self.assertFalse(r['call_summaries_assumed'])
        self.assertNotIn(0x100c, r['call_free_prefix_sites'])

    def test_saved_image_bytes_are_not_live_load_values(self):
        r = analyze([d(32, 6, 0, 0x1008), 0x4e800020, 0x60000000, 0x4e800020])
        value = r['frontiers'][0]['registers']['r6']
        self.assertTrue(value['symbol'].startswith('memory32@'))
        self.assertNotEqual(value['offset'], 0x60000000)

    def test_unknown_compare_keeps_both_guarded_paths(self):
        words = [d(10, 4 << 2, 7, 0), d(16, 12, 18, 8),
                 0x48000101, 0x4e800020]
        r = analyze(words)
        self.assertEqual(len(r['frontiers']), 2)
        self.assertEqual({f['reason'] for f in r['frontiers']},
                         {'unproved call barrier', 'conditional local return; external memory/context obligations remain'})
        self.assertTrue(all(f['guards'] for f in r['frontiers']))

    def test_known_unsigned_compare_does_not_sign_extend(self):
        words = [d(14, 7, 0, -1), d(10, 0, 7, 0xffff),
                 d(16, 12, 0, 8), 0x4e800020, 0x48000101, 0x4e800020]
        r = analyze(words)
        self.assertEqual(len(r['frontiers']), 1)
        self.assertEqual(r['frontiers'][0]['pc'], 0x100c)

    def test_constant_count_update_loop(self):
        # Original toy: two 8-byte zero writes at entrySP+40 and +48.
        mtctr = 31 << 26 | 5 << 21 | 9 << 16 | 467 << 1
        words = [d(14, 5, 0, 2), mtctr, d(14, 6, 1, 32), d(14, 7, 0, 0),
                 d(62, 7, 6, 9), d(16, 16, 0, -4), 0x4e800020]
        r = analyze(words);f = r['frontiers'][0]
        self.assertEqual([m['address']['offset'] for m in f['memory']], [40, 48])
        self.assertEqual(f['route'].count(0x1010), 2)
        self.assertEqual(f['registers']['CTR']['offset'], 0)

    def test_unknown_loop_has_a_hard_budget(self):
        with self.assertRaisesRegex(ValueError, 'budget'):
            analyze([d(16, 16, 0, 0), 0x4e800020], step_budget=8, path_budget=4)

    def test_local_saved_value_is_truncated_at_store_width(self):
        mflr = 31 << 26 | 12 << 21 | 8 << 16 | 339 << 1
        mtlr = 31 << 26 | 12 << 21 | 8 << 16 | 467 << 1
        words = [mflr, d(36, 12, 1, -12), d(32, 12, 1, -12), mtlr, 0x4e800020]
        r = analyze(words)
        self.assertIn('low32(LR', r['frontiers'][0]['registers']['LR']['symbol'])
        known = analyze(words, initial_lr=0x2000)
        self.assertEqual(known['frontiers'][0]['registers']['LR']['offset'], 0x2000)
        self.assertFalse(known['execution_admitted'])

    def test_unknown_store_invalidates_saved_slot(self):
        mflr = 31 << 26 | 12 << 21 | 8 << 16 | 339 << 1
        words = [mflr, d(36, 12, 1, -12), d(36, 7, 8, 0),
                 d(32, 12, 1, -12), 0x4e800020]
        r = analyze(words)
        self.assertTrue(r['frontiers'][0]['registers']['r12']['symbol'].startswith('memory32@'))

    def test_unsupported_transfer_does_not_fall_through(self):
        r = analyze([0x4e800421, d(14, 3, 0, 1), 0x4e800020])
        self.assertEqual(r['call_free_prefix_sites'], [0x1000])
        self.assertEqual(r['frontiers'][0]['reason'], 'unsupported computed transfer')

    def test_no_fabricated_carry_result(self):
        subfe = 31 << 26 | 3 << 21 | 6 << 16 | 7 << 11 | 136 << 1
        r = analyze([d(12, 6, 7, -1), subfe, 0x4e800020])
        self.assertTrue(r['frontiers'][0]['registers']['r3']['symbol'].startswith('opaque_'))

    def test_hash_range_decode_and_budget_refusals(self):
        for kw in ({'expected_sha256': '0'*64}, {'site_budget': 1},
                   {'end': 0x1009}, {'step_budget': 0}, {'path_budget': 0}):
            with self.assertRaises(ValueError):
                analyze([0x60000000, 0x4e800020], **kw)
        for words in ([0], [0x48000008, 0x4e800020], [0x60000000]):
            with self.assertRaises(ValueError):
                analyze(words)

    def test_input_context_read_set(self):
        words = [d(62, 26, 1, -16), d(10, 0, 6, 0), 0x4e800020]
        r = analyze(words)
        self.assertEqual(r['frontiers'][0]['read_before_definition'], ['LR', 'XER.SO', 'r1', 'r26', 'r6'])


class PackedMetadata(unittest.TestCase):
    def table(self, entries):
        data = bytearray(1024)
        for i, (start, packed) in enumerate(entries):
            data[64+i*8:72+i*8] = start.to_bytes(4, 'big')+packed.to_bytes(4, 'big')
        return bytes(data)

    def test_exact_entry_and_extent(self):
        data = self.table([(0x1080, 0x40000802), (0x10c0, 0x40000401)])
        e = obs.pdata_entry(data, 0x1000, 64, 16, 0x1080)
        self.assertEqual((e['entry'], e['end'], e['prolog_words']), (0x1080, 0x10a0, 2))

    def test_nearest_symbol_is_not_entry_authority(self):
        data = self.table([(0x1080, 0x40000802)])
        with self.assertRaises(ValueError):
            obs.pdata_entry(data, 0x1000, 64, 8, 0x1084)

    def test_duplicate_or_invalid_packed_extent(self):
        for entries in ([(0x1080, 0x40000802)]*2, [(0x1080, 0x40000000)],
                        [(0x1080, 0x40000102)], [(0x1080, 0x400fffff)],
                        [(0x1080, 0x00000802)], [(0x1080, 0xc0000802)]):
            with self.assertRaises(ValueError):
                obs.pdata_entry(self.table(entries), 0x1000, 64, len(entries)*8, 0x1080)

    def test_directory_bounds(self):
        data = self.table([(0x1080, 0x40000802)])
        for rva, size in ((-8, 8), (65, 8), (64, 9), (64, 0), (1024, 8)):
            with self.assertRaises(ValueError):
                obs.pdata_entry(data, 0x1000, rva, size, 0x1080)

    def test_source_coordinate_overflow(self):
        data = self.table([(0x1080, 0x40000802)])
        for base in (-1, 0xfffffff0):
            with self.assertRaises(ValueError):
                obs.pdata_entry(data, base, 64, 8, 0x1080)


if __name__ == '__main__':
    unittest.main()
