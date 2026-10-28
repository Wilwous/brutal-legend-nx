// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/fill.h"
#include <bit>
namespace rex::kernel::xboxkrnl {
#include "sdk-fill.inc"
}
namespace brutal::horizon {
void FillFrame::fill(uint32_t destination,uint32_t length,uint32_t pattern){
 static_assert(std::endian::native==std::endian::little);
 Scope::current(guest_.context).import_target(0x8203001c);image_.admit_import(0x126,XexImage::Signature::fill_u32_out);
 if(!destination||(destination&3)||!length||length>64||(length&3)||uint64_t(destination)+length>0x100000000ull)throw Error(Fault::bounds,"fill bounded aligned OUT contract");
 Scope::current(guest_.context).admit_range(destination,length);auto pin=guest_.space().pin(destination,length);auto output=guest_.space().view(destination,length,Access::write);
 if(admitted)admitted();
 alignas(8) std::array<uint32_t,16> scratch{};
 ++entered;++ImportFrame::sdk_entered;diagnostic::emit(diagnostic::Kind::import_enter,0x126);
 rex::kernel::xboxkrnl::RtlFillMemoryUlong_entry(mapped_void(scratch.data(),destination),length,pattern);
 ++returned;diagnostic::emit(diagnostic::Kind::import_return,0x126);
 guest_.space().write(output,std::as_bytes(std::span(scratch)).first(length));++committed;++ImportFrame::sdk_completed;diagnostic::emit(diagnostic::Kind::import_commit,0x126);
}
extern "C" void __imp__RtlFillMemoryUlong(PPCContext&ctx,uint8_t*base){if(base)throw Error(Fault::argument,"raw fill base");FillFrame::current(ctx).fill(ctx.r3.u32,ctx.r4.u32,ctx.r5.u32);}
}
