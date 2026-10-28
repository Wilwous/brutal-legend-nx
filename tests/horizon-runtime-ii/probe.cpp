// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#if STAGE_MAX >= 11
#include "ii-metadata.h"
#endif
#if STAGE_MAX >= 10
#include "runtime/horizon/query.h"
#endif
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
// Exception-safe ownership for the fixture's four finite gate-controlled workers.
// On partial startup or controller failure, release the gate and join every
// started worker before the arrays and captured data can be destroyed.
struct BatchGuard {
 std::array<std::unique_ptr<GuestTask>,4>&tasks;brutal::horizon::Event&gate;std::array<bool,4> started{},joined{};
 BatchGuard(std::array<std::unique_ptr<GuestTask>,4>&t,brutal::horizon::Event&g):tasks(t),gate(g){}
 ~BatchGuard(){try{gate.signal();for(unsigned i=0;i<4;++i)if(started[i]&&!joined[i]){if(!tasks[i]||!tasks[i]->join())std::terminate();joined[i]=true;}}catch(...){std::terminate();}}
};
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
 auto call=[&](uint32_t pc){guest.context.lr=0x8201feed;Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);
#if STAGE_MAX >= 10
 QueryFrame query(image,guest);
#endif
 HookFrame hooks(scope,xex_hooks,observed);scope.invoke(pc);};
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
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event release;std::array<unsigned,4> result{};BatchGuard batch(tasks,release);
 for(unsigned i=0;i<4;++i){guests[i]=std::make_unique<GuestThread>(space,image.life(),slots,t,7+i,image.tls_template());tasks[i]=std::make_unique<GuestTask>(*guests[i]);c(batch.started[i]=tasks[i]->start([&,i]{auto&g=*guests[i];g.context.r3.u64=4;g.context.lr=0x8201feed;Scope scope(g,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,g);
#if STAGE_MAX >= 10
 QueryFrame query(image,g);
#endif
 HookFrame hooks(scope,xex_hooks,observed);ready[i].signal();if(!release.wait())throw Error(Fault::unsupported,"II worker gate");image.invoke(scope,xex_entry);result[i]=g.context.r3.u32;}),"concurrent generated worker starts");}
 for(unsigned i=0;i<4;++i){c(batch.started[i]&&ready[i].wait(),"four native thread-local scopes active");}c.refuse(Fault::ownership,[&]{guests[0]->cleanup();},"guest cannot outlive active execution");release.signal();
 for(unsigned i=0;i<4;++i){c(batch.started[i]&&(batch.joined[i]=tasks[i]->join())&&!tasks[i]->callback_error(),"native worker join");c(result[i]==24+i&&guests[i]->get_tls()==8+i,"isolated TLS and generated results");guests[i]->cleanup();}c.r.workers=4;slots.release(t);c(image.unload()&&space.live()==0,"concurrent image cleanup");
 for(auto pc:{uint32_t(0x82010048),uint32_t(0x82010074),uint32_t(0x8201012c),uint32_t(0x82010138)}){
  Space s;XexImage mod(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots reg;auto slot=reg.allocate();GuestThread g(s,mod.life(),reg,slot);GuestTask task(g);brutal::synthetic::CooperativeStop stop;bool stopped=false;
  const auto root=pc==0x82010074?xex_forever:pc>=0x82010100?xex_nested:xex_original_entry;
  c(task.start([&]{g.context.r3.u64=3;g.context.lr=0x8201feed;Scope scope(g,mod.functions(),mod.life().identity,&stop,[&](uint32_t at){if(at==pc)stop.request();},expansion_plan_ptr);mod.lend(scope);HookFrame hooks(scope,xex_hooks,observed);try{scope.invoke(root);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}),"backedge worker starts");c(task.join()&&!task.callback_error()&&stopped,"each of four backedges stops");stop.joined();g.cleanup();stop.reclaimed();c(mod.unload()&&s.live()==0,"stop cleanup");stop.unloaded();reg.release(slot);
 }
}
#if STAGE_MAX >= 10
void services(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();auto target=space.reserve(8192),output=space.allocate(8192);auto pointer=output.address+4096-12;
 auto invoke=[&](uint32_t address,uint32_t destination,uint32_t kind){
  GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());guest.context.r3.u64=address;guest.context.r4.u64=destination;guest.context.r5.u64=kind;guest.context.lr=0x8201feed;
  Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(target);scope.lend(output);QueryFrame frame(image,guest);HookFrame hooks(scope,xex_hooks,observed);scope.invoke(xex_query_only);
  return std::array<uint32_t,4>{guest.context.r3.u32,frame.entered,frame.returned,frame.committed};
 };
 auto fields=[&]{std::array<uint32_t,7> result{};for(unsigned i=0;i<7;++i)result[i]=space.load<uint32_t>(pointer+4*i);return result;};
 auto row=invoke(target.address,pointer,0);c(row==std::array<uint32_t,4>{0,1,1,1},"actual SDK reserve query and OUT commit");c(fields()==std::array<uint32_t,7>{target.address,target.address,1,8192,0x2000,1,0x20000},"independent reserved metadata oracle");
 space.commit(target);row=invoke(target.address,pointer,0);c(row[0]==0&&row[3]==1,"committed query succeeds");c(fields()==std::array<uint32_t,7>{target.address,target.address,4,8192,0x1000,4,0x20000},"committed state and current/initial permissions");
 row=invoke(target.address+4096,pointer,0);c(fields()==std::array<uint32_t,7>{target.address+4096,target.address,4,4096,0x1000,4,0x20000},"page-aligned interior query translation");
 space.protect(target,brutal::foundation::Permission::read);invoke(target.address,pointer,0);c(fields()[2]==4&&fields()[5]==2,"allocation protection preserved across protect");
 space.protect(target,brutal::foundation::Permission::none);invoke(target.address,pointer,0);c(fields()[4]==0x1000&&fields()[5]==1,"no-access committed metadata is not a readable page");c.refuse(Fault::protection,[&]{space.read(target.address,4);},"query success does not bypass guest protection");
 space.protect(target,brutal::foundation::Permission::read_write);space.decommit(target);invoke(target.address,pointer,0);c(fields()[4]==0x2000&&fields()[5]==4,"decommit preserves stored protection like selected SDK heap");space.commit(target);
 auto before=space.read(pointer,28);row=invoke(target.address,pointer,UINT32_MAX);c(row==std::array<uint32_t,4>{0xc000000d,1,1,0},"actual SDK invalid selector status");c(space.read(pointer,28)==before,"SDK error leaves OUT untouched");
 for(auto kind:{1u,2u})c.refuse(Fault::unsupported,[&]{invoke(target.address,pointer,kind);},"unverified selectors remain denied");
 for(auto bad:{uint32_t(0),pointer+1,output.address+8192-24}){before=space.read(pointer,28);c.refuse(bad==output.address+8192-24?Fault::ownership:Fault::bounds,[&]{invoke(target.address,bad,0);},"bad OUT pointer refused");c(space.read(pointer,28)==before,"preflight refusal atomicity");}
 c.refuse(Fault::bounds,[&]{invoke(target.address+1,pointer,0);},"unaligned query address");c.refuse(Fault::ownership,[&]{invoke(0xdeadb000,pointer,0);},"foreign address not introspected");c.refuse(Fault::unsupported,[&]{invoke(image.section(1).address,pointer,0);},"unrepresented executable permission denied");
 space.protect(output,brutal::foundation::Permission::read);c.refuse(Fault::protection,[&]{invoke(target.address,pointer,0);},"OUT requires write before SDK");space.protect(output,brutal::foundation::Permission::write);row=invoke(target.address,pointer,0);c(row[0]==0&&row[3]==1,"OUT-only write-only storage accepted without read");space.protect(output,brutal::foundation::Permission::read_write);
 space.protect(target,brutal::foundation::Permission::write);c.refuse(Fault::unsupported,[&]{invoke(target.address,pointer,0);},"write-only query not mapped to fake RW");space.protect(target,brutal::foundation::Permission::read_write);
 image.admit_import(0xee,XexImage::Signature::query_basic28);c(true,"exact query import signature");c.refuse(Fault::unsupported,[&]{image.admit_import(0xee,XexImage::Signature::unverified);},"unverified query ABI");
 auto wrong=xex_generated;wrong.query=nullptr;{Space other;c.refuse(Fault::unsupported,[&]{XexImage bad(other,xex_bytes,{xex_bytes,wrong,xex_entry});},"query code/image symbol identity");c(other.live()==0,"binding failure allocates no guest image");}
 {ModuleLife alias(image.life().identity);GuestThread foreign(space,alias,slots,slot);
  {Scope scope(foreign,image.functions(),alias.identity,nullptr,{},expansion_plan_ptr);c.refuse(Fault::argument,[&]{QueryFrame denied(image,foreign);},"query frame belongs to actual image object");}
  foreign.cleanup();c(alias.close(),"foreign synthetic identity cleanup");}
 space.release(target);space.release(output);slots.release(slot);c(image.unload()&&space.live()==0&&space.leases()==0,"service resources balanced");
 // Bounded worker tests: ownership survives active query; close admission/stop,
 // or change OUT protection before publication. No guest continuation on error.
 for(bool cancel:{true,false}){
  Space s;XexImage mod(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots registry;auto t=registry.allocate();auto in=s.allocate(4096),out=s.allocate(4096);std::array<std::byte,28> marker;marker.fill(std::byte{0xa5});s.write(out.address,marker);GuestThread guest(s,mod.life(),registry,t);GuestTask task(guest);brutal::synthetic::CooperativeStop stop;brutal::horizon::Event ready,release;bool stopped=false;unsigned entered=0,returned=0,committed=0;
  c(task.start([&]{guest.context.r3.u64=in.address;guest.context.r4.u64=out.address;guest.context.r5.u64=0;guest.context.lr=0x8201feed;Scope scope(guest,mod.functions(),mod.life().identity,&stop,{},expansion_plan_ptr);mod.lend(scope);scope.lend(in);scope.lend(out);QueryFrame frame(mod,guest);HookFrame hooks(scope,xex_hooks,observed);frame.admitted=[&]{ready.signal();if(!release.wait())throw Error(Fault::unsupported,"active query gate");};
   try{scope.invoke(xex_query_only);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}
   catch(...){entered=frame.entered;returned=frame.returned;committed=frame.committed;throw;}
   entered=frame.entered;returned=frame.returned;committed=frame.committed;
  }),"active query worker starts");
  c(ready.wait(),"query admitted with retained pointers");c.refuse(Fault::ownership,[&]{s.decommit(in);},"query input cannot decommit while leased");c(!task.join(0),"failed join retains resources");
  if(cancel){c(!mod.unload(),"image unload refused during query");c.refuse(Fault::stale,[&]{mod.admit_import(0xee,XexImage::Signature::query_basic28);},"new query denied after closing");c(stop.request(),"stop requested during query");}
  else{s.protect(out,brutal::foundation::Permission::read);}
  release.signal();c(task.join(),"query worker joins");
  if(cancel){c(stopped&&!task.callback_error()&&entered==1&&returned==1&&committed==1,"accepted actual SDK query commits then stops");stop.joined();}
  else{c(task.callback_error()&&entered==1&&returned==1&&committed==0,"actual SDK return cannot silently publish success after OUT refusal");c(s.read(out.address,28)==std::vector<std::byte>(marker.begin(),marker.end()),"failed OUT publication preserves marker");}
  guest.cleanup();if(cancel)stop.reclaimed();s.release(in);s.release(out);registry.release(t);c(mod.unload()&&s.live()==0&&s.leases()==0,"query worker final cleanup");if(cancel)stop.unloaded();
 }
}
#endif

#if STAGE_MAX >= 11
void diagnostic_cases(Check c){
 using brutal::diagnostic::Kind;using brutal::diagnostic::Trace;using brutal::diagnostic::Binding;
 for(unsigned mode=0;mode<8;++mode){auto trace=std::make_unique<Trace>(ii_metadata);uint64_t identity=0,module=0;Kind expected=Kind::refusal;uint32_t expected_pc=0;
  {Binding binding(trace.get(),0,0);Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());identity=guest.identity;module=guest.module_instance();guest.context.r3.u64=4;guest.context.lr=0x8201feed;
   if(mode==5){guest.context.msr=0;c.refuse(Fault::argument,[&]{Scope denied(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);},"incomplete context diagnostic");expected=Kind::abi_rejected;}
   else if(mode>=6){GuestTask task(guest);
    if(mode==7){c(!task.start([]{},NativeThread::Injection::creation_failure),"injected native creation failure");c.refuse(Fault::argument,[&]{task.join(0);},"unstarted join is not success");expected=Kind::refusal;}
    else{expected=Kind::callback_failure;c(task.start([&]{Scope scope(guest,image.functions(),image.life().identity,nullptr,[](uint32_t){throw Error(Fault::unsupported,"original callback failure");},expansion_plan_ptr);scope.invoke(xex_add64);}),"diagnostic callback worker starts");c(task.join()&&task.callback_error(),"callback exception propagated to owner");}
   }else{
    auto record=space.allocate(4);guest.context.r3.u64=record.address;
    {Scope scope(guest,image.functions(),image.life().identity,nullptr,[&](uint32_t pc){if(mode==1&&pc==0x820100cc)guest.context.ctr.u64=0xdeadbeec;if(mode==3&&pc==xex_record_pair)Scope::deny_exception(guest.context);if(mode==4&&pc==xex_record_pair)Scope::deny_instruction(guest.context);},expansion_plan_ptr);image.lend(scope);scope.lend(record);ImportFrame imports(image,guest);QueryFrame query(image,guest);HookFrame hooks(scope,xex_hooks,observed);
     if(mode==0){expected_pc=xex_record_pair+4;c.refuse(Fault::ownership,[&]{scope.invoke(xex_record_pair);},"invalid memory path captured");c(guest.context.r3.u64==record.address,"no continuation after memory refusal");}
     if(mode==1){expected=Kind::branch_rejected;expected_pc=0x820100cc;guest.context.r3.u64=4;c.refuse(Fault::unsupported,[&]{image.invoke(scope,xex_entry);},"invalid branch path captured");c(guest.context.r3.u64==12,"unknown target never entered");}
     if(mode==2){expected=Kind::branch_rejected;expected_pc=xex_bad_return+8;c.refuse(Fault::unsupported,[&]{scope.invoke(xex_bad_return);},"bad return path captured");}
     if(mode==3){expected=Kind::exceptional_rejected;expected_pc=xex_record_pair;c.refuse(Fault::unsupported,[&]{scope.invoke(xex_record_pair);},"synthetic exceptional hook refuses");c(scope.reads==0,"no read after exceptional refusal");}
     if(mode==4){expected=Kind::instruction_rejected;expected_pc=xex_record_pair;c.refuse(Fault::unsupported,[&]{scope.invoke(xex_record_pair);},"synthetic unsupported instruction gate refuses");c(scope.reads==0,"no read after instruction refusal");}
    }space.release(record);
   }
   guest.cleanup();slots.release(slot);c(image.unload()&&space.live()==0&&space.leases()==0,"diagnostic case cleanup");
  }
  auto events=trace->snapshot();bool found=false,cleanup=false;
  for(auto event:events){if(event.kind==expected&&event.thread==identity&&event.module==module&&(expected_pc==0||event.pc==expected_pc))found=true;if(event.kind==Kind::cleanup&&event.thread==identity)cleanup=true;}
  c(found,"structured reason/module/thread/site matches actual refusal");c(cleanup,"cleanup recorded after refusal");c(trace->metadata.image==ii_metadata.image&&trace->metadata.codegen==ii_metadata.codegen&&trace->metadata.sdk==ii_metadata.sdk,"image/codegen/SDK fingerprints retained");c(trace->dropped()==0&&!trace->broken(),"bounded case diagnostic integrity");c.r.diagnostics+=unsigned(events.size());c.r.dropped+=unsigned(trace->dropped());
 }
}
void hardening(Check c,unsigned cycles){
 uint32_t seed=0xb11d;auto random=[&]{seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;};
 for(unsigned round=0;round<cycles;++round){Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event gate;std::array<unsigned,4> values{},expected{};BatchGuard batch(tasks,gate);
  for(unsigned i=0;i<4;++i){auto start=7+random()%32,count=random()%8;expected[i]=start+count+13;guests[i]=std::make_unique<GuestThread>(space,image.life(),slots,t,start,image.tls_template());tasks[i]=std::make_unique<GuestTask>(*guests[i]);c(batch.started[i]=tasks[i]->start([&,i,count]{auto&g=*guests[i];g.context.r3.u64=count;g.context.lr=0x8201feed;Scope scope(g,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,g);QueryFrame query(image,g);HookFrame hooks(scope,xex_hooks,observed);ready[i].signal();if(!gate.wait())throw Error(Fault::unsupported,"II stress gate");image.invoke(scope,xex_entry);values[i]=g.context.r3.u32;}),"hardening worker starts");}
  for(unsigned i=0;i<4;++i){c(batch.started[i]&&ready[i].wait(),"four concurrent import scopes ready");}gate.signal();for(unsigned i=0;i<4;++i){c(batch.started[i]&&(batch.joined[i]=tasks[i]->join())&&!tasks[i]->callback_error(),"hardening join succeeds");c(values[i]==expected[i],"seeded multicall oracle");guests[i]->cleanup();}slots.release(t);c(image.unload()&&space.live()==0&&space.leases()==0,"load/unload round balanced");
 }
 {Space s;ModuleLife module(0x4949);Slots slots;auto slot=slots.allocate();std::array<std::unique_ptr<GuestThread>,4> guests;std::array<std::unique_ptr<GuestTask>,4> tasks;std::array<brutal::horizon::Event,4> ready;brutal::horizon::Event gate;
  {BatchGuard batch(tasks,gate);for(unsigned i=0;i<4;++i){guests[i]=std::make_unique<GuestThread>(s,module,slots,slot);tasks[i]=std::make_unique<GuestTask>(*guests[i]);batch.started[i]=tasks[i]->start([&,i]{GuestThread::Execution execution(*guests[i]);ready[i].signal();if(!gate.wait())throw Error(Fault::unsupported,"partial startup gate");guests[i]->set_tls(i);},i==2?NativeThread::Injection::creation_failure:NativeThread::Injection::none);c(batch.started[i]==(i!=2),"partial startup result");}for(unsigned i=0;i<4;++i){if(batch.started[i])c(ready[i].wait(),"partial startup worker active");}}
  for(unsigned i=0;i<4;++i){c(!tasks[i]->callback_error(),"partial startup cleanup callback");guests[i]->cleanup();}slots.release(slot);c(module.close()&&s.live()==0&&s.leases()==0,"partial native startup rolls back after joining");
 }
 for(int stage=0;stage<4;++stage){Space s;c.refuse(Fault::exhausted,[&]{XexImage failed(s,xex_bytes,{xex_bytes,xex_generated,xex_entry},stage);},"partial image initialization refuses");c(s.live()==0,"transactional image rollback");}
 {Space s(16384);std::array<Space::Handle,4> all;for(auto&h:all)h=s.allocate(4096);c.refuse(Fault::exhausted,[&]{s.allocate(1);},"bounded allocation pressure");s.release_all(all);c(s.live()==0,"allocation pressure cleanup");}
 {Space s;XexImage image(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto t=slots.allocate();GuestThread retained(s,image.life(),slots,t);auto pointer=image.functions()[0].implementation;c(!image.unload(),"active module owner prevents final unload");c.refuse(Fault::stale,[&]{Scope denied(retained,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);},"closed module rejects stale scope");c.refuse(Fault::no_thread,[&]{pointer(retained.context,nullptr);},"unbound stale native function pointer cannot enter guest");retained.cleanup();slots.release(t);c(image.unload()&&s.live()==0,"stale-reference cleanup");}
}
#endif

}
RuntimeIIResult runtime_ii_probe(unsigned cycles,bool tracing){RuntimeIIResult r;
#if STAGE_MAX >= 11
 std::unique_ptr<brutal::diagnostic::Trace> trace;try{if(tracing)trace=std::make_unique<brutal::diagnostic::Trace>(ii_metadata);}catch(const std::bad_alloc&){std::fprintf(stderr,"diagnostic buffer allocation failed\n");r.failed=1;return r;}brutal::diagnostic::Binding root_trace(trace.get(),0,0);
#else
 (void)tracing;
#endif
constexpr unsigned budget=
#ifdef __SWITCH__
1;
#else
8;
#endif
 if(cycles<1||cycles>budget){throw Error(Fault::argument,"II test budget");}
 const auto sdk=ImportFrame::sdk_completed.load(),sdk_entered=ImportFrame::sdk_entered.load(),entries=Scope::guest_entries.load();for(auto&s:observed_sites){s=false;}try{foundation({r,0});abi({r,1});threads_and_stop({r,1});
#if STAGE_MAX >= 10
 services({r,2});
#endif
#if STAGE_MAX >= 11
 diagnostic_cases({r,3});hardening({r,3},cycles);
#endif
 unsigned memory_seen=0,back_seen=0;for(size_t i=0;i<std::size(expansion_sites);++i){auto site=expansion_sites[i];if(observed_sites[i]&&(site.kind==SiteKind::read||site.kind==SiteKind::write))++memory_seen;if(observed_sites[i]&&site.kind==SiteKind::branch&&site.target<=site.pc)++back_seen;}Check c{r,1};c(memory_seen==(STAGE_MAX>=10?37u:34u),"all declared memory hooks observed");c(back_seen==4,"all declared backedges observed");}catch(const std::exception&e){std::fprintf(stderr,"II error: %s\n",e.what());++r.failed;}r.sdk_calls=unsigned(ImportFrame::sdk_completed.load()-sdk);r.sdk_entered=unsigned(ImportFrame::sdk_entered.load()-sdk_entered);for(auto f:xex_generated.functions){if(f.name.starts_with("__imp__"))++r.imports;}r.guest_entries=unsigned(Scope::guest_entries.load()-entries);const auto totals=Space::totals();r.remaining=unsigned(totals.first);r.leases=unsigned(totals.second);if(r.remaining||r.leases)++r.failed;
#if STAGE_MAX >= 11
 if(trace){auto events=trace->snapshot();r.diagnostics+=unsigned(events.size());r.dropped+=unsigned(trace->dropped());if(trace->broken()||r.dropped)++r.failed;}
#endif
 return r;}
