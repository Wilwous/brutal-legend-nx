// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Representation diagnostic using the real SDK constructor, never a title input.
#include <rex/system/thread_state.h>
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
struct Counts{unsigned zero=0,prefill=0;};
static Counts inspect(const PPCContext&c){
 Counts counts;const PPCRegister*registers[]{&c.r0,&c.r2,&c.r3,&c.r4,&c.r5,&c.r6,&c.r7,&c.r8,&c.r9,&c.r10,&c.r11,&c.r12,&c.r14,&c.r15,&c.r16,&c.r17,&c.r18,&c.r19,&c.r20,&c.r21,&c.r22,&c.r23,&c.r24,&c.r25,&c.r26,&c.r27,&c.r28,&c.r29,&c.r30,&c.r31};
 for(auto*r:registers){std::array<unsigned char,8>bytes{};std::memcpy(bytes.data(),r,bytes.size());for(auto b:bytes){counts.prefill+=b==0xa5;counts.zero+=b==0;}}return counts;
}
int main(){
 alignas(PPCContext)std::array<unsigned char,sizeof(PPCContext)>direct_storage;std::fill(direct_storage.begin(),direct_storage.end(),0xa5);auto*direct=new(direct_storage.data())PPCContext{};auto control=inspect(*direct);direct->~PPCContext();
 using State=rex::runtime::ThreadState;alignas(State)std::array<unsigned char,sizeof(State)>sdk_storage;std::fill(sdk_storage.begin(),sdk_storage.end(),0xa5);
 auto*state=new(sdk_storage.data())State(12345,0x13570000,0x24680000,nullptr);auto observed=inspect(*state->context());const bool parameters=state->context()->r1.u64==0x13570000&&state->context()->r13.u64==0x24680000;state->~State();
 std::printf("{\"observation\":\"SDK ThreadState constructor vs direct aggregate\",\"result\":\"OBSERVED\",\"untouched_gpr_bytes\":240,\"direct_zero_bytes\":%u,\"sdk_zero_bytes\":%u,\"sdk_retained_prefill_bytes\":%u,\"sdk_sp_pcr_parameters_applied\":%s,\"guest_functions_invoked\":0,\"title_value_contract_verified\":false}\n",control.zero,observed.zero,observed.prefill,parameters?"true":"false");return parameters?0:1;
}
