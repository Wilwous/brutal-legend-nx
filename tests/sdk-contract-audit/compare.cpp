// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/query.h"
#include "runtime/horizon/fill.h"
#include "xex-data.h"
#include <iostream>
// Exact pinned implementation: keep its behavior separate from Windows oracle.
namespace rex::kernel::xboxkrnl {
u32 RtlCompareMemoryUlong_entry(mapped_void,u32,u32);
}
using namespace brutal::horizon;
int main(){try{unsigned positive=0,refusals=0,bytes=0;auto yes=[&](bool x){if(!x)throw std::runtime_error("compare contract assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing boundary refusal");};
 // Byte-level independent oracles distinguish all matching words from prefix
 // bytes. Neither oracle is presented as real Xbox evidence.
 for(unsigned length=4;length<=64;length+=4)for(unsigned mismatch=0;mismatch<=length/4;++mismatch){
  std::array<rex::be<uint32_t>,16> data{};auto raw=std::as_writable_bytes(std::span(data));for(unsigned i=0;i<length;++i)raw[i]=std::byte(0x10203040u>>(24-8*(i%4)));if(mismatch<length/4)raw[mismatch*4+1]^=std::byte{0xff};
  unsigned total=0,prefix=0;bool stopped=false;for(unsigned i=0;i<length;i+=4){bool equal=true;for(unsigned j=0;j<4;++j){equal&=raw[i+j]==std::byte(0x10203040u>>(24-8*j));++bytes;}if(equal)++total;if(!equal)stopped=true;if(!stopped)prefix+=4;}
  auto result=rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(data.data(),0x10000000),length,0x10203040);yes(result==total);yes(prefix==(mismatch<length/4?mismatch*4:length));
 }
 {std::array<rex::be<uint32_t>,4> data{0x10203040,0,0x10203040,0x10203040};yes(rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(data.data(),0x10000000),16,0x10203040)==3);yes(rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(nullptr,1),16,0)==0);yes(rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(nullptr,0),3,0)==0);yes(rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(nullptr,0),0,0)==0);}
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());auto ro=image.section(2);auto entries=Scope::guest_entries.load();
 {Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);ImportFrame imports(image,guest);QueryFrame query(image,guest);FillFrame fill(image,guest);
  auto native=[&]{guest.context.r3.u64=0x55555555;return imports.compare(ro.address+0x200,16,0x10203040);};yes(native()==3);yes(guest.context.r3.u64==0x55555555); // Only shim, not C++ helper, writes return register.
  auto before=ImportFrame::sdk_entered.load();auto r13=guest.context.r13.u64;guest.context.r13.u64=1;

  no([&]{native();});no([&]{ImportFrame::current(guest.context);});no([&]{FillFrame::current(guest.context);});no([&]{QueryFrame::current(guest.context);});yes(ImportFrame::sdk_entered==before);guest.context.r13.u64=r13;
  for(auto [address,length]:std::array<std::pair<uint32_t,uint32_t>,7>{{{0,4},{ro.address+1,4},{ro.address,0},{ro.address,3},{ro.address,68},{ro.address+ro.size-4,8},{guest.stack.address,4}}})no([&]{imports.compare(address,length,0);});

  NativeThread closing;yes(closing.start([&]{image.life().close();}));yes(closing.join()&&!closing.callback_error());no([&]{native();});yes(ImportFrame::sdk_entered==before);
 }
 guest.cleanup();slots.release(slot);yes(image.unload());yes(Space::totals()==std::pair<size_t,size_t>{0,0});yes(Scope::guest_entries==entries);unsigned generated_refusals=0;
 for(unsigned mode=0;mode<3;++mode){
  Space mem;XexImage img(mem,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots tls;auto t=tls.allocate();GuestThread g(mem,img.life(),tls,t,7,img.tls_template());auto out=mem.allocate(4096);auto data=img.section(3);const uint32_t address=mode==0?data.address+0x800:out.address;auto saved=mem.read(address,28);unsigned callbacks=0;uint64_t at_boundary=0;
  {Scope scope(g,img.functions(),img.life().identity,nullptr,{},expansion_plan_ptr);img.lend(scope);scope.lend(out);ImportFrame imports(img,g);QueryFrame query(img,g);FillFrame fill(img,g);HookFrame hooks(scope,xex_hooks);
   auto corrupt=[&]{++callbacks;at_boundary=ImportFrame::sdk_entered.load();g.context.r13.u64=1;};if(mode==0)imports.admitted=corrupt;else if(mode==1)query.admitted=corrupt;else fill.admitted=corrupt;
   g.context.r3.u64=mode==0?2:out.address;g.context.r4.u64=mode==1?out.address:16;g.context.r5.u64=mode==1?0:0x10203040;g.context.lr=0x8201feed;
   no([&]{scope.invoke(mode==0?xex_stage8_entry:mode==1?xex_query_only:xex_fill_only);});yes(callbacks==1&&ImportFrame::sdk_entered==at_boundary);yes(mem.read(address,28)==saved);yes(scope.writes>0);++generated_refusals;
   // Guest prologue stores already happened. This is fail-stop, not guest rollback.
  }g.cleanup();tls.release(t);mem.release(out);yes(img.unload());
 }
 {Space mem;ModuleLife m(55);Slots tls;auto t=tls.allocate();GuestThread g(mem,m,tls,t);auto before=Scope::guest_entries.load();g.context.r13.u64=1;no([&]{Scope scope(g,xex_generated.functions,m.identity,nullptr,{},expansion_plan_ptr);});yes(Scope::guest_entries==before);g.cleanup();tls.release(t);yes(m.close());}
 yes(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<bytes<<",\"native_only_guest_entries\":0,\"generated_refusals\":"<<generated_refusals<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
