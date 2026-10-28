#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original encoding/admission oracles; no SDK, device or game input."""
import importlib.util,struct,tempfile,unittest
from pathlib import Path
from fixture import create,x,prior
from audit import admit_word
class Acceptance(unittest.TestCase):
 def setUp(self):
  t=tempfile.TemporaryDirectory();self.addCleanup(t.cleanup);self.root=Path(t.name);self.m=create(self.root/'one',Path(__file__).parent/'templates')
 def test_shape(self):self.assertEqual((len(self.m['functions']),self.m['instructions'],len(self.m['memory_sites']),len(self.m['hooks'])),(20,195,46,92))
 def test_prefix(self):
  old=prior.create(self.root/'old',Path(__file__).parent/'templates',query=True);self.assertEqual(self.m['words'][:158],old['words'])
 def test_scalar_words(self):
  n=(self.m['functions']['word_logic']-self.m['code_base'])//4;self.assertEqual(self.m['words'][n:n+2],[0x7c660034,0x5483422e])
 def test_vector_words(self):
  n=(self.m['functions']['vector_blocks']-self.m['code_base'])//4;self.assertEqual(self.m['words'][n:n+5],[0x7c0018ce,0x7c2020ce,0x10400880,0x10600cc4,0x7c4029ce])
 def test_targets(self):
  for site in self.m['sites']:
   if site['kind']=='direct':self.assertIn(site['target'],[*self.m['functions'].values(),0x82030004,0x8203000c,0x82030014]);self.assertNotEqual(site['target'],self.m['entry'])
 def test_original_imports(self):
  b=(self.root/'one/fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>I',b,0x400)[0],0x5c)
 def test_all_words_admitted(self):
  for word in self.m['words']:admit_word(word)
 def test_unsupported_stay_denied(self):
  for w in [x(3,4,5,20),x(3,4,5,84),x(3,4,5,150)|1,x(3,4,5,214)|1,x(3,4,5,519),x(3,4,5,647),0x4e800420,0x44000002,0x5483422f]:
   with self.assertRaises(ValueError):admit_word(w)
 def test_no_untested_rotate(self):
  with self.assertRaises(ValueError):admit_word(0x54833a2e)
 def test_backedges(self):self.assertEqual(self.m['backedges'],[0x82010048,0x82010074,0x8201012c,0x82010138])
 def test_deterministic(self):create(self.root/'two',Path(__file__).parent/'templates');self.assertEqual((self.root/'one/fixture.xex').read_bytes(),(self.root/'two/fixture.xex').read_bytes())
 def test_pdata_count(self):self.assertEqual(struct.unpack_from('<I',(self.root/'one/fixture.xex').read_bytes(),0x1000+0x98+100+24)[0],20*8)
if __name__=='__main__':unittest.main()
