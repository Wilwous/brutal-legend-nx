// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
#include <cstring>
int main(int argc,char**argv){if(argc>2||(argc==2&&std::strcmp(argv[1],"--stress")&&std::strcmp(argv[1],"--no-trace")))return 2;auto r=runtime_ii_probe(argc==2&&!std::strcmp(argv[1],"--stress")?8:1,argc!=2||std::strcmp(argv[1],"--no-trace"));std::printf("{\"passed\":%u,\"failed\":%u,\"result\":%u,\"sdk_calls\":%u,\"sdk_entered\":%u,\"imports\":%u,\"guest_entries\":%u,\"workers\":%u,\"remaining\":%u,\"leases\":%u,\"refusals\":%u,\"diagnostics\":%u,\"dropped\":%u,\"groups\":[",r.passed,r.failed,r.result,r.sdk_calls,r.sdk_entered,r.imports,r.guest_entries,r.workers,r.remaining,r.leases,r.refusals,r.diagnostics,r.dropped);for(unsigned i=0;i<4;++i)std::printf("%s{\"passed\":%u,\"failed\":%u}",i?",":"",r.group_pass[i],r.group_fail[i]);std::puts("]}");return r.failed?1:0;}
