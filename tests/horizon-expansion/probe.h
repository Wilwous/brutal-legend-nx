// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
struct ExpansionResult {unsigned passed{},failed{},workers{},sdk_calls{},result{},remaining{},leases{};std::array<unsigned,5> stage_pass{},stage_fail{};};
ExpansionResult expansion_probe(unsigned cycles=1);
