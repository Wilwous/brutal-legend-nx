#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original encoder census independent of production C++ execution."""
import hashlib
import tempfile
import unittest
from pathlib import Path
from fixture import create
class Fixture(unittest.TestCase):
 def test_authored_census_and_byte_identity(self):
  with tempfile.TemporaryDirectory() as directory:
   first=Path(directory)/'first';second=Path(directory)/'second';meta=create(first);other=create(second)
   self.assertEqual(meta,other);self.assertEqual((first/'fixture.xex').read_bytes(),(second/'fixture.xex').read_bytes())
   self.assertEqual(hashlib.sha256((first/'fixture.xex').read_bytes()).hexdigest(),'1b507e887da3285bdde9288e47053d43fef097a7fadcd491229f15ed2f68ef4a')
   self.assertEqual((len(meta['functions']),len(meta['words']),len(meta['hooks'])),(3,188,188))
   self.assertEqual(sum(s['kind'] in ['read','write'] for s in meta['sites']),93)
   self.assertEqual(sum(s['kind']=='direct' for s in meta['sites']),7)
   self.assertEqual(meta['command_bytes'],224)
 def test_exact_stack_and_masks(self):
  with tempfile.TemporaryDirectory() as directory:
   meta=create(Path(directory)/'input');words=meta['words'];self.assertEqual(words[0],0x9421ffa0)
   masks=[word for word in words if word>>26==21]
   self.assertEqual(len(masks),2)
   for word,mb in zip(masks,[24,25]):
    self.assertEqual(((word>>11)&31,(word>>6)&31,(word>>1)&31,word&1),(0,mb,31,0))
    # Independent arithmetic oracle for SH=0 bit masks, including wrap/negative input.
    for value in [0,1,255,256,0xffffffff]:
     mask=(1<<(32-mb))-1;self.assertEqual((value&mask),value%(1<<(32-mb)))
if __name__=='__main__':unittest.main()
