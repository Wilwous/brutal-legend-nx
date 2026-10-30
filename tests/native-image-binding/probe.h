// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
struct NativeResult {unsigned positive{},negative{},failed{},actual{},sdk_entered{},sdk_completed{},functions{},remaining{},leases{};};
NativeResult native_probe();
