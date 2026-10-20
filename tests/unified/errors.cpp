// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original initialization-contract probes and explicit unsafe-behavior observations.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/ppc.h>
#include <rex/logging.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
REX_EXTERN(__imp__NtAllocateVirtualMemory);
REX_EXTERN(__imp__NtProtectVirtualMemory);
REX_EXTERN(__imp__NtFreeVirtualMemory);
REX_EXTERN(__imp__XamAlloc);
REX_EXTERN(__imp__RtlCaptureContext);
REX_EXTERN(__imp__RtlUnwind);
REX_EXTERN(__imp____C_specific_handler);
namespace rex::kernel::xboxkrnl {uint32_t FromXdkProtectFlags(uint32_t);uint32_t ToXdkProtectFlags(uint32_t);}
static uint32_t invoke(PPCFunc* f,PPCContext& c,uint8_t* b,std::initializer_list<uint32_t> a){size_t i=0;for(auto v:a)rex::ppc::ArgTranslator::SetIntegerArgumentValue(c,b,i++,v);f(c,b);return c.r3.u32;}
int main(int argc,char** argv){
 if(argc!=2&&argc!=3)return 2;bool hardened=argc==3;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
 rex::Runtime r(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");if(r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
 auto* m=r.memory();auto* base=m->virtual_membase();auto data=m->SystemHeapAlloc(4096);if(!data)return 4;PPCContext c{};int passed=0;
 auto put=[&](uint32_t o,uint32_t v){rex::memory::store_and_swap<uint32_t>(base+data+o,v);};auto get=[&](uint32_t o){return rex::memory::load_and_swap<uint32_t>(base+data+o);};
 auto check=[&](const char* name,bool ok){std::printf("{\"group\":\"%s\",\"result\":\"%s\"}\n",name,ok?"PASS":"FAIL");if(ok)++passed;};
 using namespace rex::kernel::xboxkrnl;
 check("real_SDK_protection_flag_mapping",FromXdkProtectFlags(1)==0&&FromXdkProtectFlags(2)==1&&FromXdkProtectFlags(4)==3&&FromXdkProtectFlags(0x204)==7&&FromXdkProtectFlags(0x404)==11&&ToXdkProtectFlags(0)==1&&ToXdkProtectFlags(1)==2&&ToXdkProtectFlags(3)==4);
 check("allocation_null_metadata_errors",invoke(__imp__NtAllocateVirtualMemory,c,base,{0,0,0x3000,4,0})==0xc000000d);
 put(0,0);put(4,0x7fff0000);check("allocation_exhaustion_failure_outputs",invoke(__imp__NtAllocateVirtualMemory,c,base,{data,data+4,0x3000,4,0})==0xc0000017&&get(0)==0&&get(4)==0x7fff0000);
 put(0,0x7f000000);put(4,4096);put(8,0x1234);check("invalid_heap_protect_rejected",invoke(__imp__NtProtectVirtualMemory,c,base,{data,data+4,4,data+8,0})==0xc000000d&&get(8)==0x1234);
 check("invalid_heap_free_rejected",invoke(__imp__NtFreeVirtualMemory,c,base,{data,data+4,0x8000,0})==0xc000000d);
 put(16,0xdeadbeef);bool oom=false;uint32_t status=0;try{status=invoke(__imp__XamAlloc,c,base,{0,0x70000000,data+16});}catch(const std::bad_alloc&){oom=true;}
 if(hardened){check("XamAlloc_OOM_fails_closed",oom&&get(16)==0);}else {
 if(status==0&&get(16)==0)puts("{\"finding\":\"XamAlloc_OOM_false_success\",\"status\":\"BLOCKED\",\"returned_status\":0,\"output\":0}");else return 5;}
 struct Stub {const char* name;PPCFunc* function;} stubs[]{{"RtlCaptureContext",__imp__RtlCaptureContext},{"RtlUnwind",__imp__RtlUnwind},{"__C_specific_handler",__imp____C_specific_handler}};
 for(auto stub:stubs){m->Fill(data+128,256,0x6d);bool stopped=false;try{invoke(stub.function,c,base,{data+128,0,0,0});}catch(const std::runtime_error&){stopped=true;}bool unchanged=true;for(int i=0;i<256;++i)unchanged&=base[data+128+i]==0x6d;if(!unchanged)return 6;if(hardened){check(stub.name,stopped);continue;}if(stopped)return 7;std::printf("{\"finding\":\"%s_unsupported_return\",\"status\":\"BLOCKED\"}\n",stub.name);}
 m->SystemHeapFree(data);std::printf("{\"passed\":%d,\"failed\":%d,\"blocked_findings\":%d}\n",passed,(hardened?9:5)-passed,hardened?0:4);return passed==(hardened?9:5)?0:1;
}
