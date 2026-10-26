#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent original XEX/word facts. No SDK, device access or guest execution."""
import hashlib,struct,tempfile,unittest
from pathlib import Path
from fixture import create,ROOT
from run import decode
class Encoding(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.p=Path(self.temp.name);self.m=create(self.p/'a',ROOT/'tests/horizon-xex/templates');self.b=(self.p/'a/fixture.xex').read_bytes()
 def be(self,off):return struct.unpack_from('>I',self.b,off)[0]
 def le(self,off):return struct.unpack_from('<I',self.b,0x1000+off)[0]
 def test_identity(self):self.assertEqual(hashlib.sha256(self.b).hexdigest(),'906a552244b03a2ad16e75a51e3087625b2f296850afeebec6eef5cb593496c0')
 def test_repeat(self):
  create(self.p/'b',ROOT/'tests/horizon-xex/templates');self.assertEqual(self.b,(self.p/'b/fixture.xex').read_bytes())
 def test_residency(self):self.assertEqual((len(self.b),self.be(8),self.be(0x104),self.le(0x98+56)),(0x41000,0x1000,0x40000,0x40000))
 def test_sections(self):
  self.assertEqual([self.le(0x98+224+i*40+12) for i in range(4)],[0,65536,131072,196608]);self.assertEqual([self.le(0x98+224+i*40+36) for i in range(4)],[0x40000040,0x60000020,0x40000040,0xc0000040])
 def test_instructions(self):
  self.assertEqual(len(self.m['words']),65);ops=[decode(w,self.m['code_base']+i*4) for i,w in enumerate(self.m['words'])];self.assertEqual(ops.count('bctrl'),1);self.assertIn('bl 0x82030004',ops);self.assertIn('lwz r6,84(r1)',ops)
 def test_tls(self):self.assertEqual([self.be(0x380+i*4) for i in range(4)],[64,0x82020000,32,16])
 def test_import(self):self.assertEqual((self.be(0x400),self.be(0x31000),self.be(0x31004)),(0x4c,0x11b,0x100011b));self.assertEqual(self.b[0x442:0x444],b'\0\2')
 def test_hooks_and_pdata(self):
  self.assertEqual(self.m['hooks'],[0x82010048,0x82010074]);self.assertEqual(self.le(0x98+100+24),48);self.assertEqual([self.be(0x21100+i*8) for i in range(6)],list(self.m['functions'].values()))
if __name__=='__main__':unittest.main()
