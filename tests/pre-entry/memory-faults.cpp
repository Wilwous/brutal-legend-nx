// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <rex/system/xmemory.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <cstdio>
#include <string>
REX_EXTERN(__imp__RtlCaptureContext);REX_EXTERN(__imp__RtlUnwind);REX_EXTERN(__imp____C_specific_handler);
int main(int argc,char** argv){
 if(argc!=2)return 2;std::string mode=argv[1];rex::InitLoggingEarly();
 if(mode=="capture"||mode=="unwind"||mode=="handler"){PPCContext c{};auto* f=mode=="capture"?__imp__RtlCaptureContext:mode=="unwind"?__imp__RtlUnwind:__imp____C_specific_handler;f(c,nullptr);return 86;}
 rex::memory::Memory m;if(!m.Initialize())return 3;
 if(mode=="hole"||mode=="physical-hole"){auto address=mode=="hole"?0x7f000000u:0xb0000000u;if(m.IsRangeAccessible(address,4,true))return 4;*m.TranslateVirtual<volatile uint32_t*>(address)=7;return 86;}
 const bool physical=mode.rfind("physical-",0)==0;
 const uint32_t a=physical?0xa1000000u:0x10000000u,n=physical?0x10000u:0x4000u;auto* heap=m.LookupHeap(a);
 if(!heap->AllocFixed(a,mode=="host-slack"?4096:n,mode=="host-slack"?4096:n,rex::memory::kMemoryAllocationReserve,rex::memory::kMemoryProtectNoAccess))return 5;
 if(mode!="reserved"){
 if(!heap->AllocFixed(a,mode=="host-slack"?4096:n,mode=="host-slack"?4096:n,rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite))return 6;
 }
 if((mode=="decommit"||mode=="physical-decommit")&&!heap->Decommit(a,n))return 7;
 if((mode=="release"||mode=="physical-release")&&!heap->Release(a))return 8;
 if(mode=="physical-readonly"&&!heap->Protect(a,n,rex::memory::kMemoryProtectRead))return 10;
 if(mode=="host-slack"){
 // On a 16 KiB host page, logical 4 KiB ownership cannot be enforced by mprotect.
 if(m.IsRangeAccessible(a+4096,4,true))return 9;
 *m.TranslateVirtual<volatile uint32_t*>(a+4096)=0x12345678;
 puts("{\"finding\":\"host_page_slack_accessible\",\"status\":\"BLOCKED\"}");return 79;
 }
 *m.TranslateVirtual<volatile uint32_t*>(a)=7;return 86;
}
