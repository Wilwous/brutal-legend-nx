// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/synthetic_xex_binding.h"
#include "runtime/generated_stop.h"
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
static std::atomic<uint64_t> reads{0},writes{0},completed{0},polls{0},latency_ns{0};
static std::array<std::atomic<uint64_t>,6> edge_hits{};
static std::atomic<unsigned> generated_stops{0},natural_returns{0},host_errors{0},requests_won{0};
static std::vector<double> plain_times,checked_times;
static uint64_t now_ns(){return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static void maximum(std::atomic<uint64_t>& target,uint64_t value){auto old=target.load();while(old<value&&!target.compare_exchange_weak(old,value)){} }

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
using GeneratedStop=brutal::synthetic::GeneratedStop;
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path work=argv[1],input=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(work/"empty");
 auto ab=file(input/"a/fixture.xex"),bb=file(input/"b/fixture.xex");verify(ab,a_sha);verify(bb,b_sha);
 rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();Memory memory(sdk);
 auto*ih=sdk.LookupHeap(0x82000000);auto*vh=sdk.LookupHeapByType(false,4096);const auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
 unsigned allocated=0,reclaimed=0,tls_allocated=0,tls_reclaimed=0,unloads=0;
 auto started=std::chrono::steady_clock::now();
 auto load=[&](const char*name,const std::vector<uint8_t>&bytes){auto m=object_ref<UserModule>(new UserModule(k));{std::ofstream f(work/"empty"/name,std::ios::binary);f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(f));}auto status=m->LoadFromFile(std::string("game:\\")+name);return std::pair{std::move(m),status};};
 auto finish=[&](object_ref<Thread>&t,Stop*stop=nullptr,std::chrono::milliseconds budget=std::chrono::seconds(15)){
  if(rex::thread::Wait(t->thread(),false,budget)!=rex::thread::WaitResult::kSuccess){std::fprintf(stderr,"JOIN_TIMEOUT: bounded process fail-stop, not cooperative completion\n");std::_Exit(70);}
  if(stop){check(stop->state()==Stop::State::exited);stop->joined();}
  auto tls=t->tls_ptr(),pcr=t->pcr_ptr();check(!live(sdk,t->saved_stack)&&live(sdk,tls)&&live(sdk,pcr));check(t->ReleaseHandle());t.reset();check(!live(sdk,tls)&&!live(sdk,pcr));++tls_reclaimed;if(stop)stop->reclaimed();
 };
 if(mode=="join-failure"){
  auto[m,status]=load("a.xex",ab);check(status==0);auto t=object_ref<Thread>(new Thread(k,[](Thread&){std::this_thread::sleep_for(std::chrono::seconds(20));}));check(t->CreateWithModule(m)==0);check(t->Resume()==0);finish(t,nullptr,std::chrono::milliseconds(10));throw std::runtime_error("join fail-stop missed");
 }
 const unsigned cycles=mode=="repeat"?8:1;
 for(unsigned cycle=0;cycle<cycles;++cycle){
  if(mode=="initialization"){
   auto invalid=ab;invalid[0x1000]=0;auto[m,status]=load("invalid.xex",invalid);check(status!=0);auto u=m->Unload();check(u==0||u==0xc0000001u);check(m->ReleaseHandle());m.reset();++rejected;continue;
  }
  auto[a,sa]=load("a.xex",ab);auto[b,sb]=load("b.xex",bb);check(sa==0&&sb==0);
  auto binding=std::make_unique<XexBinding>(a,image_a_info,description_a(),13,*runtime.export_resolver());auto plain=std::make_unique<XexBinding>(b,image_b_info,description_b(),13,*runtime.export_resolver());
  if(mode=="policy"){
   auto invalid=description_a();auto pcs=a_functions;pcs[0]=pcs[1];invalid.functions=pcs;bool denied=false;try{XexBinding wrong(a,image_a_info,invalid,13,*runtime.export_resolver());}catch(const std::invalid_argument&){denied=true;}check(denied);++rejected;
  }
  const unsigned n=mode=="concurrent"?4:1;
  std::array<object_ref<Thread>,4>threads;std::array<Memory::Allocation,4>data;std::array<Stop,4>stop;std::array<Gate,4>gate;std::array<std::atomic<uint64_t>,4>requested{};
  unsigned pending=0;struct JoinGuard{unsigned&pending;~JoinGuard(){if(pending){std::fprintf(stderr,"ACTIVE_THREADS: fail-stop before unsafe resource unwind\n");std::_Exit(70);}}} guard{pending};
  for(unsigned id=0;id<n;++id){
   data[id]=memory.allocate(4096);++allocated;auto d=data[id];const uint32_t image_data=0x82031000+id*0x2000;
   threads[id]=object_ref<Thread>(new Thread(k,[&,id,d,image_data](Thread&self){
    auto&c=*rex::runtime::current_ppc_context();const PPCContext initial=c;unsigned sdk_seen=0;bool rendezvous=false;
    auto put=[&](unsigned off,uint32_t value){test(memory.store32be(d,off,value)==Memory::Result::ok);};
    auto get=[&](unsigned off){uint32_t value=0;test(memory.load32be(d,off,value)==Memory::Result::ok);return value;};
    auto image_write=[&](uint32_t at,std::span<std::byte> bytes){test(binding->lease().CopyMemory(at,bytes.data(),bytes.size(),Memory::Access::write)==Memory::Result::ok);};
    for(unsigned i=0;i<4;++i){const auto v=i==1?0x55667788:0x11223344;std::array<std::byte,4>bytes{std::byte(v>>24),std::byte(v>>16),std::byte(v>>8),std::byte(v)};image_write(image_data+4092+i*4,bytes);}
    std::array<std::byte,28> marker{};marker.fill(std::byte{0xa5});image_write(image_data+256,marker);image_write(image_data+512,marker);
    put(0,image_data+4092);put(4,16);put(8,0x11223344);put(12,image_data);put(16,image_data+256);put(20,0);put(24,binding->query_target());put(28,image_data+512);put(44,0);put(0x60,0x1234abcd);put(0x6c,0x55660000+id);put(0x70,0x11223344);put(0x74,0x55667788);put(0x78,0xaabbccdd);put(0xb0,2);
    std::array<std::byte,16> vector{};for(unsigned i=0;i<16;++i)vector[i]=std::byte(i);test(memory.write(d,128,vector)==Memory::Result::ok);
    const bool cancelling=mode.ends_with("-stop")||mode=="unload-import"||mode=="concurrent"||mode=="repeat"||mode=="latency";
    uint32_t selected=a_backedges[0];
    if(mode=="infinite-stop"||mode=="concurrent"||mode=="repeat"||mode=="latency")selected=a_backedges[1];
    if(mode=="nested-stop")selected=a_backedges[2];if(mode=="memory-stop")selected=a_backedges[4];if(mode=="long-stop")selected=a_long+64;if(mode=="entry-stop")selected=a_finite;
    auto run=[&](uint32_t pc,uint32_t argument,uint32_t address,bool reference=false,bool expect_stop=false,bool expect_error=false,bool measure=false){
     c=initial;c.r3.u64=argument;c.r4.u64=address;
     Scope scope(memory,sdk);scope.bind(d,4096);self.lend(scope);
     Admission admission(reference?b:a,*runtime.export_resolver(),sdk);
     PointerBinding pointer(PointerBinding::Image{},memory,reference?b:a,[&](PointerBinding::Stage stage){
      if(stage==PointerBinding::Stage::sdk_returned){++sdk_seen;if(mode=="callback"&&sdk_seen==2)throw std::runtime_error("original callback error");}
      if(!rendezvous&&((mode=="adapter-stop"&&stage==PointerBinding::Stage::scratch_ready)||((mode=="import-stop"||mode=="unload-import")&&stage==PointerBinding::Stage::sdk_returned&&sdk_seen==2))){rendezvous=true;gate[id].pause();}
     });
     GeneratedImports dispatch(admission,c,sdk.virtual_membase());
     auto lease=(reference?b:a)->AcquireImageLease();test(bool(lease));
     if(reference||mode=="benchmark"){
      // The sole unhooked admission is this finite, count-bounded memory loop.
      const auto&info=reference?image_b_info:image_a_info;const auto expected=reference?b_memory:a_memory;
      test(pc==expected&&argument<=200);scope.lend_image(reference?plain->lease():binding->lease());
      auto fn=std::find_if(info.func_mappings,info.func_mappings+12,[&](auto fn){return fn.guest==expected;});test(fn!=info.func_mappings+12);scope.allow_synthetic(expected,fn->host);
     }else binding->admit(scope,dispatch,pointer);
     unsigned progress=0;
     GeneratedStop::Observer observer;
     if(mode!="benchmark")observer=[&](uint32_t at){
      if(!rendezvous&&mode=="latency"&&at==a_backedges[1]&&++progress==128){std::lock_guard lock(gate[id].mutex);rendezvous=true;gate[id].entered=true;gate[id].cv.notify_all();}
      else if(!rendezvous&&cancelling&&mode!="import-stop"&&mode!="adapter-stop"&&mode!="unload-import"&&mode!="latency"&&at==selected){rendezvous=true;gate[id].pause();}
     };
     GeneratedStop generated(stop[id],c,a_sites,std::move(observer));
     if(mode=="policy"){
      bool denied=false;try{GeneratedStop invalid(stop[id],c,{});}catch(const std::invalid_argument&){denied=true;}test(denied);++rejected;
      denied=false;try{GeneratedStop::poll(0xdeadbeec);}catch(const std::logic_error&){denied=true;}test(denied);++rejected;
     }
     bool stopped=false,failed=false;++entries;const auto execution_begin=now_ns();
     try{if(pc==a_finite&&!reference)binding->enter(scope,c,sdk.virtual_membase());else scope.indirect(pc,c,sdk.virtual_membase());++natural_returns;}
     catch(const GeneratedStop::Stopped& signal){test(generated.owns(signal));test(stop[id].state()==Stop::State::boundary);test(requested[id].load()!=0);maximum(latency_ns,now_ns()-requested[id].load());stopped=true;++generated_stops;c=initial;}
     catch(const brutal::instrumentation::Fault&){if(mode!="error")throw;failed=true;++host_errors;c=initial;}
     catch(const std::runtime_error&e){if(mode!="callback"||std::string_view(e.what())!="original callback error")throw;failed=true;++host_errors;c=initial;}
     if(measure)(reference?plain_times:checked_times).push_back(double(now_ns()-execution_begin)/1e9);
     test(stopped==expect_stop&&failed==expect_error);
     test(c.r1.u64==initial.r1.u64&&c.r2.u64==initial.r2.u64&&c.r13.u64==initial.r13.u64&&c.lr==initial.lr);
     if(!stopped&&!failed){
      if(pc==a_finite)test(c.r3.u32==argument);
      if(pc==a_nested)test(c.r3.u32==argument*3);
      if(pc==a_memory||pc==b_memory)test(c.r3.u32==argument&&get(0x200)==argument);
      if(pc==a_long)test(c.r3.u32==argument+64);
      if(pc==a_imports||pc==description_a().code_base+196)test(c.r3.u32==0&&get(44)==0x55aa&&get(36)==3&&get(0x68)==0x55660000+id&&c.r14.u64==initial.r14.u64&&c.r15.u64==initial.r15.u64);
     }
     uint64_t hits_sum=0;for(auto hits:generated.hits)hits_sum+=hits;
     test(reference?hits_sum==0:hits_sum>0);
     if(mode=="benchmark")test(scope.statistics.reads==argument&&scope.statistics.writes==argument&&scope.statistics.completed==2*argument&&(reference?hits_sum==0:hits_sum==3*argument+1));
     polls+=hits_sum;
     for(unsigned i=0;i<6;++i){auto at=std::lower_bound(a_sites.begin(),a_sites.end(),a_backedges[i]);edge_hits[i]+=generated.hits[size_t(at-a_sites.begin())];}
     reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
     auto stats=PointerBinding::statistics();test(stats.scratch_allocations==stats.scratch_reclaims&&stats.image_leases==stats.image_reclaims);
     if(mode=="unload-import"){test(sdk_seen==2);PPCContext before=c;bool denied=false;try{admission.direct("xboxkrnl.exe",0x11b,Signature::u32_in_words_u32_u32,c,pointer);}catch(const std::logic_error&){denied=true;}test(denied&&std::memcmp(&before,&c,sizeof c)==0);++rejected;}
     if(mode=="callback"){std::array<std::byte,28>after{};test(binding->lease().CopyMemory(image_data+256,after.data(),28,Memory::Access::read)==Memory::Result::ok);test(after==marker);}
    };
    if(mode=="natural"){
     run(a_finite,10,0);run(a_finite,0,0);run(a_nested,4,0);put(0x200,0);run(a_memory,8,d.guest_address()+0x200);run(a_imports,d.guest_address(),0);run(a_long,3,0);run(description_a().code_base+196,d.guest_address(),0);
    }else if(mode=="benchmark"){
     for(unsigned pair=0;pair<5;++pair){for(unsigned step=0;step<2;++step){const bool ref=(pair+step)&1;put(0x200,0);run(ref?b_memory:a_memory,200,d.guest_address()+0x200,ref,false,false,true);}}
    }else if(mode=="request-race"){
     // A request may arrive while poll observes running. Both legal schedules
     // must converge at the next poll, without a spurious terminal-state error.
     for(unsigned iteration=0;iteration<256;++iteration){
      Stop local;std::atomic<bool>entered{false},won{false};
      GeneratedStop probe(local,c,a_sites,[&](uint32_t){entered.store(true);});
      std::jthread requester([&]{while(!entered.load())std::this_thread::yield();won=local.request();});
      bool caught=false;
      try{GeneratedStop::poll(a_sites[0]);}catch(const GeneratedStop::Stopped&signal){test(probe.owns(signal));caught=true;}
      requester.join();test(won.load());
      if(!caught){try{GeneratedStop::poll(a_sites[0]);}catch(const GeneratedStop::Stopped&signal){test(probe.owns(signal));caught=true;}}
      test(caught&&local.state()==Stop::State::boundary);
      bool terminal=false;try{GeneratedStop::poll(a_sites[0]);}catch(const std::logic_error&){terminal=true;}
      test(terminal);++rejected;
     }
    }else if(mode=="error")run(a_memory,2,0,false,false,true);
    else if(mode=="callback")run(a_imports,d.guest_address(),0,false,false,true);
    else if(mode=="policy")run(a_finite,2,0);
    else if(mode=="import-stop"||mode=="adapter-stop"||mode=="unload-import")run(a_imports,d.guest_address(),0,false,true);
    else{
     uint32_t pc=a_finite,arg=500000000,address=0;
     if(mode=="infinite-stop"||mode=="concurrent"||mode=="repeat"||mode=="latency")pc=a_infinite;
     if(mode=="nested-stop")pc=a_nested;if(mode=="long-stop")pc=a_long;
     if(mode=="memory-stop"){pc=a_memory;address=d.guest_address()+0x200;put(0x200,0);}
     run(pc,arg,address,false,true);
    }
    if(stop[id].state()==Stop::State::running){test(stop[id].request());test(stop[id].safe_boundary());}
    test(stop[id].state()==Stop::State::boundary);test(!stop[id].request());test(!stop[id].safe_boundary());
   }));check(threads[id]->CreateWithModule(a)==0);threads[id]->stop=&stop[id];++tls_allocated;++pending;
  }
  for(unsigned id=0;id<n;++id){for(unsigned other=0;other<id;++other)check(threads[id]->tls_ptr()!=threads[other]->tls_ptr()&&threads[id]->pcr_ptr()!=threads[other]->pcr_ptr());check(threads[id]->Resume()==0);}
  const bool cancelling=mode.ends_with("-stop")||mode=="unload-import"||mode=="concurrent"||mode=="repeat"||mode=="latency";
  if(cancelling){
   for(unsigned id=0;id<n;++id){gate[id].wait();check(live(sdk,threads[id]->tls_ptr())&&live(sdk,threads[id]->pcr_ptr())&&live(sdk,0x82000000));}
   if(mode=="unload-import"||mode=="concurrent"){
    check(a->Unload()==0xc0000001u);check(!a->AcquireImageLease());check(live(sdk,0x82000000));
   }
   auto request=[&](unsigned id){
    // First request invocation, after every readiness rendezvous. Publish the
    // timestamp before any caller can publish the requested state.
    uint64_t empty=0;requested[id].compare_exchange_strong(empty,now_ns());
    const bool won=stop[id].request();if(won)++requests_won;return won;
   };
   if(mode=="concurrent"){
    std::array<std::thread,8>requesters;for(auto&r:requesters)r=std::thread([&]{for(unsigned id=0;id<n;++id)request(id);});for(auto&r:requesters)r.join();check(requests_won==n);
   }else for(unsigned id=0;id<n;++id){check(request(id));check(!request(id));}
   for(unsigned id=0;id<n;++id)gate[id].release();
  }
  for(unsigned id=0;id<n;++id){finish(threads[id],&stop[id]);--pending;check(memory.release(data[id])==Memory::Result::ok);++reclaimed;}check(!errors);
  check(a->Unload()==0xc0000001u);check(!a->AcquireImageLease());binding.reset();check(a->Unload()==0);plain.reset();check(b->Unload()==0);unloads+=2;check(!live(sdk,0x82000000)&&!live(sdk,0x83000000));
  Admission closed(a,*runtime.export_resolver(),sdk);PPCContext context{};bool denied=false;try{closed.direct("xboxkrnl.exe",0x83,Signature::u32_void,context);}catch(const std::logic_error&){denied=true;}check(denied);++rejected;
  for(unsigned id=0;id<n;++id){stop[id].unloaded();check(stop[id].state()==Stop::State::unloaded);}
  check(a->ReleaseHandle()&&b->ReleaseHandle());a.reset();b.reset();
 }
 auto stats=PointerBinding::statistics();check(allocated==reclaimed&&tls_allocated==tls_reclaimed&&destructors==tls_reclaimed&&stats.scratch_allocations==stats.scratch_reclaims&&stats.image_leases==stats.image_reclaims&&stats.image_capabilities==stats.image_capability_reclaims);check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
 if(mode=="benchmark")check(plain_times.size()==5&&checked_times.size()==5);
 auto samples=[](const std::vector<double>& v){std::string out="[";for(auto seconds:v){char buffer[40];std::snprintf(buffer,sizeof buffer,"%.9f",seconds);if(out.size()>1)out+=",";out+=buffer;}return out+"]";};
 auto median=[](auto v){if(v.empty())return 0.0;std::sort(v.begin(),v.end());return v[v.size()/2];};
 std::printf("{\"group\":\"loop_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"entry_invocations\":%u,\"generated_stops\":%u,\"natural_returns\":%u,\"host_errors\":%u,\"controlled_rejections\":%u,\"requests_won\":%u,\"polls\":%llu,\"max_stop_latency_ns\":%llu,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"tls_allocations\":%u,\"tls_reclaims\":%u,\"buffer_allocations\":%u,\"buffer_reclaims\":%u,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"sdk_returns\":%llu,\"image_leases\":%llu,\"image_reclaims\":%llu,\"image_capabilities\":%llu,\"image_capability_reclaims\":%llu,\"module_unloads\":%u,\"backedge_hits\":[%llu,%llu,%llu,%llu,%llu,%llu],\"plain_median_seconds\":%.9f,\"checked_median_seconds\":%.9f,\"benchmark_pairs\":%zu,\"plain_samples\":%s,\"checked_samples\":%s,\"measured_seconds\":%.9f}\n",mode.c_str(),checks,observations.load(),entries.load(),generated_stops.load(),natural_returns.load(),host_errors.load(),rejected.load(),requests_won.load(),(unsigned long long)polls.load(),(unsigned long long)latency_ns.load(),(unsigned long long)reads.load(),(unsigned long long)writes.load(),(unsigned long long)completed.load(),tls_allocated,tls_reclaimed,allocated,reclaimed,(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims,(unsigned long long)stats.sdk_returns,(unsigned long long)stats.image_leases,(unsigned long long)stats.image_reclaims,(unsigned long long)stats.image_capabilities,(unsigned long long)stats.image_capability_reclaims,unloads,(unsigned long long)edge_hits[0].load(),(unsigned long long)edge_hits[1].load(),(unsigned long long)edge_hits[2].load(),(unsigned long long)edge_hits[3].load(),(unsigned long long)edge_hits[4].load(),(unsigned long long)edge_hits[5].load(),median(plain_times),median(checked_times),plain_times.size(),samples(plain_times).c_str(),samples(checked_times).c_str(),std::chrono::duration<double>(std::chrono::steady_clock::now()-started).count());return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
