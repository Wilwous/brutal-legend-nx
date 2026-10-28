#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""SDK-free independent binary-layout checks of the original corpus."""
import hashlib,json,struct,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
class Corpus(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.tmp=tempfile.TemporaryDirectory();cls.w=Path(cls.tmp.name)
  commands=[(['clang++','-std=c++23','-O0','-iquote',str(ROOT),str(ROOT/'tests/loader-observation/corpus.cpp'),'-o',str(cls.w/'emit')],'build'),([str(cls.w/'emit'),str(cls.w/'a')],'first'),([str(cls.w/'emit'),str(cls.w/'b')],'second')]
  for cmd,label in commands:
   r=run(cmd,cls.w,label,30)
   if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed')
 @classmethod
 def tearDownClass(cls):cls.tmp.cleanup()
 def data(self,name):return (self.w/'a'/(name+'.xex')).read_bytes()
 def be(self,b,p):return struct.unpack_from('>I',b,p)[0]
 def le(self,b,p):return struct.unpack_from('<I',b,0x1000+p)[0]
 def test_deterministic(self):
  a={p.name:hashlib.sha256(p.read_bytes()).digest() for p in (self.w/'a').iterdir()};b={p.name:hashlib.sha256(p.read_bytes()).digest() for p in (self.w/'b').iterdir()};self.assertEqual(len(a),14);self.assertEqual(a,b)
 def test_locked_manifest(self):
  expected=json.loads((ROOT/'tests/loader-observation/fixture-manifest.json').read_text())['images'];self.assertEqual({n[:-4]:hashlib.sha256(self.data(n[:-4])).hexdigest() for n in (p.name for p in (self.w/'a').iterdir())},expected)
 def test_original_entry_marker(self):
  b=self.data('plain');self.assertEqual(self.be(b,44),0x82001000)
  self.assertEqual([self.be(b,p) for p in range(0x2000,0x2014,4)],[0x3c808202,0x60840040,0x3860006e,0x90640000,0x7fe00008]);self.assertEqual(b[0x21040:0x21044],bytes(4))
 def test_raw_zero_is_only_metadata(self):
  b=self.data('raw-zero');self.assertEqual(self.le(b,0x98+224+80+16),0);self.assertEqual(b[0x21000],0xa5)
 def test_raw_short_virtual_full(self):
  b=self.data('raw-short');self.assertEqual(self.le(b,0x98+224+80+16),512);self.assertEqual(self.le(b,0x98+224+80+8),65536)
 def test_explicit_owned_tail(self):
  b=self.data('owned-tail');self.assertEqual(len(b)-4096,4*65536);self.assertEqual(self.be(b,0x284+48),0x22);self.assertEqual(self.be(b,0x104),4*65536)
 def test_unowned_overhang(self):
  b=self.data('pe-overhang');self.assertEqual(len(b)-4096,3*65536);self.assertEqual(self.le(b,0x98+224+80+8),65537)
 def test_basic_modes_distinct(self):
  for name,zero in [('basic-zero',65536),('basic-gap',0)]:
   b=self.data(name);self.assertEqual(self.be(b,0x304)&65535,1);self.assertEqual(self.be(b,0x308),2*65536);self.assertEqual(self.be(b,0x30c),zero);self.assertEqual(len(b)-4096,2*65536)
 def test_descriptor_payload_mismatch(self):
  b=self.data('none-short');self.assertEqual(self.be(b,0x280),3);self.assertEqual(len(b)-4096,2*65536)
 def test_security_size_distinct(self):
  b=self.data('security-larger');self.assertEqual(self.be(b,0x104),4*65536);self.assertEqual(len(b)-4096,3*65536)
 def test_one_byte_rounding_case(self):
  b=self.data('rounded-owned');self.assertEqual(len(b)-4096,3*65536+1);self.assertEqual(b[-1],0x7b)
 def test_bad_entry_only_metadata(self):self.assertEqual(self.be(self.data('invalid-entry'),44),0x82040000)
 def test_rejections_keep_safe_headers(self):
  for n in ['overlap','no-read','bad-pe']:
   b=self.data(n);self.assertEqual(self.be(b,8),4096);self.assertEqual(len(b),0x31000)
if __name__=='__main__':unittest.main()
