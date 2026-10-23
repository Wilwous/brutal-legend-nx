#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import struct,tempfile,unittest
from pathlib import Path
from fixture import create
from audit import audit
class Fixtures(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.p=Path(self.tmp.name);self.a=create(self.p/'a');self.b=create(self.p/'b',0x83000000,False)
 def test_scope(self):self.assertEqual((len(self.a['functions']),self.a['instructions'],len(self.a['backedges']),len(self.a['sites']),len(self.a['memory_sites'])),(9,182,6,67,45))
 def test_hook_manifest(self):
  text=(self.p/'a/fixture_manifest.toml').read_text();self.assertEqual(text.count('[[entrypoint.midasm_hook]]'),67)
  for pc in self.a['backedges']:self.assertIn('name="brutal_point_%X"'%pc,text)
  self.assertNotIn('midasm_hook',(self.p/'b/fixture_manifest.toml').read_text())
 def test_image_and_extents(self):
  data=(self.p/'a/fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>I',data,44)[0],self.a['named']['finite']);self.assertEqual(len(data),0x41000)
  for i,(pc,n) in enumerate(self.a['extents'].items()):self.assertEqual(struct.unpack_from('>II',data,0x21100+8*i),(int(pc),0x40000000|(n<<8)))
 def test_backedge_words(self):
  for pc in self.a['backedges']:
   word=struct.unpack_from('>I',bytes(self.a['code']),pc-self.a['code_base'])[0];self.assertIn(word>>26,[16,18]);self.assertFalse(word&1)
 def test_benchmark_equivalence(self):
  def extract(m):
   pc=m['named']['memory'];n=m['extents'][str(pc)];return bytes(m['code'])[pc-m['code_base']:pc-m['code_base']+4*n]
  self.assertEqual(extract(self.a),extract(self.b));self.assertEqual(len(extract(self.a)),44)
 def test_deterministic_and_no_overwrite(self):
  create(self.p/'again');self.assertEqual((self.p/'a/fixture.xex').read_bytes(),(self.p/'again/fixture.xex').read_bytes())
  with self.assertRaises(FileExistsError):create(self.p/'a')
 def test_auditor_refuses_missing_and_misplaced_hook(self):
  # Unit-only miniature emitted syntax; never compiled or used as a caller.
  m={'code':[0x48,0,0,0],'code_base':0x100,'entry':0x100,'extents':{'256':1},'functions':[0x100],'instructions':1,'backedges':[0x100],'sites':{'256':['backedge']},'instrumented':True}
  body='DEFINE_REX_FUNC(xstart) {\nloc_100:\n\t// b 0x100\n\tbrutal_point_100();\n\tgoto loc_100;\n}\n'
  self.assertEqual(audit(m,body)['instrumented_backedges'],1)
  for broken in [body.replace('\tbrutal_point_100();\n',''),body.replace('brutal_point_100','brutal_point_104'),body.replace('goto loc_100','goto loc_FC')]:
   with self.assertRaises(ValueError):audit(m,broken)
 def test_auditor_refuses_unsupported_branch(self):
  m={'code':[0x4e,0x80,0x04,0x20],'code_base':0x100,'extents':{'256':1}}
  with self.assertRaises(ValueError):audit(m,'')
if __name__=='__main__':unittest.main()
