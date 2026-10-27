// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>

namespace brutal::foundation {
enum class Fault { argument, exhausted, ownership, stale, bounds, protection, uncommitted, no_thread, slot, unsupported };
class Error : public std::runtime_error {
 public:
  Fault fault;
  Error(Fault f, const char* text) : std::runtime_error(text), fault(f) {}
};
enum class Access { read, write };
enum class Permission { none, read, read_write, write };
struct Allocation { uint64_t owner{}, id{}; uint32_t guest{}, size{}; };
struct View { Allocation allocation; uint32_t offset{}, length{}; uint64_t epoch{}; Access access{}; };

// Bounded checked memory for component execution, not a 4-GiB PPC address space.
// No raw host pointer escapes. Permissions apply to checked accesses, not mprotect.
class Memory {
  struct Region { Allocation a; bool committed{}; Permission permission{}; uint64_t epoch{1}; };
  inline static std::atomic<uint64_t> next_owner_{1};
  const uint64_t owner_{next_owner_.fetch_add(1)};
  std::vector<std::byte> bytes_;
  std::map<uint64_t, Region> regions_;
  mutable std::mutex mutex_;
  uint64_t cursor_{}, next_id_{1};
  static constexpr uint32_t base_ = 0x10000000;
  static void valid_permission(Permission p) {
    if(p!=Permission::none && p!=Permission::read && p!=Permission::read_write && p!=Permission::write)
      throw Error(Fault::argument,"unsupported memory permission");
  }
  Region& region(const Allocation& a) {
    if (a.owner != owner_) throw Error(Fault::ownership, "allocation belongs to another arena");
    auto i = regions_.find(a.id);
    if (i == regions_.end()) throw Error(Fault::stale, "allocation has been released");
    auto& r = i->second;
    if (r.a.guest != a.guest || r.a.size != a.size) throw Error(Fault::ownership, "allocation token mismatch");
    return r;
  }
  Region& check(const View& v, Access operation) {
    auto& r = region(v.allocation);
    if (r.epoch != v.epoch) throw Error(Fault::stale, "mapping epoch changed");
    if (!r.committed) throw Error(Fault::uncommitted, "memory is reserved but not committed");
    if (v.offset > r.a.size || v.length > r.a.size-v.offset) throw Error(Fault::bounds, "view exceeds allocation");
    if (v.access != operation) throw Error(Fault::protection, "view access mode mismatch");
    if ((operation == Access::read && r.permission != Permission::read && r.permission != Permission::read_write) ||
        (operation == Access::write && r.permission != Permission::write && r.permission != Permission::read_write))
      throw Error(Fault::protection, "memory protection denies access");
    return r;
  }
  size_t index(const Allocation& a, uint32_t offset=0) const { return size_t(a.guest-base_)+offset; }
 public:
  static constexpr uint32_t page_size = 4096;
  explicit Memory(uint32_t capacity=1024*1024) {
    if (!owner_ || !capacity || capacity>64*1024*1024) throw Error(Fault::argument, "invalid arena capacity");
    try { bytes_.resize(capacity); } catch (const std::bad_alloc&) { throw Error(Fault::exhausted, "arena backing allocation failed"); }
  }
  Memory(const Memory&)=delete;
  Memory& operator=(const Memory&)=delete;
  Allocation reserve(uint32_t size, uint32_t alignment=page_size) {
    std::lock_guard lock(mutex_);
    if (!size || alignment<page_size || (alignment & (alignment-1))) throw Error(Fault::argument, "invalid size/alignment");
    const uint64_t start=(uint64_t(base_)+cursor_+alignment-1)&~uint64_t(alignment-1);
    const uint64_t end=start+((uint64_t(size)+page_size-1)&~uint64_t(page_size-1));
    if (start<base_ || end>uint64_t(base_)+bytes_.size() || end>UINT32_MAX || next_id_==UINT64_MAX)
      throw Error(Fault::exhausted, "bounded guest arena exhausted");
    Allocation a{owner_,next_id_,uint32_t(start),size};
    try { regions_.emplace(a.id,Region{a}); } catch(const std::bad_alloc&) { throw Error(Fault::exhausted,"allocation metadata failed"); }
    ++next_id_; cursor_=end-base_;
    return a;
  }
  void commit(const Allocation& a, Permission p=Permission::read_write) {
    std::lock_guard lock(mutex_);valid_permission(p);auto& r=region(a);
    if (!r.committed) std::fill_n(bytes_.begin()+index(a),a.size,std::byte{});
    r.committed=true;r.permission=p;
  }
  Allocation allocate(uint32_t size, uint32_t alignment=page_size) {
    auto a=reserve(size,alignment);commit(a);return a;
  }
  void protect(const Allocation& a, Permission p) {
    std::lock_guard lock(mutex_);valid_permission(p);auto& r=region(a);
    if (!r.committed) throw Error(Fault::uncommitted,"protect requires committed memory");
    r.permission=p;
  }
  void decommit(const Allocation& a) {
    std::lock_guard lock(mutex_);auto& r=region(a);
    std::fill_n(bytes_.begin()+index(a),a.size,std::byte{});r.committed=false;++r.epoch;
  }
  void release(const Allocation& a) {
    std::lock_guard lock(mutex_);region(a);
    std::fill_n(bytes_.begin()+index(a),a.size,std::byte{});regions_.erase(a.id);
    // Addresses are quarantined for this arena's lifetime; stale guest addresses
    // cannot silently resolve to a later allocation. This is not a general heap.
  }
  View translate(uint32_t guest, uint32_t length, Access access) {
    std::lock_guard lock(mutex_);
    if (!guest || !length) throw Error(Fault::argument,"null/empty guest range");
    for (auto& [id,r]:regions_) {
      (void)id;
      if (guest>=r.a.guest && uint64_t(guest)-r.a.guest<r.a.size) {
        View v{r.a,guest-r.a.guest,length,r.epoch,access};check(v,access);return v;
      }
    }
    throw Error(Fault::bounds,"guest address is not owned by a live allocation");
  }
  struct Info {bool committed;Permission permission;};
  Info info(const Allocation& a){std::lock_guard lock(mutex_);auto&r=region(a);return {r.committed,r.permission};}
  void validate(const View& v,Access access){std::lock_guard lock(mutex_);check(v,access);}
  std::vector<std::byte> read(const View& v) {
    std::lock_guard lock(mutex_);check(v,Access::read);
    const auto i=index(v.allocation,v.offset);
    return {bytes_.begin()+i,bytes_.begin()+i+v.length};
  }
  void write(const View& v, std::span<const std::byte> data) {
    std::lock_guard lock(mutex_);check(v,Access::write);
    if (data.size()!=v.length) throw Error(Fault::bounds,"write size differs from view");
    std::copy(data.begin(),data.end(),bytes_.begin()+index(v.allocation,v.offset));
  }
  void store32(uint32_t guest,uint32_t value) {
    const std::byte data[]={std::byte(value>>24),std::byte(value>>16),std::byte(value>>8),std::byte(value)};
    write(translate(guest,4,Access::write),data);
  }
  uint32_t load32(uint32_t guest) {
    auto data=read(translate(guest,4,Access::read));uint32_t result=0;
    for(auto b:data)result=(result<<8)|std::to_integer<uint32_t>(b);
    return result;
  }
  size_t live_allocations() const { std::lock_guard lock(mutex_);return regions_.size(); }
};
} // namespace brutal::foundation
