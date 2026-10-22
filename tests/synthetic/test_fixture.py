#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original encoding/sidecar assertions; no SDK or game input."""
import struct
import tempfile
import unittest
from pathlib import Path
from fixture import BASE, create
class Fixture(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.path=Path(self.tmp.name)/'input';self.meta=create(self.path)
        self.words=list(struct.unpack('>36I',(self.path/'module.bin').read_bytes()))
    def test_original_entry_and_big_endian(self):
        self.assertEqual(self.words[:4],[0x9421ffc0,0x7c0802a6,0x90010044,0xf9c10008])
        self.assertEqual(self.words[9],0xe9c10008)
    def test_direct_call_relative_displacement(self):
        word=self.words[6];self.assertEqual(word&0xfc000003,0x48000001)
        self.assertEqual(24+(word&0x3fffffc),self.meta['functions']['direct'])
    def test_indirect_tls_and_vector_encodings(self):
        self.assertEqual(self.words[8],0x4e800421)
        self.assertEqual(self.words[20],0x80cd0000)
        self.assertEqual(self.words[25],0x10831800)
    def test_relative_nm_map(self):
        rows=[line.split() for line in (self.path/'module.map').read_text().splitlines()]
        self.assertEqual([int(r[0],16) for r in rows],list(self.meta['functions'].values()))
        self.assertTrue(all(r[1]=='T' and r[2].startswith('test_') for r in rows))
        self.assertEqual(BASE,0x82010000)
    def test_sections_and_relocation(self):
        sections=self.meta['sections'];self.assertEqual([s['offset'] for s in sections],[0,16384,32768])
        self.assertTrue(all(s['size']==8192 for s in sections))
        self.assertEqual(self.meta['relocations'],[{'offset':32784,'target':16384,'type':'abs32be'}])
    def test_no_clobber(self):
        before=(self.path/'module.bin').read_bytes()
        with self.assertRaises(FileExistsError):create(self.path)
        self.assertEqual(before,(self.path/'module.bin').read_bytes())
if __name__=='__main__':unittest.main()
