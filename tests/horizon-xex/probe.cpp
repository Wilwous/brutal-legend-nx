// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#include <atomic>
using namespace brutal::horizon;
using Stop=brutal::synthetic::CooperativeStop;
namespace {
struct Checks {
 XexResult& r;void operator()(bool value){value?++r.passed:++r.failed;}
 template<class F>void refuse(Fault fault,F f){try{f();(*this)(false);}catch(const Error&e){(*this)(e.fault==fault);}catch(...){(*this)(false);}}
};
void execute(XexImage& image,GuestThread& guest,XexResult& result,uint32_t iterations=4){
 auto sp=guest.context.r1.u64;guest.context.r3.u64=iterations;guest.context.r14.u64=0x1234567812345678;guest.context.r15.u64=0x8765432187654321;guest.context.lr=0x8201feed;
 Scope scope(guest,image.functions(),image.life().identity);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks);
 image.invoke(scope,xex_entry);result.observed_result=guest.context.r3.u32;result.sdk_calls+=imports.calls;
 for(size_t i=0;i<2;++i)result.hook_hits[i]+=hooks.hits[i];
 if(guest.context.r1.u64!=sp||guest.context.r2.u64!=0x82020000||guest.context.r13.u64!=guest.pcr.address||guest.context.lr!=0x8201feed||guest.context.r14.u64!=0x1234567812345678||guest.context.r15.u64!=0x8765432187654321)throw Error(Fault::unsupported,"generated register preservation failure");
}
void be(std::vector<uint8_t>& bytes,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(v>>(24-i*8));}
void le(std::vector<uint8_t>& bytes,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(v>>(i*8));}
}
XexResult xex_probe(){
 XexResult result;Checks check{result};
 try{
  Space space;auto policy=XexImage::Policy{xex_bytes,xex_generated,xex_entry};
  auto parsed=XexImage::parse(xex_bytes);check(parsed.entry==xex_entry&&parsed.base==0x82000000&&parsed.functions.size()==6);check(parsed.slots==64&&parsed.static_size==32&&parsed.raw_size==16);
  auto malformed=[&](Fault fault,auto change){std::vector<uint8_t> input(std::begin(xex_bytes),std::end(xex_bytes));change(input);check.refuse(fault,[&]{XexImage bad(space,input,policy);});check(space.live()==0);};
  malformed(Fault::argument,[&](auto&b){b[0]=0;});
  malformed(Fault::argument,[&](auto&b){le(b,0x1000+0x98+56,0x50000);}); // PE cannot grow resident image
  malformed(Fault::argument,[&](auto&b){le(b,0x1000+0x98+224+40+12,0);}); // overlapping section
  malformed(Fault::argument,[&](auto&b){le(b,0x1000+0x98+224+40+36,0xe0000020);}); // writable code
  malformed(Fault::argument,[&](auto&b){be(b,0x384,0x83000000);});
  malformed(Fault::argument,[&](auto&b){be(b,0x388,8);});
  malformed(Fault::unsupported,[&](auto&b){be(b,0x31000,0x99);});
  malformed(Fault::argument,[&](auto&b){be(b,44,0x83010000);});
  malformed(Fault::argument,[&](auto&b){b[0x11000]^=1;}); // valid header, wrong code identity
  std::vector<uint8_t> short_image(std::begin(xex_bytes),std::end(xex_bytes)-1);check.refuse(Fault::bounds,[&]{XexImage bad(space,short_image,policy);});
  GeneratedXex wrong=xex_generated;wrong.code_size=4;check.refuse(Fault::argument,[&]{XexImage bad(space,xex_bytes,{xex_bytes,wrong,xex_entry});});check(space.live()==0);
  for(int stage=0;stage<4;++stage){check.refuse(Fault::exhausted,[&]{XexImage failed(space,xex_bytes,policy,stage);});check(space.live()==0);}
  {Space small(128*1024);check.refuse(Fault::exhausted,[&]{XexImage failed(small,xex_bytes,policy);});check(small.live()==0);}
  XexImage image(space,xex_bytes,policy);check(space.live()==4);auto data=image.section(3),ro=image.section(2);check(space.load<uint32_t>(ro.address+0x200)==0x10203040);
  check.refuse(Fault::protection,[&]{space.store<uint32_t>(ro.address,0);});
  image.admit_import(0x11b,XexImage::Signature::compare_u32_read_words);check(true);
  check.refuse(Fault::unsupported,[&]{image.admit_import(0xee,XexImage::Signature::compare_u32_read_words);});
  check.refuse(Fault::unsupported,[&]{image.admit_import(0x12e,XexImage::Signature::unverified);});
  check.refuse(Fault::unsupported,[&]{image.admit_import(0x11b,XexImage::Signature::typed_vector);});
  Slots slots;auto slot=slots.allocate();auto initial=image.tls_template();
  {ModuleLife alias(image.life().identity);GuestThread foreign(space,alias,slots,slot);
   {Scope scope(foreign,image.functions(),alias.identity);check.refuse(Fault::argument,[&]{image.lend(scope);});check.refuse(Fault::argument,[&]{ImportFrame bad(image,foreign);});}
   foreign.cleanup();check(alias.close());}
  GuestThread guest(space,image.life(),slots,slot,7,initial);const auto sp=guest.context.r1.u64;
  check(space.read(guest.tls.address,16)==initial);check(space.load<uint32_t>(guest.pcr.address)==guest.tls.address);
  execute(image,guest,result);check(result.observed_result==17&&result.sdk_calls==1&&result.hook_hits[0]==4);
  check(guest.get_tls()==8&&guest.context.r1.u64==sp);
  guest.set_tls(7);
  {Scope scope(guest,image.functions(),image.life().identity);image.lend(scope);HookFrame hooks(scope,xex_hooks);
   check.refuse(Fault::argument,[&]{image.invoke(scope,xex_entry+4);});check.refuse(Fault::unsupported,[&]{scope.invoke(0xdeadbeec);});
   check.refuse(Fault::unsupported,[&]{Scope::indirect(guest.context,nullptr,0xdeadbeec);});
   space.store<uint32_t>(guest.context.r1.u32+84,9);guest.context.r3.u64=2;
   Scope::indirect(guest.context,nullptr,xex_combine);check(guest.context.r3.u64==11);
   auto saved_sp=guest.context.r1.u64;guest.context.r1.u64=guest.stack.address+guest.stack.size-80;
   check.refuse(Fault::argument,[&]{Scope::indirect(guest.context,nullptr,xex_combine);});guest.context.r1.u64=saved_sp;
   guest.context.r3.u64=17;guest.context.r4.u64=ro.address+0x204;
   check.refuse(Fault::protection,[&]{scope.invoke(xex_store);});check(space.load<uint32_t>(ro.address+0x204)==0);
   guest.context.r4.u64=data.address+0x400;scope.invoke(xex_store);check(space.load<uint32_t>(data.address+0x400)==17);
   guest.context.r3.u64=ro.address+0x200;guest.context.r4.u64=16;guest.context.r5.u64=0x10203040;
   check.refuse(Fault::no_thread,[&]{scope.invoke(parsed.thunk);});
   {ImportFrame imports(image,guest);
    scope.invoke(parsed.thunk);check(guest.context.r3.u64==3&&imports.calls==1);
    guest.context.r3.u64=ro.address+0x200;guest.context.r5.u64=0;scope.invoke(parsed.thunk);check(guest.context.r3.u64==1);
    guest.context.r3.u64=ro.address+0x200;guest.context.r5.u64=0xdeadbeef;scope.invoke(parsed.thunk);check(guest.context.r3.u64==0);
    for(auto pointer:{uint32_t(0),ro.address+1,ro.address+65532,guest.stack.address}){
     guest.context.r3.u64=pointer;guest.context.r4.u64=16;auto before=guest.context.r3.u64;
     check.refuse(Fault::bounds,[&]{scope.invoke(parsed.thunk);});check(guest.context.r3.u64==before);
    }
    guest.context.r3.u64=ro.address+0x200;guest.context.r4.u64=3;
    check.refuse(Fault::bounds,[&]{scope.invoke(parsed.thunk);});check(imports.calls==3);check(space.load<uint32_t>(ro.address+0x200)==0x10203040);
   }
   check.refuse(Fault::unsupported,[&]{HookFrame::poll(0xdeadbeec);});
   check.refuse(Fault::ownership,[&]{space.release(Space::Handle{0,0,ro.address,ro.size});});
  }
  GuestThread second(space,image.life(),slots,slot,41,initial);XexResult first_row,second_row;GuestTask one(guest),two(second);
  guest.set_tls(7);check(one.start([&]{execute(image,guest,first_row,3);}));check(two.start([&]{execute(image,second,second_row,5);}));check(one.join());check(two.join());
  check(!one.callback_error()&&!two.callback_error());check(first_row.observed_result==16&&second_row.observed_result==52&&first_row.sdk_calls==1&&second_row.sdk_calls==1);
  second.cleanup();guest.cleanup();check(space.live()==4);
  {GuestThread failing(space,image.life(),slots,slot,7,initial);GuestTask callback(failing);
   check(callback.start([&]{Scope scope(failing,image.functions(),image.life().identity,nullptr,[](uint32_t){throw Error(Fault::unsupported,"injected observer error");});image.lend(scope);image.invoke(scope,xex_entry);}));
   check(callback.join());check(callback.callback_error());failing.cleanup();check(space.live()==4);}
  // The actual production hook at the continuing-loop backedge stops an active call.
  GuestThread stopping(space,image.life(),slots,slot,7,initial);Stop stop;brutal::horizon::Event entered,release;std::atomic<bool> cancelled{};unsigned observed[2]{};
  GuestTask task(stopping);check(task.start([&]{
   Scope scope(stopping,image.functions(),image.life().identity,&stop,[&](uint32_t pc){if(pc==xex_hooks[1]){entered.signal();if(!release.wait())throw Error(Fault::unsupported,"hook rendezvous timeout");}});
   image.lend(scope);ImportFrame imports(image,stopping);HookFrame hooks(scope,xex_hooks);
   try{scope.invoke(xex_forever);}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;cancelled=true;stop.exited();}
   for(unsigned i=0;i<2;++i)observed[i]=hooks.hits[i];
  }));
  check(entered.wait());check(!image.unload());check.refuse(Fault::ownership,[&]{stopping.cleanup();});
  check.refuse(Fault::stale,[&]{GuestThread denied(space,image.life(),slots,slot);});
  check(!task.join(0));check(stop.request());check(!stop.request());release.signal();check(task.join());check(cancelled&&!task.callback_error()&&observed[1]==1);
  result.hook_hits[1]+=observed[1];stop.joined();stopping.cleanup();stop.reclaimed();check(image.unload());stop.unloaded();
  check(space.live()==0);check.refuse(Fault::bounds,[&]{space.load<uint32_t>(ro.address);});check.refuse(Fault::stale,[&]{image.section(0);});slots.release(slot);
  for(unsigned repeat=0;repeat<4;++repeat){Space fresh;XexImage reloaded(fresh,xex_bytes,policy);Slots local;auto t=local.allocate();GuestThread worker(fresh,reloaded.life(),local,t,7,reloaded.tls_template());XexResult row;execute(reloaded,worker,row,0);check(row.observed_result==13&&row.sdk_calls==1);worker.cleanup();check(reloaded.unload());check(fresh.live()==0);local.release(t);}
  {Space fresh;XexImage closing(fresh,xex_bytes,policy);brutal::horizon::Event ready_a,ready_b,gate;std::atomic<unsigned> closes{};NativeThread a,b;
   check(a.start([&]{ready_a.signal();if(!gate.wait())throw Error(Fault::unsupported,"close gate");if(closing.unload())++closes;}));
   check(b.start([&]{ready_b.signal();if(!gate.wait())throw Error(Fault::unsupported,"close gate");if(closing.unload())++closes;}));
   check(ready_a.wait());check(ready_b.wait());gate.signal();check(a.join());check(b.join());check(closes==2&&!a.callback_error()&&!b.callback_error());check(fresh.live()==0);}
  result.remaining=uint32_t(space.live());check(result.hook_hits[0]>0&&result.hook_hits[1]>0);
 }catch(...){check(false);}
 return result;
}
