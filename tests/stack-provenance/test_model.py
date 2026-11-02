# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import tempfile
import unittest
from pathlib import Path
from model import Object, Relation, frame_access, nested_fields, normalize
from fixture import create, MODES
from verify import verify


class ProofTests(unittest.TestCase):
    def relation(self):
        return Relation({'stack': Object(0x10000, 4096), 'fixed': Object(0x30000, 16)},
                        {'stack': Object(0x20000, 4096), 'fixed': Object(0x30000, 16)})

    def test_alias_and_same_object_order(self):
        r = self.relation()
        for a in (0, 64, 4096):
            for b in (0, 64, 4096):
                self.assertTrue(r.alias(('stack', a), ('stack', b)))
                self.assertTrue(r.ordered(('stack', a), ('stack', b)))
        self.assertTrue(r.alias(('stack', 0), ('fixed', 0)))

    def test_cross_object_order_denied(self):
        with self.assertRaises(ValueError):
            self.relation().ordered(('stack', 0), ('fixed', 0))

    def test_one_past_cross_object_alias_not_assumed(self):
        r = Relation({'a': Object(0x10000, 16), 'b': Object(0x10010, 16)},
                     {'a': Object(0x20000, 16), 'b': Object(0x30000, 16)})
        self.assertFalse(r.alias(('a', 16), ('b', 0)))

    def test_raw_integer_and_fixed_identity(self):
        r = self.relation()
        self.assertFalse(r.raw_integer_equal('stack', 96))
        self.assertTrue(r.raw_integer_equal('fixed', 4))

    def test_bounds_and_one_past(self):
        r = self.relation()
        self.assertEqual(r.pointer('stack', 4096), (0x11000, 0x21000))
        for offset, width in ((4096, 4), (-1, 0), (4094, 4)):
            with self.assertRaises(ValueError):
                r.pointer('stack', offset, width=width)

    def test_invalid_object_contracts(self):
        for bad in (Object(0x10001, 4096), Object(0x10000, 0),
                    Object(0xfffffff0, 32), Object(0x10000, 4096, live=False),
                    Object(0x10000, 4096, writable=False), Object(0x10000, 4096, epoch=2)):
            with self.assertRaises(ValueError):
                Relation({'a': Object(0x10000, 4096)}, {'a': bad})
        with self.assertRaises(ValueError):
            Relation({'a': Object(0x10000, 4096), 'b': Object(0x10010, 16)},
                     {'a': Object(0x20000, 4096), 'b': Object(0x30000, 16)})

    def test_nested_graph_normalization(self):
        outputs = []
        for base in (0x10000, 0x20000):
            data = bytearray(b'\xa5'*4096)
            for field, target in nested_fields(2048):
                data[field:field+4] = (base+target).to_bytes(4, 'big')
            # Opaque address-looking payload is NOT renamed.
            data[2300:2304] = (0x10200).to_bytes(4, 'big')
            outputs.append(normalize(data, base, nested_fields(2048)))
        self.assertEqual(*outputs)
        self.assertEqual(outputs[0][2300:2304], (0x10200).to_bytes(4, 'big'))

    def test_forged_graph_denied(self):
        with self.assertRaises(ValueError):
            normalize(bytes(4096), 0x10000, nested_fields(2048))
        data = bytearray(4096);data[0:4] = (0x10008).to_bytes(4, 'big')
        with self.assertRaises(ValueError):
            normalize(data, 0x10000, ((0, 8), (0, 8)))
        for base in (-16, 1, 0xfffffff0):
            with self.assertRaises(ValueError):
                normalize(bytes(4096), base, ())

    def test_frame_lifetime(self):
        self.assertTrue(frame_access(24, 16, 32, True))
        for pointer, active in ((24, False), (14, True), (32, True)):
            with self.assertRaises(ValueError):
                frame_access(pointer, 16, 32, active)

    def test_original_inputs(self):
        with tempfile.TemporaryDirectory() as t:
            a, b = create(Path(t)/'a'), create(Path(t)/'b')
            self.assertEqual(a, b)
            self.assertEqual(tuple(x['mode'] for x in a), MODES)
            for p in a:
                self.assertEqual(sum(r['kind']=='call' for r in p['rows']), 2)
                self.assertEqual(sum(r['kind']=='return' for r in p['rows']), 3)
                self.assertEqual(sum(bool(r['memory'] and r['memory']['update']) for r in p['rows']), 3)
            self.assertFalse(a[-1]['relocatable'])

    def test_observer_parser_refuses_invalid_extent(self):
        with tempfile.TemporaryDirectory() as t:
            p = Path(t)/'observations.bin'
            for raw in (b'', b'SPV1\0\0\0\x24', b'SPV1\0\0\0\x24'+bytes(80),
                        b'SPV1\0\0\0\x24'+bytes(1024*1024)):
                p.write_bytes(raw)
                with self.assertRaises(ValueError):
                    verify(p)


if __name__ == '__main__':
    unittest.main()
