#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import importlib.util,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('order',Path(__file__).resolve().parents[2]/'tools/observation_order.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def edge(a,b,scope='sdk',status='PROVEN',condition=None):return {'before':a,'after':b,'scope':scope,'status':status,'condition':condition,'evidence':'original fixture fact'}
def graph(edges):return {'nodes':['copy','provider','thread','entry','callback','stop'],'scopes':['sdk','historical'],'edges':edges}
class PartialOrder(unittest.TestCase):
 def test_transitive_order_without_ordering_parallel_provider(self):
  g=graph([edge('copy','thread'),edge('thread','entry')]);self.assertEqual(m.relation(g,'copy','entry','sdk'),'BEFORE');self.assertEqual(m.relation(g,'provider','entry','sdk'),'UNKNOWN')
 def test_no_cross_scope_inference(self):
  g=graph([edge('copy','entry')]);self.assertEqual(m.relation(g,'copy','entry','historical'),'UNKNOWN')
 def test_unknown_edge_does_not_close_gap(self):
  g=graph([edge('copy','thread'),edge('thread','entry',status='UNKNOWN')]);self.assertEqual(m.relation(g,'copy','entry','sdk'),'UNKNOWN')
 def test_callback_branch_requires_explicit_premise(self):
  g=graph([edge('callback','entry',condition='queued')]);self.assertEqual(m.relation(g,'callback','entry','sdk'),'UNKNOWN');self.assertEqual(m.relation(g,'callback','entry','sdk',['queued']),'BEFORE')
 def test_cycles_refused(self):
  with self.assertRaises(ValueError):m.validate(graph([edge('entry','thread'),edge('thread','entry')]))
 def test_invalid_query_is_not_unknown_evidence(self):
  with self.assertRaises(ValueError):m.relation(graph([]),'unseen','entry','sdk')
if __name__=='__main__':unittest.main()
