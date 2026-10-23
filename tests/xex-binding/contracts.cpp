// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/synthetic_xex_binding.h"
#include "binding-data.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/thread.h>
#include <CommonCrypto/CommonDigest.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <barrier>
#include <chrono>
#include <thread>
#include <cstdio>
#include <cstdlib>
using namespace rex::system;
using namespace brutal::imports;
using brutal::synthetic::XexBinding;
using Memory=brutal::foundation::SdkCheckedMemory;
using brutal::instrumentation::Scope;
static unsigned checks=0;
static double raw_seconds=0,checked_seconds=0;static unsigned copy_iterations=0;
static std::atomic<unsigned> observations{0},errors{0},destructors{0},entries{0},rejected{0};
static std::atomic<uint64_t> reads{0},writes{0},completed{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("main oracle "+std::to_string(checks));}
static void test(bool v){++observations;if(!v)throw std::runtime_error("worker oracle "+std::to_string(observations.load()));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state;}
class Thread final:public XThread{
 std::function<void(Thread&)> body_;
 public:
 uint32_t saved_stack=0;
 Thread(KernelState*k,std::function<void(Thread&)>f):XThread(k,65536,0,0,0,1,true),body_(std::move(f)){}
 ~Thread()override{++destructors;}
 void lend(Scope&s){s.lend(stack_alloc_base_,this,stack_limit_,stack_base_-stack_limit_);s.lend(pcr_address_,this,pcr_address_,0x2d8);s.lend(tls_static_address_,this,tls_static_address_,tls_total_size_);}
 void Execute()override{kernel_state()->OnThreadExecute(this);saved_stack=stack_alloc_base_;
  const uint32_t owned[]{stack_alloc_base_,pcr_address_,tls_static_address_};for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,this);
  try{auto&c=*rex::runtime::current_ppc_context();test(c.r13.u32==pcr_ptr()&&!(c.r1.u32&15));c.r1.u64=stack_base_-256;body_(*this);}
  catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;}
  for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,nullptr);Exit(0);
 }
};
static auto file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("fixture file missing");return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});}
static void verify(const std::vector<uint8_t>&bytes,std::span<const unsigned char>expected){std::array<unsigned char,32>sha{};check(bytes.size()==0x41000&&CC_SHA256(bytes.data(),CC_LONG(bytes.size()),sha.data()));check(std::equal(sha.begin(),sha.end(),expected.begin(),expected.end()));}
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path work=argv[1],input=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(work/"empty");
 auto a_bytes=file(input/"a/fixture.xex"),b_bytes=file(input/"b/fixture.xex");verify(a_bytes,image_a_sha);verify(b_bytes,image_b_sha);
 rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();Memory memory(sdk);
 auto*ih=sdk.LookupHeap(0x82000000);auto*vh=sdk.LookupHeapByType(false,4096);auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
 unsigned allocated=0,reclaimed=0,tls_allocated=0,tls_reclaimed=0;
 auto alloc=[&](unsigned n){++allocated;return memory.allocate(n);};auto free=[&](auto&a){check(memory.release(a)==Memory::Result::ok);++reclaimed;};
 auto load=[&](const char*name,const std::vector<uint8_t>&bytes){auto m=object_ref<UserModule>(new UserModule(k));{std::ofstream f(work/"empty"/name,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f));}auto status=m->LoadFromFile(std::string("game:\\")+name);return std::pair{std::move(m),status};};
 auto destroy=[&](object_ref<UserModule>&m,bool was_loaded=true){auto status=m->Unload();check(was_loaded?status==0:(status==0||status==0xc0000001u));check(m->ReleaseHandle());m.reset();};
 auto finish=[&](object_ref<Thread>&t,std::chrono::milliseconds budget=std::chrono::seconds(15)){if(rex::thread::Wait(t->thread(),false,budget)!=rex::thread::WaitResult::kSuccess){std::fprintf(stderr,"JOIN_TIMEOUT: fixture fail-stop before runtime/resource teardown\n");std::_Exit(70);}auto tls=t->tls_ptr(),pcr=t->pcr_ptr();check(!live(sdk,t->saved_stack)&&live(sdk,tls)&&live(sdk,pcr));check(t->ReleaseHandle());t.reset();check(!live(sdk,tls)&&!live(sdk,pcr));++tls_reclaimed;};
 auto bind=[&](object_ref<UserModule>m,bool second){return std::make_unique<XexBinding>(m,second?image_b_info:image_a_info,second?description_b():description_a(),7,*runtime.export_resolver());};
 auto negative=[&](std::string kind){auto bytes=a_bytes;
  auto be=[&](unsigned off,uint32_t value){for(unsigned i=0;i<4;++i)bytes.at(off+i)=uint8_t(value>>(24-8*i));};
  if(kind=="code")bytes[0x11000+20]^=1;
  if(kind=="entry")be(44,0x820100c8);
  if(kind=="sections"){auto n=0x1000+0x98+224+2*40+12;bytes[n]=0;bytes[n+1]=0;bytes[n+2]=1;bytes[n+3]=0;}
  if(kind=="tls")be(0x380,257);
  if(kind=="import")be(0x31000,3);
  if(kind=="initialization")bytes[0x1000]=0;
  auto[m,status]=load("negative.xex",bytes);bool refused=status!=0;
  if(!refused)try{auto binding=bind(m,false);}catch(const std::invalid_argument&){refused=true;}
  check(refused);++rejected;destroy(m,status==0);check(!live(sdk,0x82000000));
 };
 const auto started=std::chrono::steady_clock::now();
 if(mode=="join-failure"){
  auto[a,status]=load("a.xex",a_bytes);check(status==0);auto t=object_ref<Thread>(new Thread(k,[](Thread&){std::this_thread::sleep_for(std::chrono::seconds(20));}));check(t->CreateWithModule(a)==0);check(t->Resume()==0);
  finish(t,std::chrono::milliseconds(10));throw std::runtime_error("join fail-stop was not reached");
 }else if(mode=="negative")for(auto name:{"code","entry","sections","tls","import","initialization"})negative(name);
 else{
  const unsigned cycles=mode=="repeat"?8:1;
  for(unsigned cycle=0;cycle<cycles;++cycle){auto[a,sa]=load("a.xex",a_bytes);auto[b,sb]=load("b.xex",b_bytes);check(sa==0&&sb==0);
   auto ba=bind(a,false),bb=bind(b,true);
   if(mode=="mismatch"){
    bool refused=false;try{XexBinding wrong(a,image_b_info,description_b(),7,*runtime.export_resolver());}catch(const std::invalid_argument&){refused=true;}check(refused);++rejected;
    auto desc=description_a();desc.entry+=4;refused=false;try{XexBinding wrong(a,image_a_info,desc,7,*runtime.export_resolver());}catch(const std::invalid_argument&){refused=true;}check(refused);++rejected;
   }else if(mode=="copy-benchmark"){
    Scope scope(memory,sdk);scope.lend_image(ba->lease());
    constexpr uint32_t address=0x82020200;const volatile uint32_t*source=reinterpret_cast<const volatile uint32_t*>(sdk.virtual_membase()+address);
    uint64_t raw_sum=0,checked_sum=0;copy_iterations=1024;
    auto t0=std::chrono::steady_clock::now();for(unsigned i=0;i<copy_iterations;++i)raw_sum+=__builtin_bswap32(*source);
    auto t1=std::chrono::steady_clock::now();for(unsigned i=0;i<copy_iterations;++i)checked_sum+=brutal::instrumentation::load<uint32_t>(address);
    auto t2=std::chrono::steady_clock::now();raw_seconds=std::chrono::duration<double>(t1-t0).count();checked_seconds=std::chrono::duration<double>(t2-t1).count();
    check(raw_sum==checked_sum&&raw_sum==uint64_t(0x10203040)*copy_iterations&&scope.statistics.reads==copy_iterations&&scope.statistics.completed==copy_iterations);
   }else if(mode=="permissions"){
    std::array<uint8_t,4>bytes{};check(ba->lease().CopyMemory(0x82020200,bytes.data(),4,Memory::Access::read)==Memory::Result::ok);
    check(ba->lease().CopyMemory(0x82020200,bytes.data(),4,Memory::Access::write)==Memory::Result::protection);
    check(ba->lease().CopyMemory(0x82010000,bytes.data(),4,Memory::Access::fetch)==Memory::Result::unsupported_fetch);
    check(ba->lease().CopyMemory(0x8202fffe,bytes.data(),4,Memory::Access::read)==Memory::Result::invalid_range);
    check(ih->CopyOwnedMemory(0x82000000,a->xex_module(),0x82010000,bytes.data(),4,Memory::Access::read)==Memory::Result::unsupported_heap);
    check(ih->CopyOwnedImageMemory(0x82000000,b->xex_module(),0x82010000,bytes.data(),4,Memory::Access::read)==Memory::Result::ownership);
    check(ih->Protect(0x82030000,65536,1));check(ba->lease().CopyMemory(0x82030400,bytes.data(),4,Memory::Access::write)==Memory::Result::protection);check(ih->Protect(0x82030000,65536,3));
   }else{
    const unsigned n=mode=="concurrent"?4:2;std::barrier start(n);std::array<object_ref<Thread>,4>ts;std::array<Memory::Allocation,4>ds,data;
    unsigned pending=0;
    struct JoinGuard{unsigned&pending;~JoinGuard(){if(pending){std::fprintf(stderr,"ACTIVE_THREADS: fixture fail-stop before unsafe unwind\n");std::_Exit(70);}}} guard{pending};
    for(unsigned id=0;id<n;++id){ds[id]=alloc(4096);data[id]=alloc(8192);auto d=ds[id],buffer=data[id];bool second=id&1;auto module=second?b:a;auto*binding=second?bb.get():ba.get();
     ts[id]=object_ref<Thread>(new Thread(k,[&,id,d,buffer,module,binding,second](Thread&self){
      auto&c=*rex::runtime::current_ppc_context();Admission admission(module,*runtime.export_resolver(),sdk);PointerBinding pointer(memory,buffer);
      auto put=[&](unsigned off,uint32_t value){test(memory.store32be(d,off,value)==Memory::Result::ok);};auto get=[&](unsigned off){uint32_t v=0;test(memory.load32be(d,off,v)==Memory::Result::ok);return v;};
      const auto addr=buffer.guest_address();const auto initial=c;start.arrive_and_wait();
      for(unsigned rep=0;rep<(mode=="benchmark"?100:mode=="concurrent"?8:1);++rep){
       for(unsigned i=0;i<4;++i)test(memory.store32be(buffer,4092+i*4,i==1?0x55667788:0x11223344)==Memory::Result::ok);
       put(0,mode=="pointer"?0:addr+4092);put(4,16);put(8,0x11223344);put(12,addr);put(16,addr+256);put(20,mode=="sdk-error"?0xffffffff:0);put(24,mode=="target"?0xdeadbeec:binding->query_target());put(28,addr+512);put(44,0);put(0x60,0x1234abcd);put(0x6c,0x55660000+id);put(0x70,0x11223344);put(0x74,0x55667788);put(0x78,0xaabbccdd);
       std::array<std::byte,16>v{};for(unsigned i=0;i<16;++i)v[i]=std::byte(i);test(memory.write(d,128,v)==Memory::Result::ok);
       Scope scope(memory,sdk);scope.bind(d,4096);scope.bind(buffer,8192);self.lend(scope);GeneratedImports dispatch(admission,c,sdk.virtual_membase());binding->admit(scope,dispatch,pointer);c=initial;c.r3.u64=d.guest_address();bool refused=false;++entries;
       try{binding->enter(scope,c,sdk.virtual_membase());}catch(const Memory::BufferError&){refused=true;}catch(const brutal::instrumentation::Fault&){refused=true;}
       test(refused==(mode=="pointer"||mode=="target"));
       if(refused){++rejected;test(get(44)==0);c=initial;}else{test(c.r1.u64==initial.r1.u64&&c.r2.u64==initial.r2.u64&&c.r13.u64==initial.r13.u64&&c.lr==initial.lr);test(get(36)==3&&get(0x64)==0x1234abcd&&get(0x68)==0x55660000+id);test(c.r3.u32==(mode=="sdk-error"?0xc000000d:0));test(get(44)==(mode=="sdk-error"?0:0x55aa));test(scope.statistics.completed==(mode=="sdk-error"?33:35));}
       std::array<uint8_t,4>image_result{};test(binding->lease().CopyMemory((second?0x83000000:0x82000000)+0x30400,image_result.data(),4,Memory::Access::read)==Memory::Result::ok);test(image_result==std::array<uint8_t,4>{0x10,0x20,0x30,0x40});
       reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
      }
     }));check(ts[id]->CreateWithModule(module)==0);++tls_allocated;++pending;
    }
    for(unsigned id=0;id<n;++id){for(unsigned other=0;other<id;++other)check(ts[id]->tls_ptr()!=ts[other]->tls_ptr()&&ts[id]->pcr_ptr()!=ts[other]->pcr_ptr());check(ts[id]->Resume()==0);}for(unsigned id=0;id<n;++id){finish(ts[id]);--pending;free(ds[id]);free(data[id]);}check(!errors);
   }
   ba.reset();bb.reset();destroy(a);destroy(b);check(!live(sdk,0x82000000)&&!live(sdk,0x83000000));
  }
 }
 auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&allocated==reclaimed&&tls_allocated==tls_reclaimed&&destructors==tls_reclaimed);check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
 auto elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count();
 std::printf("{\"group\":\"xex_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"entry_invocations\":%u,\"controlled_rejections\":%u,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"tls_allocations\":%u,\"tls_reclaims\":%u,\"buffer_allocations\":%u,\"buffer_reclaims\":%u,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"sdk_returns\":%llu,\"copy_iterations\":%u,\"raw_copy_seconds\":%.9f,\"checked_copy_seconds\":%.9f,\"measured_seconds\":%.9f}\n",mode.c_str(),checks,observations.load(),entries.load(),rejected.load(),(unsigned long long)reads.load(),(unsigned long long)writes.load(),(unsigned long long)completed.load(),tls_allocated,tls_reclaimed,allocated,reclaimed,(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims,(unsigned long long)stats.sdk_returns,copy_iterations,raw_seconds,checked_seconds,elapsed);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
