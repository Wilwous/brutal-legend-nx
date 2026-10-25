#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Portable original preflight units, separate from production-source auditing."""
import copy,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'));import audit_entry_instrumentation as a
sys.path.insert(0,str(ROOT/'tests/entry-contracts'));from fixture import create
class Preflight(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.l,self.m=create(Path(self.tmp.name)/'fixture');self.base=self.l['base'];self.sections=[dict(address=self.base+i*65536,size=65536,flags=f) for i,f in enumerate([0x40000040,0x60000020,0x40000040,0xc0000040])]
 def test_owner(self):self.assertTrue(a.layout(self.base,0x40000,self.sections,[(self.base+0x30420,4,'write')]))
 def test_outside_section(self):
  s=copy.deepcopy(self.sections);s[-1]['size']+=1
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,s)
 def test_overlap(self):
  s=copy.deepcopy(self.sections);s[-1]['address']=s[-2]['address']
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,s)
 def test_missing(self):
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,self.sections[:-1],[(self.base+0x30420,4,'write')])
 def test_readonly(self):
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,self.sections,[(self.base+0x20200,4,'write')])
 def test_code_section(self):
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,self.sections,[(self.base+0x20200,4,'code')])
 def test_overflow(self):
  with self.assertRaises(ValueError):a.layout(self.base,0x40000,self.sections,[(0xfffffffc,8,'write')])
 def test_identity(self):
  self.assertTrue(a.identities(self.m['bindings'],self.m['bindings']))
 def test_hash(self):
  changed=copy.deepcopy(self.m['bindings']);changed['xex_sha256']='0'*64
  with self.assertRaises(ValueError):a.identities(self.m['bindings'],changed)
 def test_original_context(self):
  original=dict(stack_owner='original-owner',image_lease='original-lease',r1=0x10000);self.assertTrue(a.context(original,original))
 def test_foreign_owner(self):
  original=dict(stack_owner='original-owner',image_lease='original-lease',r1=0x10000);bad=dict(original,stack_owner='foreign')
  with self.assertRaises(ValueError):a.context(original,bad)
 def test_invalid_register(self):
  original=dict(stack_owner='original-owner',image_lease='original-lease',r1=0x10000);bad=dict(original,r1=0)
  with self.assertRaises(ValueError):a.context(original,bad)
 def test_no_lease(self):
  original=dict(stack_owner='original-owner',image_lease=None)
  with self.assertRaises(ValueError):a.context(original,original)
 def test_llvm_refuses(self):
  with self.assertRaises(ValueError):a.llvm_policy('@entry_policy = constant [1 x i8] c"x"',self.m)
 def test_plain_llvm_payload(self):
  expected={'original':1};raw=a.cfg.canonical(expected);encoded=raw.decode().replace('"',r'\22');ir='@entry_policy = constant ['+str(len(raw))+' x i8] c"'+encoded+'", align 1';self.assertTrue(a.llvm_policy(ir,expected))
 def test_asan_llvm_payload(self):
  expected={'original':1};raw=a.cfg.canonical(expected);encoded=raw.decode().replace('"',r'\22');n=str(len(raw));ir='@entry_policy = constant { ['+n+' x i8], [32 x i8] } { ['+n+' x i8] c"'+encoded+'", [32 x i8] zeroinitializer }, align 32';self.assertTrue(a.llvm_policy(ir,expected))
  with self.assertRaises(ValueError):a.llvm_policy(ir,{'original':2})
 def test_asan_llvm_shape(self):
  expected={'original':1};raw=a.cfg.canonical(expected);encoded=raw.decode().replace('"',r'\22');n=str(len(raw));ir='@entry_policy = constant { ['+n+' x i8], [32 x i8] } { [999 x i8] c"'+encoded+'", [32 x i8] zeroinitializer }, align 32'
  with self.assertRaises(ValueError):a.llvm_policy(ir,expected)
 def test_data_only(self):
  source=a.policy_source(self.m);self.assertNotIn('PPCFunc',source);self.assertNotIn('main(',source);self.assertIn('entry_policy',source)
if __name__=='__main__':unittest.main()
