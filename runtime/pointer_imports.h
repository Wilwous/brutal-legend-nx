// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "sdk_checked_memory.h"
#include <rex/ppc.h>
#include <functional>
#include <atomic>
#include <span>
#include <rex/system/user_module.h>
#include <rex/system/kernel_state.h>
#include <optional>
namespace brutal::imports {
class Admission;
class PointerBinding {
  friend class Admission;
 public:
  using Memory=brutal::foundation::SdkCheckedMemory;
  struct Statistics { uint64_t scratch_allocations,scratch_reclaims,sdk_returns,image_leases,image_reclaims,image_capabilities,image_capability_reclaims; };
  static Statistics statistics(){return {scratch_allocations_.load(),scratch_reclaims_.load(),sdk_returns_.load(),image_leases_.load(),image_reclaims_.load(),image_capabilities_.load(),image_capability_reclaims_.load()};}
  enum class Stage { validated, scratch_ready, sdk_returned };
  // Optional trusted host diagnostics; receives no buffer pointer/address.
  using Observer=std::function<void(Stage)>;
  PointerBinding(Memory& memory,Memory::Allocation allocation,Observer observer={})
    :memory_(memory),allocation_(std::move(allocation)),observer_(std::move(observer)){}
  // Explicit image capability; retains the module, but acquires a fresh lease
  // per admitted synchronous call. A retained birth lease pins this image
  // incarnation; an already closed capability can never revive on reload.
  struct Image {};
  PointerBinding(Image,Memory& memory,rex::system::object_ref<rex::system::UserModule> module,Observer observer={})
    :memory_(memory),module_(std::move(module)),observer_(std::move(observer)){
    if(!module_||!memory_.uses(*module_->kernel_state()->memory()))throw std::invalid_argument("image pointer owner");
    auto lease=module_->AcquireImageLease();if(lease){image_anchor_.emplace(std::move(*lease));++image_capabilities_;}
  }
  ~PointerBinding(){if(image_anchor_){image_anchor_.reset();++image_capability_reclaims_;}}
 private:
  static inline std::mutex image_transactions_;
  static inline std::atomic<uint64_t> image_leases_{0},image_reclaims_{0},image_capabilities_{0},image_capability_reclaims_{0};
  std::optional<rex::system::UserModule::ImageLease> image_anchor_;
  rex::system::object_ref<rex::system::UserModule> module_;
  bool accepts_module(const rex::system::UserModule* module)const{return !module_||module_.get()==module;}
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
  // Shared adapter semantics below serve heap and image capabilities. Image
  // transactions serialize across bindings; SDK mutations still require trusted
  // quiescence. Final checked copy revalidates the entire span before writing.
  std::optional<rex::system::UserModule::ImageLease> active_image_;
  bool contains(uint64_t address){
    if(!module_)return memory_.contains_locked(allocation_,address,1);
    std::byte byte{};
    return address<=UINT32_MAX&&active_image_->CopyMemory(uint32_t(address),&byte,1,Memory::Access::read)==Memory::Result::ok;
  }
  template<class F> void with_buffer(rex::memory::Memory& sdk,uint64_t address,uint64_t length,bool output,bool preserve,F&& callback){
    if(!module_){memory_.with_buffer(allocation_,address,length,output,preserve,std::forward<F>(callback));return;}
    if(!image_anchor_)throw Memory::BufferError(Memory::Result::ownership);
    if(Memory::active_transaction_)throw std::logic_error("nested image pointer transaction");
    std::lock_guard image_lock(image_transactions_);
    std::lock_guard memory_lock(memory_.mutex_);
    Memory::TransactionMarker transaction(&memory_);
    auto lease=module_->AcquireImageLease();
    if(!lease)throw Memory::BufferError(Memory::Result::ownership);
    active_image_.emplace(std::move(*lease));
    ++image_leases_;
    struct Reset {PointerBinding& binding;~Reset(){binding.active_image_.reset();++image_reclaims_;}} reset{*this};
    if(!address||address>UINT32_MAX||!length||length>65536||length>0x100000000ull-address)
      throw Memory::BufferError(Memory::Result::invalid_range);
    std::vector<std::byte> bytes(size_t(length),std::byte{});
    auto status=active_image_->CopyMemory(uint32_t(address),bytes.data(),length,Memory::Access::read);
    if(status!=Memory::Result::ok)throw Memory::BufferError(status);
    if(output){
      bool writable=false;
      for(const auto& section:module_->xex_module()->pe_sections())
        if(address>=section.address&&address-section.address<=section.size&&length<=section.size-(address-section.address))
          writable=(section.flags&0x80000000u)!=0;
      if(!writable)throw Memory::BufferError(Memory::Result::protection);
      for(uint64_t at=address;at<address+length;at=std::min(address+length,(at/4096+1)*4096)){
        rex::memory::HeapAllocationInfo info{};
        if(!sdk.LookupHeap(uint32_t(at))->QueryRegionInfo(uint32_t(at),&info)||
           !(info.state&rex::memory::kMemoryAllocationCommit)||
           !(info.protect&(rex::memory::kMemoryProtectWrite|rex::memory::kMemoryProtectWriteCombine)))
          throw Memory::BufferError(Memory::Result::protection);
      }
    }
    if(!preserve)std::fill(bytes.begin(),bytes.end(),std::byte{});
    if(std::invoke(std::forward<F>(callback),std::span<std::byte>(bytes))&&output){
      status=active_image_->CopyMemory(uint32_t(address),bytes.data(),length,Memory::Access::write);
      if(status!=Memory::Result::ok)throw Memory::BufferError(status);
    }
  }
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
      if(context.r3.u64>UINT32_MAX||
         context.r5.u64>UINT32_MAX||context.r5.u32==1||context.r5.u32==2)
        throw std::invalid_argument("query contract");
    }else throw std::invalid_argument("pointer ordinal not supported");
    if(address&3)throw std::invalid_argument("pointer alignment");
    with_buffer(sdk,address,length,output,preserve,[&](std::span<std::byte> bytes)->bool{
      // Query only within the admitted heap allocation or leased image.
      // Validation returns no pointer to guest backing.
      if(ordinal==0xee){
        // Image queries additionally require a readable owned section.
        if(!contains(context.r3.u64))throw std::invalid_argument("query target ownership");
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
