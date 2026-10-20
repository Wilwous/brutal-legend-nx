// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Tests explicit original raw-context conventions, not an inferred Xbox vector ABI.
#include <rex/ppc.h>
#include <cstdio>
#include <cstring>
REX_FUNC(original_vector){
 for(unsigned lane=0;lane<4;++lane)ctx.v2.u32[lane]=(ctx.v2.u32[lane]^ctx.v3.u32[lane])+ctx.r3.u32;
 ctx.f1.f64*=2;
}
int main(){PPCContext c{};alignas(16) uint8_t guest[16];const uint8_t expected[16]{0x01,0x23,0x45,0x67,0x89,0xab,0xcd,0xef,0x10,0x32,0x54,0x76,0x98,0xba,0xdc,0xfe};std::memcpy(guest,expected,16);
 uint32_t input[4]{0x01234567,0x89abcdef,0x10325476,0x98badcfe};
 for(unsigned i=0;i<4;++i)c.v2.u32[i]=rex::memory::load_and_swap<uint32_t>(guest+i*4);
 c.v3.u32[0]=1;c.v3.u32[1]=2;c.v3.u32[2]=4;c.v3.u32[3]=8;c.r3.u64=7;c.r2.u64=0x1234;c.r13.u64=0x5678;c.r14.u64=0x9abc;c.v14.u64[0]=0xfedcba9876543210;c.f1.f64=1.25;PPCContext saved=c;
 original_vector(c,nullptr);
 for(unsigned i=0;i<4;++i)if(c.v2.u32[i]!=((input[i]^(1u<<i))+7))return 1;
 if(c.f1.f64!=2.5||c.r2.u64!=saved.r2.u64||c.r13.u64!=saved.r13.u64||c.r14.u64!=saved.r14.u64||std::memcmp(&c.v14,&saved.v14,16)||reinterpret_cast<uintptr_t>(&c.v2)%16)return 2;
 puts("{\"group\":\"explicit_raw_vector_context\",\"result\":\"PASS\",\"typed_ABI\":\"UNKNOWN\"}");return 0;
}
