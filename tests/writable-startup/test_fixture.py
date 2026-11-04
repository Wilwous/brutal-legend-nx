#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent exact write-site and initialization oracle on original data."""
import importlib.util,tempfile,struct,unittest
from pathlib import Path
from fixture import create,ROOT
spec=importlib.util.spec_from_file_location('decoder',ROOT/'tests/horizon-acceptance/run.py');decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)
class OriginalWrites(unittest.TestCase):
 def test_encoded_effects(self):
  with tempfile.TemporaryDirectory() as td:
   p=Path(td)/'input';m=create(p);words=m['words'];b=(p/'fixture.xex').read_bytes();assembly=[decoder.decode(w,0x82010000+4*i).strip() for i,w in enumerate(words)]
   self.assertEqual(len(words),34);self.assertEqual(len(m['hooks']),34);self.assertEqual(len(m['functions']),5)
   self.assertEqual(assembly[20:22],['stw r3,0(r14)','stw r3,4(r14)']);self.assertEqual(words[20:22],[0x906e0000,0x906e0004])
   self.assertEqual(b[0x21020:0x21028],bytes(8));self.assertEqual(struct.pack('>II',21+7+3,21+7+3),bytes([0,0,0,31,0,0,0,31]))
   self.assertEqual(sum(s['kind'] in ['read','write'] for s in m['sites']),13)
if __name__=='__main__':unittest.main()
