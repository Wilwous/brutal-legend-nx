#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independently authored metadata fixtures; no proprietary inputs."""
from pathlib import Path
import struct
import sys
import unittest
import zlib

sys.path.insert(0, str(Path(__file__).resolve().parents[2]/'tools'))
from image_initialization import (span, compression_accounting, xdbf_entries,
                                  relocation_shape, binding_identity,
                                  demand_intervals, selected_pages, png_chunks,
                                  guest_to_image, permission_slices)


class Tests(unittest.TestCase):
    def test_span(self):
        span(8, 0, 8)
        for args in [(-1, 1, 8), (7, 2, 8), (0, -1, 8), (True, 0, 8)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                span(*args)

    def test_normal_no_implicit_zero(self):
        r = compression_accounting(2, [16, 16], 32, 32)
        self.assertEqual(r['explicit_basic_zero_ranges'], [])
        self.assertFalse(r['outside_extent_zero_authority'])

    def test_basic_explicit_zero(self):
        r = compression_accounting(1, [16], 16, 16, [(4, 4), (4, 0)])
        self.assertEqual(r['explicit_basic_zero_ranges'], [(4, 8)])
        self.assertEqual(r['unclassified_basic_remainder'], 4)

    def test_basic_union_not_normal(self):
        with self.assertRaises(ValueError):
            compression_accounting(2, [16], 16, 16, [(4, 12)])

    def test_descriptor_refusals(self):
        for args in [(2, [16], 32, 16), (2, [16], 16, 8),
                     (2, [0xffffffff, 1], 0x100000000, 0x100000000),
                     (3, [16], 16, 16), (2, [-1], -1, -1),
                     (1, [16], 16, 16, [(16, 1)])]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                compression_accounting(*args)

    def xdbf(self):
        return struct.pack('>6I', 0x58444246, 0x10000, 1, 1, 0, 0)+struct.pack('>HQII', 2, 7, 0, 4)+b'test'

    def test_resource_extent(self):
        r = xdbf_entries(self.xdbf())
        self.assertEqual((r['entries'][0]['start'], r['entries'][0]['end']), (42, 46))
        self.assertFalse(r['payload_grammar_verified'])

    def test_resource_bounds(self):
        for data in [self.xdbf()[:23], self.xdbf()[:-1],
                     struct.pack('>6I', 0x58444246, 0x10000, 0xffffffff, 1, 0, 0),
                     struct.pack('>6I', 0x58444246, 0x10000, 0, 1, 0, 0),
                     self.xdbf()[:4]+bytes(4)+self.xdbf()[8:]]:
            with self.subTest(size=len(data)), self.assertRaises(ValueError):
                xdbf_entries(data)

    def chunk(self, kind, data):
        return struct.pack('>I', len(data))+kind+data+struct.pack('>I', zlib.crc32(kind+data) & 0xffffffff)

    def png(self):
        return b'\x89PNG\r\n\x1a\n'+self.chunk(b'IHDR', struct.pack('>IIBBBBB', 1, 1, 8, 0, 0, 0, 0))+self.chunk(b'IDAT', zlib.compress(b'\0\0'))+self.chunk(b'IEND', b'')

    def test_png_container(self):
        rows = png_chunks(self.png())
        self.assertEqual([r['kind'] for r in rows], ['IHDR', 'IDAT', 'IEND'])
        self.assertEqual(rows[-1]['end'], len(self.png()))

    def test_png_container_refusals(self):
        bad = bytearray(self.png()); bad[20] ^= 1
        for data in [bytes(bad), self.png()[:-1], self.png()+b'x', self.png()[:33]]:
            with self.subTest(data=data), self.assertRaises(ValueError):
                png_chunks(data)

    def test_png_header_and_idat_refusals(self):
        signature = b'\x89PNG\r\n\x1a\n'
        ihdr = self.png()[8:33]; idat = self.chunk(b'IDAT', b'x'); end = self.chunk(b'IEND', b'')
        for data in [signature+ihdr+end,
                     signature+self.chunk(b'IHDR', struct.pack('>IIBBBBB', 0, 1, 8, 0, 0, 0, 0))+idat+end,
                     signature+ihdr+ihdr+idat+end,
                     signature+ihdr+idat+self.chunk(b'tEXt', b'')+idat+end]:
            with self.subTest(data=data), self.assertRaises(ValueError):
                png_chunks(data)

    def test_relocation_shape(self):
        data = struct.pack('<IIHH', 0x1000, 12, 0x3004, 0)
        r = relocation_shape(data, 0, len(data), 0x2000)
        self.assertEqual((r['blocks'], r['encoded_words'], r['fixups_applied']), (1, 2, 0))
        self.assertFalse(r['applicability_proven'])

    def test_relocation_refusals(self):
        for data in [struct.pack('<II', 0x1000, 6), struct.pack('<II', 0x1001, 8),
                     struct.pack('<II', 0x1000, 16), struct.pack('<II', 0x1000, 9), b'x']:
            with self.subTest(data=data), self.assertRaises(ValueError):
                relocation_shape(data, 0, len(data), 0x2000)

    def test_relocation_complete_extent_required(self):
        with self.assertRaises(ValueError):
            relocation_shape(bytes(8), 0, 12, 16)

    def test_relocation_target_bounds(self):
        with self.assertRaises(ValueError):
            relocation_shape(struct.pack('<IIHH', 0x1000, 12, 0x3fff, 0), 0, 12, 0x1001)

    def test_binding_identity(self):
        before = bytes.fromhex('00010010')+b'keep'
        r = binding_identity(before, b'ptr!keep', [dict(offset=0, ordinal=16, variable_only=True)])
        self.assertEqual((r['different_bytes'], r['unexplained_bytes']), (4, 0))
        self.assertFalse(r['historical_values_reusable'])

    def test_unexplained_binding_difference(self):
        r = binding_identity(bytes.fromhex('00010010')+b'x', b'ptr!y', [dict(offset=0, ordinal=16, variable_only=True)])
        self.assertEqual(r['unexplained_bytes'], 1)

    def test_binding_refusals(self):
        slot = dict(offset=0, ordinal=16, variable_only=True)
        for slots in [[slot, slot], [dict(slot, ordinal=17)], [dict(slot, variable_only=False)], [dict(slot, offset=4)]]:
            with self.subTest(slots=slots), self.assertRaises(ValueError):
                binding_identity(bytes.fromhex('00010010'), b'ptr!', slots)

    def test_binding_budget_before_iteration(self):
        class Oversized:
            def __len__(self):
                return 64*1024*1024+1
        with self.assertRaises(ValueError):
            binding_identity(Oversized(), Oversized(), [])

    def test_demand_partition(self):
        rows = demand_intervals(10, 50, 16, [(14, 17)])
        self.assertEqual(sum(r['bytes'] for r in rows), 40)
        self.assertEqual([r['normal_slice_references'] for r in rows], [1, 1, 0, 0])
        self.assertTrue(all(not r['proven_unused'] and not r['execution_admitted'] for r in rows))

    def test_demand_refusals(self):
        for args in [(0, 10, 3, []), (10, 10, 4, []), (0, 10, 4, [(1, 1)])]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                demand_intervals(*args)

    def test_selected_pages(self):
        self.assertEqual(selected_pages([(17, 18), (31, 33)], 16, 16, 64), [16, 32])

    def test_missing_selected_source_refusal(self):
        with self.assertRaises(ValueError):
            selected_pages([(17, 18)], 16, 17, 64)

    def test_page_budget_before_allocation(self):
        with self.assertRaises(ValueError):
            demand_intervals(0, 1 << 32, 1, [])

    def test_coordinate_translation(self):
        # Original-container offset64 and decoded offset16 are distinct.
        self.assertEqual(guest_to_image(0x1010, 0x1000, 4, 32), 16)
        self.assertNotEqual(guest_to_image(0x1010, 0x1000, 4, 32), 64)
        self.assertEqual(guest_to_image(0x1010, 0x1000, 4, 32), guest_to_image(0x1008, 0x1000, 8, 32)+8)

    def test_coordinate_refusals(self):
        for args in [(0xfff, 0x1000, 1, 32), (0x101f, 0x1000, 2, 32),
                     (0xffffffff, 0xfffffff0, 1, 32)]:
            with self.subTest(args=args), self.assertRaises(ValueError):
                guest_to_image(*args)

    def test_known_page_does_not_grant_section_gap(self):
        p = permission_slices(0x1000, 4096, [dict(start=0x1000, end=0x1ff0, flags=3)])
        self.assertFalse(p['single_section_covers_page'])
        self.assertEqual(p['undeclared_bytes'], 16)
        self.assertIsNone(p['slices'][-1]['flags'])
        self.assertTrue(all(not r['access_granted'] for r in p['slices']))

    def test_permission_overlap_refused(self):
        with self.assertRaises(ValueError):
            permission_slices(0x1000, 4096, [dict(start=0x1000, end=0x2000, flags=3)]*2)


if __name__ == '__main__':
    unittest.main()
