// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Only fixture loading, capabilities and oracles are host-authored. The PPC
// entry, argument preparation, branches, memory and arithmetic are generated.
#include "ppc_config.h"
#include "../tls-import/fixture.h"
#include "runtime/generated_imports.h"
#include "layout.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/thread.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <barrier>
#include <condition_variable>
#include <cstdio>
#include <source_location>
using namespace rex::system;
using namespace brutal::imports;
using brutal::instrumentation::Scope;
using Memory=brutal::foundation::SdkCheckedMemory;
struct ExpectedCallbackError:std::runtime_error{ExpectedCallbackError():std::runtime_error("original callback error"){} };
static unsigned checks=0;
static std::atomic<unsigned> observations{0},errors{0},destructors{0},tls_reclaims{0},threads{0};
static std::atomic<unsigned> expected_rejections{0};
static std::atomic<uint64_t> reads{0},writes{0},completed{0},entries{0},import_attempts{0},import_returns{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("main oracle "+std::to_string(checks));}
static void test(bool v,std::source_location at=std::source_location::current()){++observations;if(!v)throw std::runtime_error("worker oracle line "+std::to_string(at.line()));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
static uint32_t thunk(uint32_t base,uint32_t ord){unsigned n=0;for(auto e:tls_fixture::matrix_imports)if(std::string_view(e.library)=="xboxkrnl.exe"){if(e.ordinal==ord)return base+0x20004+n*8;++n;}throw std::logic_error("import metadata");}
static uint32_t mapped(uint32_t base,uint32_t pc){return base+0x1000+pc-pc_entry;}
class Thread final:public XThread{
 std::function<void(Thread&)> body_;
 public:
 Thread(KernelState*k,std::function<void(Thread&)>body):XThread(k,65536,0,0,0,1,true),body_(std::move(body)){}
 ~Thread()override{++destructors;}
 uint32_t saved_stack=0;
 uint32_t low()const{return stack_limit_;}uint32_t high()const{return stack_base_;}
 void lend(Scope&s){s.lend(stack_alloc_base_,this,stack_limit_,stack_base_-stack_limit_);s.lend(pcr_address_,this,pcr_address_,0x2d8);s.lend(tls_static_address_,this,tls_static_address_,tls_total_size_);}
 void Execute()override{
  kernel_state()->OnThreadExecute(this);
  saved_stack=stack_alloc_base_;
  const uint32_t owned[]{stack_alloc_base_,pcr_address_,tls_static_address_};
  try{
   for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,this);
   body_(*this);
  }catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;}
  for(auto a:owned)memory()->LookupHeap(a)->SetAllocationOwner(a,nullptr);
  Exit(0);
 }
};
static std::vector<std::byte> read(Memory&m,const Memory::Allocation&a,unsigned off,unsigned size){std::vector<std::byte>b(size);test(m.read(a,off,b)==Memory::Result::ok);return b;}
static uint32_t word(Memory&m,const Memory::Allocation&a,unsigned off){uint32_t n=0;test(m.load32be(a,off,n)==Memory::Result::ok);return n;}
int main(int argc,char**argv){if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 const std::filesystem::path p=argv[1],input=argv[2];const std::string mode=argv[3];std::filesystem::create_directories(p/"empty");
 std::ifstream bin(input/"caller.bin",std::ios::binary);std::vector<uint8_t>program((std::istreambuf_iterator<char>(bin)),{});check(program.size()>128&&program.size()<4096);
 rex::Runtime runtime(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto&sdk=*runtime.memory();auto*k=runtime.kernel_state();auto*imageheap=sdk.LookupHeap(0x82000000);auto*systemheap=sdk.LookupHeapByType(false,4096);
 const auto imagebaseline=imageheap->GetUnreservedPageCount(),systembaseline=systemheap->GetUnreservedPageCount();Memory memory(sdk);unsigned allocations=0,reclaims=0;
 auto alloc=[&](unsigned size,bool commit=true){++allocations;return memory.allocate(size,commit);};
 auto free=[&](const Memory::Allocation&a){check(memory.release(a)==Memory::Result::ok);++reclaims;};
 auto load=[&](const char*name,uint32_t base){auto bytes=tls_fixture::image(base,0x41,true);std::copy(program.begin(),program.end(),bytes.begin()+0x2000);
  {std::ofstream f(p/"empty"/name,std::ios::binary);f.write(reinterpret_cast<char*>(bytes.data()),bytes.size());check(bool(f));}
  auto module=object_ref<UserModule>(new UserModule(k));check(module->LoadFromFile(std::string("game:\\")+name)==0);auto lease=module->AcquireImageLease();test(bool(lease)&&module->xex_module()->entry_point()==base+0x1000);test(std::memcmp(sdk.virtual_membase()+base+0x1000,program.data(),program.size())==0);return module;};
 auto a=load("a.xex",0x82000000);auto b=load("b.xex",0x83000000);
 auto finish=[&](object_ref<Thread>&t){check(rex::thread::Wait(t->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);auto tls=t->tls_ptr(),pcr=t->pcr_ptr();check(live(sdk,tls)&&live(sdk,pcr)&&!live(sdk,t->saved_stack));check(t->ReleaseHandle());t.reset();check(!live(sdk,tls)&&!live(sdk,pcr));++tls_reclaims;};
 auto spawn=[&](std::function<void(Thread&)>body){auto t=object_ref<Thread>(new Thread(k,std::move(body)));check(t->CreateWithModule(a)==0);++threads;return t;};
 std::mutex mutex;std::condition_variable cv;bool ready=false,release=false;
 std::barrier start(mode=="concurrent"?4:1);
 auto exercise=[&](Thread&self,unsigned id,Memory::Allocation descriptor,Memory::Allocation data){
  const auto desc=descriptor.guest_address(),address=data.guest_address();
  const auto store=[&](unsigned off,uint32_t value){test(memory.store32be(descriptor,off,value)==Memory::Result::ok);};
  auto fill=[&]{std::vector<std::byte>zero(8192,std::byte{0xa5});test(memory.write(data,0,zero)==Memory::Result::ok);
   for(unsigned n=0;n<4;++n)test(memory.store32be(data,4092+n*4,n==1?0x55667788:0x11223344)==Memory::Result::ok);
   std::vector<std::byte>clear(256);test(memory.write(descriptor,0,clear)==Memory::Result::ok);
   store(0,address+4092);store(4,16);store(8,0x11223344);store(12,address);store(16,address+0x100);store(20,0);store(24,thunk(0x83000000,0xee));store(28,address+0x200);
   store(0x60,0x1234abcd);store(0x6c,0x55660000+id);store(0x70,0x11223344);store(0x74,0x55667788);store(0x78,0xaabbccdd);
   std::array<std::byte,16>v{};for(unsigned n=0;n<16;++n)v[n]=std::byte(n);test(memory.write(descriptor,0x80,v)==Memory::Result::ok);
  };
  auto&c=*rex::runtime::current_ppc_context();c.r2.u64=0x1234567887654321ull;c.r14.u64=0x8877665544332211ull;c.r15.u64=0xabcdef12fedcba34ull;c.f14.u64=0x3ff8000000000000ull;c.v20.u64[0]=0x1122334455667788ull;c.v20.u64[1]=0x8877665544332211ull;
  test(c.r13.u32==self.pcr_ptr()&&!(c.r1.u32&15)&&c.r1.u32>=self.low()&&c.r1.u32<=self.high());
  c.r1.u64=self.high()-256; // Explicit original caller linkage area.
  c.lr=0x13579bdf;
  const PPCContext original=c;Admission admission(b,*runtime.export_resolver(),sdk);
  auto execute=[&](bool failure=false,Signature signature=Signature::u32_in_words_u32_u32,PointerBinding::Observer observer={}){
   auto lease=b->AcquireImageLease();test(bool(lease));Scope scope(memory,sdk);scope.bind(descriptor,4096);scope.bind(data,8192);self.lend(scope);
   scope.allow_synthetic(mapped(0x83000000,pc_entry),fn_entry);scope.allow_synthetic(mapped(0x83000000,pc_echo),fn_echo);
   PointerBinding binding(memory,data,observer);GeneratedImports dispatch(admission,c,sdk.virtual_membase());
   dispatch.bind(fn_compare,thunk(0x83000000,0x11b),0x11b,signature,binding);
   dispatch.bind(fn_critical,thunk(0x83000000,0x12e),0x12e,Signature::void_inout28,binding);
   // Indirect-only gate has no synthetic function identity.
   dispatch.bind_indirect(thunk(0x83000000,0xee),0xee,Signature::u32_address_out28_u32,binding);
   c=original;c.r3.u64=desc;++entries;bool caught=false;
   try{scope.indirect(mapped(0x83000000,pc_entry),c,sdk.virtual_membase());}
   catch(const Memory::BufferError&){caught=true;}
   catch(const brutal::instrumentation::Fault&){caught=true;}
   catch(const std::logic_error&){caught=true;}
   catch(const ExpectedCallbackError&){caught=true;}
   reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;import_attempts+=dispatch.attempts;import_returns+=dispatch.returns;
   test(caught==failure);if(caught)++expected_rejections;
   if(caught){test(word(memory,descriptor,0x2c)==0);c=original;} // Host containment, NOT guest unwind.
   else{
    test(scope.statistics.reads+scope.statistics.writes==scope.statistics.completed);
    test(c.r1.u64==original.r1.u64&&c.r2.u64==original.r2.u64&&c.r13.u64==original.r13.u64&&c.lr==original.lr&&c.r14.u64==original.r14.u64&&c.r15.u64==original.r15.u64);
    test(c.f14.u64==original.f14.u64&&!std::memcmp(&c.v20,&original.v20,sizeof c.v20));
    test(word(memory,descriptor,0x64)==0x1234abcd&&word(memory,descriptor,0x68)==0x55660000+id);
    auto tag=read(memory,descriptor,0xa0,8);test(tag==std::vector<std::byte>({std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x44},std::byte{0xff},std::byte{0xdd},std::byte{0xbb},std::byte{0x55}}));
    auto vector=read(memory,descriptor,0x90,16);for(unsigned n=0;n<16;++n)test(vector[n]==std::byte(n*2));
   }
   return scope.statistics;
  };
  start.arrive_and_wait();
  if(mode=="valid"||mode=="concurrent"||mode=="repeat"||mode=="lifetime"){
   for(unsigned n=0;n<(mode=="concurrent"?8:1);++n){fill();auto before=read(memory,data,4092,16);
    auto stats=execute(false,Signature::u32_in_words_u32_u32,[&](auto stage){if(mode=="lifetime"&&stage==PointerBinding::Stage::sdk_returned&&c.r3.u64==address+0x200){
     std::unique_lock lock(mutex);ready=true;cv.notify_one();if(!cv.wait_for(lock,std::chrono::seconds(5),[&]{return release;}))throw std::runtime_error("lifetime gate timeout");
    }});
    test(stats.reads==18&&stats.writes==12&&stats.completed==30);
    test(c.r3.u64==0&&word(memory,descriptor,0x24)==3&&word(memory,descriptor,0x28)==0&&word(memory,descriptor,0x2c)==0x55aa);
    test(before==read(memory,data,4092,16));test(word(memory,data,0x100)==address&&word(memory,data,0x10c)==16384);
    test(read(memory,data,0x200,2)==std::vector<std::byte>({std::byte{1},std::byte{0}})&&word(memory,data,0x210)==0xffffffff);
   }
  }else if(mode=="invalid"){
   for(auto [off,value]:std::array<std::pair<unsigned,uint32_t>,13>{{{0,0},{0,address+4093},{4,0},{4,3},{4,65540},{0,address+8188},{0,0xfffffffc},{16,0},{16,address+8192},{28,address+8192},{28,0},{28,address+1},{16,address+1}}}){
    fill();store(off,value);const unsigned protected_offset=off==28?0x200:0x100;auto before=read(memory,data,protected_offset,28);execute(true);test(before==read(memory,data,protected_offset,28));
   }
  }else if(mode=="errors"){
   fill();store(20,0xffffffff);auto before=read(memory,data,0x100,0x200);auto stats=execute();test(c.r3.u32==0xc000000d&&word(memory,descriptor,0x28)==0xc000000d&&word(memory,descriptor,0x2c)==0);test(before==read(memory,data,0x100,0x200));test(stats.completed==28);
   for(auto stage:{PointerBinding::Stage::validated,PointerBinding::Stage::scratch_ready,PointerBinding::Stage::sdk_returned}){fill();before=read(memory,data,0,8192);execute(true,Signature::u32_in_words_u32_u32,[&](auto at){if(stage==at)throw ExpectedCallbackError();});test(before==read(memory,data,0,8192));}
  }else if(mode=="policy"){
   for(auto target:{0xdeadbeecu,mapped(0x83000000,pc_entry),mapped(0x83000000,pc_echo),thunk(0x83000000,0x11b),thunk(0x83000000,0x12e),0x830010fcu,address,0x83001001u,0u}){fill();store(24,target);execute(true);}
   for(auto signature:{Signature::unverified,Signature::typed_vector_argument,Signature::typed_vector_return,Signature::mixed_vector,Signature::variadic}){fill();execute(true,signature);}
  }else if(mode=="permissions"){
   auto*h=sdk.LookupHeap(address);
   fill();test(h->Protect(address+4096,4096,1));execute();test(h->Protect(address+4096,4096,3)); // IN may read RO.
   fill();store(16,address+4092);auto before=read(memory,data,4092,28);test(h->Protect(address+4096,4096,1));execute(true);test(before==read(memory,data,4092,28));test(h->Protect(address+4096,4096,3));
   fill();store(28,address+4092);before=read(memory,data,4092,28);test(h->Protect(address+4096,4096,1));execute(true);test(before==read(memory,data,4092,28));test(h->Protect(address+4096,4096,3));
   fill();test(h->Decommit(address+4096,4096));execute(true);test(h->AllocFixed(address+4096,4096,4096,2,3));
   // Keep IN on the first committed page while testing OUT/INOUT commitment.
   for(unsigned target:{16u,28u}){
    fill();for(unsigned i=0;i<4;++i)test(memory.store32be(data,0x300+i*4,0x11223344)==Memory::Result::ok);
    store(0,address+0x300);store(target,address+4092);auto prefix=read(memory,data,4092,4);
    test(h->Decommit(address+4096,4096));execute(true);test(prefix==read(memory,data,4092,4));test(h->AllocFixed(address+4096,4096,4096,2,3));
   }
   fill();store(16,address+4092);execute();test(word(memory,data,4092)==address);
   fill();store(28,address+4092);execute();test(word(memory,data,4092+16)==0xffffffff);
  }else if(mode=="freed"){
   fill();test(memory.release(data)==Memory::Result::ok);execute(true);
  }else if(mode=="uncommitted"){
   // Descriptor remains valid; pointer adapter is the first reader of data.
   std::vector<std::byte>clear(256);test(memory.write(descriptor,0,clear)==Memory::Result::ok);store(0,address);store(4,16);store(8,0x11223344);store(24,thunk(0x83000000,0xee));execute(true);
  }else if(mode=="checked"){
   fill();Scope scope(memory,sdk);scope.bind(data,8192);self.lend(scope);
   scope.allow_synthetic(mapped(0x83000000,pc_load),fn_load);scope.allow_synthetic(mapped(0x83000000,pc_store),fn_store);
   auto run=[&](PPCFunc*fn,uint32_t ptr,bool fail){c=original;c.r4.u64=ptr;c.r3.u64=0x12345678;bool caught=false;try{scope.direct(fn,c,sdk.virtual_membase());}catch(const brutal::instrumentation::Fault&){caught=true;}test(caught==fail);if(caught)++expected_rejections;};
   auto*h=sdk.LookupHeap(address);run(fn_store,address+4094,false);test(read(memory,data,4094,4)==std::vector<std::byte>({std::byte{0x12},std::byte{0x34},std::byte{0x56},std::byte{0x78}}));
   test(h->Protect(address+4096,4096,1));auto before=read(memory,data,4094,4);run(fn_store,address+4094,true);test(before==read(memory,data,4094,4));run(fn_load,address+4094,false);test(h->Protect(address+4096,4096,3));
   test(h->Decommit(address+4096,4096));run(fn_load,address+4094,true);test(h->AllocFixed(address+4096,4096,4096,2,3));
   run(fn_load,0,true);run(fn_store,address+8190,true);run(fn_load,self.high(),true);
   test(memory.release(data)==Memory::Result::ok);run(fn_load,address,true);
   reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
  }else if(mode=="gates"){
   fill();Scope scope(memory,sdk);scope.bind(descriptor,4096);scope.bind(data,8192);self.lend(scope);
   PointerBinding binding(memory,data);c=original;c.r3.u64=address+4092;c.r4.u64=16;c.r5.u64=0x11223344;
   auto refuse=[&](auto f){bool caught=false;try{f();}catch(const std::logic_error&){caught=true;}catch(const brutal::instrumentation::Fault&){caught=true;}test(caught);if(caught)++expected_rejections;};
   refuse([&]{GeneratedImports::direct(fn_compare,c,sdk.virtual_membase());});
   scope.allow_synthetic(mapped(0x83000000,pc_echo),fn_echo);
   refuse([&]{GeneratedImports::indirect(mapped(0x83000000,pc_echo),c,sdk.virtual_membase());});
   GeneratedImports outer(admission,c,sdk.virtual_membase());outer.bind(fn_compare,thunk(0x83000000,0x11b),0x11b,Signature::u32_in_words_u32_u32,binding);
   refuse([&]{outer.bind(fn_compare,thunk(0x83000000,0x12e),0x12e,Signature::void_inout28,binding);});
   refuse([&]{outer.bind_indirect(0,0xee,Signature::u32_address_out28_u32,binding);});
   PPCContext foreign=c;refuse([&]{GeneratedImports::direct(fn_compare,foreign,sdk.virtual_membase());});
   refuse([&]{GeneratedImports::direct(fn_compare,c,nullptr);});
   {GeneratedImports nested(admission,c,sdk.virtual_membase());refuse([&]{GeneratedImports::direct(fn_compare,c,sdk.virtual_membase());});}
   GeneratedImports::direct(fn_compare,c,sdk.virtual_membase());test(c.r3.u32==3&&outer.attempts==1&&outer.returns==1);
   import_attempts+=outer.attempts;import_returns+=outer.returns;
  }else if(mode=="unsupported"){
   fill();Scope scope(memory,sdk);scope.bind(data,8192);self.lend(scope);
   scope.allow_synthetic(mapped(0x83000000,pc_atomic),fn_atomic);scope.allow_synthetic(mapped(0x83000000,pc_partial),fn_partial);scope.allow_synthetic(mapped(0x83000000,pc_load),fn_load);
   for(auto fn:{fn_atomic,fn_partial}){c=original;c.r4.u64=address;bool caught=false;try{scope.direct(fn,c,sdk.virtual_membase());}catch(const brutal::instrumentation::Fault&e){caught=e.kind==brutal::instrumentation::Kind::unsupported_raw_atomic;}test(caught);if(caught)++expected_rejections;}
   c.r4.u64=0x7f000000;bool mmio=false;try{scope.direct(fn_load,c,sdk.virtual_membase());}catch(const brutal::instrumentation::Fault&e){mmio=e.kind==brutal::instrumentation::Kind::mmio;}test(mmio);if(mmio)++expected_rejections;
   for(auto target:{address,mapped(0x83000000,pc_entry),0x83001001u,0u}){bool caught=false;try{scope.indirect(target,c,sdk.virtual_membase());}catch(const brutal::instrumentation::Fault&){caught=true;}test(caught);if(caught)++expected_rejections;} // No instruction-fetch fallback.
   // Guest dispatch/unwind has no admitted signature or registered entry.
   bool denied=false;try{admission.direct("xboxkrnl.exe",0x136,Signature::unverified,c);}catch(const std::logic_error&){denied=true;}test(denied);if(denied)++expected_rejections;
   reads+=scope.statistics.reads;writes+=scope.statistics.writes;completed+=scope.statistics.completed;
  }else throw std::invalid_argument("mode");
 };
 const unsigned cycles=mode=="repeat"?8:1;
 for(unsigned cycle=0;cycle<cycles;++cycle){std::array<object_ref<Thread>,4> ts;std::array<Memory::Allocation,4> ds,bs;unsigned n=mode=="concurrent"?4:1;
  for(unsigned id=0;id<n;++id){ds[id]=alloc(4096);bs[id]=alloc(8192,mode!="uncommitted");ts[id]=spawn([&,id,d=ds[id],data=bs[id]](Thread&self){exercise(self,id,d,data);});}
  for(unsigned id=0;id<n;++id){for(unsigned other=0;other<id;++other)check(ts[id]->tls_ptr()!=ts[other]->tls_ptr()&&ts[id]->pcr_ptr()!=ts[other]->pcr_ptr());check(ts[id]->Resume()==0);}
  if(mode=="lifetime"){
   {std::unique_lock lock(mutex);check(cv.wait_for(lock,std::chrono::seconds(5),[&]{return ready;}));}
   check(memory.try_release(bs[0])==Memory::ReleaseAttempt::busy);check(b->Unload()==0xc0000001u);check(!b->AcquireImageLease()&&live(sdk,0x83000000));
   {std::lock_guard lock(mutex);release=true;}cv.notify_one();
  }
  for(unsigned id=0;id<n;++id){finish(ts[id]);free(ds[id]);if(mode=="freed"||mode=="checked"){check(memory.release(bs[id])==Memory::Result::ownership);++reclaims;}else free(bs[id]);}
  check(!errors);
  if(mode=="repeat"&&cycle+1<cycles){check(b->Unload()==0);check(b->ReleaseHandle());b.reset();b=load("b.xex",0x83000000);}
 }
 const auto stats=PointerBinding::statistics();check(stats.scratch_allocations==stats.scratch_reclaims&&allocations==reclaims&&threads==tls_reclaims&&threads==destructors);
 check(b->Unload()==0);check(b->ReleaseHandle());b.reset();check(a->Unload()==0);check(a->ReleaseHandle());a.reset();check(imageheap->GetUnreservedPageCount()==imagebaseline&&systemheap->GetUnreservedPageCount()==systembaseline);
 std::printf("{\"group\":\"generated_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"expected_cpp_rejections\":%u,\"entry_invocations\":%llu,\"memory_reads\":%llu,\"memory_writes\":%llu,\"memory_completed\":%llu,\"import_attempts\":%llu,\"import_returns\":%llu,\"tls_allocations\":%u,\"tls_reclaims\":%u,\"thread_destructors\":%u,\"buffer_allocations\":%u,\"buffer_reclaims\":%u,\"scratch_allocations\":%llu,\"scratch_reclaims\":%llu,\"sdk_returns\":%llu}\n",mode.c_str(),checks,observations.load(),expected_rejections.load(),(unsigned long long)entries.load(),(unsigned long long)reads.load(),(unsigned long long)writes.load(),(unsigned long long)completed.load(),(unsigned long long)import_attempts.load(),(unsigned long long)import_returns.load(),threads.load(),tls_reclaims.load(),destructors.load(),allocations,reclaims,(unsigned long long)stats.scratch_allocations,(unsigned long long)stats.scratch_reclaims,(unsigned long long)stats.sdk_returns);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
