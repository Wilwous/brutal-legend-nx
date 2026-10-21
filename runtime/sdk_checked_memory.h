// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <rex/system/xmemory.h>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <vector>

namespace brutal::foundation {
// Original host adapter. SDK and MMIO configuration must outlive this object.
// Reset/Restore/teardown and MMIO registration require quiescence. Guest page
// policy is configured by trusted SDK setup, never inferred from host mappings.
class SdkCheckedMemory {
  struct Record { uint32_t address, size; bool live = true; };
 public:
  using Result = rex::memory::CheckedMemoryResult;
  using Access = rex::memory::CheckedMemoryAccess;
  class Allocation {
    friend class SdkCheckedMemory;
    const SdkCheckedMemory* manager_ = nullptr;
    std::shared_ptr<Record> record_;
   public:
    Allocation() = default;
    // Integer metadata only; no pointer to guest backing is exposed.
    uint32_t guest_address() const { return record_ ? record_->address : 0; }
  };
  explicit SdkCheckedMemory(rex::memory::Memory& memory) : memory_(memory) {}
  SdkCheckedMemory(const SdkCheckedMemory&) = delete;
  SdkCheckedMemory& operator=(const SdkCheckedMemory&) = delete;
  ~SdkCheckedMemory() {
    for (auto& record : records_) if (record->live)
      memory_.LookupHeap(record->address)->ReleaseOwned(record->address,record.get());
  }
  Allocation allocate(uint32_t size, bool commit = true) {
    if (!size || size > 1024*1024) throw std::invalid_argument("checked allocation range");
    std::lock_guard lock(mutex_);
    if (records_.size()>=4096) throw std::bad_alloc(); // Bound lifetime-token storage.
    auto record = std::make_shared<Record>(Record{0,size});
    // Reserve host-aligned groups, but keep 4-KiB guest page policy independent.
    auto* heap = memory_.LookupHeap(0x10000000);
    uint32_t rounded = 0;
    if (!rex::memory::CheckedAllocationRoundUp(size,16384,rounded) ||
        !heap->AllocRange(0x10000000,0x1fffffff,rounded,16384,
          rex::memory::kMemoryAllocationReserve |
            (commit ? rex::memory::kMemoryAllocationCommit : 0),
          commit ? 3 : 0,false,&record->address)) throw std::bad_alloc();
    try {
      heap->SetAllocationOwner(record->address,record.get());
      records_.push_back(record);
    } catch (...) { heap->Release(record->address); throw; }
    Allocation handle; handle.manager_ = this; handle.record_ = std::move(record);
    return handle;
  }
  Result release(const Allocation& handle) {
    std::lock_guard lock(mutex_);
    if (!valid(handle)) return Result::ownership;
    auto& r = *handle.record_;
    if (!memory_.LookupHeap(r.address)->ReleaseOwned(r.address,handle.record_.get()))
      return Result::ownership;
    r.live = false;
    return Result::ok;
  }
  Result read(const Allocation& h, uint64_t offset, std::span<std::byte> output) {
    return transfer(h,offset,output.data(),output.size(),Access::read);
  }
  Result write(const Allocation& h, uint64_t offset, std::span<const std::byte> input) {
    return transfer(h,offset,const_cast<std::byte*>(input.data()),input.size(),Access::write);
  }
  Result fetch(const Allocation& h, uint64_t offset, std::span<std::byte> output) {
    // Always fails closed: this SDK lacks guest execute permission metadata.
    return transfer(h,offset,output.data(),output.size(),Access::fetch);
  }
  Result store32be(const Allocation& h,uint64_t offset,uint32_t value) {
    const std::array<std::byte,4> bytes{std::byte(value>>24),std::byte(value>>16),
                                     std::byte(value>>8),std::byte(value)};
    return write(h,offset,bytes);
  }
  Result load32be(const Allocation& h,uint64_t offset,uint32_t& value) {
    std::array<std::byte,4> bytes{};
    auto result = read(h,offset,bytes);
    if (result == Result::ok) {
      uint32_t decoded=0;
      for (auto b:bytes) decoded=(decoded<<8)|std::to_integer<uint32_t>(b);
      value=decoded;
    }
    return result;
  }
 private:
  bool valid(const Allocation& h) const {
    return h.manager_==this && h.record_ && h.record_->live;
  }
  Result transfer(const Allocation& h,uint64_t offset,void* buffer,uint64_t length,Access access) {
    std::lock_guard lock(mutex_);
    if (!valid(h)) return Result::ownership;
    const auto& r=*h.record_;
    // Subtraction first prevents both host-width and guest-width overflow.
    if (!length || length>65536 || offset>r.size || length>uint64_t(r.size)-offset)
      return Result::invalid_range;
    return memory_.LookupHeap(r.address)->CopyOwnedMemory(r.address,h.record_.get(),
                uint32_t(uint64_t(r.address)+offset),buffer,length,access);
  }
  rex::memory::Memory& memory_;
  std::mutex mutex_;
  // Retain distinct lifetime tokens; no same-owner ABA through this interface.
  std::vector<std::shared_ptr<Record>> records_;
};
} // namespace brutal::foundation
