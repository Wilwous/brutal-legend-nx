// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Arithmetic only: no guest allocator, guest pointer, or large memory request.
#include <rex/system/xmemory.h>
#include <cstdio>
#include <cstdint>
int main(){
 unsigned checked=0;uint32_t random=0x80217ad3;
 for(unsigned shift=0;shift<32;++shift){uint32_t multiple=uint32_t(1)<<shift;
  for(unsigned i=0;i<1030;++i){random=random*1664525u+1013904223u;
   uint32_t values[]{0,1,multiple-1,multiple,0xfffff001u,0xffffffffu,random};uint32_t value=values[i<6?i:6];
   uint64_t expected=value?uint64_t(value/multiple)+(value%multiple!=0):1;expected*=multiple;
   uint32_t output=0x12345678;bool ok=rex::memory::CheckedAllocationRoundUp(value,multiple,output);
   if(ok!=(expected<=0xffffffffull)|| (ok?output!=expected:output!=0x12345678))return 1;++checked;
  }
 }
 uint32_t output=7;if(rex::memory::CheckedAllocationRoundUp(16,0,output)||output!=7)return 2;
 std::printf("{\"group\":\"checked_allocation_rounding\",\"result\":\"PASS\",\"cases\":%u}\n",checked+1);return 0;
}
