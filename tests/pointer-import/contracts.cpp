// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original host-driven PPCContext import dispatch; no game or guest entry code.
#include "../tls-import/fixture.h"
#include "runtime/import_admission.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/thread.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <barrier>
#include <condition_variable>
#include <cstdio>
#include <dlfcn.h>
using namespace rex::system;
using namespace brutal::imports;
using Memory=brutal::foundation::SdkCheckedMemory;
static unsigned checks=0;
static std::atomic<unsigned> buffer_allocations{0},buffer_reclaims{0};
static std::atomic<unsigned> errors{0},observations{0},destructors{0},tls_allocations{0},tls_reclaims{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("pointer oracle "+std::to_string(checks));}
static void test(bool v){++observations;if(!v)throw std::runtime_error("worker pointer oracle "+std::to_string(observations.load()));}
template<class F>static void reject(F f){bool caught=false;try{f();}catch(const std::logic_error&){caught=true;}catch(const Memory::BufferError&){caught=true;}test(caught);}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
static auto read(Memory& m,const Memory::Allocation&a,uint32_t offset,uint32_t length){std::vector<std::byte>b(length);test(m.read(a,offset,b)==Memory::Result::ok);return b;}
static uint32_t word(const std::vector<std::byte>&b,size_t p){uint32_t v=0;for(unsigned i=0;i<4;++i)v=(v<<8)|std::to_integer<uint32_t>(b.at(p+i));return v;}
static uint32_t thunk(uint32_t base,uint32_t ordinal){unsigned index=0;for(auto entry:tls_fixture::matrix_imports)if(std::string_view(entry.library)=="xboxkrnl.exe"){if(entry.ordinal==ordinal)return base+0x20004+index*8;++index;}throw std::logic_error("fixture import");}
class Thread:public XThread{
 std::function<void(Thread&)> body_;
 public:
 Thread(KernelState*k,std::function<void(Thread&)>body):XThread(k,65536,0,0,0,1,true),body_(std::move(body)){}
 ~Thread()override{++destructors;}
 void Execute()override{kernel_state()->OnThreadExecute(this);try{body_(*this);}catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;}Exit(0);}
};
static thread_local Admission* callback_admission=nullptr;
static thread_local PointerBinding* callback_binding=nullptr;
static uint64_t callback(uint64_t tag,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,uint64_t pointer){
 auto&c=*rex::runtime::current_ppc_context();c.r3.u64=pointer;c.r4.u64=16;c.r5.u64=0x11223344;
 callback_admission->direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,*callback_binding);return tag^c.r3.u64;
}
REX_HOOK(pointer_stack_callback,callback)
int main(int argc,char**argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 std::filesystem::path p=argv[1];std::string mode=argv[2];std::filesystem::create_directories(p/"empty");
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto&sdk=*r.memory();auto*k=r.kernel_state();auto*heap=sdk.LookupHeap(0x82000000);auto*sys=sdk.LookupHeapByType(false,4096);
 const auto baseline=heap->GetUnreservedPageCount(),sysbaseline=sys->GetUnreservedPageCount();
 Memory memory(sdk);
 auto alloc=[&](uint32_t size,bool commit=true){auto value=memory.allocate(size,commit);++buffer_allocations;return value;};
 auto free=[&](const Memory::Allocation&value){auto result=memory.release(value);if(result==Memory::Result::ok)++buffer_reclaims;return result;};
 auto load=[&](std::string name,uint32_t base){auto bytes=tls_fixture::image(base,0x41,true);{std::ofstream f(p/"empty"/name,std::ios::binary);f.write(reinterpret_cast<char*>(bytes.data()),bytes.size());check(bool(f));}auto m=object_ref<UserModule>(new UserModule(k));check(m->LoadFromFile("game:\\"+name)==0);return m;};
 auto a=load("a.xex",0x82000000);
 auto spawn=[&](std::function<void(Thread&)> body){auto t=object_ref<Thread>(new Thread(k,std::move(body)));check(t->CreateWithModule(a)==0);++tls_allocations;return t;};
 auto finish=[&](object_ref<Thread>&t){check(rex::thread::Wait(t->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);auto tls=t->tls_ptr(),pcr=t->pcr_ptr();check(live(sdk,tls)&&live(sdk,pcr));check(t->ReleaseHandle());t.reset();check(!live(sdk,tls)&&!live(sdk,pcr));++tls_reclaims;};
 if(mode=="lifetime"){
  auto b=load("b.xex",0x83000000);auto allocation=alloc(8192);auto address=allocation.guest_address();
  std::mutex mutex;std::condition_variable cv;bool ready=false,release=false;
  auto t=spawn([&](Thread&){Admission ad(b,*r.export_resolver(),sdk);PointerBinding binding(memory,allocation,[&](auto stage){if(stage==PointerBinding::Stage::sdk_returned){std::unique_lock lock(mutex);ready=true;cv.notify_one();if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return release;}))throw std::runtime_error("call gate");}});
   auto&c=*rex::runtime::current_ppc_context();c.r3.u64=address;ad.indirect(thunk(0x83000000,0x12e),Signature::void_inout28,c,binding);});
  check(t->Resume()==0);{std::unique_lock lock(mutex);check(cv.wait_for(lock,std::chrono::seconds(5),[&]{return ready;}));}
  check(memory.try_release(allocation)==Memory::ReleaseAttempt::busy);check(b->Unload()==0xc0000001u);check(!b->AcquireImageLease()&&live(sdk,0x83000000));
  {std::lock_guard lock(mutex);release=true;}cv.notify_one();finish(t);check(!errors);check(b->Unload()==0);check(b->ReleaseHandle());b.reset();
  auto bytes=read(memory,allocation,0,28);check(bytes[0]==std::byte{1}&&word(bytes,16)==0xffffffff);check(free(allocation)==Memory::Result::ok&&!live(sdk,address));
 }else if(mode=="concurrent"){
  std::barrier start(4);std::array<object_ref<Thread>,4> threads;std::array<Memory::Allocation,4> allocations;
  for(unsigned id=0;id<4;++id){allocations[id]=alloc(8192);auto allocation=allocations[id];threads[id]=spawn([&,allocation,id](Thread&){
   Admission ad(a,*r.export_resolver(),sdk);PointerBinding binding(memory,allocation);auto&c=*rex::runtime::current_ppc_context();start.arrive_and_wait();
   for(unsigned n=0;n<16;++n){c.r3.u64=allocation.guest_address()+4092;ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);test(c.r3.u32==allocation.guest_address()+4092);}
   auto bytes=read(memory,allocation,4092,28);test(bytes[0]==std::byte{1}&&word(bytes,16)==0xffffffff);(void)id;
  });}
  for(auto&t:threads)check(t->Resume()==0);for(auto&t:threads)finish(t);check(!errors);for(auto&allocation:allocations)check(free(allocation)==Memory::Result::ok);
 }else{
  unsigned cycles=mode=="repeat"?8:1;
  for(unsigned cycle=0;cycle<cycles;++cycle){auto allocation=alloc(16384);auto address=allocation.guest_address();
   auto t=spawn([&,allocation,address](Thread&self){Admission ad(a,*r.export_resolver(),sdk);PointerBinding binding(memory,allocation);auto&c=*rex::runtime::current_ppc_context();
    test(c.r13.u64==self.pcr_ptr()&&rex::memory::load_and_swap<uint32_t>(sdk.virtual_membase()+self.pcr_ptr())==self.tls_ptr());
    test(rex::memory::load_and_swap<uint32_t>(sdk.virtual_membase()+self.tls_ptr())==0x41424344);
    auto fill=[&](unsigned off,unsigned len){std::vector<std::byte>b(len,std::byte{0xa5});test(memory.write(allocation,off,b)==Memory::Result::ok);};
    auto words=[&]{for(unsigned i=0;i<4;++i)test(memory.store32be(allocation,4092+i*4,i==1?0x55667788:0x11223344)==Memory::Result::ok);};
    auto compare=[&](bool indirect=false){c.r3.u64=address+4092;c.r4.u64=16;c.r5.u64=0x11223344;if(indirect)ad.indirect(thunk(0x82000000,0x11b),Signature::u32_in_words_u32_u32,c,binding);else ad.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,binding);test(c.r3.u32==3);};
    if(mode=="contracts"||mode=="repeat"){
     words();auto before=read(memory,allocation,4092,16);compare();compare(true);test(before==read(memory,allocation,4092,16));
     test(sdk.LookupHeap(address)->Protect(address+4096,4096,1));compare();test(sdk.LookupHeap(address)->Protect(address+4096,4096,3));
     fill(4092,28);auto saved=c;c.r3.u64=address+4092;auto input=c;ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);test(std::memcmp(&input,&c,sizeof c)==0);
     auto bytes=read(memory,allocation,4092,28);test(bytes[0]==std::byte{1}&&bytes[1]==std::byte{0}&&bytes[2]==std::byte{0xa5}&&bytes[3]==std::byte{0xa5});test(word(bytes,4)==0&&word(bytes,16)==0xffffffff&&word(bytes,20)==0&&word(bytes,24)==0);test(word(bytes,8)==0xa5a5a5a5&&word(bytes,12)==0xa5a5a5a5);
     fill(4092,28);c.r3.u64=address;c.r4.u64=address+4092;c.r5.u64=0;ad.indirect(thunk(0x82000000,0xee),Signature::u32_address_out28_u32,c,binding);test(c.r3.u32==0);bytes=read(memory,allocation,4092,28);test(word(bytes,0)==address&&word(bytes,4)==address&&word(bytes,8)==4&&word(bytes,12)==16384&&word(bytes,16)==0x1000&&word(bytes,20)==4&&word(bytes,24)==0x20000);test(c.r1.u64==saved.r1.u64&&c.r2.u64==saved.r2.u64&&c.r13.u64==saved.r13.u64);
    }else if(mode=="invalid"){
     words();for(unsigned kind=0;kind<13;++kind){c.r3.u64=address+4092;c.r4.u64=16;c.r5.u64=0x11223344;
      switch(kind){case 0:c.r3.u64=0;break;case 1:c.r3.u64++;break;case 2:c.r4.u64=0;break;case 3:c.r4.u64=3;break;case 4:c.r4.u64=65540;break;case 5:c.r4.u64=0x100000010ull;break;case 6:c.r3.u64=0x100000000ull+address;break;case 7:c.r3.u64=0xfffffffc;break;case 8:c.r3.u64=address+16384;break;case 9:c.r5.u64=0x100000001ull;break;case 10:c.r3.u64=address-4;break;case 11:c.r3.u64=address+16380;break;case 12:c.r3.u64=0xe0000000;break;}
      auto before=c;reject([&]{ad.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,binding);});test(std::memcmp(&before,&c,sizeof c)==0);
     }
     c.r3.u64=address;reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c);});reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::typed_vector_argument,c,binding);});
     PointerBinding empty(memory,{});reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,empty);});
     Memory foreign(sdk);PointerBinding wrong(foreign,allocation);reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,wrong);});
    }else if(mode=="permissions"){
     fill(4092,28);auto before=read(memory,allocation,4092,28);auto*heap=sdk.LookupHeap(address);
     test(heap->Protect(address+4096,4096,1));c.r3.u64=address+4092;reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);});test(before==read(memory,allocation,4092,28));test(heap->Protect(address+4096,4096,3));
     test(heap->Decommit(address+4096,4096));reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);});test(heap->AllocFixed(address+4096,4096,4096,2,3));
     auto uncommitted=alloc(8192,false);PointerBinding ub(memory,uncommitted);c.r3.u64=uncommitted.guest_address();reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,ub);});test(free(uncommitted)==Memory::Result::ok);
     auto stale=alloc(8192);auto old=stale.guest_address();test(free(stale)==Memory::Result::ok);auto replacement=alloc(8192);PointerBinding sb(memory,stale);c.r3.u64=old;reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,sb);});test(free(replacement)==Memory::Result::ok);test(memory.try_release(stale)==Memory::ReleaseAttempt::invalid);
    }else if(mode=="policy"){
     unsigned denied=0;for(auto entry:tls_fixture::matrix_imports){
      const bool supported=(std::string_view(entry.library)=="xboxkrnl.exe"&&(entry.ordinal==0x11b||entry.ordinal==0x12e||entry.ordinal==0xee));
      if(supported)continue;++denied;
      for(auto signature:{Signature::u32_in_words_u32_u32,Signature::void_inout28,Signature::u32_address_out28_u32}){auto saved=c;reject([&]{ad.direct(entry.library,entry.ordinal,signature,c,binding);});test(std::memcmp(&saved,&c,sizeof c)==0);}
     }
     test(denied==30&&PointerBinding::statistics().sdk_returns==0);
     c.r3.u64=address;auto saved=c;
     reject([&]{ad.indirect(0x82020000,Signature::void_inout28,c,binding);});
     reject([&]{ad.direct("xboxkrnl.exe.extra",0x12e,Signature::void_inout28,c,binding);});
     for(auto signature:{Signature::typed_vector_argument,Signature::typed_vector_return,Signature::mixed_vector,Signature::variadic})reject([&]{ad.direct("xboxkrnl.exe",0x12e,signature,c,binding);});
     test(std::memcmp(&saved,&c,sizeof c)==0);
    }else if(mode=="errors"){
     fill(4092,28);auto before=read(memory,allocation,4092,28);c.r3.u64=address;c.r4.u64=address+4092;c.r5.u64=0xffffffff;
     ad.direct("xboxkrnl.exe",0xee,Signature::u32_address_out28_u32,c,binding);test(c.r3.u32==0xc000000d&&before==read(memory,allocation,4092,28));
     for(unsigned stage=0;stage<3;++stage){PointerBinding throwing(memory,allocation,[&](auto at){if(at==PointerBinding::Stage::scratch_ready){
      test(memory.try_release(allocation)==Memory::ReleaseAttempt::busy);
      std::array<std::byte,4> bytes{};test(memory.read(allocation,0,bytes)==Memory::Result::ownership);
      test(memory.write(allocation,0,bytes)==Memory::Result::ownership);test(memory.release(allocation)==Memory::Result::ownership);
      bool refused=false;try{memory.allocate(16);}catch(const std::logic_error&){refused=true;}test(refused);
      reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);});
     }if(unsigned(at)==stage)throw std::runtime_error("original callback error");});c.r3.u64=address+4092;auto saved=c;auto pages=sys->GetUnreservedPageCount();bool caught=false;try{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,throwing);}catch(const std::runtime_error&){caught=true;}test(caught&&std::memcmp(&saved,&c,sizeof c)==0&&sys->GetUnreservedPageCount()==pages);test(before==read(memory,allocation,4092,28));}
    }else if(mode=="external-release"){
     fill(4092,28);PointerBinding changed(memory,allocation,[&](auto stage){if(stage==PointerBinding::Stage::sdk_returned){test(sdk.LookupHeap(address)->Release(address));++buffer_reclaims;}});
     c.r3.u64=address+4092;auto saved=c;reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,changed);});test(!live(sdk,address)&&std::memcmp(&saved,&c,sizeof c)==0);
    }else if(mode=="mutation"){
     fill(4092,28);auto before=read(memory,allocation,4092,28);PointerBinding changed(memory,allocation,[&](auto stage){if(stage==PointerBinding::Stage::sdk_returned)test(sdk.LookupHeap(address)->Protect(address+4096,4096,1));});c.r3.u64=address+4092;auto saved=c;
     reject([&]{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,changed);});test(std::memcmp(&saved,&c,sizeof c)==0&&before==read(memory,allocation,4092,28));test(sdk.LookupHeap(address)->Protect(address+4096,4096,3));
    }else if(mode=="abi"){
     words();callback_admission=&ad;callback_binding=&binding;PPCContext saved=c;
     auto value=rex::ppc::GuestToHostFunction<uint64_t>(pointer_stack_callback,uint64_t(0x1122334455667788ull),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint64_t(address+4092));
     test(value==(0x1122334455667788ull^3)&&std::memcmp(&saved,&c,sizeof c)==0);
     PointerBinding throwing(memory,allocation,[](auto){throw std::runtime_error("callback propagation");});callback_binding=&throwing;bool caught=false;try{rex::ppc::GuestToHostFunction<uint64_t>(pointer_stack_callback,uint64_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint32_t(0),uint64_t(address+4092));}catch(const std::runtime_error&){caught=true;}test(caught&&std::memcmp(&saved,&c,sizeof c)==0);callback_binding=nullptr;callback_admission=nullptr;
    }else if(mode=="init-zero"||mode=="init-throw"){
     auto arm=reinterpret_cast<void(*)(unsigned,unsigned)>(dlsym(RTLD_DEFAULT,"fixture_arm_tls_failure"));auto count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_tls_failure_count"));test(arm&&count);fill(4092,28);auto before=read(memory,allocation,4092,28);c.r3.u64=address+4092;auto saved=c;auto pages=sys->GetUnreservedPageCount();arm(mode=="init-zero"?1:2,28);bool caught=false;try{ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);}catch(const std::bad_alloc&){caught=true;}arm(0,0);test(caught&&count()==1&&sys->GetUnreservedPageCount()==pages&&std::memcmp(&saved,&c,sizeof c)==0);test(before==read(memory,allocation,4092,28));ad.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,binding);
    }else throw std::invalid_argument("mode");
   });check(t->Resume()==0);finish(t);check(!errors);check(free(allocation)==(mode=="external-release"?Memory::Result::ownership:Memory::Result::ok));
   if(mode=="repeat"&&cycle+1<cycles){check(a->Unload()==0);check(a->ReleaseHandle());a.reset();a=load("a.xex",0x82000000);}
  }
 }
 auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&buffer_allocations==buffer_reclaims);
 check(!errors&&tls_allocations==tls_reclaims);check(a->Unload()==0);check(a->ReleaseHandle());a.reset();check(heap->GetUnreservedPageCount()==baseline&&sys->GetUnreservedPageCount()==sysbaseline);
 std::printf("{\"group\":\"pointer_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"tls_allocations\":%u,\"tls_reclaims\":%u,\"thread_destructors\":%u,\"buffer_allocations\":%u,\"buffer_reclaims\":%u,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"sdk_returns\":%llu}\n",mode.c_str(),checks,observations.load(),tls_allocations.load(),tls_reclaims.load(),destructors.load(),buffer_allocations.load(),buffer_reclaims.load(),(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims,(unsigned long long)stats.sdk_returns);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
