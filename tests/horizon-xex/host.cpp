// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
int main(){auto r=xex_probe();std::printf("{\"passed\":%u,\"failed\":%u,\"result\":%u,\"sdk_calls\":%u,\"hook_hits\":[%u,%u],\"remaining\":%u}\n",r.passed,r.failed,r.observed_result,r.sdk_calls,r.hook_hits[0],r.hook_hits[1],r.remaining);return r.failed?1:0;}
