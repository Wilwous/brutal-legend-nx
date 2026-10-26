// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
int main(){auto r=foundation_probe();std::printf("{\"passed\":%u,\"failed\":%u,\"calls\":%llu,\"reads\":%llu,\"writes\":%llu,\"safe_points\":%llu,\"groups\":[",r.passed,r.failed,(unsigned long long)r.generated_calls,(unsigned long long)r.checked_reads,(unsigned long long)r.checked_writes,(unsigned long long)r.safe_points);for(unsigned i=0;i<6;++i)std::printf("%s{\"passed\":%u,\"failed\":%u}",i?",":"",r.stage_passed[i],r.stage_failed[i]);std::puts("]}");return r.failed?1:0;}
