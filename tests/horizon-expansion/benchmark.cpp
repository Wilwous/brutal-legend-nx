// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Host-only baseline, never included in the NRO.
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#include <chrono>
#include <cstdio>
using namespace brutal::horizon;
using Clock=std::chrono::steady_clock;
namespace {
bool first=true;
void emit(const char*name,std::vector<double> us){double sum=0,minimum=us.at(0),maximum=minimum;for(auto v:us){sum+=v;minimum=std::min(minimum,v);maximum=std::max(maximum,v);}std::printf("%s\"%s\":{\"samples\":%zu,\"mean_us\":%.6f,\"min_us\":%.6f,\"max_us\":%.6f}",first?"":",",name,us.size(),sum/us.size(),minimum,maximum);first=false;}
template<class F>void measure(const char*name,unsigned count,F fn){std::vector<double> us;us.reserve(count);for(unsigned i=0;i<count;++i){auto t=Clock::now();fn();us.push_back(std::chrono::duration<double,std::micro>(Clock::now()-t).count());}emit(name,std::move(us));}
void require(bool b){if(!b)throw Error(Fault::unsupported,"benchmark contract failed");}
}
int main(){try{
 std::printf("{\"host_only\":true,\"units\":\"microseconds\",\"measurements\":{");
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto token=slots.allocate();
 {GuestThread guest(space,image.life(),slots,token,7,image.tls_template());auto buffer=space.allocate(4096);
  {Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks);
   measure("generated_add64_checked_dispatch",2000,[&]{guest.context.r3.u64=1;guest.context.r4.u64=2;guest.context.lr=0x8201feed;scope.invoke(xex_add64);require(guest.context.r3.u64==3);});
   measure("checked_be_store_load32",2000,[&]{space.store<uint32_t>(buffer.address,0x10203040);require(space.load<uint32_t>(buffer.address)==0x10203040);});
   measure("checked_pointer_snapshot64",1000,[&]{require(space.read(buffer.address,64).size()==64);});
   measure("generated_xex_two_sdk_calls",64,[&]{auto seed=guest.get_tls();guest.context.r3.u64=4;guest.context.lr=0x8201feed;scope.invoke(xex_entry);require(guest.context.r3.u64==seed+11);});
  }
  measure("guest_stack_pcr_tls_init_cleanup",32,[&]{GuestThread other(space,image.life(),slots,token);other.cleanup();});
  measure("host_native_guest_start_join_cleanup",8,[&]{GuestThread other(space,image.life(),slots,token);GuestTask task(other);require(task.start([&]{GuestThread::Execution call(other);other.set_tls(31);}));require(task.join()&&!task.callback_error()&&other.get_tls()==31);other.cleanup();});
  space.release(buffer);guest.cleanup();
 }
 slots.release(token);require(image.unload()&&space.live()==0);
 measure("synthetic_image_load_unload_with_arena",8,[&]{Space arena;XexImage mod(arena,xex_bytes,{xex_bytes,xex_generated,xex_entry});require(mod.unload()&&arena.live()==0);});
 std::vector<double> latency;
 for(unsigned i=0;i<8;++i){Space arena;XexImage mod(arena,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots registry;auto t=registry.allocate();GuestThread guest(arena,mod.life(),registry,t);GuestTask task(guest);brutal::synthetic::CooperativeStop stop;brutal::horizon::Event ready,gate;std::atomic<bool> stopped{};
  require(task.start([&]{guest.context.lr=0x8201feed;Scope scope(guest,mod.functions(),mod.life().identity,&stop,[&](uint32_t pc){if(pc==0x82010074){ready.signal();if(!gate.wait())throw Error(Fault::unsupported,"latency gate");}},expansion_plan_ptr);mod.lend(scope);HookFrame hooks(scope,xex_hooks);try{scope.invoke(xex_forever);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}));
  require(ready.wait());auto begin=Clock::now();require(stop.request());gate.signal();require(task.join()&&!task.callback_error()&&stopped);latency.push_back(std::chrono::duration<double,std::micro>(Clock::now()-begin).count());stop.joined();guest.cleanup();stop.reclaimed();require(mod.unload()&&arena.live()==0);stop.unloaded();registry.release(t);
 }
 emit("host_stop_request_to_join",std::move(latency));require(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::printf("},\"failed\":0,\"remaining\":0,\"leases\":0}\n");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"benchmark failure: %s\n",e.what());return 1;}}
