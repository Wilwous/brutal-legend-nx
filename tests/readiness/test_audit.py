#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original text fixtures only; no SDK or game inputs required."""
import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('readiness', ROOT/'tools/audit_readiness.py')
audit = importlib.util.module_from_spec(spec); spec.loader.exec_module(audit)

class AuditTests(unittest.TestCase):
    def corpus(self, source, entry='xstart'):
        with tempfile.TemporaryDirectory() as work:
            p = Path(work); (p/'original_recomp.0.cpp').write_text(source)
            return audit.analyze(p, entry)

    def test_direct_cycle_and_unresolved(self):
        d = self.corpus('''DEFINE_REX_FUNC(xstart) {
callee(ctx, base); REX_CALL_INDIRECT_FUNC(ctx.ctr.u32);
}
DEFINE_REX_FUNC(callee) { xstart(ctx, base); __imp__OriginalApi(ctx, base); }
DEFINE_REX_FUNC(unreachable) { __imp__Never(ctx, base); }
''')
        self.assertEqual(d['summary']['closure_functions'], 2)
        self.assertEqual(d['summary']['closure_imports'], ['OriginalApi'])
        self.assertEqual(d['summary']['closure']['indirect_sites'], 1)
        self.assertEqual(d['closure']['xstart']['indirect'][0]['resolution'], 'unresolved')

    def test_comments_and_instruction_location(self):
        row = audit.body_inventory('''/* REX_RAW_ADDR(1); fake(ctx, base); */
// __imp__Fake(ctx, base); REX_LOAD_U32(0);
\t// lwz r3,0(r4)
REX_LOAD_U32(ctx.r4.u32);
\t// stwcx. r3,0,r4
REX_RAW_ADDR(ctx.r4.u32);
''')
        self.assertEqual(row['calls'], [])
        self.assertEqual([s['instruction_ordinal'] for s in row['sites']], [0, 1])
        self.assertEqual(row['sites'][1]['family'], 'atomic')
        self.assertEqual(row['sites'][1]['direction'], 'conditional_write')

    def test_operation_families(self):
        for mnemonic, family in [('lwarx','atomic'), ('ldarx','atomic'), ('stdcx.','atomic'),
                                 ('lvx128','full_vector'), ('stvx','full_vector'),
                                 ('lvlx128','partial_vector'), ('lvewx','partial_vector'),
                                 ('dcbzl','cache_zero'), ('new_op','unknown_raw')]:
            self.assertEqual(audit.operation('REX_RAW_ADDR',mnemonic)['family'], family)
        self.assertEqual(audit.operation('REX_MM_LOAD_U16','lhz')['mmio'], 'hint')
        self.assertEqual(audit.operation('REX_STORE_U64','std')['width_bytes'], 8)

    def test_unknown_targets_and_helpers(self):
        row = audit.body_inventory('REX_CALL_INDIRECT_FUNC(0x1000); ppc_trap(ctx); REX_RAW_ADDR(0);')
        self.assertEqual(row['indirect'][0]['resolution'], 'literal_unadmitted')
        self.assertEqual(row['helpers'][0]['helper'], 'ppc_trap')
        self.assertEqual(row['sites'][0]['seam'], 'reject_or_unverified')
        self.assertEqual(row['sites'][0]['instruction_ordinal'], -1)

    def test_missing_and_duplicate_entries(self):
        for source in ['DEFINE_REX_FUNC(other) {}', 'DEFINE_REX_FUNC(xstart) {} DEFINE_REX_FUNC(xstart) {}']:
            with self.assertRaises(ValueError): self.corpus(source)

    def test_aggregate_and_determinism(self):
        source = 'DEFINE_REX_FUNC(xstart) { REX_STORE_U8(0,0); }'
        a = self.corpus(source); self.assertEqual(a,self.corpus(source))
        self.assertEqual(a['summary']['all']['macro_sites'], 1)
        self.assertEqual(a['summary']['closure']['by_direction'], {'write':1})
        self.assertEqual(a['summary']['mediation'], 'not established by lexical analysis')

    def test_output_refuses_public_checkout(self):
        output = ROOT/'tests/readiness/refused.json'
        result = subprocess.run([sys.executable,str(ROOT/'tools/audit_readiness.py'),
                                 '--generated',str(ROOT/'tests/readiness'), '--output',str(output)],
                                capture_output=True,timeout=5)
        self.assertEqual(result.returncode,2); self.assertFalse(output.exists())

if __name__ == '__main__': unittest.main()
