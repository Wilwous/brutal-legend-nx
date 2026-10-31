// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/checked_memory.h"
#include <utility>
#include <array>
#include <exception>
namespace brutal::horizon {
using foundation::Error;using foundation::Fault;using foundation::Permission;using foundation::Access;
// Logical guest mappings, never fixed host mappings or executable host storage.
// Global order: Space lock -> underlying checked arena lock. No reverse path.
class Space {
 public:
  struct Handle {uint64_t owner{},id{};uint32_t address{},size{};};
 private:
  struct Region {Handle h;foundation::Allocation backing;size_t leases{};Permission initial{Permission::none};bool initialized{};};
  inline static std::atomic<size_t> live_total_{},lease_total_{};
  foundation::Memory memory_;std::mutex mutex_;std::map<uint64_t,Region> regions_;
  inline static std::atomic<uint64_t> serial_{1};
  uint64_t owner_{serial_.fetch_add(1)},next_{1};
  Region& owned(Handle h){
    if(h.owner!=owner_)throw Error(Fault::ownership,"foreign space handle");
    auto i=regions_.find(h.id);if(i==regions_.end())throw Error(Fault::stale,"released space handle");
    if(i->second.h.address!=h.address||i->second.h.size!=h.size)throw Error(Fault::ownership,"forged space handle");
    return i->second;
  }
  Region& range(uint32_t address,uint32_t size){
    if(!address||!size||uint64_t(address)+size>0x100000000ull)throw Error(Fault::bounds,"invalid guest span");
    for(auto& [id,r]:regions_){(void)id;if(address>=r.h.address&&uint64_t(address)+size<=uint64_t(r.h.address)+r.h.size)return r;}
    throw Error(Fault::bounds,"guest span is not resident in one owned region");
  }
  void unlease(Handle h)noexcept{try{std::lock_guard lock(mutex_);auto& r=owned(h);if(!r.leases)std::terminate();--r.leases;--lease_total_;}catch(...){std::terminate();}}
 public:
  explicit Space(uint32_t capacity=4*1024*1024):memory_(capacity){}
  Space(const Space&)=delete;
  ~Space(){for(auto&[id,r]:regions_){(void)id;if(r.leases)std::terminate();}live_total_-=regions_.size();}
  Handle reserve(uint32_t size,uint32_t guest=0){
    std::lock_guard lock(mutex_);
    if(guest&&(guest&4095))throw Error(Fault::argument,"guest mapping alignment");
    if(guest&&uint64_t(guest)+size>0x100000000ull)throw Error(Fault::bounds,"guest mapping overflow");
    auto a=memory_.reserve(size);uint32_t address=guest?guest:a.guest;
    try{
      for(auto&[id,r]:regions_){(void)id;if(address<uint64_t(r.h.address)+r.h.size&&r.h.address<uint64_t(address)+size)throw Error(Fault::ownership,"overlapping guest mapping");}
      Handle h{owner_,next_,address,size};regions_.emplace(next_,Region{h,a});++next_;++live_total_;return h;
    }catch(...){memory_.release(a);throw;}
  }
  Handle allocate(uint32_t size,uint32_t guest=0){auto h=reserve(size,guest);try{commit(h);return h;}catch(...){release(h);throw;}}
  void commit(Handle h,Permission p=Permission::read_write){std::lock_guard lock(mutex_);auto&r=owned(h);memory_.commit(r.backing,p);if(!r.initialized){r.initial=p;r.initialized=true;}}
  void protect(Handle h,Permission p){std::lock_guard lock(mutex_);memory_.protect(owned(h).backing,p);}
  void decommit(Handle h){std::lock_guard lock(mutex_);auto&r=owned(h);if(r.leases)throw Error(Fault::ownership,"active memory lease");memory_.decommit(r.backing);}
  void release(Handle h){std::lock_guard lock(mutex_);auto&r=owned(h);if(r.leases)throw Error(Fault::ownership,"active memory lease");memory_.release(r.backing);regions_.erase(h.id);--live_total_;}
  void release_all(std::span<const Handle> handles){
    std::lock_guard lock(mutex_);
    for(size_t i=0;i<handles.size();++i){
      if(owned(handles[i]).leases)throw Error(Fault::ownership,"active lease prevents transactional release");
      for(size_t j=0;j<i;++j)if(handles[i].id==handles[j].id)throw Error(Fault::argument,"duplicate release token");
    }
    for(auto h:handles){memory_.release(owned(h).backing);regions_.erase(h.id);--live_total_;}
  }
  // Context teardown only: original private registrations may already have
  // been retired by a trusted host caller. Never accept them for entry again.
  // Check every surviving token/lease before releasing any surviving region.
  void release_survivors(std::span<const Handle> handles){
    std::lock_guard lock(mutex_);
    for(size_t i=0;i<handles.size();++i){auto h=handles[i];
      if(h.owner!=owner_)throw Error(Fault::ownership,"foreign context registration");
      if(!h.id||h.id>=next_)throw Error(Fault::stale,"unissued context registration");
      for(size_t j=0;j<i;++j)if(handles[j].id==h.id)throw Error(Fault::argument,"duplicate context registration");
      if(regions_.contains(h.id)&&owned(h).leases)throw Error(Fault::ownership,"context registration is leased");
    }
    for(auto h:handles)if(regions_.contains(h.id)){memory_.release(owned(h).backing);regions_.erase(h.id);--live_total_;}
  }
  class Lease {
    Space* space_{};Handle handle_{};
   public:
    Lease(Space& space,Handle h):space_(&space),handle_(h){std::lock_guard lock(space.mutex_);++space.owned(h).leases;++lease_total_;}
    Lease(const Lease&)=delete;Lease&operator=(const Lease&)=delete;
    Lease(Lease&& other)noexcept:space_(std::exchange(other.space_,nullptr)),handle_(other.handle_){}
    ~Lease(){if(space_)space_->unlease(handle_);}
  };
  Lease pin(uint32_t address,uint32_t size){
    Handle h;{std::lock_guard lock(mutex_);h=range(address,size).h;}
    // A release between lookup and acquisition makes this exact token stale;
    // it cannot pin a later allocation at the same logical address.
    return Lease(*this,h);
  }
  struct Query {uint32_t base,allocation_base,allocation_size,region_size;bool committed;Permission initial,current;};
  Query query(uint32_t address){
    std::lock_guard lock(mutex_);auto&r=range(address,1);auto info=memory_.info(r.backing);
    if((address&4095)||(r.h.address&4095)||(r.h.size&4095))throw Error(Fault::unsupported,"query requires whole guest pages");
    return {address,r.h.address,r.h.size,r.h.size-(address-r.h.address),info.committed,r.initial,info.permission};
  }
  // A copied capability with a mapping epoch, never a host pointer. Permissions
  // are rechecked on every use; decommit/recommit permanently invalidates it.
  class View {friend class Space;Handle handle;foundation::View backing;View(Handle h,foundation::View v):handle(h),backing(v){}
   public: View(const View&)=default;};
  View view(uint32_t address,uint32_t size,Access access){
    std::lock_guard lock(mutex_);auto&r=range(address,size);diagnostic::AllocationBinding allocation(owner_,r.h.id);
    return {r.h,memory_.translate(r.backing.guest+(address-r.h.address),size,access)};
  }
  // Full logical span, exact allocation token, current commitment/permissions.
  // Leases prevent release/decommit; protection can still change and is rechecked
  // at each admission and actual access. No page or permission model is duplicated.
  void validate_owned_window(Handle h,uint32_t address,uint32_t size,Access access,bool also_read=false){
    std::lock_guard lock(mutex_);auto&r=owned(h);
    if(access!=Access::read&&access!=Access::write)throw Error(Fault::argument,"unknown owned-window access");
    if(!size||address<r.h.address||uint64_t(address)+size>uint64_t(r.h.address)+r.h.size||uint64_t(address)+size>0x100000000ull)
      throw Error(Fault::bounds,"owned window exceeds one allocation");
    diagnostic::AllocationBinding allocation(owner_,r.h.id);
    if(also_read)memory_.validate(memory_.translate(r.backing.guest+(address-r.h.address),size,Access::read),Access::read);
    memory_.validate(memory_.translate(r.backing.guest+(address-r.h.address),size,access),access);
  }
  std::vector<std::byte> read(const View& v){
    std::lock_guard lock(mutex_);auto&r=owned(v.handle);diagnostic::AllocationBinding allocation(owner_,r.h.id);
    if(v.backing.allocation.owner!=r.backing.owner||v.backing.allocation.id!=r.backing.id)
      throw Error(Fault::ownership,"view backing mismatch");
    return memory_.read(v.backing);
  }
  void write(const View& v,std::span<const std::byte> bytes){
    std::lock_guard lock(mutex_);auto&r=owned(v.handle);diagnostic::AllocationBinding allocation(owner_,r.h.id);
    if(v.backing.allocation.owner!=r.backing.owner||v.backing.allocation.id!=r.backing.id)
      throw Error(Fault::ownership,"view backing mismatch");
    memory_.write(v.backing,bytes);
  }
  // Trusted bounded leaf transaction only: callback must not retain scratch or
  // reenter Space. No guest pointer is exposed; callback failure publishes nothing.
  template<class F> void update(const View& input,const View& output,F transform){
    std::lock_guard lock(mutex_);auto&r=owned(input.handle);auto&o=owned(output.handle);diagnostic::AllocationBinding allocation(owner_,r.h.id);
    if(r.h.id!=o.h.id||input.backing.allocation.id!=r.backing.id||input.backing.allocation.owner!=r.backing.owner||
       output.backing.allocation.id!=r.backing.id||output.backing.allocation.owner!=r.backing.owner||
       input.backing.offset!=output.backing.offset||input.backing.length!=output.backing.length)
      throw Error(Fault::ownership,"INOUT view mismatch");
    memory_.validate(input.backing,Access::read);memory_.validate(output.backing,Access::write);
    auto scratch=memory_.read(input.backing);transform(std::span(scratch));memory_.write(output.backing,scratch);
  }
  size_t leases(){std::lock_guard lock(mutex_);size_t n=0;for(auto&[id,r]:regions_){(void)id;n+=r.leases;}return n;}
  std::vector<std::byte> read(uint32_t address,uint32_t size){
    std::lock_guard lock(mutex_);auto&r=range(address,size);diagnostic::AllocationBinding allocation(owner_,r.h.id);
    return memory_.read(memory_.translate(r.backing.guest+(address-r.h.address),size,Access::read));
  }
  void write(uint32_t address,std::span<const std::byte> bytes){
    if(bytes.empty()||bytes.size()>UINT32_MAX)throw Error(Fault::bounds,"invalid guest write size");
    std::lock_guard lock(mutex_);auto&r=range(address,uint32_t(bytes.size()));diagnostic::AllocationBinding allocation(owner_,r.h.id);
    memory_.write(memory_.translate(r.backing.guest+(address-r.h.address),uint32_t(bytes.size()),Access::write),bytes);
  }
  template<class T> T load(uint32_t address){auto bytes=read(address,sizeof(T));T value=0;for(auto b:bytes)value=T((value<<8)|std::to_integer<unsigned>(b));return value;}
  template<class T> void store(uint32_t address,T value){std::array<std::byte,sizeof(T)> bytes{};for(size_t i=0;i<sizeof(T);++i){bytes[sizeof(T)-1-i]=std::byte(value&255);value=T(value>>8);}write(address,bytes);}
  static std::pair<size_t,size_t> totals(){return {live_total_.load(),lease_total_.load()};}
  size_t live(){std::lock_guard lock(mutex_);return regions_.size();}
};
// Admission closes before memory can be reclaimed. This is original fixture
// ownership, not a scheduler or an Xbox object handle implementation.
class ModuleLife {
  std::mutex mutex_;size_t leases_{};bool closing_{},alive_{true};
  diagnostic::Trace* trace_{diagnostic::current_trace()};
  inline static std::atomic<uint64_t> next_instance_{1};
 public:
  const uint64_t identity;const uint64_t instance{next_instance_.fetch_add(1)};
  explicit ModuleLife(uint64_t id):identity(id){if(!id)throw Error(Fault::argument,"module identity");}
  ~ModuleLife(){if(leases_)std::terminate();}
  class Lease {
    ModuleLife* module_;
   public:
    explicit Lease(ModuleLife& m):module_(&m){std::lock_guard lock(m.mutex_);if(!m.alive_||m.closing_)throw Error(Fault::stale,"module admission closed");++m.leases_;}
    Lease(const Lease&)=delete;
    ~Lease(){std::lock_guard lock(module_->mutex_);--module_->leases_;}
  };
  bool close(){std::lock_guard lock(mutex_);closing_=true;diagnostic::emit_for(trace_,diagnostic::Kind::module_close,instance,diagnostic::context.thread,uint32_t(leases_));if(leases_)return false;alive_=false;return true;}
};
}
