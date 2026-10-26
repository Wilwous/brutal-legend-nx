#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Public original encoding/census units. Does not claim production execution."""
import hashlib,struct,tempfile,unittest
from pathlib import Path
from fixture import create
from audit import audit
class Original(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.path=Path(self.temp.name)/'fixture';self.m=create(self.path,Path(__file__).parent/'templates')
 def test_shape(self):self.assertEqual((len(self.m['functions']),self.m['instructions'],len(self.m['hooks']),len(self.m['memory_sites']),len(self.m['backedges'])),(10,83,44,20,4))
 def test_pdata(self):
  b=(self.path/'fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('<I',b,0x1000+0x98+100+24)[0],80)
  for i,(name,pc) in enumerate(self.m['functions'].items()):self.assertEqual(struct.unpack_from('>II',b,0x21100+i*8),(pc,(self.m['sizes'][name]<<8)|0x40000000))
 def test_exact_new_words(self):
  start=(self.m['functions']['float_add']-self.m['code_base'])//4;self.assertEqual(self.m['words'][start:start+2],[0xfc21102a,0x4e800020])
 def test_backedges(self):self.assertEqual(self.m['backedges'],[0x82010048,0x82010074,0x8201012c,0x82010138])
 def test_determinism(self):
  other=Path(self.temp.name)/'other';create(other,Path(__file__).parent/'templates');self.assertEqual(hashlib.sha256((self.path/'fixture.xex').read_bytes()).digest(),hashlib.sha256((other/'fixture.xex').read_bytes()).digest())
 def test_two_import_shape(self):
  m=create(Path(self.temp.name)/'two',Path(__file__).parent/'templates',initialize=True);self.assertEqual((len(m['functions']),m['instructions'],len(m['hooks']),len(m['memory_sites'])),(11,99,52,25))
 def test_two_import_ordinals(self):
  path=Path(self.temp.name)/'two';m=create(path,Path(__file__).parent/'templates',initialize=True);b=(path/'fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>IIII',b,0x31000),(0x11b,0x100011b,0x12e,0x100012e));self.assertEqual(struct.unpack_from('>I',b,44)[0],m['entry'])
 def skeleton(self):
  # An original auditor unit fixture, not a replacement generated caller.
  text='';sites=set(self.m['hooks'])
  for i,w in enumerate(self.m['words']):
   pc=self.m['code_base']+i*4;text+='// unit instruction\n'
   if pc in sites:text+=f'horizon_point_{pc:X}();\n'
   op=w>>26;macro={32:'REX_LOAD_U32',36:'REX_STORE_U32',37:'REX_STORE_U32',58:'REX_LOAD_U64',62:'REX_STORE_U64'}.get(op)
   if macro:text+=macro+'(0);\n'
  return text
 def test_auditor(self):self.assertEqual(audit(self.m,self.skeleton())['memory_emitted'],20)
 def test_wrong_hook(self):
  with self.assertRaises(ValueError):audit(self.m,self.skeleton().replace('horizon_point_82010000();','horizon_point_82010004();',1))
 def test_missing_memory(self):
  with self.assertRaises(ValueError):audit(self.m,self.skeleton().replace('REX_STORE_U32(0);','',1))
 def test_wrong_manifest(self):
  self.m['memory_sites']=[]
  with self.assertRaises(ValueError):audit(self.m,self.skeleton())
if __name__=='__main__':unittest.main()
