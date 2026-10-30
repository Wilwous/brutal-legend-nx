// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "native-data.h"
#include <iostream>
using namespace brutal::horizon;using namespace brutal::horizon::native_image;
namespace rex::kernel::xboxkrnl {
#include "sdk-compare.inc"
}
namespace {
thread_local Scope* active_scope{};
struct Active {Scope*previous;Active(Scope&s):previous(active_scope){require(!previous,"nested hook scope");active_scope=&s;}~Active(){active_scope=previous;}};
struct Frame {Space&space;Image&image;GuestThread&guest;unsigned entered{},completed{};inline static thread_local Frame*current{};Frame(Space&s,Image&i,GuestThread&g):space(s),image(i),guest(g){require(!current,"nested native import frame");current=this;}~Frame(){current=nullptr;}};
}
extern "C" void __imp__RtlCompareMemoryUlong(PPCContext&ctx,uint8_t*base){
 require(!base&&Frame::current&&&Frame::current->guest.context==&ctx,"native import context");auto&f=*Frame::current;auto&scope=Scope::current(ctx);scope.import_target(0x82030004);
 uint32_t a=ctx.r3.u32,n=ctx.r4.u32;require(n&&n<=64&&(n%4)==0&&(a%4)==0&&a>=0x82020200&&uint64_t(a)+n<=0x82020240,"native IN contract");scope.admit_range(a,n);auto bytes=f.space.read(a,n);std::array<rex::be<uint32_t>,16>scratch{};for(size_t i=0;i<n/4;++i){uint32_t v=0;for(unsigned j=0;j<4;++j)v=(v<<8)|std::to_integer<unsigned>(bytes[i*4+j]);scratch[i]=v;}
 ++f.entered;auto result=rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(scratch.data(),a),n,ctx.r5.u32);require(result<=n/4,"native SDK result");ctx.r3.u64=result;++f.completed;
}
void native_poll(uint32_t pc){require(active_scope,"missing native hook scope");active_scope->point(pc);}
NativeResult native_probe(){NativeResult result;auto ok=[&](bool v){v?++result.positive:++result.failed;};auto deny=[&](auto f){try{f();++result.failed;}catch(const Error&){++result.negative;}};
 try{
  Space space(4*1024*1024);auto plan=native_plan();Image image(space,plan,native_bytes,identity(plan));ModuleLife life(plan.module);Slots slots;auto slot=slots.allocate();auto initial=space.read(plan.tls_address,plan.tls_file_size);
  auto functions=std::span(native_functions,native_function_count);Binding bind(image,space,life,native_identity,native_identity,functions,native_bounds,native_control);
  auto stale=native_identity;stale.generated[0]^=1;deny([&]{Binding wrong(image,space,life,stale,native_identity,functions,native_bounds,native_control);});stale=native_identity;stale.source[0]^=1;deny([&]{Binding wrong(image,space,life,stale,native_identity,functions,native_bounds,native_control);});
  auto wrong_bounds=std::vector<Boundary>(std::begin(native_bounds),std::end(native_bounds));wrong_bounds[0].size+=4;
  deny([&]{Binding wrong(image,space,life,native_identity,native_identity,functions,wrong_bounds,native_control);});
  auto wrong_sites=std::vector<Site>(std::begin(native_sites),std::end(native_sites));wrong_sites[0].target^=4;ControlPlan wrong_control{wrong_sites,native_roots,native_control.exit};
  deny([&]{Binding wrong(image,space,life,native_identity,native_identity,functions,native_bounds,wrong_control);});
  GuestThread guest(space,life,slots,slot,7,initial);ok(space.load<uint32_t>(guest.tls.address)==5);ok(space.load<uint32_t>(guest.pcr.address)==guest.tls.address);ok(guest.context.r2.u64==0x82020000);ok(guest.context.f1.f64==0&&guest.context.ctr.u64==0&&guest.context.lr==0);
  auto run=[&]{bind.startup(guest);guest.context.lr=native_control.exit;auto sp=guest.context.r1.u64;Scope scope(guest,functions,plan.module,nullptr,{},&native_control);bind.lend(scope);Active active(scope);Frame frame(space,image,guest);deny([&]{image.unload();});bind.invoke(scope,plan.entry);result.actual=guest.context.r3.u32;result.sdk_entered+=frame.entered;result.sdk_completed+=frame.completed;result.functions=unsigned(scope.calls);ok(guest.context.r1.u64==sp&&guest.context.lr==native_control.exit);};
  run();ok(result.actual==32);ok(space.load<uint32_t>(0x82040000)==29&&space.load<uint32_t>(0x82040004)==32);ok(result.sdk_entered==1&&result.sdk_completed==1);ok(result.functions==4);
  space.store<uint32_t>(guest.tls.address,99);deny([&]{bind.startup(guest);});space.store<uint32_t>(guest.tls.address,5);
  auto r13=guest.context.r13.u64;guest.context.r13.u64=1;deny([&]{Scope s(guest,functions,plan.module);});guest.context.r13.u64=r13;
  {guest.context.lr=native_control.exit;Scope scope(guest,functions,plan.module,nullptr,{},&native_control);bind.lend(scope);deny([&]{bind.invoke(scope,plan.entry+4);});deny([&]{scope.invoke(0xdeadbeec);});deny([&]{Scope::indirect(guest.context,nullptr,0xdeadbeec);});deny([&]{Scope::deny_exception(guest.context);});deny([&]{space.store<uint32_t>(plan.base+0x20000,1);});deny([&]{space.read(0x82050000,4);});}
  {brutal::synthetic::CooperativeStop stop;unsigned polls=0;guest.context.lr=native_control.exit;Scope scope(guest,functions,plan.module,&stop,[&](uint32_t){if(++polls==3)stop.request();},&native_control);bind.lend(scope);Active active(scope);bool stopped=false;try{scope.invoke(native_forever);}catch(const Scope::Stopped&){stopped=true;}ok(stopped);}
  {guest.context.lr=native_control.exit;Scope scope(guest,functions,plan.module,nullptr,[&](uint32_t pc){if(pc==native_helper+4)guest.context.lr=1;},&native_control);bind.lend(scope);Active active(scope);deny([&]{scope.invoke(native_helper);});guest.context.lr=native_control.exit;}
  {GuestTask failed(guest);ok(!failed.start([]{},NativeThread::Injection::creation_failure));}
  GuestTask task(guest);bool started=task.start([&]{run();});bool joined=task.join();ok(started&&joined);ok(!task.callback_error());
  guest.cleanup();slots.release(slot);ok(life.close());image.unload();ok(space.live()==0&&space.leases()==0);result.remaining=unsigned(space.live());result.leases=unsigned(space.leases());
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';++result.failed;}return result;
}
