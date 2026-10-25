// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/leaf.h"
#include <cstring>
#include <functional>
REX_EXTERN(leaf_82010000);
using namespace brutal::foundation;
using brutal::horizon::LeafScope;
LeafResult run_leaf_probe(){
  LeafResult result;
  auto check=[&](bool ok){if(ok)++result.passed;else ++result.failed;};
  auto refuses=[&](Fault expected,auto action){
    try {action();check(false);}catch(const Error& e){check(e.fault==expected);}catch(...){check(false);}
  };
  try {
    Memory memory(64*1024);result.memory_initialized=true;
    auto data=memory.allocate(8192);PPCContext ctx{};
    ctx.r4.u64=data.guest;ctx.r1.u64=0x12340;ctx.r2.u64=0x23450;ctx.r13.u64=0;
    ctx.r14.u64=0x1122334455667788;ctx.f14.u64=0x7654321012345678;
    ctx.v14.u64[0]=0xabcdef;ctx.lr=0x82020000;
    auto before=ctx;
    memory.store32(data.guest,41);
    refuses(Fault::no_thread,[&]{leaf_82010000(ctx,nullptr);});
    {
      LeafScope scope(memory,ctx);
      leaf_82010000(ctx,nullptr);
      result.entry_invoked=scope.entries==1;result.expected_result=ctx.r3.u64==42&&memory.load32(data.guest)==42;
      check(result.entry_invoked);check(result.expected_result);
      check(scope.reads==1&&scope.writes==1);
      // Bytewise compare the complete real SDK context except the only changed GPR.
      before.r3=ctx.r3;check(std::memcmp(&before,&ctx,sizeof(ctx))==0);
      check(ctx.r1.u64==0x12340&&ctx.r2.u64==0x23450&&ctx.r13.u64==0);
      check(memory.read(memory.translate(data.guest,4,Access::read))==std::vector<std::byte>{std::byte{0},std::byte{0},std::byte{0},std::byte{42}});
      PPCContext foreign{};
      refuses(Fault::no_thread,[&]{leaf_82010000(foreign,nullptr);});
      refuses(Fault::argument,[&]{leaf_82010000(ctx,reinterpret_cast<uint8_t*>(uintptr_t(32)));});
      refuses(Fault::unsupported,[&]{LeafScope nested(memory,ctx);});
      memory.protect(data,Permission::read);
      refuses(Fault::protection,[&]{leaf_82010000(ctx,nullptr);});
      check(memory.load32(data.guest)==42&&scope.writes==1); // refused store had no effect
      memory.protect(data,Permission::read_write);
      ctx.r4.u64=data.guest+4094;memory.store32(ctx.r4.u32,100);
      leaf_82010000(ctx,nullptr);check(memory.load32(ctx.r4.u32)==101); // owned cross-page span
      ctx.r4.u64=data.guest+data.size-2;
      refuses(Fault::bounds,[&]{leaf_82010000(ctx,nullptr);});
      ctx.r4.u64=0;
      refuses(Fault::argument,[&]{leaf_82010000(ctx,nullptr);});
      ctx.r4.u64=UINT32_MAX-1;
      refuses(Fault::bounds,[&]{leaf_82010000(ctx,nullptr);});
      ctx.r4.u64=0x7fc80000;
      refuses(Fault::bounds,[&]{leaf_82010000(ctx,nullptr);}); // MMIO outside arena
      ctx.r4.u64=data.guest;memory.store32(data.guest,UINT32_MAX);
      leaf_82010000(ctx,nullptr);check(ctx.r3.u64==0x100000000ull&&memory.load32(data.guest)==0);
      memory.decommit(data);
      refuses(Fault::uncommitted,[&]{leaf_82010000(ctx,nullptr);});
      memory.commit(data);check(memory.load32(data.guest)==0);
      Memory other(4096);auto alien=other.allocate(4);
      refuses(Fault::ownership,[&]{memory.release(alien);});
      other.release(alien);check(other.live_allocations()==0);
      memory.release(data);
      refuses(Fault::bounds,[&]{leaf_82010000(ctx,nullptr);});
      refuses(Fault::stale,[&]{memory.release(data);});
      result.invocations=scope.entries;result.reads=scope.reads;result.writes=scope.writes;
    }
    refuses(Fault::no_thread,[&]{leaf_82010000(ctx,nullptr);});
    check(memory.live_allocations()==0);result.cleanup=memory.live_allocations()==0;
    refuses(Fault::argument,[&]{Memory empty(0);});
    Memory small(4096);
    refuses(Fault::exhausted,[&]{small.allocate(UINT32_MAX);});
    check(small.live_allocations()==0);
  }catch(...){++result.failed;}
  return result;
}
