// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Actual SDK XexModule/UserModule transactions; independently authored inputs.
#include "xex-fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/system/xex_module.h>
#include <rex/system/user_module.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <dlfcn.h>
using namespace rex::system;
using Image=rex::runtime::XexModule;
static unsigned checks=0;
static std::atomic<unsigned> destructors{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("ownership oracle "+std::to_string(checks));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
class Tracked final:public UserModule {public:using UserModule::UserModule;~Tracked()override{++destructors;}};
static void save(const std::filesystem::path&p,const std::vector<uint8_t>&bytes){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f));}
static void be(std::vector<uint8_t>&b,size_t o,uint32_t v){image_fixture::put(b,o,v,4,true);}
static auto basic(bool truncated=false){auto b=image_fixture::xex();be(b,0x300,16);image_fixture::put(b,0x306,1,2,true);be(b,0x308,0x30000);be(b,0x30c,0);if(truncated)b.resize(0x1040);return b;}
static auto missing_import(){auto b=image_fixture::xex();be(b,20,4);be(b,48,0x103ff);be(b,52,0x400);
 be(b,0x400,72);be(b,0x404,16);be(b,0x408,1);std::memcpy(b.data()+0x40c,"xboxkrnl.exe",13);
 be(b,0x41c,44);image_fixture::put(b,0x41c+38,1,2,true);be(b,0x41c+40,0x82020000);be(b,0x21000,0x0000ffff);return b;}
