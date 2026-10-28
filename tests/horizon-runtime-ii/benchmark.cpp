// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Host only. Identical work with tracing on/off; not Switch performance.
#include "runtime/horizon/query.h"
#include "xex-data.h"
#include "ii-metadata.h"
#include <chrono>
#include <cstdio>
#include <cmath>
#include <memory>
#include <cstring>
using namespace brutal::horizon;
using Clock=std::chrono::steady_clock;
namespace {
bool first=true;
void require(bool b){if(!b)throw Error(Fault::unsupported,"II benchmark contract failed");}
void emit(const char*name,const std::vector<double>&us,unsigned work){double sum=0,minimum=us.at(0),maximum=minimum;for(auto v:us){sum+=v;minimum=std::min(minimum,v);maximum=std::max(maximum,v);}double mean=sum/us.size(),variance=0;for(auto v:us)variance+=(v-mean)*(v-mean);std::printf("%s\"%s\":{\"samples\":%zu,\"operations_per_sample\":%u,\"mean_us\":%.6f,\"min_us\":%.6f,\"max_us\":%.6f,\"stddev_us\":%.6f}",first?"":",",name,us.size(),work,mean,minimum,maximum,std::sqrt(variance/us.size()));first=false;}
template<class F>void measure(const char*name,unsigned samples,unsigned count,brutal::diagnostic::Trace&trace,F fn){std::vector<double> values;for(unsigned s=0;s<samples;++s){trace.clear();auto start=Clock::now();for(unsigned i=0;i<count;++i)fn();values.push_back(std::chrono::duration<double,std::micro>(Clock::now()-start).count()/count);require(trace.dropped()==0&&!trace.broken());}emit(name,values,count);}
}
int main(int argc,char**argv){if(argc>2||(argc==2&&std::strcmp(argv[1],"--reverse")))return 2;try{std::printf("{\"host_only\":true,\"units\":\"microseconds_per_operation\",\"modes\":[");
 for(unsigned index=0;index<2;++index){unsigned mode=argc==2?1-index:index;auto trace=std::make_unique<brutal::diagnostic::Trace>(ii_metadata);brutal::diagnostic::Binding root(mode?trace.get():nullptr,0,0);std::printf("%s{\"tracing\":%s,\"measurements\":{",index?",":"",mode?"true":"false");first=true;
  Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto token=slots.allocate();
  {GuestThread guest(space,image.life(),slots,token,7,image.tls_template());auto data=space.allocate(4096);space.store<uint32_t>(data.address,1);space.store<uint32_t>(data.address+4,2);brutal::synthetic::CooperativeStop stop;
   {Scope scope(guest,image.functions(),image.life().identity,&stop,{},expansion_plan_ptr);image.lend(scope);scope.lend(data);ImportFrame imports(image,guest);QueryFrame query(image,guest);HookFrame hooks(scope,xex_hooks);
    measure("generated_dispatch",20,32,*trace,[&]{guest.context.r3.u64=1;guest.context.r4.u64=2;guest.context.lr=0x8201feed;scope.invoke(xex_add64);require(guest.context.r3.u64==3);});
    measure("checked_memory_store_load32",20,32,*trace,[&]{space.store<uint32_t>(data.address,1);require(space.load<uint32_t>(data.address)==1);});
    measure("pointer_snapshot64",20,32,*trace,[&]{require(space.read(data.address,64).size()==64);});
    measure("sdk_admission_component",20,32,*trace,[&]{image.admit_import(0xee,XexImage::Signature::query_basic28);});
    measure("generated_query_sdk_out28",20,32,*trace,[&]{guest.context.r3.u64=image.section(3).address;guest.context.r4.u64=image.section(3).address+0xa00;guest.context.r5.u64=0;guest.context.lr=0x8201feed;scope.invoke(xex_query_only);require(guest.context.r3.u64==0);});
    measure("guest_tls_read",20,32,*trace,[&]{require(guest.get_tls()==7);});
    measure("running_stop_poll_component",20,32,*trace,[&]{scope.poll(0x82010074);});
    measure("generated_nested_loops_hooks",20,32,*trace,[&]{guest.context.lr=0x8201feed;scope.invoke(xex_nested);require(guest.context.r3.u64==6);});
   }
   measure("guest_stack_pcr_tls_init_cleanup",8,4,*trace,[&]{GuestThread other(space,image.life(),slots,token);other.cleanup();});
   measure("host_native_guest_start_join_cleanup",8,1,*trace,[&]{GuestThread other(space,image.life(),slots,token);GuestTask task(other);require(task.start([&]{GuestThread::Execution e(other);other.set_tls(31);}));require(task.join()&&!task.callback_error()&&other.get_tls()==31);other.cleanup();});
   space.release(data);guest.cleanup();
  }
  slots.release(token);require(image.unload()&&space.live()==0);
  measure("image_load_unload_with_arena",12,1,*trace,[&]{Space arena;XexImage mod(arena,xex_bytes,{xex_bytes,xex_generated,xex_entry});require(mod.unload()&&arena.live()==0);});
  std::vector<double> latency;
  for(unsigned i=0;i<8;++i){trace->clear();Space arena;XexImage mod(arena,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots registry;auto t=registry.allocate();GuestThread guest(arena,mod.life(),registry,t);GuestTask task(guest);brutal::synthetic::CooperativeStop stop;brutal::horizon::Event ready,gate;bool stopped=false;
   require(task.start([&]{guest.context.lr=0x8201feed;Scope scope(guest,mod.functions(),mod.life().identity,&stop,[&](uint32_t pc){if(pc==0x82010074){ready.signal();if(!gate.wait())throw Error(Fault::unsupported,"benchmark stop gate");}},expansion_plan_ptr);mod.lend(scope);HookFrame hooks(scope,xex_hooks);try{scope.invoke(xex_forever);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}));
   require(ready.wait());auto start=Clock::now();require(stop.request());gate.signal();require(task.join()&&!task.callback_error()&&stopped);latency.push_back(std::chrono::duration<double,std::micro>(Clock::now()-start).count());stop.joined();guest.cleanup();stop.reclaimed();require(mod.unload()&&arena.live()==0);stop.unloaded();registry.release(t);require(trace->dropped()==0&&!trace->broken());
  }emit("host_stop_request_to_join",latency,1);require(Space::totals()==std::pair<size_t,size_t>{0,0});std::printf("},\"failed\":0}");
 }
 std::puts("],\"failed\":0,\"remaining\":0,\"leases\":0}");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"II benchmark failure: %s\n",e.what());return 1;}}
