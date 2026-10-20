// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original host-side safety contracts, no XEX or game functions.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
REX_EXTERN(__imp__XamAlloc);REX_EXTERN(__imp__XamFree);
REX_EXTERN(__imp__RtlCaptureContext);REX_EXTERN(__imp__RtlUnwind);REX_EXTERN(__imp____C_specific_handler);
static unsigned checks=0;
static void require(bool v){++checks;if(!v)throw std::runtime_error("safety oracle mismatch");}
static uint32_t call(PPCFunc* f,PPCContext& c,uint8_t* b,std::initializer_list<uint32_t> args){size_t i=0;for(auto v:args)rex::ppc::ArgTranslator::SetIntegerArgumentValue(c,b,i++,v);f(c,b);return c.r3.u32;}
int main(int argc,char** argv){
 if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
 std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");rex::Runtime r(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");require(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto* m=r.memory();auto* base=m->virtual_membase();PPCContext c{};
 auto data=m->SystemHeapAlloc(4096);require(data);auto get=[&](){return rex::memory::load_and_swap<uint32_t>(base+data);};auto set=[&](uint32_t v){rex::memory::store_and_swap<uint32_t>(base+data,v);};
 bool foreign=false;try{call(__imp__XamFree,c,base,{data});}catch(const std::invalid_argument&){foreign=true;}require(foreign&&m->IsRangeAccessible(data,4096,true));
 require(call(__imp__XamAlloc,c,base,{0,33,data})==0);auto allocated=get();require(allocated&&!(allocated&31)&&m->IsRangeAccessible(allocated,33,true));for(unsigned i=0;i<33;++i)require(base[allocated+i]==0);require(call(__imp__XamFree,c,base,{allocated})==0&&!m->IsRangeAccessible(allocated,1,false));
 bool rejected=false;try{call(__imp__XamFree,c,base,{allocated});}catch(const std::invalid_argument&){rejected=true;}require(rejected);
 for(uint32_t size:{0x70000000u,0xffffffffu,0xfffff001u}){set(0x12345678);bool oom=false;try{call(__imp__XamAlloc,c,base,{0,size,data});}catch(const std::bad_alloc&){oom=true;}require(oom&&get()==0);}
 require(call(__imp__XamAlloc,c,base,{0,64,data})==0);auto live=get();bool interior=false;try{call(__imp__XamFree,c,base,{live+4});}catch(const std::invalid_argument&){interior=true;}require(interior&&m->IsRangeAccessible(live,64,true));require(call(__imp__XamFree,c,base,{live})==0);
 for(auto args:{std::initializer_list<uint32_t>{0,8,0},{0,8,0x7f000000},{0,8,data+1},{1,8,data},{0,0,data}}){set(0x12345678);bool error=false;try{call(__imp__XamAlloc,c,base,args);}catch(const std::invalid_argument&){error=true;}require(error&&get()==0x12345678);}
 auto readonly=m->SystemHeapAlloc(0x4000,0x4000);require(readonly);rex::memory::store_and_swap<uint32_t>(base+readonly,0x44556677);auto* readonly_heap=m->LookupHeap(readonly);require(readonly_heap->Protect(readonly,0x4000,rex::memory::kMemoryProtectRead));
 bool readonly_rejected=false;try{call(__imp__XamAlloc,c,base,{0,8,readonly});}catch(const std::invalid_argument&){readonly_rejected=true;}require(readonly_rejected&&rex::memory::load_and_swap<uint32_t>(base+readonly)==0x44556677);require(readonly_heap->Protect(readonly,0x4000,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite));m->SystemHeapFree(readonly);
 // A stale XAM pointer must not free a foreign allocation that reuses its address.
 require(call(__imp__XamAlloc,c,base,{0,32,data})==0);auto stale=get();auto* old_heap=m->LookupHeap(stale);uint32_t old_size=0;require(old_heap->QuerySize(stale,&old_size));require(old_heap->Release(stale));
 require(old_heap->AllocFixed(stale,old_size,4096,rex::memory::kMemoryAllocationReserve|rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite));
 bool stale_rejected=false;try{call(__imp__XamFree,c,base,{stale});}catch(const std::invalid_argument&){stale_rejected=true;}require(stale_rejected&&m->IsRangeAccessible(stale,old_size,true));require(old_heap->Release(stale));
 // A bounded fully occupied allocation range must report OOM in Debug too.
 auto* bounded=m->LookupHeap(0x18000000);uint32_t owned_range=0,failed_range=0x12345678;
 require(bounded->AllocRange(0x18000000,0x18003fff,0x4000,0x4000,rex::memory::kMemoryAllocationReserve|rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite,false,&owned_range));
 require(!bounded->AllocRange(0x18000000,0x18003fff,4096,4096,rex::memory::kMemoryAllocationReserve|rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite,false,&failed_range)&&failed_range==0);require(bounded->Release(owned_range));
 int other_owner=0;auto tagged=m->SystemHeapAlloc(32,32,rex::memory::kSystemHeapDefault,&other_owner);require(tagged);bool tagged_rejected=false;try{call(__imp__XamFree,c,base,{tagged});}catch(const std::invalid_argument&){tagged_rejected=true;}require(tagged_rejected&&m->IsRangeAccessible(tagged,32,true));require(m->SystemHeapFreeOwned(tagged,&other_owner));require(!m->SystemHeapFreeOwned(tagged,&other_owner));
 auto* heap=m->LookupHeap(0x10000000);constexpr uint32_t a=0x10000000,n=0x4000;
 uint32_t rejected_address=0x12345678;
 require(!heap->Alloc(0xffffffffu,4096,rex::memory::kMemoryAllocationCommit,3,false,&rejected_address)&&rejected_address==0);
 rejected_address=0x12345678;require(!heap->AllocSystemHeap(0xfffff001u,4096,rex::memory::kMemoryAllocationCommit,3,false,&rejected_address)&&rejected_address==0);
 rejected_address=0x12345678;require(!heap->AllocRange(a,a+n-1,0xffffffffu,4096,rex::memory::kMemoryAllocationCommit,3,false,&rejected_address)&&rejected_address==0);
 rejected_address=0x12345678;require(!heap->Alloc(16,0xffffffffu,rex::memory::kMemoryAllocationCommit,3,false,&rejected_address)&&rejected_address==0);
 require(!heap->AllocFixed(a,0xffffffffu,4096,rex::memory::kMemoryAllocationCommit,3));require(!heap->AllocFixed(0x40000000,4096,4096,rex::memory::kMemoryAllocationCommit,3));require(!heap->AllocFixed(0x3ffff000,8192,4096,rex::memory::kMemoryAllocationCommit,3));
 require(heap->AllocFixed(a,n,n,rex::memory::kMemoryAllocationReserve,rex::memory::kMemoryProtectNoAccess));require(!m->IsRangeAccessible(a,1,false));
 require(heap->AllocFixed(a,n,n,rex::memory::kMemoryAllocationCommit,rex::memory::kMemoryProtectRead|rex::memory::kMemoryProtectWrite));require(m->IsRangeAccessible(a,n,true));require(!m->IsRangeAccessible(a+n,1,true));require(!m->IsRangeAccessible(a+n-1,2,true));require(!m->IsRangeAccessible(0xfffffff0,32,false));require(!m->IsRangeAccessible(a,0,false));
 require(heap->Protect(a,n,rex::memory::kMemoryProtectRead));require(m->IsRangeAccessible(a,n,false)&&!m->IsRangeAccessible(a,n,true));require(heap->Decommit(a,n));require(!m->IsRangeAccessible(a,n,false));require(!heap->Decommit(a,0));require(!heap->Decommit(0x7ffff000,0x4000));require(heap->Release(a));require(!heap->Release(a));require(!heap->Release(0x40000000));rex::memory::HeapAllocationInfo info{};require(!heap->QueryRegionInfo(0x40000000,&info));
 for(auto* f:{__imp__RtlCaptureContext,__imp__RtlUnwind,__imp____C_specific_handler}){m->Fill(data+128,128,0x5d);bool stopped=false;try{call(f,c,base,{data+128});}catch(const std::runtime_error&){stopped=true;}require(stopped);for(unsigned i=0;i<128;++i)require(base[data+128+i]==0x5d);}
 m->SystemHeapFree(data);std::printf("{\"group\":\"pre_entry_contracts\",\"result\":\"PASS\",\"assertions\":%u}\n",checks);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s after %u checks\n",e.what(),checks);return 1;}
}
