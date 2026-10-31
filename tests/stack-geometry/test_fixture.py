#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import copy,hashlib,json,tempfile,unittest
from pathlib import Path
from fixture import create,dep,cfg,BASE

class GeometryTests(unittest.TestCase):
    def setUp(self):
        self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
        self.p=Path(self.tmp.name);self.m=create(self.p/'one');self.raw=(self.p/'one/geometry.bin').read_bytes()
    def test_literal_reference_words(self):
        self.assertEqual(int.from_bytes(self.raw[:4],'big'),0x3941fe10)
        self.assertEqual(int.from_bytes(self.raw[8:12],'big'),0x48000019)
        self.assertEqual(self.m['rows'][8]['text'],'std r22,-24(r1)')
    def test_six_stores_no_loads(self):
        m=[r['memory'] for r in self.m['rows'] if r['memory']]
        self.assertEqual(len(m),6);self.assertTrue(all(r['write'] for r in m))
        self.assertEqual(sorted(r['width'] for r in m),[4,4,8,8,8,8])
    def test_union_envelope_allocation_are_distinct(self):
        f=self.m['footprint'];touched=set()
        for r in f:touched.update(range(r['offset'],r['offset']+r['width']))
        self.assertEqual(len(touched),40);self.assertEqual(max(touched)-min(touched)+1,492)
        self.assertEqual(len(touched-set(range(-32,32))),12)
        self.assertFalse(dep.covered(dep.intervals([(x['offset'],x['offset']+x['width']) for x in f]),-32,32))
    def test_frame_update_is_not_a_store_update(self):
        self.assertEqual(self.m['rows'][3]['text'],'stw r1,0(r10)')
        self.assertEqual(self.m['rows'][4]['text'],'mr r1,r10')
        self.assertFalse(any(r['memory'] and r['memory']['update'] for r in self.m['rows']))
    def test_closed_call_and_restored_root_lr(self):
        self.assertEqual(self.m['rows'][2]['target'],BASE+32)
        self.assertEqual(self.m['rows'][6]['text'],'mtlr r7')
        self.assertEqual(sum(r['kind']=='return' for r in self.m['rows']),2)
    def test_deterministic_generation_inputs(self):
        other=create(self.p/'two');self.assertEqual(other,self.m)
        for n in ['geometry.bin','geometry.map','geometry.s']:self.assertEqual((self.p/'one'/n).read_bytes(),(self.p/'two'/n).read_bytes())
    def test_parser_denies_unknown_incoming_pointer(self):
        raw=bytearray(self.raw);raw[:4]=(0x394bfe10).to_bytes(4,'big') # r11 has no stack provenance
        pcs=[r['pc'] for i,r in enumerate(self.m['rows']) if i not in (6,7)]
        report=dep.analyze(bytes(raw),BASE,BASE,pcs,BASE+24,hashlib.sha256(raw).hexdigest())
        self.assertNotEqual(report['memory'][-1]['address']['base'],'r1')
    def test_changed_input_identity_refuses(self):
        with self.assertRaises(ValueError):dep.analyze(self.raw,BASE,BASE,[BASE],BASE+24,'0'*64)
    def test_unsupported_nested_control_refuses(self):
        raw=bytearray(self.raw);raw[8:12]=(0x4e800421).to_bytes(4,'big')
        with self.assertRaises(ValueError):dep.analyze(bytes(raw),BASE,BASE,[r['pc'] for i,r in enumerate(self.m['rows']) if i not in (6,7)],BASE+24,hashlib.sha256(raw).hexdigest())
    def test_persisted_proof_not_promoted_to_execution(self):
        pcs=[r['pc'] for i,r in enumerate(self.m['rows']) if i not in (6,7)]
        r=dep.analyze(self.raw,BASE,BASE,pcs,BASE+24,self.m['raw_sha256']);r=json.loads(json.dumps(r))
        dep.verify(r,self.raw,BASE,BASE,pcs,BASE+24,self.m['raw_sha256'])
        bad=copy.deepcopy(r);bad['execution_admitted']=True
        with self.assertRaises(ValueError):dep.verify(bad,self.raw,BASE,BASE,pcs,BASE+24,self.m['raw_sha256'])

if __name__=='__main__':unittest.main()
