#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original fixtures; no proprietary input, allocation or instruction execution."""
from pathlib import Path
import sys,unittest,hashlib
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools'))
from native_provenance import ledger,interval_identity,audit_draft
class Tests(unittest.TestCase):
 def source(self):return {'start':4096,'end':4104,'kind':'DERIVED','offset':0,'source_size':8,'sha256':hashlib.sha256(b'original').hexdigest(),'evidence':'authored transform'}
 def test_accounting(self):
  a=ledger(4096,4112,[self.source()],[{'start':4104,'end':4108,'id':'section'}]);self.assertEqual(a['totals']['DERIVED'],8);self.assertEqual(a['totals']['UNBACKED'],4);self.assertEqual(a['totals']['UNKNOWN'],4)
 def test_split_preserves_offset(self):
  a=ledger(4096,4104,[self.source()],boundaries=[4099]);self.assertEqual([r['source_offset'] for r in a['regions']],[0,3]);self.assertEqual(sum(a['totals'].values()),8)
 def test_deterministic(self):self.assertEqual(ledger(4096,4104,[self.source()]),ledger(4096,4104,[self.source()]))
 def test_no_inference_from_claim(self):self.assertEqual(ledger(1,9,[],[{'start':1,'end':9}])['totals']['UNBACKED'],8)
 def test_zero_rule(self):
  z={'start':1,'end':9,'kind':'EXPLICIT ZERO-FILL','zero_rule':'original author fixture','evidence':'producer'};self.assertEqual(ledger(1,9,[z])['totals']['EXPLICIT ZERO-FILL'],8)
 def test_refusals(self):
  cases=[lambda:ledger(1,1,[]),lambda:ledger(1,(1<<32)+1,[]),lambda:ledger(4096,4104,[self.source(),self.source()]),lambda:ledger(4097,4104,[self.source()]),lambda:ledger(4096,4104,[dict(self.source(),offset=1)]),lambda:ledger(4096,4104,[dict(self.source(),sha256='x'*64)]),lambda:ledger(4096,4104,[dict(self.source(),evidence='')]),lambda:ledger(4096,4104,[dict(self.source(),kind='UNKNOWN')]),lambda:ledger(4096,4104,[dict(self.source(),kind='EXPLICIT ZERO-FILL')]),lambda:ledger(1,9,[],boundaries=[10]),lambda:ledger(1,9,[],[{'start':0,'end':9}]),lambda:interval_identity(b'abc',2,2)]
  for n,f in enumerate(cases):
   with self.subTest(refusal=n):
    with self.assertRaises(ValueError):f()
 def test_no_admission(self):
  c={'schema':'native-image-audit-draft-v1','ledger':ledger(4096,4112,[self.source()]),'native_origin':'title'};a=audit_draft(c);self.assertFalse(a['full_title_admission']);self.assertEqual(a['proven_bytes'],8);self.assertEqual(a['unresolved_bytes'],8);self.assertEqual(set(a['reasons']),{'unresolved image intervals','current native schema does not admit title provenance','relocation policy incomplete','startup contract incomplete','import contracts incomplete','code/binding contract incomplete'})
 def test_interval_hash(self):self.assertEqual(interval_identity(b'prefixoriginal',6,8),hashlib.sha256(b'original').hexdigest())
if __name__=='__main__':unittest.main()
