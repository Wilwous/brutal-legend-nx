// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
struct RuntimeIIResult {unsigned passed{},failed{},result{},sdk_calls{},guest_entries{},workers{},remaining{},leases{},refusals{},diagnostics{},dropped{};std::array<unsigned,4> group_pass{},group_fail{};};
RuntimeIIResult runtime_ii_probe(unsigned cycles=1,bool tracing=true);
