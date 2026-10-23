#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Portable checks of the original words; SDK execution is a separate matrix."""
import hashlib,struct,tempfile,unittest
from pathlib import Path
from fixture import create,BASE
class Encoding(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup);self.p=Path(self.temp.name)/'input';self.meta=create(self.p);self.words=struct.unpack('>59I',(self.p/'caller.bin').read_bytes())
 def test_identity(self):
  self.assertEqual(hashlib.sha256((self.p/'caller.bin').read_bytes()).hexdigest(),'413aec6948ab77e4b4c03c507e211ec9930d8e2d852e9bc9e46ff1a9f86c09d8')
  self.assertEqual(self.meta['instructions'],59);self.assertEqual(len(self.meta['functions']),8)
 def test_direct_calls(self):
  targets=[]
  for i,w in enumerate(self.words):
   if w>>26==18:
    self.assertEqual(w&3,1);targets.append((i*4+(w&0x3fffffc))&0xffffffff)
  self.assertEqual(targets,[self.meta['functions'][n] for n in ['echo','compare','critical']])
 def test_failure_branch(self):
  i,w=next((i,w) for i,w in enumerate(self.words) if w>>26==16)
  self.assertEqual(w>>16,0x4082);target=i*4+(w&0xfffc)
  self.assertEqual(self.words[target//4],(58<<26)|(14<<21)|(1<<16)|16)
 def test_gate_bodies(self):
  for n in ['compare','critical']:self.assertEqual(self.words[self.meta['functions'][n]//4],0x4e800020)
  self.assertNotIn('critical',self.meta['synthetic_reachable'])
 def test_stack_and_output(self):
  self.assertEqual(self.meta['stack_argument_offset'],84);self.assertEqual(self.meta['gpr_argument_return_bits'],64)
  self.assertIn((36<<26)|(11<<21)|(1<<16)|84,self.words)
  self.assertIn((32<<26)|(11<<21)|(1<<16)|84,self.words)
  self.assertNotIn('DEFINE_REX_FUNC',(self.p/'layout.h').read_text())
 def test_no_overwrite(self):
  before=(self.p/'caller.bin').read_bytes()
  with self.assertRaises(FileExistsError):create(self.p)
  self.assertEqual((self.p/'caller.bin').read_bytes(),before)
if __name__=='__main__':unittest.main()
