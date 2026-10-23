// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/synthetic_xex_binding.h"
#include "runtime/bounded_cfg.h"
#include "cfg-data.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/thread.h>
#include <CommonCrypto/CommonDigest.h>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <thread>
#include <cstdio>
using namespace rex::system;
using namespace brutal::imports;
using brutal::synthetic::BoundedCfg;
using brutal::synthetic::GeneratedStop;
using Stop=brutal::synthetic::CooperativeStop;
using Memory=brutal::foundation::SdkCheckedMemory;
using brutal::instrumentation::Scope;
static unsigned checks=0;static std::atomic<unsigned>observations{0},errors{0},destroyed{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("main contract");}
static void test(bool v){++observations;if(!v)throw std::runtime_error("worker contract");}
class Thread final:public XThread{
 std::function<void(Thread&)>body_;
 public:
 Stop&stop;uint32_t stack=0;
 Thread(KernelState*k,Stop&s,std::function<void(Thread&)>f):XThread(k,65536,0,0,0,1,true),body_(std::move(f)),stop(s){}
 ~Thread()override{++destroyed;}
 uint32_t low()const{return stack_limit_;}uint32_t high()const{return stack_base_;}
 void lend(Scope&s){s.lend(stack_alloc_base_,this,stack_limit_,stack_base_-stack_limit_);s.lend(pcr_address_,this,pcr_address_,0x2d8);s.lend(tls_static_address_,this,tls_static_address_,tls_total_size_);}
 void Execute()override{kernel_state()->OnThreadExecute(this);stack=stack_alloc_base_;const uint32_t owned[]{stack_alloc_base_,pcr_address_,tls_static_address_};for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,this);
  try{auto&c=*rex::runtime::current_ppc_context();c.r1.u64=stack_base_-256;body_(*this);}catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;}
  for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,nullptr);
  if(stop.state()==Stop::State::running)stop.request();if(stop.state()==Stop::State::requested)stop.safe_boundary();stop.exited();Exit(0);
 }
};
static auto file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("missing fixture");return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)),{});}
static bool hash(const std::vector<uint8_t>&b,std::span<const unsigned char>expected){std::array<unsigned char,32>h{};return CC_SHA256(b.data(),CC_LONG(b.size()),h.data())&&std::equal(h.begin(),h.end(),expected.begin(),expected.end());}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state;}
struct Gate{std::mutex mutex;std::condition_variable cv;bool ready=false,released=false;
 void pause(){std::unique_lock l(mutex);ready=true;cv.notify_all();if(!cv.wait_for(l,std::chrono::seconds(10),[&]{return released;}))throw std::runtime_error("observer watchdog");}
 void wait(){std::unique_lock l(mutex);if(!cv.wait_for(l,std::chrono::seconds(10),[&]{return ready;}))throw std::runtime_error("host watchdog");}
 void release(){std::lock_guard l(mutex);released=true;cv.notify_all();}};
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path input=argv[1],work=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(work/"empty");
 auto image=file(input/"fixture.xex"),manifest=file(input/"region.json");check(hash(image,image_sha));check(hash(manifest,manifest_sha));
 if(mode=="hash"){manifest[0]^=1;check(!hash(manifest,manifest_sha));std::printf("{\"group\":\"cfg_hash\",\"result\":\"PASS\",\"assertions\":%u,\"manifest_refusals\":1,\"guest_invocations\":0}\n",checks);return 0;}
 rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();Memory memory(sdk);auto*ih=sdk.LookupHeap(0x82000000);auto*vh=sdk.LookupHeapByType(false,4096);const auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
 Stop stop;Gate gate;uint64_t visits=0,reads=0,writes=0,completed=0,imports=0,calls=0,returns=0;unsigned natural=0,approved=0,refused=0,stopped=0,memory_errors=0,policy_refusals=0,strict_import_refusals=0;std::vector<std::array<uint64_t,3>>observed_edges;std::vector<uint64_t>stop_hits;unsigned active=0;
 struct Guard{unsigned&active;~Guard(){if(active){std::fprintf(stderr,"ACTIVE_THREAD fail-stop\n");std::_Exit(70);}}}guard{active};
 {
 auto module=object_ref<UserModule>(new UserModule(k));{std::ofstream f(work/"empty/fixture.xex",std::ios::binary);f.write(reinterpret_cast<const char*>(image.data()),image.size());check(bool(f));}check(module->LoadFromFile("game:\\fixture.xex")==0);
 auto binding=std::make_unique<brutal::synthetic::XexBinding>(module,cfg_image_info,description(),image_functions.size()+4,*runtime.export_resolver());auto data=memory.allocate(4096);check(memory.store32be(data,0,0xa5a5a5a5)==Memory::Result::ok);check(memory.store32be(data,4,0xa5a5a5a5)==Memory::Result::ok);
 auto thread=object_ref<Thread>(new Thread(k,stop,[&](Thread&self){
  auto&c=*rex::runtime::current_ppc_context();const PPCContext initial=c;test(c.r13.u32==self.pcr_ptr()&&!(c.r1.u32&15));c.r3.u64=mode=="zero"?0:3;c.r4.u64=data.guest_address();c.r5.u64=(mode=="unknown")?0xdead0000:pc_leaf;
  uint32_t root=pc_flow;
  if(mode=="prefix"||mode=="memory"||mode=="exception")root=pc_prefix;
  if(mode=="memory")c.r4.u64=0;
  if(mode=="escape")root=pc_escape;
  if(mode=="stop-loop")root=pc_spin;
  if(mode=="imports"||mode=="stop-import"){root=pc_imports;c.r3.u64=0x82031000;}
  if(mode=="long"||mode=="stop-long"){root=pc_long;c.r3.u64=4;}
  Scope scope(memory,sdk);scope.bind(data,4096);self.lend(scope);Admission admission(module,*runtime.export_resolver(),sdk);PointerBinding pointer(PointerBinding::Image{},memory,module);GeneratedImports dispatch(admission,c,sdk.virtual_membase());binding->admit(scope,dispatch,pointer);
  if(mode=="policy"){
   for(unsigned variant=0;variant<6;++variant){auto changed=cfg_sites;std::array<uint32_t,1>badtarget{0xdead0000};
    if(variant==0)changed[0].index=1;if(variant==1)changed[1].pc=changed[0].pc;if(variant==2)changed[0].targets=badtarget;if(variant==3)changed[0].owner=1;if(variant==4)changed[0].kind=static_cast<BoundedCfg::Kind>(99);if(variant==5)changed[0].action=static_cast<BoundedCfg::Action>(99);
    bool denied=false;try{BoundedCfg invalid(c,sdk.virtual_membase(),changed,cfg_roots,cfg_imports,{cfg_image_info.func_mappings,image_functions.size()+4},cfg_edges,self.low(),self.high());}catch(const std::invalid_argument&){denied=true;}test(denied);++policy_refusals;
   }
   PPCFunc*native=nullptr;for(size_t i=0;i<image_functions.size()+3;++i)if(cfg_image_info.func_mappings[i].guest==pc_unadmitted)native=cfg_image_info.func_mappings[i].host;test(native!=nullptr);
   for(auto symbol:std::array<PPCFunc*,2>{native,nullptr}){const PPCContext before=c;bool denied=false;try{GeneratedImports::direct_import(symbol,c,sdk.virtual_membase());}catch(const brutal::instrumentation::Fault&e){denied=e.kind==brutal::instrumentation::Kind::unsupported_call;}
    test(denied&&std::memcmp(&before,&c,sizeof c)==0&&dispatch.attempts==0);++strict_import_refusals;}
  }else{
   unsigned spin_hits=0;
   BoundedCfg policy(c,sdk.virtual_membase(),cfg_sites,cfg_roots,cfg_imports,{cfg_image_info.func_mappings,image_functions.size()+4},cfg_edges,self.low(),self.high(),[&](uint32_t pc){
    if(mode=="exception"&&pc==pc_prefix+4)BoundedCfg::indirect(pc_unadmitted,c,sdk.virtual_membase());
    if(mode=="callback"&&pc==pc_indirect)BoundedCfg::indirect(0xdead0000,c,sdk.virtual_membase());
    if(mode=="return"&&pc==pc_leaf+4)c.lr^=4;
    if((mode=="stop-loop"&&pc==pc_spin&&++spin_hits==8)||(mode=="stop-forward"&&pc==pc_helper)||(mode=="stop-import"&&pc==pc_import_site+4)||(mode=="stop-long"&&pc==pc_long+32))gate.pause();
   });
   GeneratedStop generated(stop,c,cfg_points);
   try{policy.enter(root);++natural;test(mode=="valid"||mode=="zero"||mode=="imports"||mode=="long");}
   catch(const BoundedCfg::Exit&e){test(mode=="prefix"&&policy.owns(e));++approved;c=initial;}
   catch(const BoundedCfg::Refused&){test(mode=="escape"||mode=="unknown"||mode=="callback"||mode=="return"||mode=="exception");++refused;c=initial;}
   catch(const GeneratedStop::Stopped&e){test(mode.starts_with("stop-")&&generated.owns(e));++stopped;c=initial;}
   catch(const brutal::instrumentation::Fault&){test(mode=="memory");++memory_errors;c=initial;}
   if(mode=="valid")test(c.r3.u64==28);if(mode=="zero")test(c.r3.u64==2);if(mode=="long")test(c.r3.u64==28);
   test(c.r1.u64==initial.r1.u64&&c.r2.u64==initial.r2.u64&&c.r13.u64==initial.r13.u64&&c.lr==initial.lr);
   if(mode=="valid"||mode=="zero")test(c.r14.u64==initial.r14.u64);
   visits=policy.statistics.visits;imports=policy.statistics.imports;calls=policy.statistics.native_calls;returns=policy.statistics.returns;
   observed_edges=policy.edges();stop_hits=generated.hits;reads=scope.statistics.reads;writes=scope.statistics.writes;completed=scope.statistics.completed;
   if(mode=="exception")test(scope.statistics.completed==0&&policy.statistics.native_calls==0&&policy.statistics.imports==0);
   if(mode=="imports"||mode=="stop-import")test(dispatch.attempts==1&&dispatch.returns==1);
   test(PointerBinding::statistics().scratch_allocations==PointerBinding::statistics().scratch_reclaims);
  }
 }));check(thread->CreateWithModule(module)==0);active=1;check(thread->Resume()==0);
 if(mode.starts_with("stop-")){gate.wait();check(module->Unload()==0xc0000001u);check(!module->AcquireImageLease());check(stop.request());check(!stop.request());gate.release();}
 if(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(15))!=rex::thread::WaitResult::kSuccess){std::fprintf(stderr,"JOIN_TIMEOUT fail-stop\n");std::_Exit(70);}active=0;check(!errors);check(stop.state()==Stop::State::exited);stop.joined();
 check(!live(sdk,thread->stack)&&live(sdk,thread->tls_ptr())&&live(sdk,thread->pcr_ptr()));auto tls=thread->tls_ptr(),pcr=thread->pcr_ptr();check(thread->ReleaseHandle());thread.reset();check(destroyed==1&&!live(sdk,tls)&&!live(sdk,pcr));stop.reclaimed();
 uint32_t first=0,second=0;check(memory.load32be(data,0,first)==Memory::Result::ok&&memory.load32be(data,4,second)==Memory::Result::ok);
 check(mode=="prefix"?(first==0x3a&&second==0x43):(first==0xa5a5a5a5&&second==0xa5a5a5a5));
 check(memory.release(data)==Memory::Result::ok);check(module->Unload()==0xc0000001u);binding.reset();check(module->Unload()==0);stop.unloaded();check(module->ReleaseHandle());module.reset();
 }
 check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&stats.image_leases==stats.image_reclaims&&stats.image_capabilities==stats.image_capability_reclaims);
 std::printf("{\"group\":\"cfg_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"natural_returns\":%u,\"approved_exits\":%u,\"refused_transfers\":%u,\"generated_stops\":%u,\"checked_failures\":%u,\"policy_refusals\":%u,\"strict_import_refusals\":%u,\"visits\":%llu,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"imports\":%llu,\"native_calls\":%llu,\"returns\":%llu,\"tls_reclaimed\":1,\"images_unloaded\":1,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"edges\":[",mode.c_str(),checks,observations.load(),natural,approved,refused,stopped,memory_errors,policy_refusals,strict_import_refusals,(unsigned long long)visits,(unsigned long long)reads,(unsigned long long)writes,(unsigned long long)completed,(unsigned long long)imports,(unsigned long long)calls,(unsigned long long)returns,(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims);
 bool comma=false;for(auto e:observed_edges)if(e[2]){std::printf("%s[%llu,%llu,%llu]",comma?",":"",(unsigned long long)e[0],(unsigned long long)e[1],(unsigned long long)e[2]);comma=true;}std::printf("],\"stop_hits\":[");for(size_t i=0;i<stop_hits.size();++i)std::printf("%s%llu",i?",":"",(unsigned long long)stop_hits[i]);std::puts("]}");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
