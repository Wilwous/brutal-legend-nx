// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original loader-only cases. No PPC function is linked or invoked.
#include "tests/image-runtime/xex-fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/xex_module.h>
#include <rex/system/user_module.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdio>
#include <stdexcept>
using Module=rex::system::UserModule;
static unsigned assertions=0;
static void check(bool ok){++assertions;if(!ok)throw std::runtime_error("loader contract "+std::to_string(assertions));}
int main(int argc,char**argv){
 if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1];std::string mode=argv[2];std::filesystem::create_directories(work/"input");
  auto bytes=image_fixture::xex();constexpr uint32_t base=0x82000000,owner_size=0x30000;
  constexpr size_t opt=0x1000+0x98,section=opt+224;
  auto le=[&](size_t at,uint32_t value){image_fixture::put(bytes,at,value,4,false);};
  auto be=[&](size_t at,uint32_t value){image_fixture::put(bytes,at,value,4,true);};
  bool accept=true,foreign=false;
  if(mode=="within"){}
  else if(mode=="unaligned-section"){le(section+40+12,0x10200);le(section+40+8,0x100);le(section+40+16,0x200);}
  else if(mode=="overstated-pe"){le(opt+56,0x90000);}
  else if(mode=="zero-fill"){le(section+80+16,0);le(section+80+20,0);le(section+80+36,0xc0000080);std::fill(bytes.begin()+0x21000,bytes.end(),0);}
  else if(mode=="hole"){le(section+80+12,0x20200);le(section+80+8,0xfe00);}
  else if(mode=="short-payload"){bytes.resize(bytes.size()-512);accept=false;}
  else if(mode=="extra-payload"){bytes.resize(bytes.size()+512);accept=false;}
  else if(mode=="beyond-owner"){le(section+80+8,0x10001);accept=false;}
  else if(mode=="discardable-beyond"){le(section+80+8,0x10001);le(section+80+36,0x42000040);accept=false;}
  else if(mode=="overlap"){le(section+40+12,0xff00);accept=false;}
  else if(mode=="wrapping-rva"){le(section+40+12,0xffff0000);accept=false;}
  else if(mode=="wrapping-size"){le(section+40+8,0xffffffff);accept=false;}
  else if(mode=="missing-read"){le(section+80+36,0x80000040);accept=false;}
  else if(mode=="missing-flags"){le(section+80+36,0);accept=false;}
  else if(mode=="allocation-collision"){foreign=true;accept=false;}
  else if(mode=="partial-rollback"){le(opt-24,0);accept=false;}
  else if(mode=="unbacked-extension"){be(0x104,owner_size+0x10000);be(0x100+0x184+48,0x22);accept=false;}
  else if(mode=="active-lease"){}
  else return 2;
  {std::ofstream f(work/"input/original.xex",std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f));}
  rex::Runtime runtime(work/"input",work/"user",work/"updates",work/"cache",work/"metadata");
  check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto&memory=*runtime.memory();auto*heap=memory.LookupHeap(base);auto*virtual_heap=memory.LookupHeapByType(false,4096);
  auto before=heap->GetUnreservedPageCount(),virtual_before=virtual_heap->GetUnreservedPageCount();int token=0;
  if(foreign){check(heap->AllocFixedOwned(base,owner_size,&token));memory.Fill(base,owner_size,0x6d);}
  auto module=rex::system::object_ref<Module>(new Module(runtime.kernel_state()));
  const auto status=module->LoadFromFile("game:\\original.xex");
  check((status==0)==accept);
  if(accept){
   auto lease=module->AcquireImageLease();check(bool(lease));check(lease->image_base()==base&&lease->image_size()==owner_size);
   uint8_t value=0;using Access=rex::memory::CheckedMemoryAccess;using Result=rex::memory::CheckedMemoryResult;
   check(lease->CopyMemory(base+0x20000+(mode=="hole"?0x200:0),&value,1,Access::write)==Result::ok);
   const uint32_t ro=base+(mode=="unaligned-section"?0x10200:0x10000);
   value=0x9c;check(lease->CopyMemory(ro,&value,1,Access::write)==Result::protection);
   check(lease->CopyMemory(base+owner_size,&value,1,Access::read)==Result::invalid_range);
   check(!memory.IsRangeAccessible(base+0x10000,1,true)&&memory.IsRangeAccessible(base+0x20000,1,true));
   if(mode=="hole")check(lease->CopyMemory(base+0x20000,&value,1,Access::read)==Result::invalid_range);
   if(mode=="zero-fill"){value=0xff;check(lease->CopyMemory(base+0x20020,&value,1,Access::read)==Result::ok&&value==0);}
   check(module->Unload()==0xc0000001u);lease.reset();check(module->Unload()==0);
  }else{
   check(module->processor_module()==nullptr&&module->guest_xex_header()==0);
   if(foreign){check(memory.TranslateVirtual<uint8_t*>(base)[0]==0x6d);check(heap->ReleaseOwned(base,&token));}
  }
  check(module->ReleaseHandle());module.reset();
  check(heap->GetUnreservedPageCount()==before&&virtual_heap->GetUnreservedPageCount()==virtual_before);
  std::printf("{\"group\":\"loader_%s\",\"result\":\"PASS\",\"assertions\":%u,\"accepted\":%s,\"image_virtual_pages_reclaimed\":true,\"guest_functions_invoked\":0}\n",mode.c_str(),assertions,accept?"true":"false");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
