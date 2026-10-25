// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/checked_tls.h"
#include "runtime/cooperative_stop.h"
#include <cstdlib>
#include <typeinfo>

namespace {
using namespace brutal::foundation;
struct Polymorphic { virtual ~Polymorphic()=default; };
struct Derived final : Polymorphic {};
thread_local unsigned native_tls=0;
}
ProbeResult portable_probe() {
  ProbeResult result;
  auto check=[&](bool ok){++(ok?result.passed:result.failed);};
  auto rejects=[&](Fault fault, auto action){
    try { action(); check(false); }
    catch(const Error& error) { check(error.fault==fault); }
  };
  try {
    static_assert(std::atomic<unsigned>::is_always_lock_free);
    void* backing=std::aligned_alloc(4096,8192);
    check(backing && reinterpret_cast<uintptr_t>(backing)%4096==0);
    std::free(backing);
    Derived derived; Polymorphic* base=&derived;
    check(dynamic_cast<Derived*>(base)==&derived && typeid(*base)==typeid(Derived));
    Memory memory(65536);
    auto a=memory.reserve(8192);
    check(a.guest%4096==0);
    rejects(Fault::uncommitted,[&]{memory.load32(a.guest);});
    memory.commit(a);
    memory.store32(a.guest+4094,0x12345678);
    check(memory.load32(a.guest+4094)==0x12345678);
    auto bytes=memory.read(memory.translate(a.guest+4094,4,Access::read));
    check(bytes[0]==std::byte{0x12} && bytes[3]==std::byte{0x78});
    rejects(Fault::bounds,[&]{memory.load32(a.guest+8190);});
    auto view=memory.translate(a.guest,4,Access::read);
    memory.protect(a,Permission::read);
    rejects(Fault::protection,[&]{memory.store32(a.guest,7);});
    memory.decommit(a);
    rejects(Fault::stale,[&]{memory.read(view);});
    memory.commit(a);
    check(memory.load32(a.guest+4094)==0);
    memory.release(a);
    rejects(Fault::stale,[&]{memory.release(a);});
    rejects(Fault::bounds,[&]{memory.load32(a.guest);});
    rejects(Fault::exhausted,[&]{memory.allocate(UINT32_MAX);});
    check(memory.live_allocations()==0);

    Tls tls(memory,4,16,{});
    {
      Tls::Scope main_scope(tls);
      const auto slot=tls.allocate_slot();
      tls.set(slot,0xaabbccdd); native_tls=77;
      std::atomic<unsigned> successes{0},errors{0};
      auto worker=[&](unsigned value){
        try {
          Tls::Scope scope(tls);
          if(tls.get(slot)!=0 || native_tls!=0) {++errors;return;}
          native_tls=value; tls.set(slot,value);
          if(tls.get(slot)==value && native_tls==value)++successes; else ++errors;
        } catch(...) {++errors;}
      };
      std::thread first(worker,11);
      // If second construction fails, join the already-owned thread first.
      try {std::thread second(worker,22);second.join();}
      catch(...) {first.join();throw;}
      first.join();
      check(successes==2 && errors==0);
      check(native_tls==77 && tls.get(slot)==0xaabbccdd);
      tls.free_slot(slot);
      rejects(Fault::slot,[&]{tls.get(slot);});
    }
    check(memory.live_allocations()==0);
    brutal::synthetic::CooperativeStop stop;
    check(stop.request() && !stop.request());
    check(stop.safe_boundary());
    stop.exited();stop.joined();stop.reclaimed();stop.unloaded();
    check(stop.state()==brutal::synthetic::CooperativeStop::State::unloaded);
  } catch(...) {++result.failed;}
  return result;
}
