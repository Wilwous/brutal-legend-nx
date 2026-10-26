// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#include <cstdio>
#include <memory>
#ifndef STAGE_MAX
#define STAGE_MAX 4
#endif
using namespace brutal::horizon;
namespace {
template<class T>concept ExposesRelease=requires(T v){v.handle;};
static_assert(!ExposesRelease<Space::View>);
std::array<std::atomic<bool>,std::size(xex_hooks)> seen_sites{};
void observed(uint32_t pc){for(size_t i=0;i<std::size(xex_hooks);++i)if(xex_hooks[i]==pc)seen_sites[i]=true;}
struct Check {
 ExpansionResult&r;unsigned stage;
 void operator()(bool b,const char* label){if(b){++r.passed;++r.stage_pass[stage-4];}else{++r.failed;++r.stage_fail[stage-4];std::fprintf(stderr,"FAIL stage %u: %s\n",stage,label);}}
 template<class F>void refuse(Fault fault,F f,const char* label){try{f();(*this)(false,label);}catch(const Error&e){(*this)(e.fault==fault,label);}catch(...){(*this)(false,label);}}
};
void memory_checks(Check c){
 Space s(256*1024),foreign(8192);auto a=s.reserve(8192,0x90000000);
 c(s.live()==1,"reservation");c.refuse(Fault::uncommitted,[&]{s.load<uint32_t>(a.address);},"uncommitted");
 s.commit(a);c(s.load<uint64_t>(a.address)==0,"zero commit");
 s.store<uint64_t>(a.address+4092,0x123456789abcdef0ull);c(s.load<uint64_t>(a.address+4092)==0x123456789abcdef0ull,"cross page endian");
 c(s.read(a.address+4092,2)==std::vector<std::byte>{std::byte{0x12},std::byte{0x34}},"big endian bytes");
 auto read=s.view(a.address,8,Access::read),write=s.view(a.address,8,Access::write);std::array<std::byte,8> bytes{};bytes[0]=std::byte{0xaa};
 s.write(write,bytes);c(s.read(read)[0]==std::byte{0xaa},"temporary view");
 auto before=s.read(a.address,8);c.refuse(Fault::bounds,[&]{s.write(write,std::span(bytes).first(7));},"short copy refused");c(s.read(a.address,8)==before,"failure atomic short write");
 c.refuse(Fault::bounds,[&]{s.write(a.address+8190,bytes);},"cross allocation write");c(s.read(a.address,8)==before,"bounds failure atomic");
 s.protect(a,brutal::foundation::Permission::read);c.refuse(Fault::protection,[&]{s.write(write,bytes);},"recheck RO view");
 s.protect(a,brutal::foundation::Permission::write);s.write(write,bytes);c.refuse(Fault::protection,[&]{s.read(read);},"read from write only");
 c.refuse(Fault::protection,[&]{s.load<uint32_t>(a.address);},"translated write only");
 s.protect(a,brutal::foundation::Permission::none);c.refuse(Fault::protection,[&]{s.write(write,bytes);},"no access");s.protect(a,brutal::foundation::Permission::read_write);
 {Space::Lease lease(s,a);c(s.leases()==1,"lease counted");c.refuse(Fault::ownership,[&]{s.decommit(a);},"lease decommit");c.refuse(Fault::ownership,[&]{s.release(a);},"lease release");}
 c(s.leases()==0,"lease balanced");s.decommit(a);s.commit(a);c(s.load<uint64_t>(a.address)==0,"recommit zeroes");
 c.refuse(Fault::stale,[&]{s.read(read);},"epoch stale read");c.refuse(Fault::stale,[&]{s.write(write,bytes);},"epoch stale write");
 auto fresh=s.view(a.address,8,Access::read);c.refuse(Fault::ownership,[&]{foreign.read(fresh);},"foreign view");
 c.refuse(Fault::protection,[&]{s.write(fresh,bytes);},"read capability cannot write");
 s.release(a);c.refuse(Fault::stale,[&]{s.read(fresh);},"released view");c.refuse(Fault::stale,[&]{s.release(a);},"double free");
 c.refuse(Fault::bounds,[&]{s.read(a.address,4);},"freed address");
 c.refuse(Fault::bounds,[&]{s.reserve(8192,0xfffff000);},"address wrap");c.refuse(Fault::argument,[&]{s.reserve(4,0x90000001);},"alignment");
 c.refuse(Fault::exhausted,[&]{s.allocate(UINT32_MAX);},"size rounding overflow");c.refuse(Fault::bounds,[&]{s.read(0xfffffffcu,8);},"span overflow");
 auto b=s.allocate(4096),d=s.allocate(4096);{Space::Lease pin(s,d);std::array group{b,d};c.refuse(Fault::ownership,[&]{s.release_all(group);},"transaction rollback");c(s.live()==2,"no partial release");}
 std::array group{b,d};s.release_all(group);
 for(unsigned i=0;i<8;++i){auto h=s.allocate(4096);s.store<uint32_t>(h.address,i);c(s.load<uint32_t>(h.address)==i,"cycle data");s.release(h);}
 auto counter=s.allocate(4096);std::mutex rmw;NativeThread one,two;
 auto increment=[&]{for(unsigned i=0;i<8;++i){std::lock_guard lock(rmw);s.store<uint32_t>(counter.address,s.load<uint32_t>(counter.address)+1);}};
 c(one.start(increment),"memory worker one");c(two.start(increment),"memory worker two");c(one.join(),"memory join one");c(two.join(),"memory join two");c(s.load<uint32_t>(counter.address)==16,"concurrent serialized RMW");s.release(counter);
 c(s.live()==0&&s.leases()==0,"memory final balance");
}
void pipeline(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();
 {GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());guest.context.r3.u64=4;guest.context.lr=0x8201feed;
  {Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks,observed);
   image.invoke(scope,xex_entry);c(guest.context.r3.u64==(STAGE_MAX>=7?18:17),"production generated entry result");c(imports.calls==(STAGE_MAX>=7?2:1),"actual SDK leaves executed");c.r.result=guest.context.r3.u32;c.r.sdk_calls+=imports.calls;}
  guest.cleanup();}
 slots.release(slot);c(image.unload(),"image unload");c(space.live()==0&&space.leases()==0,"pipeline balance");
}
#if STAGE_MAX >= 5
void thread_checks(Check c){
 NativeThread state;c(state.state()==NativeThread::State::empty,"thread empty");brutal::horizon::Event entered,leave;
 c(state.start([&]{entered.signal();if(!leave.wait())throw Error(Fault::unsupported,"state gate");}),"thread starts");c(entered.wait(),"native entered");c(state.state()==NativeThread::State::running,"thread running");c(!state.join(0),"bounded join timeout");c(state.state()==NativeThread::State::running,"timeout retains state");leave.signal();c(state.join(),"native joins");c(state.state()==NativeThread::State::joined,"thread joined");
 NativeThread injected;c(!injected.start([]{},64*1024,NativeThread::Injection::creation_failure),"injected create failure");c(injected.state()==NativeThread::State::failed,"explicit failure state");
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();
 // Construct over poisoned object storage, then compare the entire context with
 // an explicitly initialized representation, including register union high bits.
 alignas(GuestThread) std::array<std::byte,sizeof(GuestThread)> storage;storage.fill(std::byte{0xa5});
 auto* poison=std::construct_at(reinterpret_cast<GuestThread*>(storage.data()),space,image.life(),slots,slot);
 PPCContext expected;std::memset(static_cast<void*>(&expected),0,sizeof(expected));expected.msr=PPCContext{}.msr;expected.r1=poison->context.r1;expected.r2=poison->context.r2;expected.r13=poison->context.r13;
 c(std::memcmp(&expected,&poison->context,sizeof(expected))==0,"all context bytes initialized");
 auto msr=poison->context.msr;poison->context.msr=0;c.refuse(Fault::argument,[&]{GuestThread::Execution bad(*poison);},"bad MSR startup");poison->context.msr=msr;
 poison->context.r2.u64=0;c.refuse(Fault::argument,[&]{GuestThread::Execution bad(*poison);},"bad r2 startup");poison->context.r2.u64=0x82020000;
 space.store<uint64_t>(poison->pcr.address+0x30,0);c.refuse(Fault::argument,[&]{GuestThread::Execution bad(*poison);},"bad PCR self");space.store<uint64_t>(poison->pcr.address+0x30,poison->pcr.address);
 poison->cleanup();std::destroy_at(poison);c(space.live()==4,"poison fixture cleanup");
 std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event gate;
 std::array<unsigned,4> values{},imports{};std::array<uint64_t,4> identities{};std::atomic<unsigned> live{},maximum{};
 for(unsigned i=0;i<4;++i){guests[i]=std::make_unique<GuestThread>(space,image.life(),slots,slot,7+i,image.tls_template());identities[i]=guests[i]->identity;tasks[i]=std::make_unique<GuestTask>(*guests[i]);
  c(tasks[i]->start([&,i]{auto&guest=*guests[i];guest.context.r3.u64=4;guest.context.lr=0x8201feed;
   Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame sdk(image,guest);HookFrame hooks(scope,xex_hooks,observed);
   auto n=++live;auto old=maximum.load();while(n>old&&!maximum.compare_exchange_weak(old,n)){}
   ready[i].signal();if(!gate.wait())throw Error(Fault::unsupported,"four worker gate");
   image.invoke(scope,xex_entry);values[i]=guest.context.r3.u32;imports[i]=sdk.calls;--live;
  }),"guest worker starts");}
 for(auto&e:ready){c(e.wait(),"guest worker ready");}c(maximum==4&&live==4,"four simultaneously active guests");
 c.refuse(Fault::ownership,[&]{guests[0]->cleanup();},"cleanup before join");c.refuse(Fault::ownership,[&]{slots.release(slot);},"pinned TLS registration");
 gate.signal();
 for(unsigned i=0;i<4;++i){c(tasks[i]->join(),"guest worker join");c(!tasks[i]->callback_error(),"guest callback success");c(values[i]==(STAGE_MAX>=7?18u:17u)+i&&imports[i]==(STAGE_MAX>=7?2u:1u),"isolated TLS and real concurrent SDK calls");c(guests[i]->get_tls()==8+i,"TLS update isolated");}
 for(unsigned i=0;i<4;++i){for(unsigned j=0;j<i;++j)c(identities[i]!=identities[j],"unique synthetic identities");guests[i]->cleanup();++c.r.workers;c.r.sdk_calls+=imports[i];}
 c.refuse(Fault::no_thread,[&]{Scope::current(guests[0]->context);},"native thread-local binding removed");
 slots.release(slot);c(image.unload(),"thread image unload");c(space.live()==0&&space.leases()==0,"thread final balance");
 for(unsigned repeat=0;repeat<4;++repeat){Space fresh;ModuleLife module(0x8000+repeat);Slots registry;auto token=registry.allocate();GuestThread guest(fresh,module,registry,token);GuestTask task(guest);c(task.start([&]{GuestThread::Execution execution(guest);guest.set_tls(100+repeat);}),"repeat guest start");c(task.join()&&!task.callback_error(),"repeat join");c(guest.get_tls()==100+repeat,"repeat TLS");guest.cleanup();registry.release(token);c(module.close()&&fresh.live()==0,"repeat cleanup");}
}
#endif

