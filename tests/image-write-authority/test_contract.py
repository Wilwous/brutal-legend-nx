#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original symbolic observation examples; no PPC execution or runtime model."""
import unittest

def store(state,offset,payload):
 return state[:offset]+tuple(payload)+state[offset+len(payload):]

class ObservationBoundary(unittest.TestCase):
 def setUp(self):
  self.a=tuple('a'+str(i) for i in range(8));self.b=tuple('b'+str(i) for i in range(8))
  self.left=(17,34,51,68);self.right=(85,102,119,136)
 def test_complete_overwrite_hides_initial_symbols_after_both_stores(self):
  # Algebraic substitution removes every initial symbol, without sampling values.
  for initial in [self.a,self.b]:
   result=store(store(initial,0,self.left),4,self.right)
   self.assertEqual(result,self.left+self.right)
 def test_early_or_intermediate_observer_distinguishes_initial_state(self):
  self.assertNotEqual(self.a,self.b)
  self.assertNotEqual(store(self.a,0,self.left),store(self.b,0,self.left))
 def test_alias_after_overwrite_changes_guest_visible_effect(self):
  final=self.left+self.right
  self.assertNotEqual(store(final,7,('alias',)),final)
 def test_missing_last_byte_does_not_establish_initialization_independence(self):
  self.assertNotEqual(store(store(self.a,0,self.left),4,self.right[:3]),store(store(self.b,0,self.left),4,self.right[:3]))
if __name__=='__main__':unittest.main()
