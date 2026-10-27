#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import hashlib,struct,tempfile,unittest
from pathlib import Path
from fixture import create,x
from audit import admit_word
class Corpus(unittest.TestCase):
 def setUp(self):
  t=tempfile.TemporaryDirectory();self.addCleanup(t.cleanup);self.root=Path(t.name);self.m=create(self.root/'one',Path(__file__).parent/'templates')
 def test_shape(self):self.assertEqual((len(self.m['functions']),self.m['instructions'],len(self.m['memory_sites']),len(self.m['hooks'])),(16,138,34,68))
 def test_pdata_and_entry(self):
  b=(self.root/'one/fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>I',b,44)[0],self.m['entry']);self.assertEqual(struct.unpack_from('<I',b,0x1000+0x98+100+24)[0],128)
 def test_exact_new_words(self):
  i=(self.m['functions']['integer_flags']-self.m['code_base'])//4;self.assertEqual(self.m['words'][i:i+3],[0x7cc32014,0x7c662914,0x7c8407b4])
 def test_default_msr_carry_oracle(self):
  # Integer-wide oracle independent of the emitter's comparisons.
  a,b,c=0xffffffff,1,0;first=a+b;carry1=((a&0xffffffff)+(b&0xffffffff))//(1<<32);total=first+c+carry1
  self.assertEqual((first,carry1,total,((first&0xffffffff)+(c&0xffffffff)+carry1)//(1<<32)),(0x100000000,1,0x100000001,0))
 def test_literal_float_encoding(self):
  i=(self.m['functions']['float_mul']-self.m['code_base'])//4;self.assertEqual(self.m['words'][i],0xfc2100b2)
 def test_query_image_shape(self):
  m=create(self.root/'query',Path(__file__).parent/'templates',query=True);self.assertEqual((len(m['functions']),m['instructions'],len(m['memory_sites']),len(m['hooks'])),(17,158,37,76))
 def test_query_binding(self):
  m=create(self.root/'query',Path(__file__).parent/'templates',query=True);b=(self.root/'query/fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>II',b,0x31010),(0xee,0x10000ee));self.assertEqual(struct.unpack_from('>I',b,0x400)[0],0x5c)
 def test_determinism(self):create(self.root/'two',Path(__file__).parent/'templates');self.assertEqual((self.root/'one/fixture.xex').read_bytes(),(self.root/'two/fixture.xex').read_bytes())
 def test_no_unsafe_instruction_admitted(self):
  for word in (x(3,4,5,20),x(3,4,5,150)|1,x(3,4,5,519),x(3,4,5,266)|(1<<10),0x4e800420,0x44000002):
   with self.assertRaises(ValueError):admit_word(word)
 def test_every_declared_word_admitted(self):
  for word in self.m['words']:admit_word(word)
 def test_backedges(self):self.assertEqual(self.m['backedges'],[0x82010048,0x82010074,0x8201012c,0x82010138])
if __name__=='__main__':unittest.main()
