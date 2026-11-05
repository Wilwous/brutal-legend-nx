#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""SDK-free input identity and machine-readable coverage consistency checks."""
import hashlib,json,struct,tempfile,unittest
from pathlib import Path
from fixture import create,ROOT
class Compatibility(unittest.TestCase):
 def test_original_input_identity_and_independent_census(self):
  with tempfile.TemporaryDirectory() as tmp:
   d=Path(tmp)/'input';m=create(d);image=(d/'fixture.xex').read_bytes()
   self.assertEqual(hashlib.sha256(image).hexdigest(),'f4dd9d62d3dc8e2551dd735282d9ba8acfd526f94d55bff6a046406efccde0fc')
   words=list(struct.unpack('>59I',image[0x11000:0x11000+236]));self.assertEqual(words,m['words']);self.assertEqual(len(m['functions']),3);self.assertEqual(len(m['hooks']),59)
   # Independent opcodes, not annotations from the emitter.
   self.assertEqual(sum((v>>26) in {32,36,37,58,62} for v in words),22)
   self.assertEqual(sum((v>>26)==18 and v&1 for v in words),7)
   self.assertEqual(words.count(0x4e800020),3)
   self.assertEqual([v&65535 for v in words if v>>26==14 and (v>>21)&31==11],[338,341,340,294,339])
   self.assertEqual([v&65535 for v in words if v>>26==15 and (v>>21)&31==11],[1,1])
 def test_matrix_contracts_and_gate_separation(self):
  m=json.loads((ROOT/'docs/research/compatibility-matrix.json').read_text());self.assertEqual(m['schema'],1)
  self.assertEqual(m['title_inventory']['total_imports'],274);self.assertEqual(m['title_inventory']['function_imports'],261);self.assertEqual(m['title_inventory']['variable_imports'],13)
  self.assertEqual(m['title_inventory']['runtime_title_invocations'],0);self.assertEqual(m['title_inventory']['original_title_bindings_admitted'],0);self.assertEqual(m['game_execution'],'NO-GO')
  self.assertEqual(m['gates'],dict(G4='BLOCKED',G5='UNKNOWN',G8='BLOCKED',N_G4='BLOCKED',N_G5='PARTIAL',N_G8='BLOCKED'))
  identities=set();fields={'identity','evidence','input_state','output_state','errors','pointers','endianness','ownership','lifetime','thread','implementation','tests','title_relevance','status','classification'}
  for row in m['contracts']:
   self.assertTrue(fields<=row.keys());self.assertNotIn(row['identity'],identities);identities.add(row['identity']);self.assertIn(row['status'],{'VERIFIED GENERIC','VERIFIED SYNTHETIC','TITLE STATICALLY OBSERVED','PARTIAL','UNKNOWN','BLOCKED'})
   for test in row['tests']:self.assertTrue((ROOT/test).is_file())
  variables=[r for r in m['contracts'] if r.get('ordinal') in {14,27,89,173,342,344,403,430,446,447,448,449,614}];self.assertEqual(len(variables),13);self.assertTrue(all(r['status']=='BLOCKED' for r in variables))
  exports=[r['ordinal'] for r in m['contracts'] if r['implementation']=='runtime/horizon/sdk_services.h'];self.assertEqual(exports,[294,338,339,340,341])
 def test_source_identity_shape(self):
  m=json.loads((ROOT/'docs/research/compatibility-matrix.json').read_text())
  self.assertEqual(len(m['sources']),8)
  for source in m['sources']:
   self.assertRegex(source['sha256'],r'^[0-9a-f]{64}$');self.assertFalse(source['path'].startswith('/'));self.assertNotIn('Users/',source['path'])
if __name__=='__main__':unittest.main()
