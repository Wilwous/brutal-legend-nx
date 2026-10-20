// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Optional private loader-only compatibility probe. Never prepares/resumes a guest thread.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/system/user_module.h>
#include <rex/system/xex_module.h>
#include <cstdio>
#include <filesystem>
#include <dlfcn.h>
int main(int argc,char** argv){
 if(argc!=4)return 2;
 Dl_info owner{};if(!dladdr(reinterpret_cast<void*>(&rex::kernel::InitializeKernel),&owner)||std::filesystem::weakly_canonical(owner.dli_fname).parent_path()!=std::filesystem::weakly_canonical(argv[3])/"lib")return 7;rex::InitLoggingEarly();std::filesystem::path work=argv[2];
 rex::Runtime r(argv[1],work/"user",work/"empty-updates",work/"cache",work/"metadata");
 if(r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
 if(r.LoadXexImage("game:\\default.xex"))return 4;
 auto module=r.kernel_state()->GetExecutableModule();if(!module||!module->xex_module())return 5;
 auto* xex=module->xex_module();unsigned imports=0,unresolved=0;

 for(auto& library:*xex->import_libraries())for(auto& imported:library.imports){++imports;if(!r.export_resolver()->GetExportByOrdinal(library.name,imported.ordinal))++unresolved;}
#if defined(REX_PRE_ENTRY)
 auto db=r.kernel_state()->title_xdbf();bool valid=!unresolved&&db.is_valid();
 std::printf("{\"group\":\"private_loader_only\",\"result\":\"%s\",\"imports\":%u,\"unresolved\":%u,\"image_size\":%u,\"entry_prepared\":false,\"guest_started\":false,\"xdbf_valid\":%s,\"xdbf_status\":%u}\n",valid?"PASS":"FAIL",imports,unresolved,xex->image_size(),db.is_valid()?"true":"false",unsigned(db.status()));
 return valid?0:8;
#else
 std::printf("{\"group\":\"private_loader_only\",\"result\":\"%s\",\"imports\":%u,\"unresolved\":%u,\"image_size\":%u,\"entry_prepared\":false,\"guest_started\":false}\n",unresolved?"FAIL":"PASS",imports,unresolved,xex->image_size());
 return unresolved?6:0;
#endif
}