int main(int argc,char**argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 std::filesystem::path p=argv[1];std::string mode=argv[2];std::filesystem::create_directories(p/"empty");
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto&m=*r.memory();auto*k=r.kernel_state();auto*heap=m.LookupHeap(0x82000000);auto*system=m.LookupHeapByType(false,4096);
 auto baseline=heap->GetUnreservedPageCount(),sysbaseline=system->GetUnreservedPageCount();
 auto image=[&]{return std::make_unique<Image>(k->function_dispatcher(),k);};
 auto user=[&]{return object_ref<UserModule>(new Tracked(k));};
 auto load=[&](Image&i,std::vector<uint8_t>&b){return i.Load("original.xex","original.xex",b.data(),b.size());};
 auto bytes=image_fixture::xex();
 if(mode=="collision"){
  int owner=0;check(heap->AllocFixed(0x83000000,65536,65536,1,3));heap->SetAllocationOwner(0x83000000,&owner);check(heap->AllocFixed(0x83000000,65536,65536,2,3));
  m.Fill(0x83000000,65536,0x5a);auto a=image(),b=image(),collision=image();auto other=image_fixture::xex(0x84000000);
  check(load(*a,bytes)&&a->LoadContinue());check(load(*b,other)&&b->LoadContinue());check(!load(*collision,bytes));
  check(live(m,0x82000000)&&live(m,0x84000000)&&live(m,0x83000000));check(a->Unload());
  check(live(m,0x84000000)&&live(m,0x83000000));check(m.TranslateVirtual<uint8_t*>(0x83000000)[0]==0x5a&&m.TranslateVirtual<uint8_t*>(0x84020000)[0]==0xa5);
  check(b->Unload());check(!heap->ReleaseOwned(0x83000000,a.get()));check(heap->ReleaseOwned(0x83000000,&owner));check(heap->GetUnreservedPageCount()==baseline);
 }else if(mode=="rollback"){
  for(unsigned stage=0;stage<10;++stage){auto b=bytes;bool continuation=true;
   switch(stage){case 0:b[0x1000]=0;continuation=false;break;
    case 1:image_fixture::put(b,0x1080,0,4,false);break;
    case 2:image_fixture::put(b,0x1000+0x98+224+12,0xffff0000,4,false);break;
    case 3:b=basic(true);continuation=false;break;
    case 4:be(b,0x300,36);image_fixture::put(b,0x306,2,2,true);be(b,0x308,0x8000);be(b,0x30c,26);continuation=false;break;
    case 5:b=missing_import();break;
    case 6:be(b,0x100+0x184+2*24,0x22);break;
    case 7:be(b,16,0xffc);continuation=false;break;
    case 8:be(b,0x300,36);image_fixture::put(b,0x306,2,2,true);be(b,0x308,0);be(b,0x30c,0);continuation=false;break;
    case 9:b=missing_import();be(b,0x21000,0x0100ffff);break;
   }
   auto attempt=image();bool loaded=load(*attempt,b);if(continuation){check(loaded);check(!attempt->LoadContinue());}else check(!loaded);
   check(!attempt->loaded()&&!live(m,0x82000000)&&heap->GetUnreservedPageCount()==baseline);
   check(attempt->binary_sections().empty()&&attempt->binary_symbols().empty()&&attempt->pe_sections().empty()&&attempt->import_libraries()->empty());
   check(attempt->entry_point()==0&&attempt->GetProcAddress(uint16_t(1))==0);check(attempt->Unload());
   check(load(*attempt,bytes)&&attempt->LoadContinue());check(attempt->Unload()&&heap->GetUnreservedPageCount()==baseline);
  }
 }else if(mode=="basic-compression"){
  auto b=basic();auto a=image();check(load(*a,b)&&a->LoadContinue());check(a->image_size()==0x30000);check(a->Unload()&&heap->GetUnreservedPageCount()==baseline);
 }else if(mode=="known-function-import"){
  uint32_t ordinal=0;for(auto*e:r.export_resolver()->all_exports_by_name())if(std::strcmp(e->name,"KeTlsGetValue")==0)ordinal=e->ordinal;check(ordinal!=0);
  auto b=missing_import();be(b,0x400,76);be(b,0x41c,48);image_fixture::put(b,0x41c+38,2,2,true);be(b,0x41c+44,0x82020004);
  be(b,0x21000,ordinal);be(b,0x21004,0x01000000|ordinal);auto a=image();check(load(*a,b)&&a->LoadContinue());
  check(a->import_libraries()->size()==1&&a->import_libraries()->front().imports.size()==1);
  check(a->import_libraries()->front().imports.front().value_address==0x82020000&&a->import_libraries()->front().imports.front().thunk_address==0x82020004);
  check(a->Unload()&&heap->GetUnreservedPageCount()==baseline);
 }else if(mode=="reload"){
  auto a=image();for(unsigned i=0;i<16;++i){check(load(*a,bytes)&&a->LoadContinue()&&a->LoadContinue());check(a->pe_sections().size()==3);check(a->Unload()&&a->Unload());check(!a->ContainsAddress(0x82001000)&&heap->GetUnreservedPageCount()==baseline);}
 }else if(mode=="metadata"){
  save(p/"empty/original.xex",bytes);
  for(unsigned i=0;i<16;++i){auto a=user();check(a->LoadFromFile("game:\\original.xex")==0);auto header=a->guest_xex_header(),handle=a->hmodule_ptr();auto copied=a;
   check(a->Unload()==0&&!live(m,0x82000000)&&live(m,header));uint32_t value=0x55555555;check(a->GetOptHeader(rex::XEX_HEADER_ENTRY_POINT,&value)==0xc0000225u&&value==0x55555555);check(!a->GetProcAddressByOrdinal(1));
   check(a->ReleaseHandle());a.reset();check(live(m,header)&&live(m,handle)&&destructors==i);copied.reset();
   check(!live(m,header)&&!live(m,handle)&&destructors==i+1);check(heap->GetUnreservedPageCount()==baseline&&system->GetUnreservedPageCount()==sysbaseline);
  }
 }else if(mode=="active-references"){
  save(p/"empty/original.xex",bytes);auto a=user();check(a->LoadFromFile("game:\\original.xex")==0);check(k->RegisterUserModule(a));
  auto header=a->guest_xex_header(),handle=a->hmodule_ptr();std::mutex mutex;std::condition_variable cv;bool ready=false,release=false;std::atomic<unsigned> errors{0};
  auto* original=a.get();std::thread worker([&]{auto lease=original->AcquireImageLease();if(!lease)++errors;
   {std::unique_lock lock(mutex);ready=true;cv.notify_one();if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return release;}))std::abort();}
   if(!live(m,header)||!live(m,0x82000000)||destructors!=0)++errors;
  });
  {std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return ready;}))std::abort();}
  check(a->Unload()==0xc0000001u);check(!a->AcquireImageLease());check(live(m,header)&&live(m,0x82000000));
  k->UnloadUserModule(a,false);a.reset();check(destructors==0&&live(m,header)&&live(m,handle));
  {std::lock_guard lock(mutex);release=true;}cv.notify_one();worker.join();
  check(!errors&&destructors==1&&!live(m,header)&&!live(m,handle)&&!live(m,0x82000000));check(!k->GetModule("original.xex"));
  check(heap->GetUnreservedPageCount()==baseline&&system->GetUnreservedPageCount()==sysbaseline);
 }else if(mode=="concurrent-images"){
  std::atomic<unsigned> errors{0},done{0};std::vector<std::thread> workers;
  for(unsigned n=0;n<6;++n)workers.emplace_back([&,n]{try{auto b=image_fixture::xex(0x84000000+n*0x100000);auto a=image();for(unsigned i=0;i<8;++i){if(!load(*a,b)||!a->LoadContinue()||!a->Unload())++errors;else++done;}}catch(...){++errors;}});
  for(auto&t:workers)t.join();check(!errors&&done==48);check(heap->GetUnreservedPageCount()==baseline);
 }else if(mode=="header-failure-zero"||mode=="header-failure-throw"){
  save(p/"empty/original.xex",bytes);auto arm=reinterpret_cast<void(*)(unsigned)>(dlsym(RTLD_DEFAULT,"fixture_arm_header_failure"));auto count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_header_failure_count"));check(arm&&count);
  auto a=user();auto handle=a->hmodule_ptr();arm(mode=="header-failure-zero"?1:2);check(a->LoadFromFile("game:\\original.xex")==0xc0000017u);arm(0);
  check(count()==1&&a->guest_xex_header()==0&&a->processor_module()==nullptr&&!live(m,0x82000000));check(!k->GetModule("original.xex"));check(heap->GetUnreservedPageCount()==baseline&&live(m,handle));
  check(a->LoadFromFile("game:\\original.xex")==0);auto header=a->guest_xex_header();check(header&&a->ReleaseHandle());a.reset();check(destructors==1&&!live(m,header)&&!live(m,handle));check(heap->GetUnreservedPageCount()==baseline&&system->GetUnreservedPageCount()==sysbaseline);
 }else return 2;
 std::printf("{\"group\":\"xex_ownership_%s\",\"result\":\"PASS\",\"assertions\":%u}\n",mode.c_str(),checks);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
