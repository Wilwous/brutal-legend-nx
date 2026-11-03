// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original metadata only. Calls actual SDK helper and PPC export bridge; no PPC body.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <rex/system/user_module.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <vector>
REX_EXTERN(__imp__RtlImageXexHeaderField);
static unsigned assertions=0;
static void check(bool ok){++assertions;if(!ok)throw std::runtime_error("header fixture assertion "+std::to_string(assertions));}
int main(int argc,char**argv){
 if(argc<3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
  rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");
  check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto& memory=*runtime.memory();auto* base=memory.virtual_membase();unsigned outstanding=0;
  for(int i=2;i<argc;++i){
   std::ifstream file(argv[i],std::ios::binary);std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
   check(bytes.size()>=24&&bytes.size()<=256);
   auto be=[&](unsigned o){return uint32_t(bytes[o])<<24|uint32_t(bytes[o+1])<<16|uint32_t(bytes[o+2])<<8|bytes[o+3];};
   // Bounds safety only; duplicate records intentionally measure SDK first-match policy.
   check(be(0)==0x58455832&&be(8)==bytes.size()&&be(20)<=(bytes.size()-24)/8);
   int owner=0;auto address=memory.SystemHeapAlloc(4096,32,0,&owner);check(address!=0);++outstanding;
   check(memory.IsRangeAccessible(address,bytes.size(),true));std::memcpy(base+address,bytes.data(),bytes.size());
   uint32_t helper=0xdeadbeef;
   auto status=rex::system::UserModule::GetOptHeader(&memory,reinterpret_cast<const rex::xex2_header*>(base+address),rex::xex2_header_keys(0x20401),&helper);
   PPCContext context{};context.r3.u64=address;context.r4.u64=0x20401;
   __imp__RtlImageXexHeaderField(context,base);
   check(std::memcmp(base+address,bytes.data(),bytes.size())==0);
   std::printf("{\"case\":\"%s\",\"base\":%u,\"result\":%u,\"helper\":%u,\"status\":%u}\n",std::filesystem::path(argv[i]).stem().string().c_str(),address,context.r3.u32,helper,uint32_t(status));
   check(memory.SystemHeapFreeOwned(address,&owner));--outstanding;
  }
  check(outstanding==0);std::printf("{\"assertions\":%u,\"outstanding_fixture_allocations\":%u,\"PPC_bodies\":0}\n",assertions,outstanding);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
