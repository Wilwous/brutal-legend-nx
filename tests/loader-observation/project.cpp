// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Real pinned SDK loader observations only; no XThread construction or guest call.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>
#include <filesystem>
#include <cstdio>
static void pages(rex::memory::BaseHeap* h){
 std::printf("[");for(unsigned i=0;i<5;++i){rex::memory::HeapAllocationInfo info{};bool ok=h->QueryRegionInfo(0x82000000+i*65536,&info);
  std::printf("%s{\"query\":%s,\"base\":%u,\"state\":%u,\"protect\":%u,\"allocation_base\":%u,\"allocation_size\":%u,\"region_size\":%u}",i?",":"",ok?"true":"false",info.base_address,info.state,info.protect,info.allocation_base,info.allocation_size,info.region_size);}
 std::printf("]");
}
int main(int argc,char**argv){
 if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{std::filesystem::path root=argv[1],name=argv[2],work=argv[3];if(name!=name.filename())return 2;
  rex::Runtime runtime(root,work/"user",work/"updates",work/"cache",work/"metadata");
  if(runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
  auto&mem=*runtime.memory();auto*heap=mem.LookupHeap(0x82000000);auto*v=mem.LookupHeapByType(false,4096);auto before=heap->GetUnreservedPageCount(),vb=v->GetUnreservedPageCount();
  auto module=rex::system::object_ref<rex::system::UserModule>(new rex::system::UserModule(runtime.kernel_state()));
  auto status=module->LoadFromFile("game:\\"+name.string());
  std::printf("{\"load_status\":%u,\"pages_during\":",status);pages(heap);
  unsigned marker=0,content=0,tail=0;int mr=-1,cr=-1,tr=-1;bool active_refused=false;
  if(!status){auto lease=module->AcquireImageLease();if(!lease)return 4;
   mr=int(lease->CopyMemory(0x82020040,&marker,4,rex::memory::CheckedMemoryAccess::read));
   cr=int(lease->CopyMemory(0x82020000,&content,1,rex::memory::CheckedMemoryAccess::read));
   tr=int(lease->CopyMemory(0x82030000,&tail,1,rex::memory::CheckedMemoryAccess::read));
   active_refused=module->Unload()==0xc0000001u&&!module->AcquireImageLease();lease.reset();if(module->Unload())return 5;
  }
  if(!module->ReleaseHandle())return 5;module.reset();
  bool balanced=heap->GetUnreservedPageCount()==before&&v->GetUnreservedPageCount()==vb;
  std::printf(",\"marker_copy\":%d,\"marker\":%u,\"content_copy\":%d,\"content\":%u,\"tail_copy\":%d,\"tail\":%u,\"active_unload_refused\":%s,\"balanced\":%s,\"pages_after\":",mr,marker,cr,content,tr,tail,active_refused?"true":"false",balanced?"true":"false");pages(heap);
  std::printf(",\"guest_invocations\":0}\n");return balanced?0:6;
 }catch(const std::exception&e){std::fprintf(stderr,"probe error: %s\n",e.what());return 7;}
}
