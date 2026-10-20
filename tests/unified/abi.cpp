// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original arithmetic and callback oracles; no PPC game instructions or data.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/ppc.h>
#include <rex/logging.h>
#include <rex/system/thread_state.h>
#include <cstdio>
#include <filesystem>
#include <stdexcept>
#include <cstring>
static int passed=0,failed=0;
static void check(const char* name,bool ok){std::printf("{\"group\":\"%s\",\"result\":\"%s\"}\n",name,ok?"PASS":"FAIL");ok?++passed:++failed;}
static uint64_t wide(uint64_t a,uint64_t b){return (a^0xfedcba9876543210ull)+b;}
static uint64_t stack_wide(uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint64_t a){return a;}
static double precision(double a,double b){return a*2+b;}
REX_HOOK(fixture_wide,wide)
REX_HOOK(fixture_stack_wide,stack_wide)
REX_HOOK(fixture_precision,precision)
static uint32_t inner_sp=0,outer_sp=0;
static uint64_t expected_r2=0,expected_r13=0;
static bool nested_ok=false;
REX_FUNC(fixture_inner){
  inner_sp=ctx.r1.u32;
  nested_ok=ctx.r2.u64==expected_r2&&ctx.r13.u64==expected_r13&&!(ctx.r1.u32&15)&&inner_sp+0x70==outer_sp;
  ctx.r3.u64=ctx.r3.u64+7;
}
REX_FUNC(fixture_outer){
  outer_sp=ctx.r1.u32;
  nested_ok=ctx.r2.u64==expected_r2&&ctx.r13.u64==expected_r13;
  auto value=rex::ppc::GuestToHostFunction<uint64_t>(fixture_inner,uint64_t(0x1234567800000000));
  nested_ok=nested_ok&&value==0x1234567800000007ull;
  ctx.r3.u64=value+9;
}
REX_FUNC(fixture_throw){(void)ctx;(void)base;throw std::runtime_error("original callback fixture");}
REX_FUNC(fixture_vector){
  (void)base;
  ctx.v2.u32[0]+=ctx.v3.u32[0];ctx.v2.u32[1]^=ctx.v3.u32[1];
  ctx.r3.u64=(uint64_t(ctx.v2.u32[0])<<32)|ctx.v2.u32[1];
}
int main(int argc,char** argv){
  if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
  try{
    std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
    rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");
    if(runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
    auto* memory=runtime.memory();auto* base=memory->virtual_membase();
    uint32_t stack=memory->SystemHeapAlloc(0x4000,16),pcr=memory->SystemHeapAlloc(0x2d8,16);
    if(!stack||!pcr)return 4;
    memory->Zero(pcr,0x2d8);rex::memory::store_and_swap<uint32_t>(base+pcr+0x74,stack);
    {
      rex::runtime::ThreadState thread(77,stack+0x3e00,pcr,memory);rex::runtime::ThreadState::Bind(&thread);
      auto& ctx=*thread.context();ctx.r2.u64=0xaabbccdd;expected_r2=ctx.r2.u64;expected_r13=pcr;
      uint32_t original_sp=ctx.r1.u32;uint64_t original_r2=ctx.r2.u64;
      ctx.r3.u64=0x1122334455667788ull;ctx.r4.u64=0x100000002ull;
      fixture_wide(ctx,base);check("GPR64_arguments_and_return",ctx.r3.u64==((0x1122334455667788ull^0xfedcba9876543210ull)+0x100000002ull));
      // Independent byte construction, not the SDK setter, defines the input oracle.
      const uint8_t input[8]{0x12,0x34,0x56,0x78,0x9a,0xbc,0xde,0xf0};
      std::memcpy(base+ctx.r1.u32+0x54,input,8);fixture_stack_wide(ctx,base);
      check("stack64_big_endian_load",ctx.r3.u64==0x123456789abcdef0ull);
      auto returned=rex::ppc::GuestToHostFunction<uint64_t>(fixture_stack_wide,uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint64_t(0x89abcdef76543210ull));
      check("stack64_callback_store",returned==0x89abcdef76543210ull);
      ctx.f1.f64=1.5;ctx.f2.f64=2.25;fixture_precision(ctx,base);check("FPR_arguments_return",ctx.f1.f64==5.25);
      auto v=rex::ppc::GuestToHostFunction<uint64_t>(fixture_outer,uint64_t(3));
      check("nested_callbacks_r1_r2_r13",nested_ok&&v==0x1234567800000010ull&&outer_sp+0x70==original_sp&&ctx.r1.u32==original_sp&&ctx.r2.u64==original_r2&&ctx.r13.u32==pcr);
      PPCContext saved=ctx;bool caught=false;
      try{rex::ppc::GuestToHostFunction<void>(fixture_throw);}catch(const std::runtime_error&){caught=true;}
      check("callback_host_exception_restores_context",caught&&std::memcmp(&saved,&ctx,sizeof(ctx))==0);
      ctx.v2.u32[0]=7;ctx.v2.u32[1]=0x12345678;ctx.v3.u32[0]=11;ctx.v3.u32[1]=0x00ff00ff;
      fixture_vector(ctx,base);check("raw_context_vector_arguments",ctx.r3.u64==((uint64_t(18)<<32)|(0x12345678^0x00ff00ff)));
      check("stack_alignment_and_r1_preservation",!(ctx.r1.u32&15)&&ctx.r1.u32==original_sp);
      check("guest_pointer_roundtrip",memory->HostToGuestVirtual(memory->TranslateVirtual(pcr))==pcr);
      {rex::CallFrame frame(ctx);check("CallFrame_r2_r13",frame.ctx.r2.u64==ctx.r2.u64&&frame.ctx.r13.u64==ctx.r13.u64);}
      rex::ppc::ImportFunction<uint64_t(uint64_t)> typed{fixture_inner};
      outer_sp=ctx.r1.u32;
      auto typed_result=typed(uint64_t(0x100000002));
      check("typed_callback_bridge",typed_result==0x100000009ull&&ctx.r1.u32==original_sp&&ctx.r2.u64==original_r2);
      bool bad_address=false;try{rex::ppc::GuestToHostFunction<uint64_t>(uint32_t(0x1234));}catch(const std::invalid_argument&){bad_address=true;}
      check("unresolved_callback_address_rejected",bad_address&&ctx.r1.u32==original_sp);
      rex::runtime::ThreadState::Bind(nullptr);
      bool unbound=false;try{rex::ppc::GuestToHostFunction<void>(fixture_inner);}catch(const std::logic_error&){unbound=true;}
      check("unbound_callback_rejected",unbound);
    }
    memory->SystemHeapFree(pcr);memory->SystemHeapFree(stack);
    std::printf("{\"passed\":%d,\"failed\":%d}\n",passed,failed);return failed?1:0;
  }catch(const std::exception& e){std::fprintf(stderr,"fixture exception: %s\n",e.what());return 5;}
}
