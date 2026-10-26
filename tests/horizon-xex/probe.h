// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <cstdint>
struct XexResult {unsigned passed{},failed{},observed_result{},sdk_calls{},hook_hits[2]{},remaining{};};
XexResult xex_probe();
