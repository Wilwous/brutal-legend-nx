// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original guards around exact open-source SDK leaf functions extracted by run.py.
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <limits>
struct FakeMemory {
  uint32_t base=0x1000;std::vector<uint8_t> bytes=std::vector<uint8_t>(284);unsigned accesses=0;
  uint8_t* TranslateVirtual(uint32_t address){
    ++accesses;
    if(address<base || uint64_t(address)-base+4>bytes.size())throw std::out_of_range("SDK attempted out-of-range TLS memory access");
    return bytes.data()+address-base;
  }
};
namespace memory {
template<class T> T load_and_swap(uint8_t* p){T n=0;for(unsigned i=0;i<sizeof(T);++i)n=(n<<8)|p[i];return n;}
template<class T> void store_and_swap(uint8_t* p,T n){for(unsigned i=0;i<sizeof(T);++i)p[i]=uint8_t(n>>((sizeof(T)-i-1)*8));}
constexpr uint32_t kMemoryProtectRead=1,kMemoryProtectWrite=2,kMemoryProtectNoCache=4,kMemoryProtectWriteCombine=8;
}
#define THREAD_CLASS(Name) struct Name { \
 FakeMemory backing;uint32_t tls_static_address_=0x1000,tls_dynamic_address_=0x101c,tls_total_size_=284; \
 FakeMemory* memory(){return &backing;} bool GetTLSValue(uint32_t,uint32_t*);bool SetTLSValue(uint32_t,uint32_t); };
THREAD_CLASS(OriginalThread)
THREAD_CLASS(PatchedThread)
#include "original_tls.inc"
#include "patched_tls.inc"
using u32=uint32_t;using mapped_void=uintptr_t;using mapped_u32=uint32_t*;
constexpr uint32_t X_STATUS_SUCCESS=0,X_STATUS_NOT_IMPLEMENTED=0xc0000002;
unsigned warnings=0;
#define REXKRNL_WARN(...) (++warnings)
#include "original_stfs.inc"
#include "patched_stfs.inc"
constexpr uint32_t X_PAGE_NOACCESS=1,X_PAGE_READONLY=2,X_PAGE_READWRITE=4,X_PAGE_EXECUTE_READ=0x20,X_PAGE_EXECUTE_READWRITE=0x40,X_PAGE_NOCACHE=0x200,X_PAGE_WRITECOMBINE=0x400;
#include "protect_flags.inc"
void require(bool condition){if(!condition)throw std::runtime_error("SDK leaf regression assertion failed");}
int main(){try{
 OriginalThread old;PatchedThread fixed;uint32_t value=0;
 require(fixed.SetTLSValue(63,0x01234567));require(fixed.GetTLSValue(63,&value)&&value==0x01234567);
 auto previous=fixed.backing.accesses;
 for(auto slot:{64u,70u,71u,0x40000000u,UINT32_MAX}){
  require(!fixed.GetTLSValue(slot,&value));require(!fixed.SetTLSValue(slot,7));
 }
 require(fixed.backing.accesses==previous);require(!fixed.GetTLSValue(0,nullptr));
 bool baseline_oob=false;try{old.GetTLSValue(64,&value);}catch(const std::out_of_range&){baseline_oob=true;}
 require(baseline_oob);require(old.SetTLSValue(0x40000000u,99));require(old.GetTLSValue(0,&value)&&value==99);
 fixed.tls_dynamic_address_=fixed.tls_static_address_-1;require(!fixed.GetTLSValue(0,&value));
 fixed.tls_dynamic_address_=UINT32_MAX-1;fixed.tls_static_address_=UINT32_MAX-2;fixed.tls_total_size_=8;
 require(!fixed.GetTLSValue(0,&value));require(!fixed.SetTLSValue(0,1));
 fixed.tls_static_address_=0x1000;fixed.tls_dynamic_address_=0x101c;fixed.tls_total_size_=28;
 require(!fixed.GetTLSValue(0,&value));
 std::cout<<"{\"test\":\"sdk_tls_bounds_wrap_null_metadata\",\"result\":\"PASS\",\"baseline_oob_reproduced\":true}\n";
 uint32_t out=0x12345678;
 require(OriginalStfsCreateDevice_entry(0,0,&out)==0&&out==0x12345678);
 require(StfsCreateDevice_entry(0,0,&out)==X_STATUS_NOT_IMPLEMENTED&&out==0x12345678);
 require(StfsCreateDevice_entry(UINTPTR_MAX,0,reinterpret_cast<uint32_t*>(uintptr_t(1)))==X_STATUS_NOT_IMPLEMENTED);
 require(StfsControlDevice_entry(UINTPTR_MAX,0,UINTPTR_MAX,UINT32_MAX,UINTPTR_MAX,UINT32_MAX)==X_STATUS_NOT_IMPLEMENTED);
 require(warnings==4);
 std::cout<<"{\"test\":\"sdk_stfs_explicit_failure_no_pointer_access\",\"result\":\"PASS\"}\n";
 require(FromXdkProtectFlags(X_PAGE_NOACCESS)==0);
 require(FromXdkProtectFlags(X_PAGE_READONLY)==1);
 require(FromXdkProtectFlags(X_PAGE_READWRITE)==3);
 require(FromXdkProtectFlags(X_PAGE_EXECUTE_READ)==1);
 require(FromXdkProtectFlags(X_PAGE_EXECUTE_READWRITE)==3);
 require(FromXdkProtectFlags(X_PAGE_READWRITE|X_PAGE_NOCACHE|X_PAGE_WRITECOMBINE)==15);
 std::cout<<"{\"test\":\"sdk_protection_flag_mapping\",\"result\":\"PASS\"}\n";
 return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
