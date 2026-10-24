#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Portable checks of original fixture semantics; not an execution substitute."""
import copy,json,struct,tempfile,unittest
from pathlib import Path
from fixture import create,cfg
class Fixture(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.path=Path(self.tmp.name);self.l,self.m=create(self.path/'a')
 def test_repeat(self):
  other,m=create(self.path/'b');self.assertEqual(self.m,m);self.assertEqual((self.path/'a/fixture.xex').read_bytes(),(self.path/'b/fixture.xex').read_bytes())
 def test_counts(self):
  self.assertEqual(len(self.m['policy']['sites']),24);self.assertEqual(len(self.m['policy']['edges']),21);self.assertEqual(len(self.l['functions']),5)
 def test_eight_stores(self):
  rows=cfg.decode_range(bytes(self.l['code']),self.l['code_base'],self.l['code_base']+len(self.l['code']),self.l['code_base']);selected=[r for r in rows if r['pc'] in self.l['stores']];self.assertEqual(len(selected),8);self.assertEqual([r['memory']['width'] for r in selected],[8,8,8,8,4,4,4,4]);self.assertEqual(sum(r['memory']['update'] for r in selected),1)
 def test_signed_image_ea(self):
  for i,pc in enumerate(self.l['stores'][-2:]):
   r=cfg.decode_range(bytes(self.l['code']),pc,pc+4,self.l['code_base'])[0];self.assertLess(r['memory']['offset'],0);self.assertEqual(self.l['base']+0x36000+r['memory']['offset'],self.l['base']+0x30420+4*i)
 def test_extent_disagreement(self):
  b=(self.path/'a/fixture.xex').read_bytes();self.assertEqual(struct.unpack_from('>I',b,0x104)[0],0x40000);self.assertEqual(struct.unpack_from('<I',b,0x1000+0x98+56)[0],0x60000)
 def test_no_import_admission(self):self.assertEqual(self.m['policy']['imports'],[])
 def test_main_boundary(self):
  p=self.m['policy'];self.assertEqual(sum(s['owner'] in [self.l['named']['startup'],self.l['named']['save']] for s in p['sites']),20);self.assertEqual(next(s for s in p['sites'] if s['pc']==self.l['boundary'])['action'],'exit')
 def test_namespace(self):
  forged=copy.deepcopy(self.m);forged['namespace']='0'*64
  with self.assertRaises(ValueError):cfg.validate(forged,self.m)
 def test_stop(self):self.assertEqual(cfg.stop_proof(self.m),dict(sites=24,stops=24,backedges=0,uncovered_cycles=0))
 def test_binding_drift(self):
  forged=copy.deepcopy(self.m);forged['bindings']['image_code_sha256']='0'*64
  with self.assertRaises(ValueError):cfg.validate(forged,self.m)
if __name__=='__main__':unittest.main()
