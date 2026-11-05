// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "generated.h"
#include <optional>
namespace brutal::horizon::compatibility {
// Selected native profile of pinned SDK exports, not an Xbox KPROCESS/APC model.
// One module/Space, <=16 registered live threads,64 TLS cells, startup slot0
// reserved by GuestThread. Invalid capacity indices use the SDK's 0/1 results;
// reserved slot0 is explicitly unsupported. Calls serialize through this owner.
class SdkServices {
 public:
 enum Ordinal:uint32_t {fill_ulong=294,tls_alloc=338,tls_free=339,tls_get=340,tls_set=341};
 struct Thread {uint64_t process,thread;};
 private:
 struct Entry {GuestThread* guest;const Space::Handle tls;Space::Lease pin;Entry(GuestThread&g):guest(&g),tls(g.tls),pin(g.space(),tls){}};
 inline static std::atomic<uint64_t> serial_{1};const uint64_t identity_{serial_.fetch_add(1)};
 Space& space_;ModuleLife& module_;Slots& slots_;std::optional<ModuleLife::Lease> life_;
 std::mutex mutex_;std::array<std::optional<Slots::Token>,64> allocated_{};std::map<uint64_t,std::unique_ptr<Entry>> threads_;bool closed_{};
 Entry& current(Thread token,PPCContext&ctx){
  if(closed_)throw Error(Fault::stale,"SDK service owner closed");
  auto i=threads_.find(token.thread);
  if(token.process!=identity_||i==threads_.end()||&i->second->guest->context!=&ctx)throw Error(Fault::ownership,"SDK current thread identity");
  auto&s=Scope::current(ctx);s.validate_context();if(!s.belongs_to(module_,space_))throw Error(Fault::ownership,"SDK scope/module identity");return *i->second;
 }
 uint32_t cell(const Entry&g,uint32_t slot)const{return g.tls.address+GuestThread::static_tls_size+slot*4;}
 static void supported(uint32_t slot){if(slot==0)throw Error(Fault::unsupported,"SDK profile reserves startup TLS slot zero");}
 public:
 uint64_t entered{},completed{};
 SdkServices(Space&s,ModuleLife&m,Slots&slots):space_(s),module_(m),slots_(slots),life_(std::in_place,m){}
 SdkServices(const SdkServices&)=delete;
 ~SdkServices(){try{close();}catch(...){std::terminate();}}
 Thread register_thread(GuestThread&g){
  std::lock_guard lock(mutex_);ModuleLife::Lease active(module_);if(closed_||!g.belongs_to(module_)||!g.uses_slots(slots_)||&g.space()!=&space_||threads_.size()>=16||threads_.contains(g.identity))throw Error(Fault::ownership,"SDK thread registration");
  if(g.tls.size!=GuestThread::static_tls_size+64*4)throw Error(Fault::unsupported,"SDK profile TLS extent");
  threads_.emplace(g.identity,std::make_unique<Entry>(g));return {identity_,g.identity};
 }
 void unregister_thread(Thread t){std::lock_guard lock(mutex_);if(t.process!=identity_||!threads_.erase(t.thread))throw Error(Fault::ownership,"SDK thread withdrawal");}
 // Caller admits a known original hook target separately. This method requires
 // an active real Scope/GuestThread execution guard; it never creates a fallback.
 void invoke(Thread token,PPCContext&ctx,Space::Handle fill_owner={}){
  std::lock_guard lock(mutex_);ModuleLife::Lease active(module_);auto&g=current(token,ctx);++entered;
  const uint32_t ordinal=ctx.r11.u32;std::optional<uint32_t> result;
  switch(ordinal){
   case fill_ulong:{
    const auto address=ctx.r3.u32,length=ctx.r4.u32,pattern=ctx.r5.u32;
    if(!length||length>65536||(address&3)||(length&3))throw Error(Fault::unsupported,"aligned nonempty bounded fill subset only");
    Scope::current(ctx).admit_range(address,length);space_.validate_owned_window(fill_owner,address,length,Access::write);
    std::vector<std::byte> bytes(length);for(uint32_t i=0;i<length;++i)bytes[i]=std::byte(pattern>>(24-(i&3)*8));space_.write(space_.view(address,length,Access::write),bytes);break;
   }
   case tls_alloc:{
    std::optional<Slots::Token> t;
    try{t=slots_.allocate();}catch(const Error&e){if(e.fault!=Fault::exhausted)throw;result=UINT32_MAX;break;}
    try{supported(t->index);space_.store<uint32_t>(cell(g,t->index),0);}catch(...){slots_.release(*t);throw;}
    allocated_[t->index]=*t;result=t->index;break;
   }
   case tls_free:{
    auto slot=ctx.r3.u32;if(slot>=64){result=0;break;}supported(slot);if(!allocated_[slot]){result=0;break;}
    slots_.validate(*allocated_[slot]);std::array<Space::WordWrite,16> cells{};size_t count=0;
    for(auto&[id,entry]:threads_){(void)id;cells[count++]={entry->tls,cell(*entry,slot),0};}
    space_.write_words(std::span(cells).first(count));slots_.release(*allocated_[slot]);allocated_[slot].reset();result=1;break;
   }
   case tls_get:{auto slot=ctx.r3.u32;if(slot>=64){result=0;break;}supported(slot);result=space_.load<uint32_t>(cell(g,slot));break;}
   case tls_set:{auto slot=ctx.r3.u32;if(slot>=64){result=0;break;}supported(slot);space_.store<uint32_t>(cell(g,slot),ctx.r4.u32);result=1;break;}
   default:throw Error(Fault::unsupported,"SDK ordinal not admitted");
  }
  if(result)ctx.r3.u64=*result; // void fill has no invented return value.
  ++completed;
 }
 void close(){std::lock_guard lock(mutex_);if(closed_)return;for(auto&t:allocated_)if(t){slots_.release(*t);t.reset();}threads_.clear();life_.reset();closed_=true;}
 size_t registered(){std::lock_guard lock(mutex_);return threads_.size();}
};
}
