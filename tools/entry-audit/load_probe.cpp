// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Loader-only metadata probe. Never constructs XThread or resolves/calls PPCFunc.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>
#include <filesystem>
#include <cstdio>
int main(int argc,char**argv){
 if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  const std::filesystem::path root=argv[1],name=argv[2],work=argv[3];
  if(name.empty()||name!=name.filename()||!std::filesystem::is_regular_file(root/name))return 2;
  std::filesystem::create_directories(work);
  rex::Runtime runtime(root,work/"user",work/"updates",work/"cache",work/"metadata");
  if(runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
  auto&memory=*runtime.memory();auto*xheap=memory.LookupHeap(0x82000000);auto*vheap=memory.LookupHeapByType(false,4096);
  const auto xb=xheap->GetUnreservedPageCount(),vb=vheap->GetUnreservedPageCount();
  auto module=rex::system::object_ref<rex::system::UserModule>(new rex::system::UserModule(runtime.kernel_state()));
  const auto status=module->LoadFromFile("game:\\"+name.string());
  std::printf("{\"probe\":\"loader-only\",\"load_status\":%u,\"load_success\":%s,\"guest_functions_invoked\":0,\"threads_created\":0,\"sections\":[",status,status==0?"true":"false");
  if(status==0){auto*x=module->xex_module();bool comma=false;
   for(const auto&s:x->pe_sections()){std::printf("%s{\"address\":%u,\"size\":%u,\"flags\":%u}",comma?",":"",s.address,s.size,s.flags);comma=true;}
   if(module->Unload()!=0)return 5;
  }
  if(!module->ReleaseHandle())return 5;module.reset();
  const bool reclaimed=xheap->GetUnreservedPageCount()==xb&&vheap->GetUnreservedPageCount()==vb;
  std::printf("],\"image_virtual_pages_reclaimed\":%s}\n",reclaimed?"true":"false");
  if(!reclaimed)return 5;
  return status==0?0:4; // A real loader rejection is NEVER a successful load.
 }catch(const std::exception&e){std::fprintf(stderr,"probe failure: %s\n",e.what());return 6;}
}
