#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original static words only; no PPC execution, code generation or title input."""
import copy
import hashlib
import json
from pathlib import Path
import sys
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
import startup_dependencies as dep


def image(words):
    data = bytearray(512)
    for pc, word in words.items():
        data[pc-0x1000:pc-0x1000+4] = word.to_bytes(4, 'big')
    return bytes(data)


class StaticContracts(unittest.TestCase):
    def setUp(self):
        # Independently authored toy: LR save, helper, frame, pointer construction,
        # scalar global store, nop. No resemblance to a full title startup ABI.
        self.words = {0x1000: 0x7c0802a6, 0x1004: 0x4800007d,
                      0x1008: 0x9421ff40, 0x100c: 0x38c10018,
                      0x1010: 0x3ca00002, 0x1014: 0x90e5000c,
                      0x1018: 0x60000000, 0x101c: 0,
                      0x1080: 0xfb01ffe8, 0x1084: 0x9001fff4,
                      0x1088: 0x4e800020}
        self.pcs = [p for p in self.words if p != 0x101c]
        self.raw = image(self.words)
        self.digest = hashlib.sha256(self.raw).hexdigest()
        self.report = self.analyze()

    def analyze(self, **kw):
        args = dict(raw=self.raw, base=0x1000, entry=0x1000, normal_pcs=self.pcs,
                    stop=0x101c, expected_sha256=self.digest)
        args.update(kw)
        return dep.analyze(**args)

    def replaced(self, pc, word):
        raw = image({**self.words, pc: word})
        return self.analyze(raw=raw, expected_sha256=hashlib.sha256(raw).hexdigest())

    def test_closed_route(self):
        self.assertEqual([r['pc'] for r in self.report['sites']],
                         [0x1000, 0x1004, 0x1080, 0x1084, 0x1088, 0x1008,
                          0x100c, 0x1010, 0x1014, 0x1018])
        self.assertEqual(len(self.report['memory']), 4)

    def test_input_values_remain_symbolic(self):
        self.assertEqual(self.report['read_before_definition'], ['LR', 'r1', 'r24', 'r7'])
        self.assertEqual(self.report['memory_effect_input_symbols'], ['LR', 'r1', 'r24', 'r7'])
        self.assertFalse(self.report['execution_admitted'])

    def test_call_overwrites_lr_but_saved_payload_is_root_lr(self):
        self.assertEqual(self.report['calls'], [{'pc': 0x1004, 'target': 0x1080, 'continuation': 0x1008}])
        self.assertEqual(self.report['returns'], [{'pc': 0x1088, 'target': 0x1008, 'matched': True}])
        self.assertEqual(self.report['memory'][1]['payload']['base'], 'LR')

    def test_old_base_payload_precedes_stwu_update(self):
        m = self.report['memory'][2]
        self.assertEqual(m['address']['amount'], -192)
        self.assertEqual(m['payload']['amount'], 0)
        self.assertEqual(self.report['final_written_registers']['r1']['amount'], -192)
        self.assertEqual(self.report['final_written_registers']['r1']['bits'], 32)

    def test_later_stack_address_uses_updated_base(self):
        d = self.replaced(0x100c, 0x90e10018)  # original toy store at updated sp+24
        self.assertEqual(d['memory'][3]['address']['amount'], -168)

    def test_exact_union_does_not_fill_holes(self):
        self.assertEqual(self.report['stack_intervals'], [[-192, -188], [-24, -16], [-12, -8]])
        self.assertEqual(self.report['stack_distinct_bytes'], 16)
        self.assertEqual(self.report['stack_envelope'], [-192, -8])

    def test_narrow_window_is_not_an_entire_footprint(self):
        self.assertFalse(dep.covered(self.report['stack_intervals'], -32, 32))
        self.assertTrue(dep.covered(self.report['stack_intervals'], -192, 0))
        with self.assertRaises(ValueError): dep.covered([], 1, 1)

    def test_constant_address_and_no_invented_owner(self):
        m = self.report['memory'][3]
        self.assertEqual(m['address']['amount'], 0x2000c)
        self.assertEqual(m['address']['kind'], 'constant')
        self.assertFalse(m['alignment_proven_at_runtime'])

    def test_base_zero_means_zero_not_gpr_zero(self):
        d = self.replaced(0x1014, 0x90e0000c)
        self.assertEqual(d['memory'][-1]['address']['amount'], 12)

    def test_unknown_address_not_concretized(self):
        d = self.replaced(0x1014, 0x90ea000c)  # r10 is never defined
        self.assertEqual(d['memory'][-1]['address']['base'], 'r10')
        self.assertIn('r10', d['memory_effect_input_symbols'])

    def test_signed_constant_and_low32_address(self):
        d = self.replaced(0x1010, 0x3ca0ffff)
        self.assertEqual(d['memory'][-1]['address']['amount'], 0xffff000c)
        self.assertEqual(d['final_written_registers']['r5']['amount'], (1 << 64)-65536)

    def test_dead_identity_read_not_an_effect_dependency(self):
        d = self.replaced(0x1018, 0x7d4a5378)  # mr r10,r10
        self.assertIn('r10', d['read_before_definition'])
        self.assertNotIn('r10', d['memory_effect_input_symbols'])

    def test_nop_has_no_false_context_dependency(self):
        row = self.report['sites'][-1]
        self.assertEqual(row['reads'], [])
        self.assertEqual(row['writes'], [])

    def test_read_before_definition_at_update_is_not_lost(self):
        row = next(r for r in self.report['sites'] if r['pc'] == 0x1008)
        self.assertEqual(row['read_before_definition'], ['r1'])

    def test_proof_recomputed_against_trusted_binding(self):
        self.assertIn('NO EXECUTION', dep.verify(self.report, self.raw, 0x1000, 0x1000, self.pcs, 0x101c, self.digest))
        for field, value in [('entry', 0x1004), ('execution_admitted', True), ('runtime_hooks_observed', True)]:
            bad = copy.deepcopy(self.report); bad[field] = value
            with self.assertRaises(ValueError): dep.verify(bad, self.raw, 0x1000, 0x1000, self.pcs, 0x101c, self.digest)

    def test_json_roundtrip_remains_verifiable(self):
        restored = json.loads(json.dumps(self.report))
        self.assertIn('NO EXECUTION', dep.verify(restored, self.raw, 0x1000, 0x1000,
                                               self.pcs, 0x101c, self.digest))

    def test_stale_source_identity(self):
        with self.assertRaises(ValueError): self.analyze(expected_sha256='0'*64)

    def test_changed_site_or_boundary_refused(self):
        for args in [dict(entry=0x1004), dict(stop=0x1008), dict(stop=0x101e),
                     dict(normal_pcs=self.pcs[:-1]), dict(normal_pcs=self.pcs+self.pcs[:1])]:
            with self.assertRaises(ValueError): self.analyze(**args)

    def test_unreferenced_extra_site_refused(self):
        with self.assertRaises(ValueError): self.analyze(normal_pcs=self.pcs+[0x1090])

    def test_out_of_source_or_overflow(self):
        for args in [dict(stop=0x2000), dict(base=0xfffffff0), dict(normal_pcs=[0x0ffc]+self.pcs)]:
            with self.assertRaises(ValueError): self.analyze(**args)

    def test_unknown_conditional_and_indirect_refused(self):
        for word in [0x41820004, 0x4e800420, 0x4e800421]:
            with self.assertRaises(ValueError): self.replaced(0x1018, word)

    def test_unmatched_return_refused(self):
        with self.assertRaises(ValueError): self.replaced(0x1018, 0x4e800020)

    def test_cycle_and_call_escape_refused(self):
        for word in [0x48000000, 0x48000101]:
            with self.assertRaises(ValueError): self.replaced(0x1018, word)

    def test_unsupported_or_invalid_forms_refused(self):
        for word in [0, 0x7c602028, 0x9400fff0, 0x7c0802a7, 0xe8000001]:
            with self.assertRaises(ValueError): self.replaced(0x1018, word)

    def test_non_affine_ori_remains_unknown(self):
        d = self.replaced(0x1018, 0x614a0001)  # ori r10,r10,1
        self.assertEqual(d['final_written_registers']['r10']['kind'], 'unknown')

    def test_excluded_instruction_never_decoded_or_admitted(self):
        self.assertEqual(self.report['excluded_stop']['word'], 0)
        self.assertFalse(self.report['excluded_stop']['executed'])
        self.assertFalse(self.report['runtime_hooks_observed'])
        self.assertFalse(self.report['whole_title_reachability'])


if __name__ == '__main__':
    unittest.main()
