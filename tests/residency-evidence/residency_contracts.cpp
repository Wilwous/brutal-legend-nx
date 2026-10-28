// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original residency/owner fixtures. No game input, PPC linkage or dispatch.
#include "tests/image-runtime/xex-fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <array>
static unsigned checks=0;
static void check(bool ok){++checks;if(!ok)throw std::runtime_error("residency contract "+std::to_string(checks));}
int main(int argc,char**argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1];std::string mode=argv[2];std::filesystem::create_directories(work/"input");auto bytes=image_fixture::xex();
  constexpr uint32_t base=0x82000000,size=0x30000;constexpr size_t table=0x1000+0x98+224;bool accepted=true,neighbor=false,zero=false;
  auto be=[&](size_t at,uint32_t value){image_fixture::put(bytes,at,value,4,true);};auto le=[&](size_t at,uint32_t value){image_fixture::put(bytes,at,value,4,false);};
  if(mode=="ordinary"){}
  else if(mode=="basic-zero"){be(0x300,16);image_fixture::put(bytes,0x306,1,2,true);be(0x308,0x20000);be(0x30c,0x10000);bytes.resize(0x21000);zero=true;}
  else if(mode=="missing-pages"){bytes.resize(bytes.size()-65536);accepted=false;}
  else if(mode=="descriptor-zero"){be(0x284,3);accepted=false;}
  else if(mode=="descriptor-kind"){be(0x284,0x1f);accepted=false;}
  else if(mode=="descriptor-kind-zero"){be(0x284,0x10);accepted=false;}
  else if(mode=="descriptor-kind-four"){be(0x284,0x14);accepted=false;}
  else if(mode=="descriptor-wrap"){be(0x284,0xfffffff1);accepted=false;}
  else if(mode=="misaligned-base"){be(36,base+1);be(0x210,base+1);accepted=false;}
  else if(mode=="permissions"){le(table+80+36,0);accepted=false;}
  else if(mode=="overlap"){le(table+80+12,65536);accepted=false;}
  else if(mode=="foreign-overrun"){le(table+80+8,0x20000);neighbor=true;accepted=false;}
  else if(mode=="ordinary-neighbor"){neighbor=true;}
  else if(mode=="tail-one-byte"){le(table+80+8,0x10001);accepted=false;}
  else if(mode=="last-byte"){le(table+80+8,0xffff);}
  else if(mode=="basic-overflow"||mode=="basic-overrun"||mode=="basic-truncated"){
   be(0x300,16);image_fixture::put(bytes,0x306,1,2,true);be(0x308,mode=="basic-overflow"?0xffffffff:0x20000);be(0x30c,mode=="basic-overrun"?0x10001:0x10000);bytes.resize(mode=="basic-truncated"?0x20fff:0x21000);accepted=false;
  }
  else if(mode=="short-descriptor"){be(0x284+48,3);accepted=false;}
  else if(mode=="decommitted"||mode=="noaccess"||mode=="repeat"||mode=="active"){}
  else return 2;
  {std::ofstream f(work/"input/original.xex",std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f));}
  rex::Runtime runtime(work/"input",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto&memory=*runtime.memory();auto*heap=memory.LookupHeap(base);auto*virtual_heap=memory.LookupHeapByType(false,4096);auto before=heap->GetUnreservedPageCount(),vb=virtual_heap->GetUnreservedPageCount();int token=0;
  if(neighbor){check(heap->AllocFixedOwned(base+size,65536,&token));memory.Fill(base+size,65536,0x6d);}
  const unsigned repetitions=mode=="repeat"?8:1;
  for(unsigned i=0;i<repetitions;++i){
   auto module=rex::system::object_ref<rex::system::UserModule>(new rex::system::UserModule(runtime.kernel_state()));auto status=module->LoadFromFile("game:\\original.xex");check(status==(accepted?0u:0xc0000001u));
   if(accepted){auto lease=module->AcquireImageLease();check(bool(lease));check(lease->image_base()==base&&lease->image_size()==size);using Result=rex::memory::CheckedMemoryResult;using Access=rex::memory::CheckedMemoryAccess;uint8_t value=0;
    check(lease->CopyMemory(base+size,&value,1,Access::read)==Result::invalid_range);
    check(lease->CopyMemory(base+65536,&value,1,Access::write)==Result::protection);
    check(lease->CopyMemory(base+131072,&value,1,Access::read)==Result::ok&&value==(zero?0:0xa5));
    std::array<uint8_t,2> sentinel{0x77,0x77};
    check(lease->CopyMemory(base+0x1ffff,sentinel.data(),2,Access::read)==Result::invalid_range);
    check(sentinel==std::array<uint8_t,2>{0x77,0x77});
    if(mode=="last-byte"){
     check(lease->CopyMemory(base+size-2,&value,1,Access::read)==Result::ok);
     check(lease->CopyMemory(base+size-1,&value,1,Access::read)==Result::invalid_range);
    }
    if(zero){value=0xff;check(lease->CopyMemory(base+size-1,&value,1,Access::read)==Result::ok&&value==0);}
    if(mode=="decommitted"){
     check(heap->Decommit(base+131072,65536));value=0x77;
     check(lease->CopyMemory(base+131072,&value,1,Access::read)==Result::uncommitted&&value==0x77);
    }
    if(mode=="noaccess"){
     check(heap->Protect(base+131072,65536,rex::memory::kMemoryProtectNoAccess));value=0x77;
     check(lease->CopyMemory(base+131072,&value,1,Access::read)==Result::protection&&value==0x77);
    }
    check(module->Unload()==0xc0000001u);check(!module->AcquireImageLease());lease.reset();check(module->Unload()==0);
   }else check(!module->AcquireImageLease()&&module->guest_xex_header()==0&&module->processor_module()==nullptr);
   check(module->ReleaseHandle());module.reset();check(heap->GetUnreservedPageCount()==before-(neighbor?1:0)&&virtual_heap->GetUnreservedPageCount()==vb);
  }
  if(neighbor){check(memory.TranslateVirtual<uint8_t*>(base+size)[0]==0x6d);check(heap->ReleaseOwned(base+size,&token));}
  check(heap->GetUnreservedPageCount()==before&&virtual_heap->GetUnreservedPageCount()==vb);
  std::printf("{\"group\":\"residency_%s\",\"result\":\"PASS\",\"checks\":%u,\"accepted\":%s,\"repetitions\":%u,\"balanced\":true,\"guest_functions_invoked\":0}\n",mode.c_str(),checks,accepted?"true":"false",repetitions);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
