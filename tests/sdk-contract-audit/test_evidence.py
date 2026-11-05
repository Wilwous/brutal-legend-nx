#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Public evidence scope consistency, not an Xbox behavior oracle."""
import json,unittest
from pathlib import Path
R=Path(__file__).resolve().parents[2]
class Evidence(unittest.TestCase):
 def test_compare_is_existing_and_not_numerically_replaced(self):
  e=json.loads((R/'docs/research/sdk-contract-evidence.json').read_text());c=e['compare']
  self.assertEqual(c['identity'],'xboxkrnl.exe:283:RtlCompareMemoryUlong');self.assertFalse(c['numerical_correction_applied']);self.assertFalse(c['duplicate_registration_added']);self.assertIn('UNKNOWN',c['exact_Xbox360_prefix_units_invalid_memory']);self.assertEqual(c['counterexample']['sdk_result_words'],3);self.assertEqual(c['counterexample']['prefix_oracle_bytes'],4)
  self.assertEqual(len(e['sources']),3)
  for s in e['sources'][:2]:self.assertRegex(s['commit'],r'^[0-9a-f]{40}$');self.assertRegex(s['sha256'],r'^[0-9a-f]{64}$');self.assertIn(s['commit'],s['url'])
 def test_matrix_keeps_title_denied_and_legacy_paths(self):
  m=json.loads((R/'docs/research/compatibility-matrix.json').read_text());self.assertEqual(m['title_inventory']['original_title_bindings_admitted'],0);self.assertEqual(m['title_inventory']['runtime_title_invocations'],0);self.assertEqual(m['game_execution'],'NO-GO');self.assertEqual(m['gates'],dict(G4='BLOCKED',G5='UNKNOWN',G8='BLOCKED',N_G4='BLOCKED',N_G5='PARTIAL',N_G8='BLOCKED'))
  rows=[r for r in m['contracts'] if r.get('ordinal')==283];self.assertEqual(len(rows),1);self.assertEqual(rows[0]['implementation'],'tests/horizon-xex/import.cpp');self.assertIn('UNKNOWN',rows[0]['readiness']);self.assertTrue((R/rows[0]['implementation']).is_file())
if __name__=='__main__':unittest.main()
