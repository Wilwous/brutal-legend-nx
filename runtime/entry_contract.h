// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "synthetic_xex_binding.h"
#include <rex/system/xthread.h>
namespace brutal::synthetic {
// Explicit host-supplied ORIGINAL fixture state. No Xbox title startup inference.
struct EntryState {uint64_t sp,lr,r2,r13,input;std::array<uint64_t,4> saved;};
inline void validate_entry_state(PPCContext&c,rex::memory::Memory&memory,
 rex::system::XThread&thread,uint32_t allocation_base,uint32_t low,uint32_t high,
 const EntryState&expected){
 auto deny=[](){throw std::invalid_argument("original startup contract refused");};
 if(rex::system::XThread::GetCurrentThread()!=&thread||rex::runtime::current_ppc_context()!=&c||
    c.r1.u64!=expected.sp||c.lr!=expected.lr||c.r2.u64!=expected.r2||c.r13.u64!=expected.r13||
    c.r13.u64!=thread.pcr_ptr()||c.r17.u64!=expected.input||
    c.r20.u64!=expected.saved[0]||c.r21.u64!=expected.saved[1]||c.r22.u64!=expected.saved[2]||c.r23.u64!=expected.saved[3]||
    low>=high||expected.sp>high||expected.sp<low||expected.sp-low<320||(expected.sp&15))deny();
 // Validate the full conservative frame/save window without modifying it.
 std::array<std::byte,320> bytes{};auto*heap=memory.LookupHeap(allocation_base);
 if(!heap||heap->CopyOwnedMemory(allocation_base,&thread,uint32_t(expected.sp-320),bytes.data(),bytes.size(),foundation::SdkCheckedMemory::Access::read)!=foundation::SdkCheckedMemory::Result::ok)deny();
 for(uint64_t at=expected.sp-320;at<expected.sp;at=std::min(expected.sp,(at/4096+1)*4096)){
  rex::memory::HeapAllocationInfo info{};
  if(!heap->QueryRegionInfo(uint32_t(at),&info)||!(info.state&rex::memory::kMemoryAllocationCommit)||!(info.protect&rex::memory::kMemoryProtectWrite))deny();
 }
}
}
