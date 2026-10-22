// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Selected SAFE real exports. All pointer inputs refer to original owned bytes.
#include "xex-fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
REX_EXTERN(__imp__ExGetXConfigSetting);REX_EXTERN(__imp__RtlCompareMemoryUlong);
REX_EXTERN(__imp__RtlImageXexHeaderField);REX_EXTERN(__imp__RtlInitAnsiString);
REX_EXTERN(__imp__RtlInitializeCriticalSection);REX_EXTERN(__imp__RtlInitializeCriticalSectionAndSpinCount);
REX_EXTERN(__imp__RtlNtStatusToDosError);REX_EXTERN(__imp__XGetAVPack);REX_EXTERN(__imp__XGetLanguage);
REX_EXTERN(__imp__KeQueryPerformanceFrequency);
static unsigned checks=0;static void check(bool v){++checks;if(!v)throw std::runtime_error("import oracle "+std::to_string(checks));}
static uint32_t call(PPCFunc* fn,PPCContext& c,uint8_t*b,std::initializer_list<uint32_t>args){unsigned i=0;for(auto v:args)rex::ppc::ArgTranslator::SetIntegerArgumentValue(c,b,i++,v);fn(c,b);return c.r3.u32;}
int main(int argc,char**argv){if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 std::filesystem::path p=argv[1];std::filesystem::create_directories(p/"empty");rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");
 check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&m=*r.memory();auto*b=m.virtual_membase();PPCContext c{};
 auto a=m.SystemHeapAlloc(8192);check(a!=0);auto get=[&](unsigned o){return rex::memory::load_and_swap<uint32_t>(b+a+o);};
 auto word=[&](unsigned o,uint32_t v){rex::memory::store_and_swap<uint32_t>(b+a+o,v);};
 auto shortword=[&](unsigned o){return rex::memory::load_and_swap<uint16_t>(b+a+o);};
 m.Fill(a,8192,0x55);
 check(call(__imp__ExGetXConfigSetting,c,b,{0xffff,0,a,4,a+16})==0xc00000efu);check(shortword(16)==0&&get(0)==0x55555555);
 check(call(__imp__ExGetXConfigSetting,c,b,{2,0xffff,a,4,a+16})==0xc00000f0u);check(shortword(16)==0&&get(0)==0x55555555);
 check(call(__imp__ExGetXConfigSetting,c,b,{2,2,a,3,a+16})==0xc0000023u);check(shortword(16)==0&&get(0)==0x55555555);
 check(call(__imp__ExGetXConfigSetting,c,b,{2,2,a,4,a+16})==0);check(shortword(16)==4&&get(0)==0x1000);
 for(unsigned i=0;i<4;++i)word(4*i,i==2?0xdeadbeef:0x12345678);
 check(call(__imp__RtlCompareMemoryUlong,c,b,{a,16,0x12345678})==3);
 check(call(__imp__RtlCompareMemoryUlong,c,b,{a+1,16,0x12345678})==0);
 check(call(__imp__RtlCompareMemoryUlong,c,b,{a,15,0x12345678})==0);
 auto bytes=image_fixture::xex();std::memcpy(b+a,bytes.data(),4096);
 check(call(__imp__RtlImageXexHeaderField,c,b,{a,0x10100})==0x82001000);
 check(call(__imp__RtlImageXexHeaderField,c,b,{a,0xfefe0000})==0);
 const char original[]="owned";std::memcpy(b+a+128,original,sizeof(original));call(__imp__RtlInitAnsiString,c,b,{a+64,a+128});
 check(shortword(64)==5&&shortword(66)==6&&get(68)==a+128);
 m.Fill(a+64,32,0x55);call(__imp__RtlInitializeCriticalSection,c,b,{a+64});
 check(b[a+64]==1&&get(80)==0xffffffff&&get(84)==0&&get(88)==0);
 check(call(__imp__RtlInitializeCriticalSectionAndSpinCount,c,b,{a+64,257})==0);check(b[a+65]==2);
 check(call(__imp__RtlNtStatusToDosError,c,b,{0})==0);check(call(__imp__RtlNtStatusToDosError,c,b,{0xc0000008})==6);
 check(call(__imp__RtlNtStatusToDosError,c,b,{0xdeadbeef})==317);
 check(call(__imp__XGetAVPack,c,b,{})==6);check(call(__imp__XGetLanguage,c,b,{})==1);
 check(call(__imp__KeQueryPerformanceFrequency,c,b,{})>0);
 m.SystemHeapFree(a);std::printf("{\"group\":\"image_imports\",\"result\":\"PASS\",\"assertions\":%u,\"selected_exports\":10}\n",checks);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
