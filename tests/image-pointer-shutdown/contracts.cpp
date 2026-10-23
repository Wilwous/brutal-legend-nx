// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/synthetic_xex_binding.h"
#include "binding-data.h"
#include "runtime/cooperative_stop.h"
#include <condition_variable>
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

static std::atomic<unsigned> observations{0},errors{0},destructors{0},entries{0},rejected{0};
static std::atomic<uint64_t> reads{0},writes{0},completed{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("main oracle "+std::to_string(checks));}
static void test(bool v){++observations;if(!v)throw std::runtime_error("worker oracle "+std::to_string(observations.load()));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state;}
class Thread final:public XThread{
 std::function<void(Thread&)> body_;
 public:
 uint32_t saved_stack=0;
 brutal::synthetic::CooperativeStop* stop=nullptr;
 Thread(KernelState*k,std::function<void(Thread&)>f):XThread(k,65536,0,0,0,1,true),body_(std::move(f)){}
 ~Thread()override{++destructors;}
 void lend(Scope&s){s.lend(stack_alloc_base_,this,stack_limit_,stack_base_-stack_limit_);s.lend(pcr_address_,this,pcr_address_,0x2d8);s.lend(tls_static_address_,this,tls_static_address_,tls_total_size_);}
 void Execute()override{kernel_state()->OnThreadExecute(this);saved_stack=stack_alloc_base_;
  const uint32_t owned[]{stack_alloc_base_,pcr_address_,tls_static_address_};for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,this);
  try{auto&c=*rex::runtime::current_ppc_context();test(c.r13.u32==pcr_ptr()&&!(c.r1.u32&15));c.r1.u64=stack_base_-256;body_(*this);}
  catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;if(stop){stop->request();stop->safe_boundary();}}
  for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,nullptr);if(stop)stop->exited();Exit(0);
 }
};
static auto file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("fixture file missing");return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});}
static void verify(const std::vector<uint8_t>&bytes,std::span<const unsigned char>expected){std::array<unsigned char,32>sha{};check(bytes.size()==0x41000&&CC_SHA256(bytes.data(),CC_LONG(bytes.size()),sha.data()));check(std::equal(sha.begin(),sha.end(),expected.begin(),expected.end()));}
struct Gate {
 std::mutex mutex;std::condition_variable cv;bool entered=false,proceed=false;
 void pause(){std::unique_lock lock(mutex);entered=true;cv.notify_all();if(!cv.wait_for(lock,std::chrono::seconds(10),[&]{return proceed;}))throw std::runtime_error("observer watchdog");}
 void wait(){std::unique_lock lock(mutex);if(!cv.wait_for(lock,std::chrono::seconds(10),[&]{return entered;}))throw std::runtime_error("host rendezvous watchdog");}
 void release(){std::lock_guard lock(mutex);proceed=true;cv.notify_all();}
};
using Stop=brutal::synthetic::CooperativeStop;
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path work=argv[1],input=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(work/"empty");
 auto bytes=file(input/"a/fixture.xex");verify(bytes,image_a_sha);
 rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();Memory memory(sdk);
 auto*ih=sdk.LookupHeap(0x82000000);auto*vh=sdk.LookupHeapByType(false,4096);auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
 unsigned buffers=0,buffer_reclaims=0,tls_allocated=0,tls_reclaimed=0,unloads=0;std::atomic<unsigned> stops{0};
 Stop order;check(!order.safe_boundary());bool invalid_order=false;try{order.joined();}catch(const std::logic_error&){invalid_order=true;}check(invalid_order&&order.state()==Stop::State::running);
 auto started=std::chrono::steady_clock::now();
 auto load=[&](bool invalid=false){auto m=object_ref<UserModule>(new UserModule(k));auto data=bytes;if(invalid)data[0x1000]=0;{std::ofstream f(work/"empty/a.xex",std::ios::binary);f.write(reinterpret_cast<const char*>(data.data()),data.size());check(bool(f));}auto status=m->LoadFromFile("game:\\a.xex");return std::pair{std::move(m),status};};
 auto finish=[&](object_ref<Thread>&t,Stop*stop=nullptr,std::chrono::milliseconds budget=std::chrono::seconds(15)){
  if(rex::thread::Wait(t->thread(),false,budget)!=rex::thread::WaitResult::kSuccess){std::fprintf(stderr,"JOIN_TIMEOUT: fixture fail-stop before runtime/resource teardown\n");std::_Exit(70);}
  if(stop){check(stop->state()==Stop::State::exited);stop->joined();}
  auto tls=t->tls_ptr(),pcr=t->pcr_ptr();check(!live(sdk,t->saved_stack)&&live(sdk,tls)&&live(sdk,pcr));check(t->ReleaseHandle());t.reset();check(!live(sdk,tls)&&!live(sdk,pcr));++tls_reclaimed;if(stop)stop->reclaimed();
 };
 if(mode=="join-failure"){
  auto[m,status]=load();check(status==0);auto t=object_ref<Thread>(new Thread(k,[](Thread&){std::this_thread::sleep_for(std::chrono::seconds(20));}));check(t->CreateWithModule(m)==0);check(t->Resume()==0);finish(t,nullptr,std::chrono::milliseconds(10));throw std::runtime_error("join fail-stop missed");
 }
 const unsigned cycles=mode=="repeat"?8:1;
 for(unsigned cycle=0;cycle<cycles;++cycle){
  auto[m,status]=load(mode=="initialization");
  if(mode=="initialization"){check(status!=0);auto failed=m->Unload();check(failed==0||failed==0xc0000001u);check(m->ReleaseHandle());m.reset();check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);++rejected;continue;}
  check(status==0);
  auto binding=std::make_unique<XexBinding>(m,image_a_info,description_a(),7,*runtime.export_resolver());
  auto copy=[&](uint32_t address,std::span<std::byte> span,bool write){check(binding->lease().CopyMemory(address,span.data(),span.size(),write?Memory::Access::write:Memory::Access::read)==Memory::Result::ok);};
  auto put_image=[&](uint32_t address,uint32_t value){std::array<std::byte,4> b{std::byte(value>>24),std::byte(value>>16),std::byte(value>>8),std::byte(value)};copy(address,b,true);};
  const unsigned n=mode=="concurrent"?4:1;
  std::array<object_ref<Thread>,4> threads;std::array<Memory::Allocation,4> descriptors;std::array<Stop,4> stop;Gate gate;std::barrier start(n);
  unsigned pending=0;
  struct JoinGuard {unsigned&pending;~JoinGuard(){if(pending){std::fprintf(stderr,"ACTIVE_THREADS: fail-stop before unsafe unwind\n");std::_Exit(70);}}} guard{pending};
  for(unsigned id=0;id<n;++id){
   descriptors[id]=memory.allocate(4096);++buffers;auto d=descriptors[id];uint32_t data=0x82031000+id*0x2000;
   for(unsigned i=0;i<4;++i)put_image(data+4092+i*4,i==1?0x55667788:0x11223344);
   // Known marker bytes make rollback and selected-field writes observable.
   std::array<std::byte,28> initial{};initial.fill(std::byte{0xa5});copy(data+256,initial,true);copy(data+512,initial,true);
   threads[id]=object_ref<Thread>(new Thread(k,[&,id,d,data](Thread&self){
    auto&c=*rex::runtime::current_ppc_context();const auto initial_context=c;Admission admission(m,*runtime.export_resolver(),sdk);
    unsigned sdk_seen=0,scratch_seen=0;bool injected=false;
    PointerBinding pointer(PointerBinding::Image{},memory,m,[&](PointerBinding::Stage stage){
     if(stage==PointerBinding::Stage::scratch_ready){++scratch_seen;if(mode=="stop-adapter"&&scratch_seen==1)gate.pause();}
     if(stage==PointerBinding::Stage::validated&&mode=="reentrant"){
      bool denied=false;try{admission.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,pointer);}catch(const std::logic_error&){denied=true;}
      test(denied);++rejected;test(memory.try_release(d)==Memory::ReleaseAttempt::busy);
     }
     if(stage==PointerBinding::Stage::sdk_returned){++sdk_seen;
      if((mode=="stop-import"&&sdk_seen==1)||(mode=="unload-active"&&sdk_seen==3))gate.pause();
      if(mode=="callback"&&sdk_seen==3){injected=true;throw std::runtime_error("original observer failure");}
      if(mode=="rollback"&&sdk_seen==3){test(ih->Protect(0x82030000,65536,1));injected=true;}
     }
    });
    auto put=[&](unsigned off,uint32_t value){test(memory.store32be(d,off,value)==Memory::Result::ok);};
    auto get=[&](unsigned off){uint32_t value=0;test(memory.load32be(d,off,value)==Memory::Result::ok);return value;};
    auto entry=[&](uint64_t compare,uint32_t length,uint32_t query_out,uint32_t critical,uint32_t query_type=0,bool refuse=false){
     put(0,uint32_t(compare));put(4,length);put(8,compare==0x82020200?0x10203040:0x11223344);put(12,data);put(16,query_out);put(20,query_type);put(24,binding->query_target());put(28,critical);put(44,0);put(0x60,0x1234abcd);put(0x6c,0x55660000+id);put(0x70,0x11223344);put(0x74,0x55667788);put(0x78,0xaabbccdd);
     std::array<std::byte,16> vector{};for(unsigned i=0;i<16;++i)vector[i]=std::byte(i);test(memory.write(d,128,vector)==Memory::Result::ok);
     Scope scope(memory,sdk);scope.bind(d,4096);self.lend(scope);GeneratedImports dispatch(admission,c,sdk.virtual_membase());binding->admit(scope,dispatch,pointer);c=initial_context;c.r3.u64=d.guest_address();++entries;bool denied=false;
     try{binding->enter(scope,c,sdk.virtual_membase());}catch(const Memory::BufferError&){denied=true;}catch(const std::invalid_argument&){denied=true;}catch(const std::runtime_error&e){if(mode!="callback"||std::string_view(e.what())!="original observer failure")throw;denied=true;}
     test(denied==refuse);if(denied){++rejected;test(get(44)==0);c=initial_context;}
     else{test(c.r1.u64==initial_context.r1.u64&&c.r2.u64==initial_context.r2.u64&&c.r13.u64==initial_context.r13.u64&&c.lr==initial_context.lr);test(get(36)==(compare==0x82020200?1:3));test(get(0x64)==0x1234abcd&&get(0x68)==0x55660000+id);test(c.r3.u32==(query_type==0xffffffff?0xc000000d:0));test(get(44)==(query_type==0xffffffff?0:0x55aa));test(scope.statistics.completed==(query_type==0xffffffff?33:35));
      if(query_type==0){
       std::array<std::byte,28> out{};test(binding->lease().CopyMemory(critical,out.data(),28,Memory::Access::read)==Memory::Result::ok);
       test(out[0]==std::byte{1}&&out[1]==std::byte{0}&&out[2]==std::byte{0xa5}&&out[3]==std::byte{0xa5});
       auto word=[&](unsigned off){uint32_t value=0;for(unsigned i=0;i<4;++i)value=value<<8|std::to_integer<uint32_t>(out[off+i]);return value;};
       test(word(4)==0&&word(8)==0xa5a5a5a5&&word(12)==0xa5a5a5a5&&word(16)==0xffffffff&&word(20)==0&&word(24)==0);
       test(binding->lease().CopyMemory(query_out,out.data(),28,Memory::Access::read)==Memory::Result::ok);
       test(word(0)==data&&word(4)==0x82000000&&word(8)==4&&word(12)==65536&&word(16)==0x1000&&word(20)==4&&word(24)==0x20000);
      }
     }
     reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
    };
    start.arrive_and_wait();
    if(mode=="boundaries"){
     entry(0x82020200,4,data+256,data+512); // Read-only IN succeeds.
     entry(0x8202fffc,8,data+256,data+512,0,true); // Two PE sections.
     entry(0x8203fffc,8,data+256,data+512,0,true); // End of image.
     entry(data+1,4,data+256,data+512,0,true); // Alignment.
     entry(data,65540,data+256,data+512,0,true); // Bounded snapshot.
     entry(data,0,data+256,data+512,0,true);
     entry(data,0xfffffffc,data+256,data+512,0,true);
     entry(0,4,data+256,data+512,0,true);
     for(unsigned bad=0;bad<5;++bad){
      c=initial_context;c.r3.u64=data;c.r4.u64=4;c.r5.u64=0x11223344;
      if(bad==0)c.r3.u64+=0x100000000ull;
      if(bad==1)c.r4.u64+=0x100000000ull;
      if(bad==2)c.r5.u64+=0x100000000ull;
      if(bad==3)c.r3.u64=0xfffffffc,c.r4.u64=8;
      if(bad==4)c.r4.u64=3;
      auto saved=c;bool denied=false;try{admission.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,pointer);}catch(const std::exception&){denied=true;}test(denied&&std::memcmp(&saved,&c,sizeof c)==0);++rejected;
     }
     // Exact end of a section is accepted, one word beyond is rejected above.
     c=initial_context;c.r3.u64=0x8202fffc;c.r4.u64=4;c.r5.u64=0;
     admission.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,pointer);test(c.r3.u32==1);
     c=initial_context;c.r3.u64=data;c.r4.u64=data+256;c.r5.u64=0xffffffff;
     std::array<std::byte,28> before{},after{};test(binding->lease().CopyMemory(data+256,before.data(),28,Memory::Access::read)==Memory::Result::ok);
     admission.direct("xboxkrnl.exe",0xee,Signature::u32_address_out28_u32,c,pointer);
     test(binding->lease().CopyMemory(data+256,after.data(),28,Memory::Access::read)==Memory::Result::ok);test(c.r3.u32==0xc000000d&&before==after);
     c=initial_context;c.r3.u64=0x83020200;c.r4.u64=data+256;c.r5.u64=0;auto saved=c;bool denied=false;
     try{admission.direct("xboxkrnl.exe",0xee,Signature::u32_address_out28_u32,c,pointer);}catch(const std::invalid_argument&){denied=true;}test(denied&&std::memcmp(&saved,&c,sizeof c)==0);++rejected;
    }else if(mode=="permissions"){
     entry(data+4092,16,0x82020200,data+512,0,true);
     entry(data+4092,16,data+256,0x82020200,0,true);
     test(ih->Protect(0x82030000,65536,1));
     c=initial_context;c.r3.u64=data+512;bool denied=false;
     try{admission.direct("xboxkrnl.exe",0x12e,Signature::void_inout28,c,pointer);}catch(const Memory::BufferError&e){denied=e.result==Memory::Result::protection;}
     test(denied);++rejected;test(ih->Protect(0x82030000,65536,3));
    }else if(mode=="stale"){
     auto other=object_ref<UserModule>(new UserModule(k));PointerBinding foreign(PointerBinding::Image{},memory,other);bool denied=false;c=initial_context;c.r3.u64=data;c.r4.u64=4;c.r5.u64=0x11223344;
     try{admission.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,foreign);}catch(const std::invalid_argument&){denied=true;}test(denied);++rejected;test(other->ReleaseHandle());other.reset();
     entry(0x83020200,4,data+256,data+512,0,true);
    }else if(mode=="rollback"){
     entry(data+4092,16,data+256,data+512,0,true);test(injected);test(ih->Protect(0x82030000,65536,3));
     std::array<std::byte,28> after{};test(binding->lease().CopyMemory(data+512,after.data(),28,Memory::Access::read)==Memory::Result::ok);test(std::all_of(after.begin(),after.end(),[](auto b){return b==std::byte{0xa5};}));
    }else{
     for(unsigned rep=0;rep<(mode=="concurrent"?8:1);++rep)entry(data+4092,16,data+256,data+512,0,mode=="callback");
     if(mode=="callback"){
      test(injected);std::array<std::byte,28> after{};test(binding->lease().CopyMemory(data+512,after.data(),28,Memory::Access::read)==Memory::Result::ok);test(std::all_of(after.begin(),after.end(),[](auto b){return b==std::byte{0xa5};}));
     }
    }
    // A cooperative request never interrupts generated C++ or an SDK import.
    if(stop[id].state()==Stop::State::running)test(stop[id].request());
    test(stop[id].safe_boundary());test(!stop[id].safe_boundary());test(!stop[id].request());++stops;
   }));check(threads[id]->CreateWithModule(m)==0);++tls_allocated;++pending;
  }
  for(unsigned id=0;id<n;++id){for(unsigned other=0;other<id;++other)check(threads[id]->tls_ptr()!=threads[other]->tls_ptr()&&threads[id]->pcr_ptr()!=threads[other]->pcr_ptr());threads[id]->stop=&stop[id];check(threads[id]->Resume()==0);}
  if(mode=="stop-import"||mode=="stop-adapter"||mode=="unload-active"){
   gate.wait();check(stop[0].request());check(stop[0].state()==Stop::State::requested);check(live(sdk,threads[0]->tls_ptr())&&live(sdk,threads[0]->pcr_ptr())&&live(sdk,0x82000000));
   auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims+1&&stats.image_leases==stats.image_reclaims+1);
   if(mode=="unload-active"){
    check(m->Unload()==0xc0000001u);check(!m->AcquireImageLease());check(live(sdk,0x82000000));
    PointerBinding closed_pointer(PointerBinding::Image{},memory,m);Admission closed(m,*runtime.export_resolver(),sdk);PPCContext c{};bool denied=false;try{closed.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,closed_pointer);}catch(const std::logic_error&){denied=true;}check(denied);++rejected;
   }
   gate.release();
  }
  for(unsigned id=0;id<n;++id){finish(threads[id],&stop[id]);--pending;check(memory.release(descriptors[id])==Memory::Result::ok);++buffer_reclaims;}
  check(!errors);
  std::unique_ptr<PointerBinding> retained;
  if(mode!="unload-active")retained=std::make_unique<PointerBinding>(PointerBinding::Image{},memory,m);
  // Retained image lease is still readable after admission closes and join.
  check(m->Unload()==0xc0000001u);check(!m->AcquireImageLease());std::array<std::byte,4> value{};copy(0x82020200,value,false);check(value==std::array<std::byte,4>{std::byte{0x10},std::byte{0x20},std::byte{0x30},std::byte{0x40}});
  PointerBinding stale(PointerBinding::Image{},memory,m);Admission closed(m,*runtime.export_resolver(),sdk);PPCContext context{};bool denied=false;try{closed.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,context,stale);}catch(const std::logic_error&){denied=true;}check(denied);++rejected;
  binding.reset();if(retained){check(m->Unload()==0xc0000001u&&live(sdk,0x82000000));retained.reset();}check(m->Unload()==0);++unloads;check(!live(sdk,0x82000000)&&!m->AcquireImageLease());denied=false;try{closed.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,context,stale);}catch(const std::logic_error&){denied=true;}check(denied);++rejected;
  for(unsigned id=0;id<n;++id){stop[id].unloaded();check(stop[id].state()==Stop::State::unloaded);}
  check(m->ReleaseHandle());m.reset();
 }
 auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&stats.image_leases==stats.image_reclaims&&stats.image_capabilities==stats.image_capability_reclaims&&buffers==buffer_reclaims&&tls_allocated==tls_reclaimed&&destructors==tls_reclaimed);
 check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
 std::printf("{\"group\":\"image_shutdown_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"entry_invocations\":%u,\"controlled_rejections\":%u,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"tls_allocations\":%u,\"tls_reclaims\":%u,\"buffer_allocations\":%u,\"buffer_reclaims\":%u,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"sdk_returns\":%llu,\"image_leases\":%llu,\"image_reclaims\":%llu,\"image_capabilities\":%llu,\"image_capability_reclaims\":%llu,\"cooperative_stops\":%u,\"module_unloads\":%u,\"measured_seconds\":%.9f}\n",mode.c_str(),checks,observations.load(),entries.load(),rejected.load(),(unsigned long long)reads.load(),(unsigned long long)writes.load(),(unsigned long long)completed.load(),tls_allocated,tls_reclaimed,buffers,buffer_reclaims,(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims,(unsigned long long)stats.sdk_returns,(unsigned long long)stats.image_leases,(unsigned long long)stats.image_reclaims,(unsigned long long)stats.image_capabilities,(unsigned long long)stats.image_capability_reclaims,stops.load(),unloads,std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
