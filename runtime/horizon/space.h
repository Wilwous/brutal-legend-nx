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
  struct Region {Handle h;foundation::Allocation backing;size_t leases{};Permission initial{Permission::none};bool initialized{},write_frozen{};};
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
  void unlease(Handle h,bool frozen)noexcept{try{std::lock_guard lock(mutex_);auto& r=owned(h);if(!r.leases||(frozen&&!r.write_frozen))std::terminate();if(frozen)r.write_frozen=false;--r.leases;--lease_total_;}catch(...){std::terminate();}}
  static void check_frozen(const Region&r,Permission p){if(r.write_frozen&&p!=Permission::read&&p!=Permission::none)throw Error(Fault::protection,"leased verification forbids writable permission");}
 public:
  explicit Space(uint32_t capacity=4*1024*1024):memory_(capacity){}
  Space(const Space&)=delete;
  ~Space(){for(auto&[id,r]:regions_){(void)id;if(r.leases)std::terminate();}live_total_-=regions_.size();}
 private:
  Handle reserve_impl(uint32_t size,uint32_t guest,bool exact){
    std::lock_guard lock(mutex_);
    if(!exact&&guest&&(guest&4095))throw Error(Fault::argument,"guest mapping alignment");
    if(exact&&!guest)throw Error(Fault::argument,"exact logical slice needs an explicit address");
    if(guest&&uint64_t(guest)+size>0x100000000ull)throw Error(Fault::bounds,"guest mapping overflow");
    auto a=memory_.reserve(size);uint32_t address=guest?guest:a.guest;
    try{
      for(auto&[id,r]:regions_){(void)id;if(address<uint64_t(r.h.address)+r.h.size&&r.h.address<uint64_t(address)+size)throw Error(Fault::ownership,"overlapping guest mapping");}
      Handle h{owner_,next_,address,size};regions_.emplace(next_,Region{h,a});++next_;++live_total_;return h;
    }catch(...){memory_.release(a);throw;}
  }
 public:
  Handle reserve(uint32_t size,uint32_t guest=0){return reserve_impl(size,guest,false);}
  // Opt-in exact logical bytes; backing still uses the one checked arena.
  // No enclosing guest page is granted access or returned by page query.
  Handle reserve_slice(uint32_t size,uint32_t guest){return reserve_impl(size,guest,true);}
  Handle allocate(uint32_t size,uint32_t guest=0){auto h=reserve(size,guest);try{commit(h);return h;}catch(...){release(h);throw;}}
  void commit(Handle h,Permission p=Permission::read_write){std::lock_guard lock(mutex_);auto&r=owned(h);check_frozen(r,p);memory_.commit(r.backing,p);if(!r.initialized){r.initial=p;r.initialized=true;}}
  void protect(Handle h,Permission p){std::lock_guard lock(mutex_);auto&r=owned(h);check_frozen(r,p);memory_.protect(r.backing,p);}
  void seal_read_only(Handle h){std::lock_guard lock(mutex_);memory_.seal_read_only(owned(h).backing);}
  foundation::Memory::Info info(Handle h){std::lock_guard lock(mutex_);return memory_.info(owned(h).backing);}
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
    friend class Space;
    Space* space_{};Handle handle_{};bool owns_freeze_{};
   public:
    Lease(Space& space,Handle h):space_(&space),handle_(h){std::lock_guard lock(space.mutex_);++space.owned(h).leases;++lease_total_;}
    Lease(const Lease&)=delete;Lease&operator=(const Lease&)=delete;
    Lease(Lease&& other)noexcept:space_(std::exchange(other.space_,nullptr)),handle_(other.handle_),owns_freeze_(std::exchange(other.owns_freeze_,false)){}
    ~Lease(){if(space_)space_->unlease(handle_,owns_freeze_);}
  };
  // An exact RW allocation owner can temporarily freeze writes for a multi-step
  // verification. This lease moves with the freeze; other pins cannot publish it.
  // Revocation to none remains allowed. Permanent seals are never bypassed.
  void freeze_writes(Handle h,Lease& pin){
    std::lock_guard lock(mutex_);auto&r=owned(h);
    if(pin.space_!=this||pin.handle_.id!=h.id||pin.owns_freeze_||r.write_frozen)
      throw Error(Fault::ownership,"invalid verification freeze owner");
    memory_.translate(r.backing.guest,h.size,Access::read);
    memory_.translate(r.backing.guest,h.size,Access::write);
    memory_.protect(r.backing,Permission::read);r.write_frozen=true;pin.owns_freeze_=true;
  }
  void publish_frozen(Lease& pin,Permission p){
    std::lock_guard lock(mutex_);
    if(pin.space_!=this||!pin.owns_freeze_)throw Error(Fault::ownership,"missing verification freeze");
    if(p!=Permission::read&&p!=Permission::read_write)throw Error(Fault::argument,"invalid frozen publication permission");
    auto&r=owned(pin.handle_);if(!r.write_frozen)throw Error(Fault::ownership,"verification freeze lost");
    memory_.translate(r.backing.guest,r.h.size,Access::read);
    memory_.protect(r.backing,p);r.write_frozen=false;pin.owns_freeze_=false;
  }
  // Consume this allocation's final pin and release atomically. A competing
  // pin refuses without changing either token; lookup/pin races cannot bind ABA.
  bool release_exclusive(Lease& pin){
    std::lock_guard lock(mutex_);
    if(pin.space_!=this)throw Error(Fault::ownership,"foreign or consumed release lease");
    auto&r=owned(pin.handle_);
    if(r.leases>1)return false;
    if(r.leases!=1)throw Error(Fault::ownership,"missing release lease");
    memory_.release(r.backing);regions_.erase(pin.handle_.id);--live_total_;--lease_total_;pin.space_=nullptr;return true;
  }
  bool release_exclusive_all(std::span<Lease*> pins){
    std::lock_guard lock(mutex_);bool busy=false;
    for(size_t i=0;i<pins.size();++i){auto*p=pins[i];
      if(!p||p->space_!=this)throw Error(Fault::ownership,"foreign group release lease");
      auto&r=owned(p->handle_);if(!r.leases)throw Error(Fault::ownership,"missing group lease");
      for(size_t j=0;j<i;++j)if(pins[j]->handle_.id==p->handle_.id)throw Error(Fault::argument,"duplicate group release lease");
      busy|=r.leases!=1;
    }
    if(busy)return false;
    for(auto*p:pins){memory_.release(owned(p->handle_).backing);regions_.erase(p->handle_.id);--live_total_;--lease_total_;p->space_=nullptr;}
    return true;
  }
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
  struct WordWrite {Handle owner;uint32_t address,value;};
  // Bounded atomic publication of disjoint BEu32 cells in this Space. Used by
  // process TLS retirement: validate every owner/permission before changing any
  // thread. The Space lock excludes protect/free between validation and writes;
  // Memory::write does no allocation. No pointer or callback escapes this lock.
  void write_words(std::span<const WordWrite> words){
    if(words.empty()||words.size()>64)throw Error(Fault::bounds,"word transaction budget");
    std::array<foundation::View,64> views{};std::array<std::array<std::byte,4>,64> bytes{};
    std::lock_guard lock(mutex_);
    for(size_t i=0;i<words.size();++i){const auto w=words[i];auto&r=owned(w.owner);
      if((w.address&3)||w.address<r.h.address||uint64_t(w.address)+4>uint64_t(r.h.address)+r.h.size||uint64_t(w.address)+4>0x100000000ull)
        throw Error(Fault::bounds,"word transaction extent/alignment");
      for(size_t j=0;j<i;++j)if(words[j].address==w.address)throw Error(Fault::ownership,"overlapping word transaction");
      views[i]=memory_.translate(r.backing.guest+(w.address-r.h.address),4,Access::write);memory_.validate(views[i],Access::write);
      for(unsigned b=0;b<4;++b)bytes[i][b]=std::byte(w.value>>(24-8*b));
    }
    for(size_t i=0;i<words.size();++i)memory_.write(views[i],bytes[i]);
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
