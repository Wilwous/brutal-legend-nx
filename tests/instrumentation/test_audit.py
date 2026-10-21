#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import importlib.util
from pathlib import Path
import unittest
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('audit_instrumentation',ROOT/'tools/audit_instrumentation.py');audit=importlib.util.module_from_spec(spec);spec.loader.exec_module(audit)
class CoverageTests(unittest.TestCase):
    def test_comments_are_not_accesses(self):
        self.assertEqual(audit.census('// REX_LOAD_U32(0);\nREX_STORE_U8(1,2);')['macros'],{'REX_STORE_U8':1})
    def test_bypasses_refused(self):
        for source in ['*(base+1)=0;','GuestPtr(base,1);','unknown(ctx, base);','ppc_longjmp(1,1);']:
            with self.assertRaises(ValueError):audit.fixture_gate(source)
    def test_seam_inventory(self):
        source='REX_LOAD_VECTOR(v,0); REX_STORE_VECTOR(0,v); REX_CALL_FUNC(f); REX_CALL_INDIRECT_FUNC(0);'+'REX_PARTIAL_VECTOR_GUARD();'*4
        self.assertEqual(audit.fixture_gate(source)['macros']['REX_PARTIAL_VECTOR_GUARD'],4)
if __name__=='__main__':unittest.main()
