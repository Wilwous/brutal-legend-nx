// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "sdk_checked_memory.h"
#include <rex/ppc.h>
#include <functional>
#include <atomic>
#include <span>
namespace brutal::imports {
class Admission;
class PointerBinding {
  friend class Admission;
 public:
  using Memory=brutal::foundation::SdkCheckedMemory;
  struct Statistics { uint64_t scratch_allocations,scratch_reclaims,sdk_returns; };
  static Statistics statistics(){return {scratch_allocations_.load(),scratch_reclaims_.load(),sdk_returns_.load()};}
  enum class Stage { validated, scratch_ready, sdk_returned };
  // Optional trusted host diagnostics; receives no buffer pointer/address.
  using Observer=std::function<void(Stage)>;
  PointerBinding(Memory& memory,Memory::Allocation allocation,Observer observer={})
    :memory_(memory),allocation_(std::move(allocation)),observer_(std::move(observer)){}
 private:
  static inline std::atomic<uint64_t> scratch_allocations_{0},scratch_reclaims_{0},sdk_returns_{0};
  Memory& memory_;
  Memory::Allocation allocation_;
  Observer observer_;
  class Scratch {
    rex::memory::Memory& sdk_;uint32_t address_=0;
   public:
    Scratch(rex::memory::Memory& sdk,uint32_t size):sdk_(sdk){
      address_=sdk_.SystemHeapAlloc(size,16,rex::memory::kSystemHeapDefault,this);
      if(!address_)throw std::bad_alloc();
      ++scratch_allocations_;
    }
    Scratch(const Scratch&)=delete;
    ~Scratch(){if(address_){if(!sdk_.LookupHeap(address_)->ReleaseOwned(address_,this))std::terminate();++scratch_reclaims_;}}
    uint32_t address()const{return address_;}
    void copy(std::span<std::byte> bytes,bool output){
      auto result=sdk_.LookupHeap(address_)->CopyOwnedMemory(address_,this,address_,bytes.data(),bytes.size(),
        output?rex::memory::CheckedMemoryAccess::read:rex::memory::CheckedMemoryAccess::write);
      if(result!=Memory::Result::ok)throw Memory::BufferError(result);
    }
  };
  void observe(Stage stage){if(observer_)observer_(stage);}
  void invoke(uint32_t ordinal,PPCFunc* shim,PPCContext& context,rex::memory::Memory& sdk){
    if(!memory_.uses(sdk))throw std::invalid_argument("pointer memory identity");
    PPCContext call=context;
    uint64_t address=0,length=0;bool output=false,preserve=false,returns_status=false;
    if(ordinal==0x11b){ // RtlCompareMemoryUlong: SDK counts matching words.
      address=context.r3.u64;length=context.r4.u64;preserve=true;
      if(context.r5.u64>UINT32_MAX||!length||(length&3))throw std::invalid_argument("word contract");
    }else if(ordinal==0x12e){ // RtlInitializeCriticalSection: selected fields only.
      address=context.r3.u64;length=28;output=preserve=true;
    }else if(ordinal==0xee){ // NtQueryVirtualMemory: bounded same-owner target.
      address=context.r4.u64;length=28;output=returns_status=true;
      if(context.r3.u64<allocation_.guest_address()||context.r3.u64>UINT32_MAX||
         context.r5.u64>UINT32_MAX||context.r5.u32==1||context.r5.u32==2)
        throw std::invalid_argument("query contract");
    }else throw std::invalid_argument("pointer ordinal not supported");
    if(address&3)throw std::invalid_argument("pointer alignment");
    memory_.with_buffer(allocation_,address,length,output,preserve,[&](std::span<std::byte> bytes)->bool{
      // Query only within this live allocation; this extra read is bounded and
      // uses the held transaction's host snapshot extent, never a guest pointer.
      if(ordinal==0xee){
        // Its allocation extent is checked by a separate metadata-only helper.
        if(!memory_.contains_locked(allocation_,context.r3.u64,1))throw std::invalid_argument("query target ownership");
      }
      observe(Stage::validated);
      Scratch scratch(sdk,uint32_t(length));scratch.copy(bytes,false);
      observe(Stage::scratch_ready);
      if(ordinal==0xee)call.r4.u64=scratch.address();else call.r3.u64=scratch.address();
      shim(call,sdk.virtual_membase());
      ++sdk_returns_;
      observe(Stage::sdk_returned);
      if(ordinal==0x11b){if(call.r3.u64>length/4)throw std::runtime_error("unexpected word return");return false;}
      if(returns_status){
        if(call.r3.u32==0xc000000du)return false;
        if(call.r3.u32!=0)throw std::runtime_error("unverified query status");
      }
      scratch.copy(bytes,true);return true;
    });
    // Only the declared scalar result escapes. Void preserves original r3;
    // neither the scratch pointer nor an altered register context can escape.
    if(ordinal!=0x12e)context.r3.u64=call.r3.u32;
  }
};
}
