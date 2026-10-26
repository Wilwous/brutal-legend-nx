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
  struct Region {Handle h;foundation::Allocation backing;size_t leases{};};
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
  void unlease(Handle h)noexcept{try{std::lock_guard lock(mutex_);auto& r=owned(h);if(!r.leases)std::terminate();--r.leases;}catch(...){std::terminate();}}
 public:
  explicit Space(uint32_t capacity=4*1024*1024):memory_(capacity){}
  Space(const Space&)=delete;
  ~Space(){for(auto&[id,r]:regions_){(void)id;if(r.leases)std::terminate();}}
  Handle reserve(uint32_t size,uint32_t guest=0){
    std::lock_guard lock(mutex_);
    if(guest&&(guest&4095))throw Error(Fault::argument,"guest mapping alignment");
    if(guest&&uint64_t(guest)+size>0x100000000ull)throw Error(Fault::bounds,"guest mapping overflow");
    auto a=memory_.reserve(size);uint32_t address=guest?guest:a.guest;
    try{
      for(auto&[id,r]:regions_){(void)id;if(address<uint64_t(r.h.address)+r.h.size&&r.h.address<uint64_t(address)+size)throw Error(Fault::ownership,"overlapping guest mapping");}
      Handle h{owner_,next_,address,size};regions_.emplace(next_,Region{h,a});++next_;return h;
    }catch(...){memory_.release(a);throw;}
  }
  Handle allocate(uint32_t size,uint32_t guest=0){auto h=reserve(size,guest);try{commit(h);return h;}catch(...){release(h);throw;}}
  void commit(Handle h,Permission p=Permission::read_write){std::lock_guard lock(mutex_);memory_.commit(owned(h).backing,p);}
  void protect(Handle h,Permission p){std::lock_guard lock(mutex_);memory_.protect(owned(h).backing,p);}
  void decommit(Handle h){std::lock_guard lock(mutex_);auto&r=owned(h);if(r.leases)throw Error(Fault::ownership,"active memory lease");memory_.decommit(r.backing);}
  void release(Handle h){std::lock_guard lock(mutex_);auto&r=owned(h);if(r.leases)throw Error(Fault::ownership,"active memory lease");memory_.release(r.backing);regions_.erase(h.id);}
  void release_all(std::span<const Handle> handles){
    std::lock_guard lock(mutex_);
    for(size_t i=0;i<handles.size();++i){
      if(owned(handles[i]).leases)throw Error(Fault::ownership,"active lease prevents transactional release");
      for(size_t j=0;j<i;++j)if(handles[i].id==handles[j].id)throw Error(Fault::argument,"duplicate release token");
    }
    for(auto h:handles){memory_.release(owned(h).backing);regions_.erase(h.id);}
  }
  class Lease {
    Space* space_{};Handle handle_{};
   public:
    Lease(Space& space,Handle h):space_(&space),handle_(h){std::lock_guard lock(space.mutex_);++space.owned(h).leases;}
    Lease(const Lease&)=delete;Lease&operator=(const Lease&)=delete;
    Lease(Lease&& other)noexcept:space_(std::exchange(other.space_,nullptr)),handle_(other.handle_){}
    ~Lease(){if(space_)space_->unlease(handle_);}
  };
  std::vector<std::byte> read(uint32_t address,uint32_t size){
    std::lock_guard lock(mutex_);auto&r=range(address,size);
    return memory_.read(memory_.translate(r.backing.guest+(address-r.h.address),size,Access::read));
  }
  void write(uint32_t address,std::span<const std::byte> bytes){
    if(bytes.empty()||bytes.size()>UINT32_MAX)throw Error(Fault::bounds,"invalid guest write size");
    std::lock_guard lock(mutex_);auto&r=range(address,uint32_t(bytes.size()));
    memory_.write(memory_.translate(r.backing.guest+(address-r.h.address),uint32_t(bytes.size()),Access::write),bytes);
  }
  template<class T> T load(uint32_t address){auto bytes=read(address,sizeof(T));T value=0;for(auto b:bytes)value=T((value<<8)|std::to_integer<unsigned>(b));return value;}
  template<class T> void store(uint32_t address,T value){std::array<std::byte,sizeof(T)> bytes{};for(size_t i=0;i<sizeof(T);++i){bytes[sizeof(T)-1-i]=std::byte(value&255);value=T(value>>8);}write(address,bytes);}
  size_t live(){std::lock_guard lock(mutex_);return regions_.size();}
};
// Admission closes before memory can be reclaimed. This is original fixture
// ownership, not a scheduler or an Xbox object handle implementation.
class ModuleLife {
  std::mutex mutex_;size_t leases_{};bool closing_{},alive_{true};
 public:
  const uint64_t identity;
  explicit ModuleLife(uint64_t id):identity(id){if(!id)throw Error(Fault::argument,"module identity");}
  ~ModuleLife(){if(leases_)std::terminate();}
  class Lease {
    ModuleLife* module_;
   public:
    explicit Lease(ModuleLife& m):module_(&m){std::lock_guard lock(m.mutex_);if(!m.alive_||m.closing_)throw Error(Fault::stale,"module admission closed");++m.leases_;}
    Lease(const Lease&)=delete;
    ~Lease(){std::lock_guard lock(module_->mutex_);--module_->leases_;}
  };
  bool close(){std::lock_guard lock(mutex_);closing_=true;if(leases_)return false;alive_=false;return true;}
};
}
