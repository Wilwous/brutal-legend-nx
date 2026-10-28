// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/query.h"
// These declarations/constants come verbatim from the fingerprinted SDK.
#include "sdk-query-types.inc"
namespace brutal::horizon {
namespace {
uint32_t flags(Permission p){switch(p){case Permission::none:return rex::memory::kMemoryProtectNoAccess;case Permission::read:return rex::memory::kMemoryProtectRead;case Permission::read_write:return rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite;default:throw Error(Fault::unsupported,"query cannot represent write-only protection");}}
class QueryBackend {
 public:
 QueryBackend* LookupHeap(uint32_t address){return QueryFrame::snapshot(address)?this:nullptr;}
 bool QueryRegionInfo(uint32_t address,rex::memory::HeapAllocationInfo* out){
  auto*info=QueryFrame::snapshot(address);if(!info||!out)return false;
  *out={info->base,info->allocation_base,flags(info->initial),info->allocation_size,info->region_size,
        rex::memory::kMemoryAllocationReserve|(info->committed?rex::memory::kMemoryAllocationCommit:0u),flags(info->current)};
  return true;
 }
};
thread_local QueryBackend backend;
}
QueryBackend* query_backend(){return &backend;}
}
#define REX_KERNEL_MEMORY() (::brutal::horizon::query_backend())
#pragma push_macro("assert_not_zero")
#undef assert_not_zero
#define assert_not_zero(value) do { if(!(value))throw brutal::horizon::Error(brutal::horizon::Fault::unsupported,"SDK query commit without reserve"); } while(false)
namespace rex::kernel::xboxkrnl {
#include "sdk-query.inc"
}
#undef assert_not_zero
#pragma pop_macro("assert_not_zero")
#undef REX_KERNEL_MEMORY
namespace brutal::horizon {
uint32_t QueryFrame::query(uint32_t address,uint32_t output,uint32_t kind){
 Scope::current(guest_.context).import_target(0x82030014);image_.admit_import(0xee,XexImage::Signature::query_basic28);
 if(kind!=0&&kind!=UINT32_MAX)throw Error(Fault::unsupported,"only basic selector zero and explicit invalid-selector refusal tested");
 if((address&4095)||(output&3)||!output)throw Error(Fault::bounds,"query address/output alignment");
 auto&scope=Scope::current(guest_.context);scope.admit_range(address,1);scope.admit_range(output,28);
 auto code=image_.section(1);if(address>=code.address&&uint64_t(address)<uint64_t(code.address)+code.size)throw Error(Fault::unsupported,"query executable protection is not represented by Space");
 auto query_pin=guest_.space().pin(address,1),output_pin=guest_.space().pin(output,28);
 auto info=guest_.space().query(address);(void)flags(info.initial);(void)flags(info.current);
 auto view=guest_.space().view(output,28,Access::write);
 if(snapshot_){throw Error(Fault::unsupported,"recursive query binding");}snapshot_=info;
 struct Clear {std::optional<Space::Query>&info;~Clear(){info.reset();}} clear{snapshot_};
 if(admitted)admitted();
 alignas(8) rex::kernel::xboxkrnl::X_MEMORY_BASIC_INFORMATION scratch{};static_assert(sizeof(scratch)==28);
 ++entered;++ImportFrame::sdk_entered;diagnostic::emit(diagnostic::Kind::import_enter,0xee);
 using rex::X_STATUS;
 auto status=rex::kernel::xboxkrnl::NtQueryVirtualMemory_entry(address,ppc_ptr_t<rex::kernel::xboxkrnl::X_MEMORY_BASIC_INFORMATION>(&scratch,output),kind);++returned;diagnostic::emit(diagnostic::Kind::import_return,0xee,status);
 if(status==X_STATUS_SUCCESS){guest_.space().write(view,std::as_bytes(std::span(&scratch,1)));++committed;diagnostic::emit(diagnostic::Kind::import_commit,0xee);}
 else if(status!=X_STATUS_INVALID_PARAMETER)throw Error(Fault::unsupported,"unverified SDK query status");
 ++ImportFrame::sdk_completed;return status;
}
extern "C" void __imp__NtQueryVirtualMemory(PPCContext& context,uint8_t*base){if(base)throw Error(Fault::argument,"raw query base");auto&frame=QueryFrame::current(context);auto result=frame.query(context.r3.u32,context.r4.u32,context.r5.u32);context.r3.u64=result;}
}
