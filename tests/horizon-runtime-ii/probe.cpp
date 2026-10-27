// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#include <bit>
#include <limits>
#include <cstdio>
#include <memory>
#ifndef STAGE_MAX
#define STAGE_MAX 9
#endif
using namespace brutal::horizon;
namespace {
std::array<std::atomic<bool>,std::size(xex_hooks)> observed_sites{};
void observed(uint32_t pc){for(size_t i=0;i<std::size(xex_hooks);++i)if(xex_hooks[i]==pc)observed_sites[i]=true;}
struct Check {RuntimeIIResult&r;unsigned group;
 void operator()(bool ok,const char*label){if(ok){++r.passed;++r.group_pass[group];}else{++r.failed;++r.group_fail[group];std::fprintf(stderr,"FAIL group %u: %s\n",group,label);}}
 template<class F>void refuse(Fault fault,F fn,const char*label){try{fn();(*this)(false,label);}catch(const Error&e){(*this)(e.fault==fault,label);++r.refusals;}catch(...){(*this)(false,label);}}
};
void foundation(Check c){
 Space s(64*1024);auto h=s.reserve(8192);c.refuse(Fault::uncommitted,[&]{s.load<uint32_t>(h.address);},"reserve is not committed");s.commit(h);s.store<uint64_t>(h.address+4092,0x123456789abcdef0ull);c(s.load<uint64_t>(h.address+4092)==0x123456789abcdef0ull,"checked cross-page endian");
 auto v=s.view(h.address,4,Access::read);s.protect(h,brutal::foundation::Permission::write);c.refuse(Fault::protection,[&]{s.read(v);},"write-only read refused");s.protect(h,brutal::foundation::Permission::read);c.refuse(Fault::protection,[&]{s.store<uint32_t>(h.address,7);},"read-only write refused");s.protect(h,brutal::foundation::Permission::read_write);
 {Space::Lease pin(s,h);c.refuse(Fault::ownership,[&]{s.release(h);},"leased allocation retained");}s.decommit(h);s.commit(h);c.refuse(Fault::stale,[&]{s.read(v);},"epoch refuses stale view");s.release(h);c(s.live()==0&&s.leases()==0,"memory balanced");
}
void abi(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();GuestThread guest(space,image.life(),slots,t,7,image.tls_template());auto sp=guest.context.r1.u64;
 auto call=[&](uint32_t pc){guest.context.lr=0x8201feed;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(pc);};
 c(space.read(guest.tls.address,16)==image.tls_template(),"XEX TLS template");c(space.load<uint32_t>(guest.pcr.address)==guest.tls.address&&space.load<uint64_t>(guest.pcr.address+0x30)==guest.pcr.address,"PCR TLS/self");
 const std::array<std::array<uint64_t,3>,8> inputs{{{0,0,0},{1,2,3},{0xffffffffu,1,0},{0xffffffffu,0xffffffffu,1},{0x100000000ull,0,0},{UINT64_MAX,1,0},{0x12345678ffffffffull,0x80000000u,0x7fffffffu},{0x80000000u,0,0xffffffffu}}};
 for(auto args:inputs){
  const uint64_t first=args[0]+args[1];const uint64_t carry1=((args[0]&UINT32_MAX)+(args[1]&UINT32_MAX))>>32;
  const uint64_t carry2=((first&UINT32_MAX)+(args[2]&UINT32_MAX)+carry1)>>32;const uint64_t sum=first+args[2]+carry1;
  const uint64_t extension=(args[1]&0x80000000u)?0xffffffff00000000ull|(args[1]&UINT32_MAX):(args[1]&UINT32_MAX);
  guest.context.r3.u64=args[0];guest.context.r4.u64=args[1];guest.context.r5.u64=args[2];guest.context.xer.ca=0;guest.context.xer.ov=1;guest.context.xer.so=1;call(xex_integer_flags);
  c(guest.context.r3.u64==sum,"independent two-stage integer oracle");c(guest.context.r6.u64==first,"64-bit wrap result");c(guest.context.xer.ca==carry2,"selected default-MSR carry32");c(guest.context.r4.u64==extension,"explicit signed word extension");c(guest.context.xer.ov==1&&guest.context.xer.so==1,"OE=0 preserves OV/SO");
  const auto signed_word=std::bit_cast<int32_t>(uint32_t(args[1]));c(guest.context.cr0.lt==(signed_word<0)&&guest.context.cr0.gt==(signed_word>0)&&guest.context.cr0.eq==(signed_word==0)&&guest.context.cr0.so==1,"signed comparison and sticky SO");
 }
 PPCRegister PPCContext::*gprs[]{&PPCContext::r3,&PPCContext::r4,&PPCContext::r5,&PPCContext::r6,&PPCContext::r7,&PPCContext::r8,&PPCContext::r9,&PPCContext::r10};
 for(unsigned i=0;i<8;++i){(guest.context.*gprs[i]).u64=i+1;}space.store<uint32_t>(uint32_t(sp)+84,9);space.store<uint32_t>(uint32_t(sp)+92,10);call(xex_ten_u32);c(guest.context.r3.u64==55,"ten scalar args across GPR and stack");
 const auto record=space.allocate(4096);space.store<uint32_t>(record.address,0x10203040);space.store<uint32_t>(record.address+4,0x50607080);
 {guest.context.lr=0x8201feed;guest.context.r3.u64=record.address;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(record);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(xex_record_pair);c(guest.context.r3.u64==0x6080a0c0,"original eight-byte struct IN pointer");}
 {guest.context.lr=0x8201feed;guest.context.r3.u64=record.address+4092;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(record);HookFrame hooks(scope,xex_hooks,observed);c.refuse(Fault::ownership,[&]{scope.invoke(xex_record_pair);},"struct second word exceeds retained capability");}
 {guest.context.lr=0x8201feed;guest.context.r3.u64=0x12345678;guest.context.r4.u64=record.address;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(record);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(xex_store);c(space.load<uint32_t>(record.address)==0x12345678,"original generated checked store");}space.release(record);
 // Literal IEEE bit-pattern oracle; no duplicate floating arithmetic in tests.
 const std::array<std::array<uint64_t,3>,6> floats{{{0x3ff8000000000000ull,0x4000000000000000ull,0x4008000000000000ull},{0x8000000000000000ull,0x4000000000000000ull,0x8000000000000000ull},{0,0xc000000000000000ull,0x8000000000000000ull},{0x0010000000000000ull,0x4000000000000000ull,0x0020000000000000ull},{1,0x4000000000000000ull,2},{0x7fefffffffffffffull,0x3fe0000000000000ull,0x7fdfffffffffffffull}}};
 for(auto bits:floats){auto csr=PPCFPSCRRegister::Platform::getcsr();guest.context.fpscr.csr=csr|PPCFPSCRRegister::FlushMask;guest.context.f1.u64=bits[0];guest.context.f2.u64=bits[1];guest.context.f14.u64=0x4045400000000000ull;call(xex_float_mul);c(guest.context.f1.u64==bits[2],"literal IEEE finite/zero/subnormal multiply oracle");c(guest.context.f14.u64==0x4045400000000000ull,"nonvolatile FPR preserved");c(PPCFPSCRRegister::Platform::getcsr()==csr,"host FP control restored");}
 for(auto bits:{uint64_t(0x7ff0000000000000ull),uint64_t(0x7ff8000000000001ull)}){guest.context.f1.u64=bits;guest.context.f2.f64=2;c.refuse(Fault::unsupported,[&]{call(xex_float_mul);},"nonfinite FPR remains denied");}
 guest.context.r14.u64=0x1234567812345678;guest.context.r15.u64=0x8765432187654321;guest.context.r31.u64=0xfeedfacecafebeef;guest.context.r3.u64=4;call(xex_entry);c(guest.context.r3.u64==24,"new nested generated entry result");c.r.result=guest.context.r3.u32;
 c(guest.context.r14.u64==0x1234567812345678&&guest.context.r15.u64==0x8765432187654321&&guest.context.r31.u64==0xfeedfacecafebeef,"nested nonvolatile preservation");c(guest.context.r1.u64==sp&&guest.context.r2.u64==0x82020000&&guest.context.r13.u64==guest.pcr.address&&guest.context.lr==0x8201feed,"r1 r2 r13 LR preserved");
 c.refuse(Fault::unsupported,[&]{call(xex_bad_return);},"invalid generated return refused");guest.context.r1.u64=sp+4;c.refuse(Fault::argument,[&]{call(xex_add64);},"misaligned callback/start context denied");guest.context.r1.u64=sp;
 {guest.context.lr=0x8201feed;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);HookFrame hooks(scope,xex_hooks,observed);c.refuse(Fault::unsupported,[&]{Scope::indirect(guest.context,nullptr,0xdeadbeec);},"unknown computed call target");PPCContext other{};c.refuse(Fault::no_thread,[&]{Scope::current(other);},"foreign callback context");}
 guest.cleanup();slots.release(t);c(image.unload()&&space.live()==0,"ABI image cleanup");
}
void threads_and_stop(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event release;std::array<unsigned,4> result{};
 for(unsigned i=0;i<4;++i){guests[i]=std::make_unique<GuestThread>(space,image.life(),slots,t,7+i,image.tls_template());tasks[i]=std::make_unique<GuestTask>(*guests[i]);c(tasks[i]->start([&,i]{auto&g=*guests[i];g.context.r3.u64=4;g.context.lr=0x8201feed;Scope scope(g,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,g);HookFrame hooks(scope,xex_hooks,observed);ready[i].signal();if(!release.wait())throw Error(Fault::unsupported,"II worker gate");image.invoke(scope,xex_entry);result[i]=g.context.r3.u32;}),"concurrent generated worker starts");}
 for(auto&e:ready){c(e.wait(),"four native thread-local scopes active");}c.refuse(Fault::ownership,[&]{guests[0]->cleanup();},"guest cannot outlive active execution");release.signal();
 for(unsigned i=0;i<4;++i){c(tasks[i]->join()&&!tasks[i]->callback_error(),"native worker join");c(result[i]==24+i&&guests[i]->get_tls()==8+i,"isolated TLS and generated results");guests[i]->cleanup();}c.r.workers=4;slots.release(t);c(image.unload()&&space.live()==0,"concurrent image cleanup");
 for(auto pc:{uint32_t(0x82010048),uint32_t(0x82010074),uint32_t(0x8201012c),uint32_t(0x82010138)}){
  Space s;XexImage mod(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots reg;auto slot=reg.allocate();GuestThread g(s,mod.life(),reg,slot);GuestTask task(g);brutal::synthetic::CooperativeStop stop;bool stopped=false;
  const auto root=pc==0x82010074?xex_forever:pc>=0x82010100?xex_nested:xex_original_entry;
  c(task.start([&]{g.context.r3.u64=3;g.context.lr=0x8201feed;Scope scope(g,mod.functions(),mod.life().identity,&stop,[&](uint32_t at){if(at==pc)stop.request();},expansion_plan_ptr);mod.lend(scope);HookFrame hooks(scope,xex_hooks,observed);try{scope.invoke(root);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}),"backedge worker starts");c(task.join()&&!task.callback_error()&&stopped,"each of four backedges stops");stop.joined();g.cleanup();stop.reclaimed();c(mod.unload()&&s.live()==0,"stop cleanup");stop.unloaded();reg.release(slot);
 }
}
}
RuntimeIIResult runtime_ii_probe(unsigned cycles,bool tracing){(void)tracing;RuntimeIIResult r;if(cycles<1||cycles>8)throw Error(Fault::argument,"II test budget");const auto sdk=ImportFrame::sdk_completed.load(),entries=Scope::guest_entries.load();for(auto&s:observed_sites)s=false;try{foundation({r,0});abi({r,1});threads_and_stop({r,1});
 unsigned memory_seen=0,back_seen=0;for(size_t i=0;i<std::size(expansion_sites);++i){auto site=expansion_sites[i];if(observed_sites[i]&&(site.kind==SiteKind::read||site.kind==SiteKind::write))++memory_seen;if(observed_sites[i]&&site.kind==SiteKind::branch&&site.target<=site.pc)++back_seen;}Check c{r,1};c(memory_seen==34,"all declared memory hooks observed");c(back_seen==4,"all declared backedges observed");}catch(const std::exception&e){std::fprintf(stderr,"II error: %s\n",e.what());++r.failed;}r.sdk_calls=unsigned(ImportFrame::sdk_completed.load()-sdk);r.guest_entries=unsigned(Scope::guest_entries.load()-entries);const auto totals=Space::totals();r.remaining=unsigned(totals.first);r.leases=unsigned(totals.second);if(r.remaining||r.leases)++r.failed;return r;}
