#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import hashlib
import struct
import tempfile
import unittest
from pathlib import Path
from assets import content
from fixture import create
class Fixture(unittest.TestCase):
 def test_production_input_identity_and_census(self):
  with tempfile.TemporaryDirectory() as directory:
   p=Path(directory)/'fixture';meta=create(p);self.assertEqual(hashlib.sha256((p/'fixture.xex').read_bytes()).hexdigest(),'0daa13ce9b2ecf31cb9e9e74b633606f88c9e203ab8b73cf6e1f74047d318016')
   self.assertEqual((len(meta['functions']),len(meta['words']),len(meta['hooks'])),(3,281,281));self.assertEqual(sum(s['kind'] in ['read','write'] for s in meta['sites']),147);self.assertEqual(meta['command_bytes'],416)
   conditional=[word for word in meta['words'] if word>>26==16];self.assertEqual(len(conditional),1);self.assertEqual((conditional[0]>>21)&31,12)
   for word in meta['words']:
    if word>>26==21:self.assertEqual(((word>>11)&31,(word>>1)&31,word&1),(0,31,0))
 def test_independent_asset_layout_and_pcm(self):
  assets=content();self.assertEqual(sum(map(len,assets.values())),39240)
  scene=assets['scene.bin'];self.assertEqual(scene[:4],b'SCN2');self.assertEqual(struct.unpack_from('>7I',scene,4),(1,24,36,32,512,584,4))
  indices=struct.unpack_from('>36H',scene,512);self.assertEqual(max(indices),23);self.assertTrue(all(i<24 for i in indices));self.assertEqual(len(assets['checker.rgba']),256)
  for name,amplitude in [('ambient.pcm',1000),('action.pcm',3000)]:
   samples=struct.unpack('<9600h',assets[name]);self.assertEqual(samples[0],-amplitude);self.assertEqual(samples[::2],samples[1::2]);self.assertTrue(all(-amplitude<=n<=amplitude for n in samples))
 def test_original_asset_identity(self):
  expected={'scene.bin':'37161a7ac70abf67aa3d6580a85c0a7eb52627a6ecefe9420ba807e0f06477f0','checker.rgba':'997f83a6210747fbfaa0e9a95678ff0b110936b5003a602b3f3ef71c7ad75dea','ambient.pcm':'563fad8ac1562e2f52f9f0cd27bef2713cb7b3832c4e477bf0facf2fdc18e17c','action.pcm':'6ca22afc713cf089b221c98e6ab076ee70db175a1d5875b7101d61afa5ece042'}
  for name,data in content().items():self.assertEqual(hashlib.sha256(data).hexdigest(),expected[name])
if __name__=='__main__':unittest.main()
