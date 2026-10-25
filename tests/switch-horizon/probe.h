// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
struct ProbeResult { unsigned passed{}, failed{}; };
// Original component contracts, shared by a host executable and a static NRO.
// No SDK, PPC execution, fixed host mappings or hardware protection claims.
ProbeResult portable_probe();
