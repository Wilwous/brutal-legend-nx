// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <cstdint>
struct FoundationResult {
 uint32_t observed_ppc_result{},remaining_allocations{};
 unsigned passed{},failed{};unsigned stage_passed[6]{},stage_failed[6]{};
 uint64_t generated_calls{},checked_reads{},checked_writes{},safe_points{};
};
// Six measured groups: memory, threads, guest TLS/PCR, generated execution,
// cooperative stop, lifecycle/rollback. Exceptions count as failures.
FoundationResult foundation_probe();
