// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original expected bytes/register results, no game input or code.
#include "synthetic.inc"
#include "runtime/sdk_checked_memory.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <algorithm>
#include <chrono>
#include <thread>
#include <atomic>
#include <cstdio>
#include <filesystem>
using Memory=brutal::foundation::SdkCheckedMemory;
static unsigned checks=0,called=0;
static void require(bool value){++checks;if(!value)throw std::runtime_error("instrumentation oracle mismatch");}
void synthetic_callee(PPCContext& ctx,uint8_t*){++called;ctx.r3.u64+=1;}
void synthetic_callback(PPCContext& ctx,uint8_t* base){syn_direct(ctx,base);throw std::runtime_error("original callback failure");}
static uint64_t oracle(const std::byte* p,unsigned n){uint64_t value=0;for(unsigned i=0;i<n;++i)value=value*256+std::to_integer<unsigned>(p[i]);return value;}
int main(int argc,char** argv){
 if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
  rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");require(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto* sdk=runtime.memory();Memory memory(*sdk);auto handle=memory.allocate(16384);uint32_t address=handle.guest_address();PPCContext ctx{};ctx.r4.u32=address;uint8_t* base=sdk->virtual_membase();
#ifdef REX_CHECKED_ACCESS_HEADER
  using namespace brutal::instrumentation;
  auto rejected=[&](auto body,Kind kind,Result expected=Result::ok){try{body();}catch(const Fault& error){require(error.kind==kind);if(expected!=Result::ok)require(error.memory_result==expected);return;}throw std::runtime_error("missing explicit rejection");};
  base=nullptr; // Any escaped base dereference would fault under the watchdog.
  rejected([&]{syn_lwz(ctx,base);},Kind::unbound);
  Scope scope(memory,*sdk);scope.bind(handle,16384);
#endif
  std::array<std::byte,64> input{},observed{};for(unsigned i=0;i<input.size();++i)input[i]=std::byte(0x80+i);
  require(memory.write(handle,0,input)==Memory::Result::ok);
  struct Load{PPCFunc* function;unsigned width;bool sign;};
  const Load loads[]={{syn_lbz,1,false},{syn_lhz,2,false},{syn_lha,2,true},{syn_lwz,4,false},{syn_mm_lwz,4,false},{syn_lwa,4,true},{syn_ld,8,false}};
  for(auto test:loads){ctx.r4.u32=address;test.function(ctx,base);uint64_t expected=oracle(input.data(),test.width);if(test.sign)expected=test.width==2?uint64_t(int64_t(int16_t(expected))):uint64_t(int64_t(int32_t(expected)));require(ctx.r3.u64==expected);}
  const Load stores[]={{syn_stb,1,false},{syn_sth,2,false},{syn_stw,4,false},{syn_mm_stw,4,false},{syn_std,8,false}};
  for(auto test:stores){std::array<std::byte,64> guard;guard.fill(std::byte{0xcc});require(memory.write(handle,0,guard)==Memory::Result::ok);ctx.r3.u64=0xfedcba9876543210ull;ctx.r4.u32=address;test.function(ctx,base);require(memory.read(handle,0,observed)==Memory::Result::ok);for(unsigned i=0;i<64;++i)require(observed[i]==(i<test.width?std::byte((0xfedcba9876543210ull>>(8*(test.width-1-i)))&255):std::byte{0xcc}));}
  require(memory.write(handle,0,input)==Memory::Result::ok);ctx.r4.u32=address;ctx.r5.u32=0;syn_lwbrx(ctx,base);require(ctx.r3.u64==0x83828180u);syn_lhbrx(ctx,base);require(ctx.r3.u64==0x8180);
  ctx.r3.u32=0x12345678;syn_stwbrx(ctx,base);require(memory.read(handle,0,observed)==Memory::Result::ok);require(observed[0]==std::byte{0x78}&&observed[3]==std::byte{0x12});
  ctx.r3.u16=0x1234;syn_sthbrx(ctx,base);require(memory.read(handle,0,observed)==Memory::Result::ok);require(observed[0]==std::byte{0x34}&&observed[1]==std::byte{0x12});
  ctx.fpscr.InitHost();ctx.r4.u32=address;
  require(memory.store32be(handle,0,0x3fc00000)==Memory::Result::ok);syn_lfs(ctx,base);require(ctx.f3.f64==1.5);
  ctx.f3.f64=1.25;syn_stfd(ctx,base);syn_lfd(ctx,base);require(ctx.f3.u64==0x3ff4000000000000ull);
  ctx.f3.f64=1.5;syn_stfs(ctx,base);require(memory.read(handle,0,observed)==Memory::Result::ok);require(oracle(observed.data(),4)==0x3fc00000);
  require(memory.write(handle,0,input)==Memory::Result::ok);ctx.r4.u32=address;ctx.r5.u32=7;syn_lvx(ctx,base);for(unsigned i=0;i<16;++i)require(ctx.v3.u8[15-i]==std::to_integer<unsigned>(input[i]));
  for(unsigned i=0;i<16;++i)ctx.v3.u8[i]=uint8_t(i+1);syn_stvx(ctx,base);require(memory.read(handle,0,observed)==Memory::Result::ok);for(unsigned i=0;i<16;++i)require(observed[i]==std::byte(16-i));require(observed[16]==input[16]);
#ifdef REX_CHECKED_ACCESS_HEADER
  for(auto test:loads){for(uint32_t offset:{1u,4095u}){require(memory.write(handle,offset,input)==Memory::Result::ok);ctx.r4.u32=address+offset;test.function(ctx,base);uint64_t expected=oracle(input.data(),test.width);if(test.sign)expected=test.width==2?uint64_t(int64_t(int16_t(expected))):uint64_t(int64_t(int32_t(expected)));require(ctx.r3.u64==expected);}}
  for(auto test:stores){ctx.r4.u32=address+4095;ctx.r3.u64=0xfedcba9876543210ull;test.function(ctx,base);require(memory.read(handle,4095,observed)==Memory::Result::ok);require(oracle(observed.data(),test.width)==(test.width==8?0xfedcba9876543210ull:0xfedcba9876543210ull&((1ull<<(test.width*8))-1)));}
  auto* heap=sdk->LookupHeap(address);require(heap->Protect(address,4096,1));ctx.r4.u32=address+4095;ctx.r3.u64=0x11;
  rejected([&]{syn_std(ctx,base);},Kind::memory,Result::protection);require(heap->Protect(address,4096,3));
  std::array<std::byte,8> before{},after{};require(memory.read(handle,4095,before)==Result::ok);
  require(heap->Protect(address+4096,4096,1));rejected([&]{syn_std(ctx,base);},Kind::memory,Result::protection);
  require(memory.read(handle,4095,after)==Result::ok&&before==after);require(heap->Protect(address+4096,4096,3));
  require(heap->Decommit(address+4096,4096));rejected([&]{syn_std(ctx,base);},Kind::memory,Result::uncommitted);
  ctx.r3.u64=0x5555;rejected([&]{syn_ld(ctx,base);},Kind::memory,Result::uncommitted);require(ctx.r3.u64==0x5555);
  require(heap->AllocFixed(address+4096,4096,4096,2,3));
  ctx.r4.u32=0xfffffffcu;rejected([&]{syn_ld(ctx,base);},Kind::memory,Result::invalid_range);
  ctx.r4.u32=address;ctx.r5.u32=0;
  for(auto* fn:{syn_lwarx,syn_ldarx,syn_stwcx,syn_stdcx,syn_lvlx,syn_lvrx,syn_stvlx,syn_stvrx})rejected([&]{fn(ctx,base);},Kind::unsupported_raw_atomic);
  rejected([&]{syn_direct(ctx,base);},Kind::unsupported_call);require(called==0);
  scope.allow_synthetic(0x2000,synthetic_callee);ctx.r3.u64=9;syn_direct(ctx,base);require(ctx.r3.u64==10&&called==1);
  ctx.ctr.u64=0x2000;syn_indirect(ctx,base);require(ctx.r3.u64==11&&called==2);
  ctx.ctr.u64=0x2001;rejected([&]{syn_indirect(ctx,base);},Kind::unsupported_call);
  {Scope nested(memory,*sdk);nested.bind(handle,16384);ctx.ctr.u64=0x2000;rejected([&]{syn_indirect(ctx,base);},Kind::unsupported_call);}
  ctx.ctr.u64=0x2000;syn_indirect(ctx,base);require(called==3);
  scope.allow_synthetic(0x3000,synthetic_callback);ctx.ctr.u64=0x3000;bool propagated=false;try{syn_indirect(ctx,base);}catch(const std::runtime_error& e){propagated=std::string_view(e.what())=="original callback failure";}require(propagated&&called==4);
  auto reserved_memory=memory.allocate(4096,false);{Scope nested(memory,*sdk);nested.bind(reserved_memory,4096);ctx.r4.u32=reserved_memory.guest_address();rejected([&]{syn_lwz(ctx,base);},Kind::memory);}require(memory.release(reserved_memory)==Memory::Result::ok);
  auto stale=memory.allocate(32);{Scope nested(memory,*sdk);nested.bind(stale,32);require(memory.release(stale)==Memory::Result::ok);ctx.r4.u32=stale.guest_address();rejected([&]{syn_lwz(ctx,base);},Kind::memory);}
  ctx.r4.u32=address;
  std::array<std::byte,8> initial{};initial.fill(std::byte{0x11});require(memory.write(handle,0,initial)==Memory::Result::ok);
  std::atomic<unsigned> errors{0};std::vector<std::thread> threads;
  for(unsigned t=0;t<4;++t)threads.emplace_back([&,t]{Scope local(memory,*sdk);local.bind(handle,16384);PPCContext c{};c.r4.u32=address;for(unsigned i=0;i<250;++i){if(t<2){c.r3.u64=t?0x2222222222222222ull:0x1111111111111111ull;syn_std(c,nullptr);}else{syn_ld(c,nullptr);if(c.r3.u64!=0x1111111111111111ull&&c.r3.u64!=0x2222222222222222ull)++errors;}}});
  for(auto& thread:threads)thread.join();require(errors==0);
#endif
  ctx.r4.u32=address;require(memory.write(handle,0,input)==Memory::Result::ok);
  std::array<long long,5> timings{};volatile uint64_t checksum=0;
  for(auto& sample:timings){auto start=std::chrono::steady_clock::now();for(unsigned i=0;i<256;++i){syn_ld(ctx,base);checksum=checksum+ctx.r3.u64;}sample=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count();}
  std::sort(timings.begin(),timings.end());
#ifdef REX_CHECKED_ACCESS_HEADER
  auto read_mmio=[](void*,void*,uint32_t)->uint32_t{throw std::runtime_error("MMIO callback must not execute");};auto write_mmio=[](void*,void*,uint32_t,uint32_t){throw std::runtime_error("MMIO callback must not execute");};
  require(sdk->AddVirtualMappedRange(0x7fc80000,0xfffffff0,16,nullptr,read_mmio,write_mmio));ctx.r4.u32=0x7fc80000;rejected([&]{syn_lwz(ctx,base);},Kind::mmio);rejected([&]{syn_mm_lwz(ctx,base);},Kind::mmio);ctx.r4.u32=0x7fc7fffc;rejected([&]{syn_ld(ctx,base);},Kind::mmio);
  std::printf("{\"group\":\"instrumented_execution\",\"result\":\"PASS\",\"assertions\":%u,\"decoded_instructions\":32,\"concurrent_operations\":1000,\"median_256_loads_ns\":%lld,\"checksum\":%llu}\n",checks,timings[2],(unsigned long long)checksum);
#else
  std::printf("{\"group\":\"default_execution\",\"result\":\"PASS\",\"assertions\":%u,\"median_256_loads_ns\":%lld,\"checksum\":%llu}\n",checks,timings[2],(unsigned long long)checksum);
#endif
  return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s after %u assertions\n",e.what(),checks);return 1;}
}
