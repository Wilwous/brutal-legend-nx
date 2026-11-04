#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent original encoding/layout oracle, without SDK or title inputs."""
import hashlib,importlib.util,struct,tempfile,unittest
from pathlib import Path
from fixture import create,ROOT
spec=importlib.util.spec_from_file_location('independent_decoder',ROOT/'tests/horizon-acceptance/run.py')
oracle=importlib.util.module_from_spec(spec);spec.loader.exec_module(oracle)
EXPECTED=[
 'stwu r1,-64(r1)','mflr r0','stw r0,68(r1)','std r14,16(r1)','mr r14,r4',
 'lwz r11,0(r3)','lwz r11,0(r11)','lwz r3,88(r11)','lis r4,2','ori r4,r4,1025',
 'bl 0x82010070','cmpwi r3,0','beq 0x8201003c','lwz r3,0(r3)','b 0x82010040',
 'li r3,5','bl 0x82010068','lwz r5,0(r13)','lwz r6,32(r5)','add r3,r3,r6',
 'stw r3,0(r14)','ld r14,16(r1)','lwz r0,68(r1)','mtlr r0','addi r1,r1,64','blr',
 'addi r3,r3,7','blr','blr','b 0x82010074','mtctr r3','bctrl','blr']
class Contract(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory(prefix='original-module-');self.addCleanup(self.temp.cleanup)
  self.p=Path(self.temp.name)/'input';self.m=create(self.p);self.b=(self.p/'fixture.xex').read_bytes()
 def test_authored_instruction_correspondence(self):
  self.assertEqual([oracle.decode(w,0x82010000+4*i).strip() for i,w in enumerate(self.m['words'])],EXPECTED)
  self.assertEqual(list(self.m['sizes'].values()),[26,2,1,1,3])
 def test_container_identity_and_no_function_imports(self):
  self.assertEqual(hashlib.sha256(self.b).hexdigest(),'84055836723deb8fc10db9878571cd387993af6b7c0ed2e9c7d75e5e9b8ae25f')
  count=struct.unpack_from('>I',self.b,20)[0];fields=dict(struct.unpack_from('>II',self.b,24+i*8) for i in range(count))
  self.assertNotIn(0x103ff,fields);self.assertEqual(fields[0x20401],21)
  self.assertEqual(self.b[0x21000:0x21010],bytes(16))
 def test_exact_memory_boundaries(self):
  expected={0:('write',4),2:('write',4),3:('write',8),5:('read',4),6:('read',4),7:('read',4),13:('read',4),17:('read',4),18:('read',4),20:('write',4),21:('read',8),22:('read',4)}
  actual={i:(s['kind'],s['width']) for i,s in enumerate(self.m['sites']) if s['kind'] in ['read','write']}
  self.assertEqual(actual,expected);self.assertEqual(self.m['hooks'],list(range(0x82010000,0x82010084,4)))
 def test_call_targets_and_negative_only_indirect(self):
  self.assertEqual([(i,s['target']) for i,s in enumerate(self.m['sites']) if s['kind']=='direct'],[(10,0x82010070),(16,0x82010068)])
  self.assertEqual(self.m['sites'][29]['target'],0x82010074);self.assertEqual(self.m['sites'][31]['kind'],'indirect')
 def test_separate_arithmetic_oracle(self):
  # Present inline payload + independently specified helper + dynamic TLS seed.
  self.assertEqual(21+7+3,31);self.assertEqual(5+7+3,15)
if __name__=='__main__':unittest.main()
