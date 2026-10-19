// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Uses only the prebuilt SDK memory subsystem; never loads an XEX or a runtime.
#include <rex/memory.h>
#include <rex/logging.h>
#include <iostream>
#include <stdexcept>
#include <string_view>
void require(bool v){if(!v)throw std::runtime_error("SDK memory probe assertion failed");}
int main(int argc,char** argv){try{
 rex::InitLoggingEarly();rex::memory::Memory m;require(m.Initialize());
 auto a=m.SystemHeapAlloc(100,4096);require(a&&a%4096==0);
 auto p=m.TranslateVirtual(a);require(p&&m.HostToGuestVirtual(p)==a);
 rex::memory::store_and_swap<uint32_t>(p,0x01234567);
 require(p[0]==1&&p[1]==0x23&&p[2]==0x45&&p[3]==0x67);
 require(rex::memory::load_and_swap<uint32_t>(p)==0x01234567);
 auto heap=m.LookupHeap(a);require(heap);
 uint32_t size=0,protection=0;require(heap->QuerySize(a,&size)&&size>=100);
 require(heap->QueryProtect(a,&protection));require(protection&rex::memory::kMemoryProtectWrite);
 require(heap->Protect(a,size,rex::memory::kMemoryProtectRead));
 require(heap->QueryProtect(a,&protection)&&protection==rex::memory::kMemoryProtectRead);
 require(rex::memory::load_and_swap<uint32_t>(p)==0x01234567);
 if(argc==2 && std::string_view(argv[1])=="--fault-readonly"){volatile uint8_t* denied=p;*denied=0xff;return 86;}
 require(heap->Protect(a,size,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite));
 m.SystemHeapFree(a);
 auto vheap=m.LookupHeapByType(false,4096);require(vheap);uint32_t b=0;
 require(vheap->Alloc(4096,4096,rex::memory::kMemoryAllocationReserve,0,false,&b));
 require(vheap->AllocFixed(b,4096,4096,rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite));
 auto q=m.TranslateVirtual(b);q[0]=0x4a;require(q[0]==0x4a);
 require(vheap->Decommit(b,4096));require(vheap->Release(b));
 std::cout<<"{\"test\":\"prebuilt_sdk_memory_allocate_translate_endian_protect_reserve_commit_decommit_release\",\"result\":\"PASS\"}\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
