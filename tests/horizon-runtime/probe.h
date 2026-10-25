// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
struct LeafResult {
  unsigned passed{},failed{},invocations{},reads{},writes{};
  bool memory_initialized{},entry_invoked{},expected_result{},cleanup{};
};
LeafResult run_leaf_probe();
