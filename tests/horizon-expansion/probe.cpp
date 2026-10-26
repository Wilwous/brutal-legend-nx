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
 auto forged=fresh;forged.backing.allocation.id++;c.refuse(Fault::ownership,[&]{s.read(forged);},"forged backing");
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
  {Scope scope(guest,image.functions(),image.life().identity);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks);
   image.invoke(scope,xex_entry);c(guest.context.r3.u64==17,"production generated entry result");c(imports.calls==1,"actual SDK compare executed");c.r.result=guest.context.r3.u32;c.r.sdk_calls+=imports.calls;}
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
   Scope scope(guest,image.functions(),image.life().identity);image.lend(scope);ImportFrame sdk(image,guest);HookFrame hooks(scope,xex_hooks);
   auto n=++live;auto old=maximum.load();while(n>old&&!maximum.compare_exchange_weak(old,n)){}
   ready[i].signal();if(!gate.wait())throw Error(Fault::unsupported,"four worker gate");
   image.invoke(scope,xex_entry);values[i]=guest.context.r3.u32;imports[i]=sdk.calls;--live;
  }),"guest worker starts");}
 for(auto&e:ready){c(e.wait(),"guest worker ready");}c(maximum==4&&live==4,"four simultaneously active guests");
 c.refuse(Fault::ownership,[&]{guests[0]->cleanup();},"cleanup before join");c.refuse(Fault::ownership,[&]{slots.release(slot);},"pinned TLS registration");
 gate.signal();
 for(unsigned i=0;i<4;++i){c(tasks[i]->join(),"guest worker join");c(!tasks[i]->callback_error(),"guest callback success");c(values[i]==17+i&&imports[i]==1,"isolated TLS and real concurrent SDK calls");c(guests[i]->get_tls()==8+i,"TLS update isolated");}
 for(unsigned i=0;i<4;++i){for(unsigned j=0;j<i;++j)c(identities[i]!=identities[j],"unique synthetic identities");guests[i]->cleanup();++c.r.workers;c.r.sdk_calls+=imports[i];}
 c.refuse(Fault::no_thread,[&]{Scope::current(guests[0]->context);},"native thread-local binding removed");
 slots.release(slot);c(image.unload(),"thread image unload");c(space.live()==0&&space.leases()==0,"thread final balance");
 for(unsigned repeat=0;repeat<4;++repeat){Space fresh;ModuleLife module(0x8000+repeat);Slots registry;auto token=registry.allocate();GuestThread guest(fresh,module,registry,token);GuestTask task(guest);c(task.start([&]{GuestThread::Execution execution(guest);guest.set_tls(100+repeat);}),"repeat guest start");c(task.join()&&!task.callback_error(),"repeat join");c(guest.get_tls()==100+repeat,"repeat TLS");guest.cleanup();registry.release(token);c(module.close()&&fresh.live()==0,"repeat cleanup");}
}
#endif

}
ExpansionResult expansion_probe(){ExpansionResult r;try{memory_checks({r,4});pipeline({r,4});
#if STAGE_MAX >= 5
thread_checks({r,5});
#endif
}catch(const std::exception&e){std::fprintf(stderr,"ERROR: %s\n",e.what());++r.failed;}return r;}
