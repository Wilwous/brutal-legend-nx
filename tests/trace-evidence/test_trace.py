#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import importlib.util
import unittest
from pathlib import Path
from fixture import fixture
spec = importlib.util.spec_from_file_location('trace', Path(__file__).resolve().parents[2] / 'tools/trace_evidence.py')
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)

class Envelope(unittest.TestCase):

    def reject(self, t):
        with self.assertRaises(ValueError):
            m.validate(t, m.digest(t))

    def test_valid_original_fixture(self):
        t = fixture()
        r = m.validate(t, m.digest(t))
        self.assertEqual(r['evidence_class'], 'SYNTHETIC_ONLY')
        self.assertEqual(r['gate_impact'], 'NONE')
        self.assertEqual(r['earlier_or_unordered_target_read_records'], [12])

    def test_missing_startup_boundary(self):
        t = fixture()
        t['events'][0]['phase'] = 'after_entry'
        self.reject(t)

    def test_missing_thread(self):
        t = fixture()
        t['events'][12]['thread'] = None
        self.reject(t)

    def test_dropped_events(self):
        t = fixture()
        t['capture']['dropped_events'] = 1
        self.reject(t)

    def test_incomplete_memory_coverage(self):
        t = fixture()
        t['capture']['coverage']['loader_host']['status'] = 'partial'
        self.reject(t)

    def test_unknown_callback(self):
        t = fixture()
        t['events'][11]['callback'] = 'unresolved'
        self.reject(t)

    def test_invalid_owner_transition(self):
        t = fixture()
        t['events'][2]['owner'] = 'foreign'
        self.reject(t)

    def test_timestamps_do_not_prove_causality(self):
        t = fixture()
        for e in t['events']:
            e['timestamp_ns'] = e['seq'] * 100
        t['events'][17]['after'] = [8]
        self.reject(t)

    def test_corrupt_data(self):
        t = fixture()
        expected = m.digest(t)
        t['events'][15]['value_sha256'] = '3' * 64
        with self.assertRaises(ValueError):
            m.validate(t, expected)

    def test_missing_startup_transfer(self):
        t = fixture()
        t['events'][14]['kind'] = 'materialize'
        self.reject(t)

    def test_deliberately_omitted_record(self):
        t = fixture()
        del t['events'][12]
        self.reject(t)

    def test_mismatched_producer_count(self):
        t = fixture()
        t['events'][-1]['producer_count'] += 1
        self.reject(t)
if __name__ == '__main__':
    unittest.main()
