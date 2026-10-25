#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import copy,sys,unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from compare_residency_models import compare
class Models(unittest.TestCase):
 def setUp(self):
  self.m={'inventory':dict(base=0x40000000,end=0x40002000,security_size=8192,regions=[dict(start=0x40000000,end=0x40002000)]),'pe':{'sections':[dict(start=0x40001000,end=0x40002000,raw_offset=1024,raw_size=4096,flags=0x40000040)]}}
  self.p={'sections':[dict(rva=4096,vsize=4096,raw_offset=1024,raw_size=4096,flags='0x40000040')]}
 def run_model(self):return compare(self.m,self.p,4096)
 def test_ordinary(self):self.assertTrue(self.run_model()['claims'][0]['sdk_exact_owner_accepts'])
 def test_tail(self):
  self.m['pe']['sections'][0]['end']+=4096;self.p['sections'][0]['vsize']+=4096;r=self.run_model();self.assertFalse(r['claims'][0]['sdk_exact_owner_accepts']);self.assertEqual(r['console_residency'],'UNKNOWN');self.assertIsNone(r['idaxex']['guest_allocation'])
 def test_no_read(self):
  self.m['pe']['sections'][0]['flags']=0x80000040;self.p['sections'][0]['flags']='0x80000040';self.assertFalse(self.run_model()['claims'][0]['sdk_exact_owner_accepts'])
 def test_independent_mismatch(self):
  self.p['sections'][0]['raw_offset']+=1
  with self.assertRaises(ValueError):self.run_model()
 def test_missing_independent(self):
  self.p['sections']=[]
  with self.assertRaises(ValueError):self.run_model()
 def test_extent_disagreement(self):
  self.m['inventory']['security_size']+=4096
  with self.assertRaises(ValueError):self.run_model()
 def test_host_buffer_not_owner(self):
  r=compare(self.m,self.p,12288);self.assertEqual(r['idaxex']['host_vector_extent'],12288);self.assertIsNone(r['idaxex']['guest_allocation']);self.assertFalse(r['comparison_loader_executed'])
 def test_budget(self):
  with self.assertRaises(ValueError):compare(self.m,self.p,2**32)
if __name__=='__main__':unittest.main()
