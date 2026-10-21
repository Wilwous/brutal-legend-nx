// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Synthetic bytes only. No game input, guest entry or game-derived code.
#include "runtime/sdk_checked_memory.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <thread>
using Memory = brutal::foundation::SdkCheckedMemory;
using Result = Memory::Result;
static unsigned checks=0;
static void require(bool v) { ++checks; if(!v) throw std::runtime_error("guest memory oracle mismatch"); }
static uint32_t mmio_reads=0;
static uint32_t mmio_read(void*,void*,uint32_t) { ++mmio_reads; return 0; }
static void mmio_write(void*,void*,uint32_t,uint32_t) { ++mmio_reads; }
int main(int argc,char** argv) {
 if(argc!=2)return 2;
 std::setbuf(stdout,nullptr); rex::InitLoggingEarly();
 try {
  std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
  rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");
  require(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto* sdk=runtime.memory();Memory memory(*sdk),foreign(*sdk);
  for(uint32_t size:{0u,0xffffffffu}){bool rejected=false;try{memory.allocate(size);}catch(const std::invalid_argument&){rejected=true;}require(rejected);}
  auto h=memory.allocate(16384);auto address=h.guest_address();auto* heap=sdk->LookupHeap(address);
  std::array<std::byte,8> pattern{std::byte{1},std::byte{2},std::byte{3},std::byte{4},std::byte{5},std::byte{6},std::byte{7},std::byte{8}},out{};
  require(memory.write(h,4092,pattern)==Result::ok);
  require(memory.read(h,4092,out)==Result::ok && out==pattern);
  require(memory.store32be(h,4095,0x89abcdef)==Result::ok);
  uint32_t decoded=0;require(memory.load32be(h,4095,decoded)==Result::ok&&decoded==0x89abcdef);
  out.fill(std::byte{0x55});require(memory.fetch(h,0,out)==Result::unsupported_fetch);
  for(auto b:out)require(b==std::byte{0x55});
  require(heap->Protect(address,4096,1));
  require(memory.write(h,0,pattern)==Result::protection); // RW neighbor shares host page.
  require(memory.write(h,4096,pattern)==Result::ok);
  auto before=std::array<std::byte,8>{};require(memory.read(h,4092,before)==Result::ok);
  require(memory.write(h,4092,pattern)==Result::protection);
  require(memory.read(h,4092,out)==Result::ok&&out==before); // no partial copy.
  require(heap->Protect(address+4096,4096,0));
  out.fill(std::byte{0x66});require(memory.read(h,4092,out)==Result::protection);
  for(auto b:out)require(b==std::byte{0x66});
  require(heap->Protect(address+4096,4096,3));require(heap->Decommit(address+8192,4096));
  require(memory.read(h,8188,out)==Result::uncommitted);
  require(memory.write(h,8188,pattern)==Result::uncommitted);
  require(memory.read(h,8192,out)==Result::uncommitted);
  require(memory.write(h,UINT64_MAX-3,pattern)==Result::invalid_range);
  require(memory.read(h,16380,out)==Result::invalid_range);
  require(memory.read(h,0,std::span<std::byte>{})==Result::invalid_range);
  require(foreign.read(h,0,out)==Result::ownership);
  Memory::Allocation empty;require(memory.read(empty,0,out)==Result::ownership);
  auto small=memory.allocate(17);require(memory.write(small,16,pattern)==Result::invalid_range);
  auto reserved=memory.allocate(4096,false);require(memory.read(reserved,0,out)==Result::uncommitted);
  require(memory.fetch(reserved,0,out)==Result::uncommitted);
  require(memory.release(reserved)==Result::ok);
  auto stale=memory.allocate(4096);auto stale_addr=stale.guest_address();auto* stale_heap=sdk->LookupHeap(stale_addr);
  require(stale_heap->Release(stale_addr));require(stale_heap->AllocFixed(stale_addr,16384,16384,3,3));
  require(memory.read(stale,0,out)==Result::ownership);require(memory.release(stale)==Result::ownership);
  require(stale_heap->Release(stale_addr));
  auto released=memory.allocate(4096);require(memory.release(released)==Result::ok);
  auto replacement=memory.allocate(4096);require(memory.read(released,0,out)==Result::ownership);
  require(memory.release(released)==Result::ownership);require(memory.write(replacement,0,pattern)==Result::ok);
  // Fully validate a write crossing from writable into inaccessible pages.
  require(heap->Protect(address,4096,3));require(heap->Protect(address+4096,4096,1));
  require(memory.read(h,4092,before)==Result::ok);
  require(memory.write(h,4092,pattern)==Result::protection);
  require(memory.read(h,4092,out)==Result::ok&&out==before);
  // SDK primitive checks overflow even without the adapter's object bounds.
  int owner=0;std::byte byte{};
  require(heap->CopyOwnedMemory(address,&owner,0xfffffffcu,&byte,8,Memory::Access::read)==Result::invalid_range);
  // MMIO lookup uses the SDK mask semantics, never invokes a device callback.
  require(sdk->AddVirtualMappedRange(0x7fc80000,0xfffffff0,16,nullptr,mmio_read,mmio_write));
  require(heap->CopyOwnedMemory(address,&owner,0x7fc7fffc,out.data(),8,Memory::Access::read)==Result::mmio);
  require(heap->CopyOwnedMemory(address,&owner,0x7fc8000f,out.data(),1,Memory::Access::write)==Result::mmio);
  require(heap->CopyOwnedMemory(address,&owner,0x7fc80010,out.data(),1,Memory::Access::read)==Result::mmio);
  require(mmio_reads==0);
  require(memory.release(h)==Result::ok);require(memory.read(h,0,out)==Result::ownership);
  // Concurrent complete writes/reads: a reader sees one whole 8-byte pattern.
  auto concurrent=memory.allocate(4096);std::array<std::byte,8> aa,bb;aa.fill(std::byte{0x11});bb.fill(std::byte{0x22});
  require(memory.write(concurrent,0,aa)==Result::ok);
  std::atomic<unsigned> errors{0},operations{0};std::vector<std::thread> threads;
  for(unsigned t=0;t<4;++t)threads.emplace_back([&,t]{for(unsigned i=0;i<500;++i){
    if(t<2){if(memory.write(concurrent,0,t?bb:aa)!=Result::ok)++errors;}
    else{std::array<std::byte,8> value{};if(memory.read(concurrent,0,value)!=Result::ok||(value!=aa&&value!=bb))++errors;}
    ++operations;
  }});
  for(auto& t:threads)t.join();require(errors==0&&operations==2000);
  // Real SDK Protect races with checked writes; heap lock covers check + copy.
  threads.clear();
  threads.emplace_back([&]{for(unsigned i=0;i<400;++i)if(!sdk->LookupHeap(concurrent.guest_address())->Protect(concurrent.guest_address(),4096,i%2?3:1))++errors;});
  threads.emplace_back([&]{for(unsigned i=0;i<2000;++i){auto result=memory.write(concurrent,0,aa);if(result!=Result::ok&&result!=Result::protection)++errors;}});
  for(auto& t:threads)t.join();require(errors==0);
  // Adapter release races with reads; only success-before-release or ownership.
  threads.clear();std::atomic<bool> start{false};
  for(unsigned t=0;t<2;++t)threads.emplace_back([&]{while(!start.load())std::this_thread::yield();for(unsigned i=0;i<1000;++i){std::array<std::byte,8> v{};auto result=memory.read(concurrent,0,v);if(result!=Result::ok&&result!=Result::ownership)++errors;}});
  start=true;require(memory.release(concurrent)==Result::ok);for(auto& t:threads)t.join();require(errors==0);
  // Fixed-work benchmark; raw path is a host-local byte copy, NOT a guest bypass.
  auto benchmark=memory.allocate(4096);std::array<std::byte,8> local{};volatile unsigned sink=0;
  require(memory.write(benchmark,0,local)==Result::ok);
  constexpr unsigned iterations=2000;
  auto begin=std::chrono::steady_clock::now();
  for(unsigned i=0;i<iterations;++i){require(memory.read(benchmark,0,out)==Result::ok);sink=sink+std::to_integer<unsigned>(out[i%8]);}
  auto middle=std::chrono::steady_clock::now();
  for(unsigned i=0;i<iterations;++i){std::memcpy(out.data(),local.data(),8);sink=sink+std::to_integer<unsigned>(out[i%8]);}
  auto end=std::chrono::steady_clock::now();
  auto ns=[](auto a,auto b){return std::chrono::duration_cast<std::chrono::nanoseconds>(b-a).count();};
  std::printf("{\"group\":\"guest_memory\",\"result\":\"PASS\",\"assertions\":%u,\"concurrent_operations\":6400,\"benchmark_iterations\":%u,\"checked_ns\":%lld,\"local_copy_ns\":%lld,\"sink\":%u}\n",checks,iterations,(long long)ns(begin,middle),(long long)ns(middle,end),sink);
  return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s after %u assertions\n",e.what(),checks);return 1;}
}
