// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <iostream>
#include <future>
#include <string_view>
int main(int argc,char**argv){unsigned cycles=1,workers=1;if(argc==2&&std::string_view(argv[1])=="--repeat")cycles=8;else if(argc==2&&std::string_view(argv[1])=="--concurrent")workers=2;else if(argc!=1)return 2;
 NativeResult r;unsigned positive=0,negative=0,failed=0,bulk=0;
 for(unsigned i=0;i<cycles;++i){if(workers==2){auto a=std::async(std::launch::async,native_probe);r=native_probe();auto other=a.get();bulk+=other.byte_comparisons;positive+=other.positive;negative+=other.negative;failed+=other.failed;}
 else r=native_probe();bulk+=r.byte_comparisons;positive+=r.positive;negative+=r.negative;failed+=r.failed;}
 std::cout<<"{\"positive\":"<<positive<<",\"negative\":"<<negative<<",\"failed\":"<<failed<<",\"result\":"<<r.actual<<",\"byte_comparisons\":"<<bulk<<",\"sdk_entered\":"<<r.sdk_entered<<",\"sdk_completed\":"<<r.sdk_completed<<",\"functions\":"<<r.functions<<",\"observed_functions\":"<<r.observed_functions<<",\"diagnostics\":"<<r.diagnostics<<",\"cycles\":"<<cycles<<",\"workers\":"<<workers<<",\"remaining\":"<<r.remaining<<",\"leases\":"<<r.leases<<"}\n";return failed?1:0;}
