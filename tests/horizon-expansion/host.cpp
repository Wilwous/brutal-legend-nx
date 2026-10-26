// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
int main(){auto r=expansion_probe();std::printf("{\"passed\":%u,\"failed\":%u,\"workers\":%u,\"sdk_calls\":%u,\"result\":%u,\"remaining\":%u,\"leases\":%u,\"stages\":[",r.passed,r.failed,r.workers,r.sdk_calls,r.result,r.remaining,r.leases);for(unsigned i=0;i<5;++i)std::printf("%s{\"passed\":%u,\"failed\":%u}",i?",":"",r.stage_pass[i],r.stage_fail[i]);std::puts("]}");return r.failed?1:0;}
