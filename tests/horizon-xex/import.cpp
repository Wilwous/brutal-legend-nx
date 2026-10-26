// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/xex.h"
// Exact pinned open-source SDK body, extracted into private build output.
// No replacement implementation and no raw guest pointer supplied to the SDK.
namespace rex::kernel::xboxkrnl {
#include "sdk-compare.inc"
}
namespace brutal::horizon {
uint32_t ImportFrame::compare(uint32_t source,uint32_t size,uint32_t pattern){
 image_.admit_import(0x11b,XexImage::Signature::compare_u32_read_words);
 auto input=image_.compare_input(source,size); // one atomic checked span snapshot
 std::array<rex::be<uint32_t>,16> scratch{};
 for(size_t i=0;i<size/4;++i){uint32_t value=0;for(unsigned j=0;j<4;++j)value=(value<<8)|std::to_integer<unsigned>(input[i*4+j]);scratch[i]=value;}
 ++sdk_entered;
 auto result=rex::kernel::xboxkrnl::RtlCompareMemoryUlong_entry(mapped_void(scratch.data(),source),size,pattern);
 if(result>size/4)throw Error(Fault::unsupported,"SDK compare result outside admitted contract");
 ++calls;++sdk_completed;return result;
}
extern "C" void __imp__RtlCompareMemoryUlong(PPCContext& context,uint8_t* base){
 if(base)throw Error(Fault::argument,"raw SDK import base");
 auto& frame=ImportFrame::current(context);
 Scope::current(context).import_target(0x82030004);
 auto result=frame.compare(context.r3.u32,context.r4.u32,context.r5.u32);
 context.r3.u64=result; // only publish the return register after all checks succeed
}
}
