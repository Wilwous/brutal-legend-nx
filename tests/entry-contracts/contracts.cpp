// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/entry_contract.h"
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
#include <dlfcn.h>
using namespace rex::system;
using namespace brutal::imports;
using brutal::synthetic::BoundedCfg;
using brutal::synthetic::GeneratedStop;
using Stop=brutal::synthetic::CooperativeStop;
using Memory=brutal::foundation::SdkCheckedMemory;
using brutal::instrumentation::Scope;
static unsigned checks=0;static std::atomic<unsigned>observations{0},errors{0},destroyed{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("main contract "+std::to_string(checks));}
static void test(bool v){++observations;if(!v)throw std::runtime_error("worker contract "+std::to_string(observations.load()));}
class Thread final:public XThread{
 std::function<void(Thread&)>body_;
 public:Stop&stop;uint32_t stack=0;
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
static void save(const std::filesystem::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());check(bool(f));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state;}
struct Gate{std::mutex mutex;std::condition_variable cv;bool ready=false,released=false;
 void pause(){std::unique_lock l(mutex);ready=true;cv.notify_all();if(!cv.wait_for(l,std::chrono::seconds(10),[&]{return released;}))throw std::runtime_error("observer watchdog");}
 void wait(){std::unique_lock l(mutex);if(!cv.wait_for(l,std::chrono::seconds(10),[&]{return ready;}))throw std::runtime_error("host watchdog");}
 void release(){std::lock_guard l(mutex);released=true;cv.notify_all();}};
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path input=argv[1],work=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(work/"empty");
 auto image=file(input/"fixture.xex"),manifest=file(input/"region.json");check(hash(image,image_sha));check(hash(manifest,manifest_sha));
 if(mode=="hash"||mode=="namespace"){
  if(mode=="hash"){image.back()^=1;check(!hash(image,image_sha));}
  else{std::string text(manifest.begin(),manifest.end());auto pos=text.find("\"namespace\":\"");check(pos!=std::string::npos);manifest[pos+13]^=1;check(!hash(manifest,manifest_sha));}
  std::printf("{\"group\":\"entry_%s\",\"result\":\"PASS\",\"assertions\":%u,\"guest_invocations\":0}\n",mode.c_str(),checks);return 0;
 }
 rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();Memory memory(sdk);auto*ih=sdk.LookupHeap(description().base);auto*vh=sdk.LookupHeapByType(false,4096);const auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
 unsigned metadata_loaded=0,metadata_rejected=0,binding_refused=0;
 auto finish_host=[&](){check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);std::printf("{\"group\":\"entry_%s\",\"result\":\"PASS\",\"assertions\":%u,\"metadata_loaded\":%u,\"metadata_rejected\":%u,\"binding_refused\":%u,\"guest_invocations\":0}\n",mode.c_str(),checks,metadata_loaded,metadata_rejected,binding_refused);};
 auto make_module=[&](std::vector<uint8_t>bytes,const std::string&name){save(work/"empty"/name,bytes);return object_ref<UserModule>(new UserModule(k));};
 if(mode=="metadata"){
  for(unsigned variant=0;variant<7;++variant){auto bytes=image;auto le=[&](size_t off,uint32_t value){for(unsigned j=0;j<4;++j)bytes.at(0x1000+off+j)=uint8_t(value>>(8*j));};constexpr unsigned table=0x98+224;
   if(variant==0)le(0x98+56,0x90000); // SizeOfImage alone does not grant ownership.
   if(variant==1)le(table+3*40+8,65537);
   if(variant==2)le(table+3*40+12,0x41000);
   if(variant==3)le(table+3*40+12,0x10000); // Overlap code: recovery loader must reject before binding.
   if(variant==4){bytes[0x1000+0x86]=5;bytes[0x1000+0x87]=0;}
   if(variant==5)le(table+40+12,0x30000); // Overlaps data and no longer owns emitted entry.
   if(variant==6)le(table+3*40+8,65532);
   std::string name="metadata"+std::to_string(variant)+".xex";auto module=make_module(bytes,name);auto status=module->LoadFromFile("game:\\"+name);
   if(variant==1||variant==2||variant==3||variant==4||variant==5){check(status!=0);check(!module->AcquireImageLease()&&module->processor_module()==nullptr&&module->guest_xex_header()==0);++metadata_rejected;}
   else{check(status==0);++metadata_loaded;auto lease=module->AcquireImageLease();check(bool(lease));check(lease->image_size()==0x40000);std::array<std::byte,8>out{};
    check(lease->CopyMemory(description().base+0x40000,out.data(),4,Memory::Access::read)!=Memory::Result::ok);
    if(variant==6){check(lease->CopyMemory(description().base+0x3fff8,out.data(),4,Memory::Access::read)==Memory::Result::ok);check(lease->CopyMemory(description().base+0x3fffc,out.data(),4,Memory::Access::read)!=Memory::Result::ok);}
    bool denied=false;try{brutal::synthetic::XexBinding b(module,cfg_image_info,description(),image_functions.size()+4,*runtime.export_resolver());}catch(const std::invalid_argument&){denied=true;}
    check(denied==(variant!=0));binding_refused+=denied;lease.reset();check(module->Unload()==0);
   }
   check(module->ReleaseHandle());module.reset();check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
  }finish_host();return 0;
 }
 if(mode=="allocation"){
  auto module=make_module(image,"failure.xex");auto arm=reinterpret_cast<void(*)(unsigned)>(dlsym(RTLD_DEFAULT,"fixture_arm_header_failure"));auto count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_header_failure_count"));check(arm&&count);arm(1);check(module->LoadFromFile("game:\\failure.xex")==0xc0000017u);arm(0);check(count()==1);check(!module->AcquireImageLease());check(module->ReleaseHandle());module.reset();finish_host();return 0;
 }
 if(mode=="binding"){
  auto module=make_module(image,"binding.xex");check(module->LoadFromFile("game:\\binding.xex")==0);
  for(unsigned variant=0;variant<5;++variant){auto desc=description();auto generated=cfg_image_info;auto code=std::vector<uint8_t>(image_code.begin(),image_code.end());
   if(variant==0){code[0]^=1;desc.code=code;}if(variant==1)desc.entry+=4;if(variant==2)generated.image_size=0x60000;if(variant==3)generated.code_base=desc.base+0x40000;if(variant==4)desc.code_base=desc.base+0x20000;
   bool denied=false;try{brutal::synthetic::XexBinding b(module,generated,desc,image_functions.size()+4,*runtime.export_resolver());}catch(const std::invalid_argument&){denied=true;}check(denied);++binding_refused;
  }
  {auto lease=module->AcquireImageLease();check(bool(lease));int foreign=0;ih->SetAllocationOwner(description().base,&foreign);std::array<std::byte,4>out{};auto result=lease->CopyMemory(description().code_base,out.data(),4,Memory::Access::read);ih->SetAllocationOwner(description().base,module->xex_module());check(result==Memory::Result::ownership);}
  check(module->Unload()==0);check(module->ReleaseHandle());module.reset();finish_host();return 0;
 }
 Gate gate;uint64_t visits=0,reads=0,writes=0,completed=0;unsigned approved=0,refused=0,stopped=0,memory_errors=0,startup_refusals=0,natural=0,resources=0,protection_refusals=0,uncommitted_refusals=0,ownership_refusals=0,range_refusals=0;std::array<unsigned,8>contract_pass{};std::vector<std::array<uint64_t,3>>observed_edges;std::vector<uint64_t>stop_hits(cfg_points.size());unsigned active=0;
 struct Guard{unsigned&active;~Guard(){if(active){std::fprintf(stderr,"ACTIVE_THREAD fail-stop\n");std::_Exit(70);}}}guard{active};
 for(unsigned iteration=0;iteration<(mode=="repeat"?3u:1u);++iteration){Stop stop;
 auto module=make_module(image,"entry"+std::to_string(iteration)+".xex");check(module->LoadFromFile("game:\\entry"+std::to_string(iteration)+".xex")==0);
 auto binding=std::make_unique<brutal::synthetic::XexBinding>(module,cfg_image_info,description(),image_functions.size()+4,*runtime.export_resolver());check(binding->lease().image_size()==0x40000);
 auto buffer=memory.allocate(8192,mode!="probe-uncommitted");auto foreign=memory.allocate(4096);const bool probe=mode.starts_with("probe-");
 auto thread=object_ref<Thread>(new Thread(k,stop,[&](Thread&self){
  auto&c=*rex::runtime::current_ppc_context();const PPCContext checkpoint=c;
  c.lr=0x45450000;c.r2.u64=0x1234567800000000ull;c.r17.u64=0x7654321089abcdefull;c.r20.u64=0x1020304050607080ull;c.r21.u64=0x2131415161718191ull;c.r22.u64=0x32425262728292a2ull;c.r23.u64=0x435363738393a3b3ull;
  c.f14.u64=0x400921fb54442d18ull;c.v14.u64[0]=0x1029384756abcdefull;c.v14.u64[1]=0xfedcba9876543210ull;const PPCContext supplied=c;
  brutal::synthetic::EntryState expected{c.r1.u64,c.lr,c.r2.u64,c.r13.u64,c.r17.u64,{c.r20.u64,c.r21.u64,c.r22.u64,c.r23.u64}};
  const auto sp=uint32_t(c.r1.u64);auto*stack_heap=sdk.LookupHeap(self.stack);uint32_t changed_page=0;bool changed_owner=false;
  Scope scope(memory,sdk);self.lend(scope);scope.bind(buffer,8192);Admission admission(module,*runtime.export_resolver(),sdk);PointerBinding pointer(PointerBinding::Image{},memory,module);GeneratedImports dispatch(admission,c,sdk.virtual_membase());binding->admit(scope,dispatch,pointer);
  if(mode=="sp-bounds"){c.r1.u64=self.low()+128;expected.sp=c.r1.u64;}if(mode=="sp-align"){c.r1.u64-=1;expected.sp=c.r1.u64;}if(mode=="sp-state")c.r1.u64-=16;if(mode=="lr-state")c.lr^=4;if(mode=="r2-state")c.r2.u64^=8;if(mode=="r13-state")c.r13.u64^=8;if(mode=="gpr-state")c.r17.u64^=1;
  if(mode=="stack-uncommitted"){changed_page=(sp-320)&~(stack_heap->page_size()-1);test(stack_heap->Decommit(changed_page,stack_heap->page_size()));}
  if(mode=="stack-owner"){stack_heap->SetAllocationOwner(self.stack,&expected);changed_owner=true;}
  bool startup_ok=true;try{brutal::synthetic::validate_entry_state(c,sdk,self,self.stack,self.low(),self.high(),expected);}catch(const std::invalid_argument&){startup_ok=false;++startup_refusals;}
  if(changed_page){test(stack_heap->AllocFixed(changed_page,stack_heap->page_size(),stack_heap->page_size(),rex::memory::kMemoryAllocationCommit,3));changed_page=0;}
  if(changed_owner){stack_heap->SetAllocationOwner(self.stack,&self);changed_owner=false;}
  const bool malformed=mode.starts_with("sp-")||mode.ends_with("-state")||mode.starts_with("stack-");test(startup_ok!=malformed);
  if(!startup_ok){c=checkpoint;return;}
  uint32_t root=pc_startup;if(mode=="entry")root=pc_startup+4;
  uint32_t probe_address=buffer.guest_address(),probe_width=8;bool probe_success=false;std::array<std::byte,8>before{},after{};
  if(probe){
   if(mode=="probe-cross32"||mode=="probe-cross32-ok"){root=pc_probe32;probe_width=4;probe_address+=4094;}else{root=pc_probe64;if(mode=="probe-cross64"||mode=="probe-cross64-ok")probe_address+=4092;}
   if(mode=="probe-unaligned")probe_address+=1;
   if(mode=="probe-foreign")probe_address=foreign.guest_address();if(mode=="probe-overflow")probe_address=0xfffffffc;if(mode=="probe-ro")probe_address=description().base+0x20200;if(mode=="probe-section-end")probe_address=description().base+0x3fffc;
   probe_success=mode=="probe-unaligned"||mode.ends_with("-ok");c.r6.u64=probe_address;c.r5.u64=0x13579bdf;
   if(mode=="probe-cross32"||mode=="probe-cross64")test(sdk.LookupHeap(probe_address)->Protect(buffer.guest_address()+4096,4096,1));
   if(mode=="probe-freed")test(memory.release(buffer)==Memory::Result::ok);
   if(mode=="probe-cross32"||mode=="probe-cross64")test(memory.read(buffer,probe_address-buffer.guest_address(),std::span(before).first(probe_width))==Memory::Result::ok);
  }
  int denied_store=-1;if(mode.starts_with("deny-store-"))denied_store=mode.back()-'0';
  uint32_t denied_address=0;unsigned denied_width=0;uint32_t protected_page=0;
  auto stack_copy=[&](uint32_t address,void*out,size_t size){return stack_heap->CopyOwnedMemory(self.stack,&self,address,out,size,Memory::Access::read);};
  BoundedCfg policy(c,sdk.virtual_membase(),cfg_sites,cfg_roots,cfg_imports,{cfg_image_info.func_mappings,image_functions.size()+4},cfg_edges,self.low(),self.high(),[&](uint32_t pc){
   if(mode=="exception"&&pc==pc_startup+4)BoundedCfg::indirect(pc_unadmitted,c,sdk.virtual_membase());
   if(mode=="continuation"&&pc==pc_startup+4)BoundedCfg::point(uint32_t(cfg_points.size()),pc_unadmitted);
   if(mode=="unknown"&&pc==pc_startup)BoundedCfg::direct(nullptr,c,sdk.virtual_membase());
   if(mode=="return"&&pc==pc_save+20)c.lr^=4;
   if(mode=="stop"&&pc==store_points[6])gate.pause();
   if(denied_store>=0&&pc==store_points[denied_store]){
    const int offsets[]{-56,-48,-40,-32,-16,-320};denied_address=denied_store<6?sp+offsets[denied_store]:description().base+0x30420+4*(denied_store-6);denied_width=denied_store<4?8:4;
    auto status=denied_store<6?stack_copy(denied_address,before.data(),denied_width):binding->lease().CopyMemory(denied_address,before.data(),denied_width,Memory::Access::read);test(status==Memory::Result::ok);protected_page=denied_address&~4095u;test(sdk.LookupHeap(denied_address)->Protect(protected_page,4096,1));
   }
  });
  GeneratedStop generated(stop,c,cfg_points);
  try{policy.enter(root);++natural;test(probe&&probe_success);}
  catch(const BoundedCfg::Exit&e){test((mode=="valid"||mode=="repeat")&&policy.owns(e));++approved;}
  catch(const BoundedCfg::Refused&){test(mode=="unknown"||mode=="entry"||mode=="return"||mode=="exception"||mode=="continuation");++refused;}
  catch(const GeneratedStop::Stopped&e){test(mode=="stop"&&generated.owns(e));++stopped;}
  catch(const brutal::instrumentation::Fault&e){test(denied_store>=0||(probe&&!probe_success));test(e.kind==brutal::instrumentation::Kind::memory);
   auto wanted=Memory::Result::ownership;
   if(denied_store>=0||mode=="probe-cross32"||mode=="probe-cross64"||mode=="probe-ro")wanted=Memory::Result::protection;
   if(mode=="probe-uncommitted")wanted=Memory::Result::uncommitted;
   if(mode=="probe-overflow")wanted=Memory::Result::invalid_range;
   test(e.memory_result==wanted);protection_refusals+=wanted==Memory::Result::protection;uncommitted_refusals+=wanted==Memory::Result::uncommitted;ownership_refusals+=wanted==Memory::Result::ownership;range_refusals+=wanted==Memory::Result::invalid_range;++memory_errors;}
  if(mode=="valid"||mode=="repeat"){
   for(unsigned i=0;i<8;++i){const int offsets[]{-56,-48,-40,-32,-16,-320};const uint32_t address=i<6?sp+offsets[i]:description().base+0x30420+4*(i-6);const unsigned width=i<4?8:4;std::array<std::byte,8>bytes{};auto status=i<6?stack_copy(address,bytes.data(),width):binding->lease().CopyMemory(address,bytes.data(),width,Memory::Access::read);test(status==Memory::Result::ok);uint64_t value=0;for(unsigned j=0;j<width;++j)value=(value<<8)|std::to_integer<unsigned>(bytes[j]);uint64_t wanted=i<4?expected.saved[i]:i==4?uint32_t(expected.lr):i==5?sp:i==6?55:64;test(value==wanted);++contract_pass[i];}
   test(scope.statistics.writes==8&&scope.statistics.completed==8&&scope.statistics.reads==0);test(c.r1.u64==sp-320&&c.r2.u64==expected.r2&&c.r13.u64==expected.r13&&c.lr==pc_startup+8);test(c.r20.u64==expected.input&&c.r21.u64==expected.input&&c.r22.u64==expected.input&&c.r23.u64==expected.input);
   PPCContext preserved=c;preserved.r1=supplied.r1;preserved.r5=supplied.r5;preserved.r6=supplied.r6;preserved.r7=supplied.r7;preserved.r20=supplied.r20;preserved.r21=supplied.r21;preserved.r22=supplied.r22;preserved.r23=supplied.r23;preserved.lr=supplied.lr;test(std::memcmp(&preserved,&supplied,sizeof supplied)==0);
  }
  if(denied_store>=0){auto status=denied_store<6?stack_copy(denied_address,after.data(),denied_width):binding->lease().CopyMemory(denied_address,after.data(),denied_width,Memory::Access::read);test(status==Memory::Result::ok&&before==after);test(scope.statistics.writes==unsigned(denied_store)+1&&scope.statistics.completed==unsigned(denied_store));if(denied_store==5)test(c.r1.u64==sp);test(sdk.LookupHeap(denied_address)->Protect(protected_page,4096,3));}
  if(probe){test(scope.statistics.writes==1&&scope.statistics.completed==unsigned(probe_success));if(mode=="probe-cross32"||mode=="probe-cross64"){test(memory.read(buffer,probe_address-buffer.guest_address(),std::span(after).first(probe_width))==Memory::Result::ok&&before==after);test(sdk.LookupHeap(probe_address)->Protect(buffer.guest_address()+4096,4096,3));}if(probe_success){test(memory.read(buffer,probe_address-buffer.guest_address(),std::span(after).first(probe_width))==Memory::Result::ok);uint64_t value=0;for(unsigned j=0;j<probe_width;++j)value=(value<<8)|std::to_integer<unsigned>(after[j]);test(value==(probe_width==4?0x13579bdf:expected.saved[3]));}}
  if(mode=="exception"||mode=="continuation"||mode=="unknown"||mode=="entry")test(scope.statistics.completed==0&&policy.statistics.native_calls==0);
  test(dispatch.attempts==0&&policy.statistics.imports==0);visits+=policy.statistics.visits;reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
  for(auto e:policy.edges())if(e[2])observed_edges.push_back(e);for(size_t i=0;i<stop_hits.size();++i)stop_hits[i]+=generated.hits[i];
  c=checkpoint;test(std::memcmp(&c,&checkpoint,sizeof c)==0);
 }));check(thread->CreateWithModule(module)==0);active=1;check(thread->Resume()==0);
 if(mode=="stop"){gate.wait();check(module->Unload()==0xc0000001u);check(!module->AcquireImageLease());check(stop.request());check(!stop.request());gate.release();}
 if(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(15))!=rex::thread::WaitResult::kSuccess){std::fprintf(stderr,"JOIN_TIMEOUT fail-stop\n");std::_Exit(70);}active=0;check(!errors);check(stop.state()==Stop::State::exited);stop.joined();
 check(!live(sdk,thread->stack)&&live(sdk,thread->tls_ptr())&&live(sdk,thread->pcr_ptr()));auto tls=thread->tls_ptr(),pcr=thread->pcr_ptr();check(thread->ReleaseHandle());thread.reset();check(destroyed==iteration+1&&!live(sdk,tls)&&!live(sdk,pcr));stop.reclaimed();++resources;
 if(mode!="probe-freed")check(memory.release(buffer)==Memory::Result::ok);check(memory.release(foreign)==Memory::Result::ok);check(module->Unload()==0xc0000001u);binding.reset();check(module->Unload()==0);stop.unloaded();check(module->ReleaseHandle());module.reset();
 check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
 }
 auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&stats.image_leases==stats.image_reclaims&&stats.image_capabilities==stats.image_capability_reclaims);
 std::printf("{\"group\":\"entry_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"approved_exits\":%u,\"refused_transfers\":%u,\"generated_stops\":%u,\"checked_failures\":%u,\"startup_refusals\":%u,\"protection_refusals\":%u,\"uncommitted_refusals\":%u,\"ownership_refusals\":%u,\"range_refusals\":%u,\"natural_returns\":%u,\"visits\":%llu,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"tls_reclaimed\":%u,\"images_unloaded\":%u,\"allocations\":%u,\"deallocations\":%u,\"store_contracts\":[",mode.c_str(),checks,observations.load(),approved,refused,stopped,memory_errors,startup_refusals,protection_refusals,uncommitted_refusals,ownership_refusals,range_refusals,natural,(unsigned long long)visits,(unsigned long long)reads,(unsigned long long)writes,(unsigned long long)completed,resources,resources,resources*2,resources*2);
 for(size_t i=0;i<8;++i)std::printf("%s%u",i?",":"",contract_pass[i]);std::printf("],\"edges\":[");bool comma=false;for(auto e:observed_edges){std::printf("%s[%llu,%llu,%llu]",comma?",":"",(unsigned long long)e[0],(unsigned long long)e[1],(unsigned long long)e[2]);comma=true;}std::printf("],\"stop_hits\":[");for(size_t i=0;i<stop_hits.size();++i)std::printf("%s%llu",i?",":"",(unsigned long long)stop_hits[i]);std::puts("]}");return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
