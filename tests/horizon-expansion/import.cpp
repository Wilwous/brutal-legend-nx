// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/xex.h"
#define REXKRNL_IMPORT_TRACE(name,format,address) brutal::horizon::ImportFrame::trace(address)
// The isolated, fingerprinted SDK helper has an unused cs_ptr parameter.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
namespace rex::kernel::xboxkrnl {
#include "sdk-initialize.inc"
}
#pragma GCC diagnostic pop
#undef REXKRNL_IMPORT_TRACE
namespace brutal::horizon {
void ImportFrame::initialize(uint32_t address){
 Scope::current(guest_.context).import_target(0x8203000c);
 image_.admit_import(0x12e,XexImage::Signature::initialize_cs_inout);
 auto handle=image_.initialize_capability(address);Space::Lease pin(guest_.space(),handle);
 auto input=guest_.space().view(address,28,Access::read),output=guest_.space().view(address,28,Access::write);
 if(admitted)admitted(); // Outside the Space lock; module/range leases remain live.
 guest_.space().update(input,output,[&](std::span<std::byte> bytes){
  alignas(8) rex::kernel::xboxkrnl::X_RTL_CRITICAL_SECTION scratch;
  static_assert(sizeof(scratch)==28);std::memcpy(&scratch,bytes.data(),sizeof(scratch));
  ++sdk_entered;diagnostic::emit(diagnostic::Kind::import_enter,0x12e);
  rex::kernel::xboxkrnl::RtlInitializeCriticalSection_entry(ppc_ptr_t<rex::kernel::xboxkrnl::X_RTL_CRITICAL_SECTION>(&scratch,address));
  diagnostic::emit(diagnostic::Kind::import_return,0x12e);
  std::memcpy(bytes.data(),&scratch,sizeof(scratch));
 });
 diagnostic::emit(diagnostic::Kind::import_commit,0x12e);
 ++calls;++initialized;++sdk_completed; // Only count a completed, committed SDK call.
}
extern "C" void __imp__RtlInitializeCriticalSection(PPCContext& context,uint8_t* base){
 if(base)throw Error(Fault::argument,"raw initializer base");
 auto& frame=ImportFrame::current(context);frame.initialize(context.r3.u32);
 // Verified SDK signature is void. No invented success status in r3.
}
}