#if STAGE_MAX >= 6
void abi_checks(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());
 c.refuse(Fault::unsupported,[&]{Scope unsafe(guest,image.functions(),image.life().identity);},"strict functions need plan");
 auto call=[&](uint32_t pc){guest.context.lr=0x8201feed;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame sdk(image,guest);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(pc);};
 guest.context.r3.u64=0x100000002ull;guest.context.r4.u64=0x200000003ull;guest.context.xer.ca=1;guest.context.xer.so=1;guest.context.r31.u64=0xfeedfacecafebeef;
 auto sp=guest.context.r1.u64;call(xex_add64);c(guest.context.r3.u64==0x300000005ull,"64 bit GPR result");c(guest.context.r4.u64==0x200000003ull,"GPR argument retained");c(guest.context.xer.ca==1&&guest.context.xer.so==1,"untouched XER preserved");c(guest.context.r31.u64==0xfeedfacecafebeef,"nonvolatile r31");
 guest.context.r3.u64=0xffffffffu;guest.context.r4.u64=1;call(xex_add64);c(guest.context.r3.u64==0x100000000ull,"32 bit arguments widen in 64 bit add");
 const auto csr=PPCFPSCRRegister::Platform::getcsr();guest.context.fpscr.csr=csr;guest.context.f1.f64=1.5;guest.context.f2.f64=2.25;guest.context.f14.f64=42.5;
 call(xex_float_add);c(guest.context.f1.f64==3.75,"exact finite FPR return");c(guest.context.f2.f64==2.25&&guest.context.f14.f64==42.5,"FPR input and nonvolatile preservation");c(PPCFPSCRRegister::Platform::getcsr()==csr,"host FP control restored");
 guest.context.f1.f64=std::numeric_limits<double>::infinity();c.refuse(Fault::unsupported,[&]{call(xex_float_add);},"nonfinite FPR contract rejected");
 call(xex_nested);c(guest.context.r3.u64==6,"two finite nested loops");c(guest.context.cr0.eq&&guest.context.cr0.so,"CR comparison inherits XER SO");
 c(guest.context.r1.u64==sp&&guest.context.r2.u64==0x82020000&&guest.context.r13.u64==guest.pcr.address&&guest.context.lr==0x8201feed,"stack r2 PCR LR preserved");
 c.refuse(Fault::unsupported,[&]{call(xex_bad_return);},"generated invalid return refused at site");
 {guest.context.lr=0;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);HookFrame hooks(scope,xex_hooks,observed);c.refuse(Fault::unsupported,[&]{scope.invoke(xex_add64);},"invalid root LR");}
 {guest.context.lr=0x8201feed;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);HookFrame hooks(scope,xex_hooks,observed);
  c.refuse(Fault::unsupported,[&]{Scope::load<uint32_t>(guest.context,guest.stack.address);},"unarmed memory site");
  c.refuse(Fault::unsupported,[&]{scope.point(xex_hooks[0]);},"orphan site");
  c.refuse(Fault::unsupported,[&]{image.functions()[0].implementation(guest.context,nullptr);},"raw function pointer bypass refused at prologue");
  c.refuse(Fault::unsupported,[&]{Scope::indirect(guest.context,nullptr,0xdeadbeec);},"unknown indirect target");
  PPCContext wrong{};c.refuse(Fault::no_thread,[&]{Scope::current(wrong);},"foreign PPC context");}
 {guest.context.r3.u64=0x11223344;guest.context.r4.u64=image.section(3).address+0x400;call(xex_store);c(space.load<uint32_t>(guest.context.r4.u32)==0x11223344,"generated checked store");}
 {GuestThread bad(space,image.life(),slots,slot,7,image.tls_template());bad.context.lr=0x8201feed;bad.context.r3.u64=1;
  Scope scope(bad,image.functions(),image.life().identity,nullptr,[&](uint32_t pc){if(pc==0x820100cc)bad.context.ctr.u64=xex_store;},expansion_plan_ptr);image.lend(scope);ImportFrame sdk(image,bad);HookFrame hooks(scope,xex_hooks,observed);
  c.refuse(Fault::unsupported,[&]{image.invoke(scope,xex_entry);},"known indirect target on wrong edge rejected");}
 guest.cleanup();slots.release(slot);c(image.unload()&&space.live()==0,"ABI cleanup");
 // Each of the four backward branches is reached under a stop request. The
 // request is issued from the hook observer before the poll's atomic snapshot.
 for(auto pc:{uint32_t(0x82010048),uint32_t(0x82010074),uint32_t(0x8201012c),uint32_t(0x82010138)}){
  Space fresh;XexImage mod(fresh,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots reg;auto t=reg.allocate();GuestThread g(fresh,mod.life(),reg,t);GuestTask task(g);brutal::synthetic::CooperativeStop stop;std::atomic<bool> stopped{};unsigned visits=0;
  uint32_t root=pc==0x82010074?xex_forever:pc>=0x82010100?xex_nested:xex_original_entry;
  c(task.start([&]{g.context.r3.u64=3;g.context.lr=0x8201feed;Scope scope(g,mod.functions(),mod.life().identity,&stop,[&](uint32_t at){if(at==pc){++visits;stop.request();}},expansion_plan_ptr);mod.lend(scope);ImportFrame sdk(mod,g);HookFrame hooks(scope,xex_hooks,observed);try{scope.invoke(root);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}),"backedge worker start");
  c(task.join()&&!task.callback_error(),"backedge worker joined");c(stopped&&visits==1,"exact backward site observed and stopped");stop.joined();g.cleanup();stop.reclaimed();c(mod.unload()&&fresh.live()==0,"backedge cleanup");stop.unloaded();reg.release(t);
 }
 unsigned memory_seen=0,backward_seen=0;
 for(size_t i=0;i<std::size(expansion_sites);++i){auto site=expansion_sites[i];if(seen_sites[i]&&(site.kind==SiteKind::read||site.kind==SiteKind::write))++memory_seen;if(seen_sites[i]&&site.kind==SiteKind::branch&&site.target<=site.pc)++backward_seen;}
 c(memory_seen==(STAGE_MAX>=7?25u:20u),"all declared scalar memory hooks observed");c(backward_seen==4,"all four backward hooks observed");
}
#endif

#if STAGE_MAX >= 7
void import_checks(Check c){
 c(!ExposesRelease<Space::View>,"temporary view hides release authority");
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();
 const auto address=image.section(3).address+0x800;
 std::array<std::byte,28> initial;initial.fill(std::byte{0x5a});space.write(address,initial);
 auto execute=[&](uint32_t pointer,bool reject=false){GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());guest.context.lr=0x8201feed;guest.context.r3.u64=4;
  Scope scope(guest,image.functions(),image.life().identity,nullptr,[&](uint32_t pc){if(pc==xex_entry+32)guest.context.r3.u64=pointer;},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks,observed);
  if(reject){c.refuse(Fault::bounds,[&]{image.invoke(scope,xex_entry);},"invalid INOUT pointer refused");c(imports.calls==1&&imports.initialized==0&&imports.trace_events==0,"refusal does not enter initializer");}
  else{image.invoke(scope,xex_entry);c(guest.context.r3.u64==18,"generated two-import result");c(imports.calls==2&&imports.initialized==1&&imports.trace_events==1,"two actual SDK calls and trace");c.r.sdk_calls+=imports.calls;}
 };
 execute(address);auto expected=initial;expected[0]=std::byte{1};expected[1]=std::byte{};for(unsigned i=4;i<8;++i)expected[i]=std::byte{};for(unsigned i=16;i<20;++i)expected[i]=std::byte{0xff};for(unsigned i=20;i<28;++i)expected[i]=std::byte{};
 c(space.read(address,28)==std::vector<std::byte>(expected.begin(),expected.end()),"actual SDK INOUT bytes and untouched fields preserved");
 for(auto bad:{uint32_t(0),address+1,image.section(2).address+0x200,image.section(3).address+65516,uint32_t(0xfffffff0)}){execute(bad,true);c(space.read(address,28)==std::vector<std::byte>(expected.begin(),expected.end()),"rejected call failure atomic");}
 image.admit_import(0x12e,XexImage::Signature::initialize_cs_inout);c(true,"exact signature admitted");c.refuse(Fault::unsupported,[&]{image.admit_import(0x12e,XexImage::Signature::typed_vector);},"vector import denied");c.refuse(Fault::unsupported,[&]{image.admit_import(0xee,XexImage::Signature::initialize_cs_inout);},"query memory still denied");
 // Original generated OUT-only fixture. This is not an additional SDK export.
 {GuestThread out(space,image.life(),slots,slot);auto output=space.allocate(4096);space.protect(output,brutal::foundation::Permission::write);out.context.r3.u64=0x12345678;out.context.r4.u64=output.address;out.context.lr=0x8201feed;
  {Scope scope(out,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(output);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(xex_store);c(scope.writes==1&&scope.reads==0,"OUT requires no input read");}
  space.protect(output,brutal::foundation::Permission::read);c(space.load<uint32_t>(output.address)==0x12345678,"generated OUT bytes");space.release(output);out.cleanup();}
 auto buffer=space.allocate(4096);auto in=space.view(buffer.address,28,Access::read),out=space.view(buffer.address,28,Access::write);unsigned entered=0;
 space.protect(buffer,brutal::foundation::Permission::read);c.refuse(Fault::protection,[&]{space.update(in,out,[&](auto){++entered;});},"INOUT requires write before SDK");c(entered==0,"no callback after preflight refusal");
 space.protect(buffer,brutal::foundation::Permission::read_write);auto before=space.read(buffer.address,28);c.refuse(Fault::unsupported,[&]{space.update(in,out,[&](auto bytes){bytes[0]=std::byte{7};throw Error(Fault::unsupported,"original rollback fixture");});},"transform exception");c(space.read(buffer.address,28)==before,"transaction rollback publishes nothing");space.release(buffer);
 auto wrong=xex_generated;wrong.initializer=nullptr;{Space other;c.refuse(Fault::unsupported,[&]{XexImage denied(other,xex_bytes,{xex_bytes,wrong,xex_entry});},"missing initializer binding");c(other.live()==0,"binding failure before allocation");}
 slots.release(slot);c(image.unload()&&space.live()==0,"import cleanup");
 // Request stop and close admission while a call is admitted and its image/data
 // leases are live. The accepted SDK leaf finishes; the next hook observes stop.
 {Space s;XexImage mod(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots registry;auto t=registry.allocate();GuestThread guest(s,mod.life(),registry,t,7,mod.tls_template());GuestTask task(guest);brutal::synthetic::CooperativeStop stop;brutal::horizon::Event ready,release;std::atomic<bool> stopped{};unsigned committed=0;
  c(task.start([&]{guest.context.lr=0x8201feed;guest.context.r3.u64=4;Scope scope(guest,mod.functions(),mod.life().identity,&stop,{},expansion_plan_ptr);mod.lend(scope);ImportFrame imports(mod,guest);HookFrame hooks(scope,xex_hooks,observed);imports.admitted=[&]{ready.signal();if(!release.wait())throw Error(Fault::unsupported,"admitted SDK gate");};try{mod.invoke(scope,xex_entry);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}committed=imports.initialized;}),"admitted SDK worker starts");
  c(ready.wait(),"SDK admission active");c(!mod.unload(),"unload held by admitted import");c.refuse(Fault::stale,[&]{mod.admit_import(0x12e,XexImage::Signature::initialize_cs_inout);},"new import after closing denied");c(!task.join(0),"active SDK join timeout retains ownership");c(stop.request(),"stop request during admitted call");release.signal();c(task.join()&&!task.callback_error(),"SDK stopped worker joined");c(stopped&&committed==1,"accepted SDK call commits before safe stop");stop.joined();guest.cleanup();stop.reclaimed();c(mod.unload()&&s.live()==0&&s.leases()==0,"admitted call cleanup");stop.unloaded();registry.release(t);
 }
}
#endif

#if STAGE_MAX >= 8
void integrated_checks(Check c,unsigned cycles){
 uint32_t random=0xb17a1;auto next=[&]{random^=random<<13;random^=random>>17;random^=random<<5;return random;};
 for(unsigned cycle=0;cycle<cycles;++cycle){
  Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();
  std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<unsigned,4> expected{},values{},completed{};std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event gate;
  for(unsigned i=0;i<4;++i){unsigned seed=7+next()%32,iterations=next()%8;expected[i]=seed+iterations+7;guests[i]=std::make_unique<GuestThread>(space,image.life(),slots,t,seed,image.tls_template());tasks[i]=std::make_unique<GuestTask>(*guests[i]);
   c(tasks[i]->start([&,i,iterations]{auto&g=*guests[i];g.context.r3.u64=iterations;g.context.lr=0x8201feed;Scope scope(g,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame sdk(image,g);HookFrame hooks(scope,xex_hooks,observed);ready[i].signal();if(!gate.wait())throw Error(Fault::unsupported,"stress gate");image.invoke(scope,xex_entry);values[i]=g.context.r3.u32;completed[i]=sdk.calls;}),"integrated worker starts");
  }
  for(auto&e:ready){c(e.wait(),"integrated worker active");}gate.signal();
  for(unsigned i=0;i<4;++i){c(tasks[i]->join()&&!tasks[i]->callback_error(),"integrated join");c(values[i]==expected[i]&&completed[i]==2,"seeded generated multicall + actual imports");guests[i]->cleanup();}
  slots.release(t);c(image.unload(),"integrated unload");c(space.live()==0&&space.leases()==0,"integrated balance");
 }
 for(int fail=0;fail<4;++fail){Space s;c.refuse(Fault::exhausted,[&]{XexImage mod(s,xex_bytes,{xex_bytes,xex_generated,xex_entry},fail);},"partial image allocation failure");c(s.live()==0&&s.leases()==0,"failed load rollback");}
 {Space small(128*1024);c.refuse(Fault::exhausted,[&]{XexImage mod(small,xex_bytes,{xex_bytes,xex_generated,xex_entry});},"real bounded arena exhaustion");c(small.live()==0,"arena failure rollback");}
 for(unsigned which=0;which<4;++which){std::vector<uint8_t> bad(xex_bytes.begin(),xex_bytes.end());Fault expected=Fault::argument;
  if(which==0){bad[0x40c]='z';}if(which==1){bad[0x3100b]=0x2f;}if(which==2){bad[0x11000]^=1;}if(which==3){bad[0x1000+0x98+224+40+14]=0;}
  if(which==1){expected=Fault::unsupported;}Space s;c.refuse(expected,[&]{XexImage mod(s,bad,{xex_bytes,xex_generated,xex_entry});},"malformed image refused");c(s.live()==0,"malformed image zero allocations");
 }
 {Space s;XexImage image(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();ModuleLife alias(image.life().identity);GuestThread foreign(s,alias,slots,t);
  {Scope scope(foreign,image.functions(),alias.identity,nullptr,{},expansion_plan_ptr);c.refuse(Fault::argument,[&]{image.lend(scope);},"foreign image ownership");c.refuse(Fault::argument,[&]{image.invoke(scope,xex_entry);},"foreign entry ownership");}
  foreign.cleanup();c(alias.close(),"foreign module cleanup");
  {GuestThread retained(s,image.life(),slots,t);c(!image.unload(),"retained thread prevents unload");c.refuse(Fault::stale,[&]{Scope denied(retained,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);},"stale function scope after closing");retained.cleanup();}
  slots.release(t);c(image.unload()&&s.live()==0,"stale admission cleanup");
 }
 {Space s(16384);std::array<Space::Handle,4> all;for(auto&h:all)h=s.allocate(4096);c.refuse(Fault::exhausted,[&]{s.allocate(1);},"allocation pressure explicit failure");s.release_all(all);c(s.live()==0,"allocation pressure balance");}
 {brutal::synthetic::CooperativeStop stop;brutal::horizon::Event gate,a_ready,b_ready;NativeThread a,b;std::atomic<unsigned> accepted{};
  c(a.start([&]{a_ready.signal();if(!gate.wait())throw Error(Fault::unsupported,"request gate");if(stop.request())++accepted;}),"stop requester A");c(b.start([&]{b_ready.signal();if(!gate.wait())throw Error(Fault::unsupported,"request gate");if(stop.request())++accepted;}),"stop requester B");c(a_ready.wait()&&b_ready.wait(),"requesters ready");gate.signal();c(a.join()&&b.join()&&!a.callback_error()&&!b.callback_error(),"requesters join");c(accepted==1&&!stop.request(),"one concurrent stop wins");c(stop.safe_boundary(),"single boundary transition");stop.exited();stop.joined();stop.reclaimed();stop.unloaded();}
 {Space s;XexImage image(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();GuestThread guest(s,image.life(),slots,t);GuestTask worker(guest);brutal::synthetic::CooperativeStop stop;brutal::horizon::Event ready,release;std::atomic<bool> stopped{};auto saved=guest.context.r1.u32+4;
  c(worker.start([&]{guest.context.lr=0x8201feed;guest.context.r3.u64=3;Scope scope(guest,image.functions(),image.life().identity,&stop,[&](uint32_t pc){if(pc==0x82010008){ready.signal();if(!release.wait())throw Error(Fault::unsupported,"memory site gate");}},expansion_plan_ptr);image.lend(scope);HookFrame hooks(scope,xex_hooks,observed);try{scope.invoke(xex_original_entry);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}),"memory-boundary worker start");
  c(ready.wait(),"checked store site reached");c(stop.request(),"stop at active memory boundary");c.refuse(Fault::ownership,[&]{guest.cleanup();},"active memory context retained");release.signal();c(worker.join()&&!worker.callback_error()&&stopped,"memory worker stopped and joined");c(s.load<uint32_t>(saved)==0,"store after stop did not execute");stop.joined();guest.cleanup();stop.reclaimed();c(image.unload()&&s.live()==0,"memory stop cleanup");stop.unloaded();slots.release(t);
 }
 c(Space::totals()==std::pair<size_t,size_t>{0,0},"all integrated logical resources reclaimed");
}
#endif

}
ExpansionResult expansion_probe(unsigned cycles){ExpansionResult r;if(cycles<1||cycles>
#ifdef __SWITCH__
1
#else
8
#endif
){throw Error(Fault::argument,"smoke/stress cycle budget");}
 const auto start_sdk=ImportFrame::sdk_completed.load();const auto start_memory=Space::totals();try{memory_checks({r,4});pipeline({r,4});
#if STAGE_MAX >= 5
thread_checks({r,5});
#endif
#if STAGE_MAX >= 6
abi_checks({r,6});
#endif
#if STAGE_MAX >= 7
import_checks({r,7});
#endif
#if STAGE_MAX >= 8
integrated_checks({r,8},cycles);
#endif
}catch(const std::exception&e){std::fprintf(stderr,"ERROR: %s\n",e.what());++r.failed;}r.sdk_calls=unsigned(ImportFrame::sdk_completed.load()-start_sdk);auto end_memory=Space::totals();r.remaining=unsigned(end_memory.first-start_memory.first);r.leases=unsigned(end_memory.second-start_memory.second);if(r.remaining||r.leases)++r.failed;return r;}
