// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "ppc_test_decls.h"
#include "owned-module.h"
#include "thread-tls.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <atomic>
#include <condition_variable>
#include <functional>
using namespace image_fixture;
REX_EXTERN(__imp__KeTlsSetValue);REX_EXTERN(__imp__KeTlsGetValue);
static unsigned checks=0;
static void check(bool v){++checks;if(!v)throw std::runtime_error("image oracle "+std::to_string(checks));}
template<class F>static void rejects(F f){bool caught=false;try{f();}catch(const std::logic_error&){caught=true;}check(caught);}
static bool live(rex::memory::Memory& m,uint32_t a){rex::memory::HeapAllocationInfo i{};return m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
static std::vector<Section> sections(const std::filesystem::path& input){
 std::ifstream f(input/"module.bin",std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(f)),{});
 if(bytes.size()!=144)throw std::runtime_error("fixture bytes");std::vector<std::byte> code;
 for(unsigned char c:bytes)code.push_back(std::byte(c));
 return {{0,8192,false,true,code},{0x4000,8192,false,false,{std::byte{0x12},std::byte{0x34},std::byte{0x56},std::byte{0x78}}},{0x8000,8192,true,false,{}}};
}
static auto imports(){return std::vector<OwnedModule::Import>{{"load",112},{"store",120}};}
static auto exports(){return std::vector<OwnedModule::Export>{{"load",module_82010070},{"store",module_82010078}};}
static std::atomic<unsigned> destroyed{0},errors{0},observations{0};
class Thread final:public rex::system::XThread {
 std::function<void(Thread&)> body_;
 public:
 Thread(rex::system::KernelState*k,std::function<void(Thread&)> body):XThread(k,65536,0,0,0,1,true),body_(std::move(body)){}
 ~Thread()override{++destroyed;}
 void Execute()override{kernel_state()->OnThreadExecute(this);try{body_(*this);}catch(const std::exception&e){std::fprintf(stderr,"thread: %s\n",e.what());++errors;}Exit(0);}
};
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{std::filesystem::path p=argv[1],input=argv[2];std::string mode=argv[3];std::filesystem::create_directories(p/"empty");
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto& sdk=*r.memory();Memory memory(sdk);auto descriptors=sections(input);
 auto make=[&](std::vector<std::byte> tls=std::vector<std::byte>{std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x44}}){return OwnedModule::load(memory,sdk,descriptors,{{0x9ffc,0x5fff}},imports(),exports(),std::move(tls));};
 if(mode=="ownership"){
  auto image=make();auto copy=image;Memory::Allocation stale;uint32_t base;
  {auto lease=image.acquire();base=lease.base();stale=lease.allocation();Scope scope(memory,sdk);lease.admit(scope);PPCContext c{};c.r4.u64=base+0x4000;lease.enter(base+112,c);check(c.r3.u32==0x12345678);
   uint32_t relocated=0;check(memory.load32be(stale,0x9ffc,relocated)==Result::ok&&relocated==base+0x5fff);
   check(image.close()==OwnedModule::Close::busy);rejects([&]{copy.acquire();});check(live(sdk,base));
  }
  check(copy.close()==OwnedModule::Close::closed);check(!live(sdk,base));check(image.close()==OwnedModule::Close::already_closed);
  rejects([&]{image.acquire();});uint32_t value=0xdeadbeef;check(memory.load32be(stale,0,value)==Result::ownership&&value==0xdeadbeef);
  auto replacement=make();{auto lease=replacement.acquire();check(lease.base()==base);check(memory.load32be(stale,0,value)==Result::ownership);}
  check(replacement.close()==OwnedModule::Close::closed);
 }else if(mode=="rollback"){
  auto reference=make();uint32_t expected;{auto l=reference.acquire();expected=l.base();}check(reference.close()==OwnedModule::Close::closed);
  for(int stage:{1,2}){bool caught=false;try{OwnedModule::load(memory,sdk,descriptors,{},imports(),exports(),{},stage);}catch(const std::runtime_error&){caught=true;}check(caught&&!live(sdk,expected));}
  auto bad=imports();bad[1].name="missing";
  rejects([&]{OwnedModule::load(memory,sdk,descriptors,{},bad,exports());});check(!live(sdk,expected));
  auto unsupported=exports();unsupported[0].context_signature=false;
  rejects([&]{OwnedModule::load(memory,sdk,descriptors,{},imports(),unsupported);});check(!live(sdk,expected));
  auto duplicate=imports();duplicate[1]=duplicate[0];rejects([&]{OwnedModule::load(memory,sdk,descriptors,{},duplicate,exports());});check(!live(sdk,expected));
  for(unsigned i=0;i<32;++i){auto m=make();uint32_t b;{auto lease=m.acquire();b=lease.base();check(b==expected);}check(m.close()==OwnedModule::Close::closed&&!live(sdk,b));}
 }else if(mode=="concurrent"){
  auto image=make();auto alias=image;std::mutex mutex;std::condition_variable cv;bool entered=false,release=false;std::atomic<unsigned> ok{0};uint32_t base=0;
  std::thread worker([&]{try{auto lease=alias.acquire();Scope scope(memory,sdk);lease.admit(scope);
   {std::lock_guard lock(mutex);base=lease.base();entered=true;}cv.notify_one();
   {std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return release;}))throw std::runtime_error("gate");}
   for(unsigned i=0;i<100;++i){PPCContext c{};c.r4.u64=base+0x4000;lease.enter(base+112,c);if(c.r3.u32==0x12345678)++ok;}
  }catch(...){++errors;}});
  {std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return entered;}))std::abort();}
  check(image.close()==OwnedModule::Close::busy);rejects([&]{image.acquire();});check(live(sdk,base));
  {std::lock_guard lock(mutex);release=true;}cv.notify_one();worker.join();check(ok==100&&!errors);
  check(image.close()==OwnedModule::Close::closed&&!live(sdk,base));
 }else if(mode=="tls"){
  auto a=make(),b=make({std::byte{0x55},std::byte{0x66},std::byte{0x77},std::byte{0x88}});
  std::mutex mutex;std::condition_variable cv;unsigned ready=0;bool release=false;
  std::array<uint32_t,2> pcr{},tls{},block_a{},block_b{};std::array<rex::system::object_ref<Thread>,2> threads;
  for(unsigned id=0;id<2;++id){threads[id]=rex::system::object_ref<Thread>(new Thread(r.kernel_state(),[&,id](Thread& self){
   auto test=[&](bool value){++observations;if(!value)throw std::runtime_error("TLS oracle");};
   auto&c=*rex::runtime::current_ppc_context();
   ThreadTls ta(a.acquire(),memory,sdk.virtual_membase(),c,1,errors,block_a[id]);
   ThreadTls tb(b.acquire(),memory,sdk.virtual_membase(),c,2,errors,block_b[id]);
   const auto& aa=ta.allocation();const auto& ab=tb.allocation();
   test(live(sdk,aa.guest_address())&&live(sdk,ab.guest_address()));
   pcr[id]=c.r13.u32;tls[id]=self.tls_ptr();test(c.r13.u32==self.pcr_ptr());
   test(rex::memory::load_and_swap<uint32_t>(sdk.virtual_membase()+pcr[id])==tls[id]);
   for(auto pair:{std::pair{1u,aa.guest_address()},std::pair{2u,ab.guest_address()}}){c.r3.u64=pair.first;c.r4.u64=pair.second;__imp__KeTlsSetValue(c,sdk.virtual_membase());test(c.r3.u32==1);}
   uint32_t value=0;test(memory.load32be(aa,0,value)==Result::ok&&value==0x11223344);test(memory.load32be(ab,0,value)==Result::ok&&value==0x55667788);
   test(memory.load32be(ab,4,value)==Result::ok&&value==0);
   test(memory.store32be(aa,4,0xaabb0000+id)==Result::ok);
   {std::unique_lock lock(mutex);++ready;cv.notify_all();if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return release;}))throw std::runtime_error("TLS gate");}
   for(auto pair:{std::pair{1u,aa.guest_address()},std::pair{2u,ab.guest_address()}}){c.r3.u64=pair.first;__imp__KeTlsGetValue(c,sdk.virtual_membase());test(c.r3.u32==pair.second);}
   test(memory.load32be(aa,4,value)==Result::ok&&value==0xaabb0000+id);
  }));check(threads[id]->Create()==0);}
  for(auto&t:threads)check(t->Resume()==0);
  {std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return ready==2;}))std::abort();}
  check(a.close()==OwnedModule::Close::busy&&b.close()==OwnedModule::Close::busy);rejects([&]{a.acquire();});
  check(pcr[0]!=pcr[1]&&tls[0]!=tls[1]&&block_a[0]!=block_a[1]&&block_b[0]!=block_b[1]);
  {std::lock_guard lock(mutex);release=true;}cv.notify_all();
  for(unsigned id=0;id<2;++id){auto&t=threads[id];check(rex::thread::Wait(t->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);
   check(live(sdk,pcr[id])&&live(sdk,tls[id])&&!live(sdk,block_a[id])&&!live(sdk,block_b[id]));check(t->ReleaseHandle());t.reset();check(!live(sdk,pcr[id])&&!live(sdk,tls[id]));}
  check(errors==0&&destroyed==2&&observations==24);check(a.close()==OwnedModule::Close::closed&&b.close()==OwnedModule::Close::closed);
 }else if(mode=="tls-rollback"){
  auto image=make();std::array<uint32_t,3> addresses{};uint32_t pcr=0,tls=0;
  auto thread=rex::system::object_ref<Thread>(new Thread(r.kernel_state(),[&](Thread& self){
   auto&c=*rex::runtime::current_ppc_context();pcr=self.pcr_ptr();tls=self.tls_ptr();
   for(int stage:{1,2,3}){
    bool caught=false;try{ThreadTls block(image.acquire(),memory,sdk.virtual_membase(),c,1,errors,addresses[stage-1],stage);}catch(const std::runtime_error&){caught=true;}
    if(!caught||!addresses[stage-1]||live(sdk,addresses[stage-1]))++errors;
    c.r3.u64=1;__imp__KeTlsGetValue(c,sdk.virtual_membase());if(c.r3.u32!=0)++errors;
   }
  }));check(thread->Create()==0);check(thread->Resume()==0);
  check(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);
  check(errors==0&&live(sdk,pcr)&&live(sdk,tls));for(auto address:addresses)check(address&&!live(sdk,address));
  check(thread->ReleaseHandle());thread.reset();check(destroyed==1&&!live(sdk,pcr)&&!live(sdk,tls));
  check(image.close()==OwnedModule::Close::closed);
 }else return 2;
 std::printf("{\"group\":\"image_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u}\n",mode.c_str(),checks,observations.load());return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
