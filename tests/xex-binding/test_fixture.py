#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Portable original XEX layout, PDATA, relocation and identity checks."""
import hashlib,struct,tempfile,unittest
from pathlib import Path
from fixture import create
class Fixture(unittest.TestCase):
 def setUp(self):
  self.t=tempfile.TemporaryDirectory();self.addCleanup(self.t.cleanup);self.p=Path(self.t.name);self.a=create(self.p/'a');self.b=create(self.p/'b',0x83000000);self.data=(self.p/'a/fixture.xex').read_bytes()
 def be(self,o):return struct.unpack_from('>I',self.data,o)[0]
 def le(self,o):return struct.unpack_from('<I',self.data,o)[0]
 def test_determinism(self):
  create(self.p/'repeat');self.assertEqual(self.data,(self.p/'repeat/fixture.xex').read_bytes());self.assertEqual(len(self.data),0x41000)
 def test_entry(self):
  self.assertEqual(self.be(44),self.a['entry']);self.assertEqual(self.a['entry'],0x820100c4);self.assertEqual(self.b['entry'],0x830100c4)
 def test_sections(self):
  sections=[]
  for i in range(4):
   p=0x1000+0x98+224+i*40;sections.append((self.le(p+12),self.le(p+8),self.le(p+36)))
  self.assertEqual(sections,[(0,65536,0x40000040),(65536,65536,0x60000020),(131072,65536,0x40000040),(196608,65536,0xc0000040)])
 def test_pdata(self):
  self.assertEqual(self.le(0x1000+0x98+96+3*8),0x20100)
  self.assertEqual([(self.be(0x21100+i*8),self.be(0x21104+i*8)) for i in range(3)],[(0x82010000,0x40002e00),(0x820100b8,0x40000300),(0x820100c4,0x40000e00)])
  self.assertNotIn('[entrypoint.functions]',(self.p/'a/fixture_manifest.toml').read_text())
 def test_import_relocations(self):
  words=struct.unpack('>63I',bytes(self.a['code']));targets=[]
  for i,w in enumerate(words):
   if w>>26==18:
    delta=w&0x3fffffc;delta=delta-(1<<26) if delta&(1<<25) else delta
    targets.append(self.a['code_base']+i*4+delta)
  self.assertEqual(targets,[0x820100b8,0x82030004,0x82030014,0x82010000])
 def test_tls(self):
  self.assertEqual([self.be(0x380+i*4) for i in range(4)],[64,0x82020000,32,16]);self.assertEqual(self.data[0x21000:0x21010],bytes(range(0x41,0x51)))
 def test_identity_changes(self):
  self.assertNotEqual(hashlib.sha256(self.data).digest(),hashlib.sha256((self.p/'b/fixture.xex').read_bytes()).digest())
  self.assertNotEqual(self.a['code'],self.b['code']);self.assertEqual(self.a['imports'][1]['thunk'],0x8203000c)
 def test_no_overwrite(self):
  with self.assertRaises(FileExistsError):create(self.p/'a')
  self.assertEqual((self.p/'a/fixture.xex').read_bytes(),self.data)
if __name__=='__main__':unittest.main()
