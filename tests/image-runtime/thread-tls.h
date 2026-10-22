// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "owned-module.h"
#include <rex/ppc.h>
#include <rex/system/thread_state.h>
#include <atomic>
REX_EXTERN(__imp__KeTlsSetValue);
namespace image_fixture {
// Host fixture module extension, indexed through a reserved SDK dynamic TLS slot.
// This is NOT XEX_HEADER_TLS_INFO initialization. Construct/destroy on the same
// bound XThread, before Exit; reserved slot must initially be zero.
class ThreadTls {
 OwnedModule::Lease lease_;
 Memory& memory_;
 uint8_t* base_;
 PPCContext& context_;
 Memory::Allocation block_;
 uint32_t slot_;
 std::atomic<unsigned>& errors_;
 bool allocated_=false,bound_=false;
 void cleanup()noexcept{
  if(bound_){context_.r3.u64=slot_;context_.r4.u64=0;__imp__KeTlsSetValue(context_,base_);if(context_.r3.u32!=1)++errors_;bound_=false;}
  if(allocated_){if(memory_.release(block_)!=Result::ok)++errors_;allocated_=false;}
 }
 public:
 ThreadTls(OwnedModule::Lease lease,Memory& memory,uint8_t* base,PPCContext& context,
           uint32_t slot,std::atomic<unsigned>& errors,uint32_t& observed_address,int fail_stage=0)
  :lease_(std::move(lease)),memory_(memory),base_(base),context_(context),slot_(slot),errors_(errors){
  if(rex::runtime::current_ppc_context()!=&context_)throw std::logic_error("unbound module TLS");
  try{
   block_=memory_.allocate(4096);allocated_=true;observed_address=block_.guest_address();
   if(fail_stage==1)throw std::runtime_error("injected TLS allocation rollback");
   const std::array<std::byte,4096> zero{};
   if(memory_.write(block_,0,zero)!=Result::ok)throw std::runtime_error("module TLS zero initialization");
   if(memory_.write(block_,0,lease_.tls_template())!=Result::ok)throw std::runtime_error("module TLS initialization");
   if(fail_stage==2)throw std::runtime_error("injected TLS template rollback");
   context_.r3.u64=slot_;context_.r4.u64=block_.guest_address();__imp__KeTlsSetValue(context_,base_);
   if(context_.r3.u32!=1)throw std::runtime_error("module TLS binding");bound_=true;
   if(fail_stage==3)throw std::runtime_error("injected TLS slot rollback");
  }catch(...){cleanup();throw;}
 }
 ThreadTls(const ThreadTls&)=delete;ThreadTls&operator=(const ThreadTls&)=delete;
 ~ThreadTls(){cleanup();}
 const Memory::Allocation& allocation()const{return block_;}
};
}
