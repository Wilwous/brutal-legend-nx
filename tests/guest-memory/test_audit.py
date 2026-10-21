#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original static-audit fixtures; no proprietary input."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('entry_audit',ROOT/'tools/audit_entry.py')
audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
class AuditTests(unittest.TestCase):
    def test_direct_cycles_and_ignored_comments(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory);(p/'synthetic_recomp.cpp').write_text('''
DEFINE_REX_FUNC(xstart) { helper(ctx, base); // phantom(ctx, base);
REX_LOAD_U32(16); REX_CALL_INDIRECT_FUNC(99); }
DEFINE_REX_FUNC(helper) { xstart(ctx, base); __imp__Synthetic(ctx, base); REX_STORE_U8(16,1); }
DEFINE_REX_FUNC(unreachable) { __imp__Excluded(ctx, base); }
''')
            result=audit.analyze(p,'xstart')
            self.assertEqual(result['closure_nodes'],3)
            self.assertEqual(result['closure_generated_functions'],2)
            self.assertEqual(result['direct_imports'],['Synthetic'])
            self.assertEqual(result['closure_access_sites'],{'load_sites':1,'store_sites':1,'indirect_call_sites':1})
            self.assertEqual(result['mediated_generated_sites'],0)
    def test_absent_entry(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaises(ValueError):audit.analyze(Path(directory),'absent')
    def test_duplicate_definition(self):
        with tempfile.TemporaryDirectory() as directory:
            p=Path(directory);(p/'synthetic_recomp.cpp').write_text('DEFINE_REX_FUNC(xstart) {} DEFINE_REX_FUNC(xstart) {}')
            with self.assertRaises(ValueError):audit.analyze(p,'xstart')
if __name__=='__main__':unittest.main()
