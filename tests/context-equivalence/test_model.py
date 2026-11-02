#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import tempfile,unittest
from pathlib import Path
from fixture import create,MODES
from model import obligations,layout,normalize_stack

class EquivalenceModel(unittest.TestCase):
    def test_conditional_linear_identities(self):
        r=obligations();self.assertFalse(r['raw_byte_equality']);self.assertFalse(r['execution_admitted']);self.assertEqual(len(r['preconditions']),7)
    def test_layout_bounds_alignment_wrap(self):
        for s in (0x10004f00,0x10005f00,0x10007f00):self.assertTrue(layout(s,s-0x3f00,16384))
        for s,b,n in [(0x1040,0x1000,16384),(0x14f1,0x1000,16384),(2**32,0,16384),(0x1400,0x1000,492)]:self.assertFalse(layout(s,b,n))
    def test_raw_different_typed_normalization_equal(self):
        normalized=[];raws=[]
        for base in (0x10001000,0x10002000):
            s=base+0x3f00;raw=bytearray([0xa5]*16384);raw[s-base-496:s-base-492]=s.to_bytes(4,'big');raws.append(raw);normalized.append(normalize_stack(raw,base,s))
        self.assertNotEqual(raws[0],raws[1]);self.assertEqual(*normalized)
    def test_opaque_address_looking_payload_not_rewritten(self):
        base=0x10001000;s=base+0x3f00;raw=bytearray(16384);raw[s-base-496:s-base-492]=s.to_bytes(4,'big');raw[s-base-40:s-base-32]=(s+16).to_bytes(8,'big')
        self.assertEqual(normalize_stack(raw,base,s)[s-base-40:s-base-32],(s+16).to_bytes(8,'big'))
    def test_unproved_pointer_normalization_refuses(self):
        with self.assertRaises(ValueError):normalize_stack(bytes(16384),0x10001000,0x10004f00)
    def test_fixed_and_escape_are_real_counterexample_functions(self):
        s,t=0x10004f00,0x10005f00
        self.assertNotEqual(int(s-0x10000000==0x4f00),int(t-0x10000000==0x4f00));self.assertNotEqual(s.to_bytes(4,'big'),t.to_bytes(4,'big'))
    def test_original_variants_and_closed_counts(self):
        with tempfile.TemporaryDirectory() as tmp:
            a=create(Path(tmp)/'a');b=create(Path(tmp)/'b');self.assertEqual(a,b);self.assertEqual(tuple(x['mode'] for x in a),MODES)
            self.assertEqual([x['stores'] for x in a],[6,6,6,7,6]);self.assertEqual(sum(r['kind']=='indirect_call' for x in a for r in x['rows']),1)
            self.assertEqual([x['relocatable_contract'] for x in a],[True,True,False,False,False])
    def test_absolute_predicate_uses_fixed_not_candidate_pointer(self):
        with tempfile.TemporaryDirectory() as tmp:
            x=next(x for x in create(Path(tmp)/'a') if x['mode']=='fixed')
            self.assertEqual(x['rows'][3]['text'],'addis r9,r1,-4096');self.assertEqual(x['rows'][4]['text'],'cmpwi r9,20224')

if __name__=='__main__':unittest.main()
