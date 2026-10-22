// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original fixtures only; compiled CLI output is linked as a separate TU.
#include "ppc_test_decls.h"
#include "module.h"
#include "layout.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <atomic>
#include <barrier>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <functional>

using namespace synthetic;
REX_EXTERN(__imp__KeTlsGetValue);
static unsigned checks=0;
static std::atomic<unsigned> thread_checks{0},thread_destructors{0},exit_scope_destructors{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("synthetic oracle "+std::to_string(checks));}
template<class F> static void invalid(F f){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}check(rejected);}
template<class F> static void fault(F f,Kind kind,Result result=Result::invalid_range){bool rejected=false;try{f();}catch(const Fault& e){rejected=e.kind==kind&&(kind!=Kind::memory||e.memory_result==result);}check(rejected);}
static std::vector<Section> sections(const std::filesystem::path& input){
 std::ifstream file(input/"module.bin",std::ios::binary);std::vector<char> raw((std::istreambuf_iterator<char>(file)),{});
 check(raw.size()==144);std::vector<std::byte> code;for(unsigned char c:raw)code.push_back(std::byte(c));
 std::vector<std::byte> ro;for(unsigned i=0;i<16;++i)ro.push_back(std::byte(i));
 std::vector<Section> result;
 for(auto s:fixture_sections)result.push_back({s.offset,s.size,s.write,s.code,s.code?code:s.write?std::vector<std::byte>{}:ro});
 return result;
}
static void functions(Module& m){
#define FN(n,addr) m.function(pc_##n,module_##addr)
 FN(entry,82010000);FN(direct,82010038);FN(indirect,82010044);FN(tls,82010050);FN(vector,82010060);FN(load,82010070);FN(store,82010078);FN(atomic,82010080);FN(partial,82010088);
#undef FN
}
static void throwing(PPCContext&,uint8_t*){throw std::runtime_error("original callback failure");}
class ExecutionThread final:public rex::system::XThread {
 std::function<int(ExecutionThread&)> body_;
 public:
 ~ExecutionThread() override {++thread_destructors;}
 uint32_t saved_stack=0,saved_pcr=0,saved_tls=0;
 ExecutionThread(rex::system::KernelState* k,std::function<int(ExecutionThread&)> body)
   :XThread(k,0x10000,0,0,0,1,true),body_(std::move(body)){}
 uint32_t low()const{return stack_limit_;} uint32_t high()const{return stack_base_;}
 void Execute() override{
  kernel_state()->OnThreadExecute(this);
  // Only this original thread uses its freshly created SDK allocations. Pin
  // this object outside Execute; no Terminate/Reset/Restore is permitted.
  saved_stack=stack_alloc_base_;saved_pcr=pcr_address_;saved_tls=tls_static_address_;
  for(auto a:{saved_stack,saved_pcr,saved_tls})memory()->LookupHeap(a)->SetAllocationOwner(a,this);
  struct ExitMarker {~ExitMarker(){++exit_scope_destructors;}} marker;
  int result=body_(*this);
  // All checked scopes have gone away before removing loans/SDK Exit frees.
  for(auto a:{saved_stack,saved_pcr,saved_tls})memory()->LookupHeap(a)->SetAllocationOwner(a,nullptr);
  Exit(result);
 }
 void lend_to(Scope& scope){
  scope.lend(saved_stack,this,stack_limit_,stack_base_-stack_limit_);
  scope.lend(saved_pcr,this,pcr_address_,0x2d8);
  scope.lend(saved_tls,this,tls_static_address_,tls_total_size_);
 }
};
int main(int argc,char** argv){
 if(argc!=4)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1],input=argv[2];std::string mode=argv[3];
  std::filesystem::create_directories(work/"empty");
  rex::Runtime runtime(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");
  check(runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})==0);
  auto* sdk=runtime.memory();Memory memory(*sdk);
  auto descriptors=sections(input);
  if(mode=="loading"){
   auto bad=descriptors;bad[1].offset=0;invalid([&]{Module m(memory,*sdk,bad,{});});
   for(uint32_t size:{0u,1u,0xfffff000u}){bad=descriptors;bad[0].size=size;invalid([&]{Module m(memory,*sdk,bad,{});});}
   bad=descriptors;bad[0].offset=0xffffc000;invalid([&]{Module m(memory,*sdk,bad,{});});
   bad=descriptors;bad[0].write=true;invalid([&]{Module m(memory,*sdk,bad,{});});
   bad=descriptors;bad[0].bytes.resize(8193);invalid([&]{Module m(memory,*sdk,bad,{});});
   for(auto r:{Relocation{0,0x4000},Relocation{0x8001,0},Relocation{0x8000,0xc000},Relocation{0xfffffffc,0}})
    invalid([&]{Module m(memory,*sdk,descriptors,{r});});
   invalid([&]{Module m(memory,*sdk,descriptors,{{0x8000,0},{0x8000,0}});});
   invalid([&]{Module m(memory,*sdk,descriptors,{},true);});
   Module m(memory,*sdk,descriptors,{{fixture_relocation_offset,fixture_relocation_target}});functions(m);auto b=m.base();
   check(!(b&16383));uint32_t target=0;check(memory.load32be(m.allocation(),0x8010,target)==Result::ok&&target==b+0x4000);
   invalid([&]{m.function(0x8000,throwing);});invalid([&]{m.function(1,throwing);});invalid([&]{m.function(0,throwing);});
   Scope scope(memory,*sdk);m.admit(scope);PPCContext c{};
   for(auto address:{0u,b+1,b+4,b+144,b+0x4000,b+0x8000,b+0xc000,0xfffffffcu})invalid([&]{m.enter(address,c);});
   fault([&]{brutal::instrumentation::store<uint32_t>(b,7);},Kind::memory,Result::protection);
   fault([&]{brutal::instrumentation::load<uint32_t>(b+0x2000);},Kind::memory,Result::uncommitted);
   auto h=m.allocation();m.unload();std::byte byte;check(memory.read(h,0,{&byte,1})==Result::ownership);
   invalid([&]{m.enter(b,c);});invalid([&]{m.admit(scope);});
   rex::memory::HeapAllocationInfo info{};check(sdk->LookupHeap(b)->QueryRegionInfo(b,&info)&&info.state==0);
  }else if(mode=="memory"||mode=="unsupported"){
   Module m(memory,*sdk,descriptors,{});functions(m);Scope scope(memory,*sdk);m.admit(scope);auto b=m.base();PPCContext c{};
   auto load=[&](uint32_t address){c.r4.u64=address;m.enter(b+pc_load,c);return c.r3.u32;};
   auto store=[&](uint32_t address,uint32_t value){c.r4.u64=address;c.r3.u64=value;m.enter(b+pc_store,c);};
   if(mode=="memory"){
    check(load(b+0x4000)==0x00010203);store(b+0x8fff,0x89abcdef);check(load(b+0x8fff)==0x89abcdef);
    std::array<std::byte,4> bytes{};check(memory.read(m.allocation(),0x8fff,bytes)==Result::ok);
    check(bytes==std::array<std::byte,4>{std::byte{0x89},std::byte{0xab},std::byte{0xcd},std::byte{0xef}});
    fault([&]{store(b+0x4000,7);},Kind::memory,Result::protection);check(load(b+0x4000)==0x00010203);
    auto* heap=sdk->LookupHeap(b);check(heap->Protect(b+0x9000,4096,1));
    fault([&]{store(b+0x8fff,7);},Kind::memory,Result::protection);check(load(b+0x8fff)==0x89abcdef);
    check(heap->Protect(b+0x9000,4096,0));fault([&]{load(b+0x8fff);},Kind::memory,Result::protection);
    check(heap->Protect(b+0x9000,4096,3));check(heap->Decommit(b+0x9000,4096));
    fault([&]{load(b+0x8fff);},Kind::memory,Result::uncommitted);
    for(auto address:{0u,0xfffffffcu,b+0xc000})fault([&]{load(address);},Kind::memory,address==0?Result::invalid_range:Result::ownership);
    c.r4.u64=b+0x4000;c.r3.u64=b+0x8040;m.enter(b+pc_vector,c);
    std::array<std::byte,16> vector{};check(memory.read(m.allocation(),0x8040,vector)==Result::ok);
    for(unsigned i=0;i<16;++i)check(vector[i]==std::byte(i*2));
    auto stale=m.base();m.unload();c.r4.u64=stale;fault([&]{module_82010070(c,nullptr);},Kind::memory,Result::ownership);
    invalid([&]{m.enter(stale,c);});
   }else{
    c.r4.u64=b+0x4000;
    fault([&]{m.enter(b+pc_atomic,c);},Kind::unsupported_raw_atomic);
    fault([&]{m.enter(b+pc_partial,c);},Kind::unsupported_raw_atomic);
    fault([&]{load(0x7fc80000);},Kind::mmio);
    std::array<std::byte,4> bytes{};bytes.fill(std::byte{0x55});
    check(memory.fetch(m.allocation(),0,bytes)==Result::unsupported_fetch);for(auto v:bytes)check(v==std::byte{0x55});
    invalid([&]{Module exception_module(memory,*sdk,descriptors,{},true);});
   }
  }else if(mode=="calls"){
   Module m(memory,*sdk,descriptors,{});functions(m);auto stack=memory.allocate(0x4000);auto b=m.base();PPCContext c{};
   auto seed=[&]{c={};std::memset(&c.f0,0x35,32*sizeof(PPCRegister));std::memset(&c.v0,0x7a,128*sizeof(PPCVRegister));c.r1.u64=stack.guest_address()+0x3e00;c.r2.u64=0x10203040;c.r13.u64=0x50607080;c.r14.u64=0x1122334455667788;c.lr=0x12345678;c.r3.u64=b+0x8000;c.r4.u64=b+0x4000;c.r12.u64=b+pc_indirect;};
   {Scope scope(memory,*sdk);m.admit(scope);scope.bind(stack,0x4000);seed();auto before=c;m.enter(b,c);
    check(c.r1.u64==before.r1.u64&&c.r2.u64==before.r2.u64&&c.r13.u64==before.r13.u64&&c.r14.u64==before.r14.u64&&c.lr==before.lr);
    check(!std::memcmp(&c.f0,&before.f0,32*sizeof(PPCRegister))&&!std::memcmp(&c.v0,&before.v0,128*sizeof(PPCVRegister)));
    for(unsigned i=0;i<3;++i){uint32_t value=0;check(memory.load32be(m.allocation(),0x8000+4*i,value)==Result::ok&&value==0x00010203+(i==0?0:i==1?1:3));}
    seed();c.r12.u64=0x1234;fault([&]{m.enter(b,c);},Kind::unsupported_call);
    seed();c.r12.u64=b+pc_indirect+1;fault([&]{m.enter(b,c);},Kind::unsupported_call);
    scope.allow_synthetic(0x2000,throwing);seed();auto sp=c.r1.u64;c.r12.u64=0x2000;bool propagated=false;
    try{m.enter(b,c);}catch(const std::runtime_error& e){propagated=std::string(e.what())=="original callback failure";}check(propagated&&c.r1.u64==sp-64); // no invented guest unwind
   }
   {Scope scope(memory,*sdk);scope.bind(m.allocation(),Module::image_size);scope.bind(stack,0x4000);scope.allow_synthetic(b,module_82010000);seed();fault([&]{m.enter(b,c);},Kind::unsupported_call);}
   seed();fault([&]{module_82010000(c,nullptr);},Kind::unbound);
  }else if(mode=="threads"){
   auto* kernel=runtime.kernel_state();auto* process=sdk->TranslateVirtual<rex::system::X_KPROCESS*>(kernel->GetTitleProcess());
   process->tls_slot_size=256;for(unsigned i=0;i<8;++i)process->tls_slot_bitmap[i]=i<2?0xffffffffu:0;
   std::barrier sync(2);std::atomic<unsigned> errors{0};std::array<uint32_t,2> pcr{},tls{},stacks{};
   std::array<rex::system::object_ref<ExecutionThread>,2> threads;
   for(unsigned id=0;id<2;++id){threads[id]=rex::system::object_ref<ExecutionThread>(new ExecutionThread(kernel,[&,id](ExecutionThread& self){
    auto record=[&](int line){std::fprintf(stderr,"THREAD %u check line %d\n",id,line);++errors;};
    auto test=[&](bool value,int line){++thread_checks;if(!value)record(line);};
    try{
     // Thread-local counters avoid a race in the test assertion accumulator.
     Module m(memory,*sdk,descriptors,{});functions(m);Scope scope(memory,*sdk);m.admit(scope);self.lend_to(scope);
     auto* c=rex::runtime::current_ppc_context();auto original=*c;pcr[id]=c->r13.u32;tls[id]=self.tls_ptr();stacks[id]=self.saved_stack;
     test(!(pcr[id]!=self.pcr_ptr()||c->r1.u64<self.low()||c->r1.u64>self.high()||(c->r1.u32&15)),__LINE__);
     c->r1.u64=self.high()-0x100; // Original harness ABI linkage/caller area.
     auto sp=c->r1.u64;auto r2=c->r2.u64;auto r13=c->r13.u64;c->r14.u64=0x1122334455667788;c->lr=0x13579bdf;
     c->r3.u64=m.base()+0x8000;c->r4.u64=m.base()+0x4000;c->r12.u64=m.base()+pc_indirect;m.enter(m.base(),*c);
     test(!(c->r1.u64!=sp||c->r2.u64!=r2||c->r13.u64!=r13||c->r14.u64!=0x1122334455667788||c->lr!=0x13579bdf),__LINE__);
     test(!(brutal::instrumentation::load<uint32_t>(uint32_t(sp)-64)!=sp),__LINE__);
     test(!(brutal::instrumentation::load<uint32_t>(pcr[id])!=tls[id]),__LINE__);
     c->r4.u64=m.base()+0x4000;c->r3.u64=m.base()+0x8040;m.enter(m.base()+pc_vector,*c);
     for(unsigned i=0;i<16;++i)test(!(brutal::instrumentation::load<uint8_t>(m.base()+0x8040+i)!=i*2),__LINE__);
     // Initially zero, then generated PCR/r13 -> real TLS slot 0 store/read.
     test(!(brutal::instrumentation::load<uint32_t>(tls[id])!=0),__LINE__);
     c->r3.u64=0xaabb0000+id;m.enter(m.base()+pc_tls,*c);
     sync.arrive_and_wait();
     c->r3.u64=0;__imp__KeTlsGetValue(*c,sdk->virtual_membase());test(!(c->r3.u32!=0xaabb0000+id),__LINE__);
     bool rejected=false;try{brutal::instrumentation::load<uint32_t>(tls[1-id]);}catch(const Fault& e){rejected=e.memory_result==Result::ownership;}test(rejected,__LINE__);
     for(auto address:{self.low()-4,self.high(),self.tls_ptr()+4094,self.pcr_ptr()+0x2d6}){
      rejected=false;try{brutal::instrumentation::load<uint32_t>(address);}catch(const Fault& e){rejected=e.memory_result==Result::ownership;}test(rejected,__LINE__);
     }
     c->r1.u64=self.low();rejected=false;try{m.enter(m.base(),*c);}catch(const Fault& e){rejected=e.memory_result==Result::ownership;}test(rejected&&c->r1.u64==self.low(),__LINE__);
     c->r13.u64=0;rejected=false;try{m.enter(m.base()+pc_tls,*c);}catch(const Fault& e){rejected=e.kind==Kind::memory&&e.memory_result==Result::invalid_range;}test(rejected,__LINE__);
     c->r13.u64=pcr[1-id];rejected=false;try{m.enter(m.base()+pc_tls,*c);}catch(const Fault& e){rejected=e.memory_result==Result::ownership;}test(rejected,__LINE__);
     *c=original;
    }catch(const Fault& e){std::fprintf(stderr,"THREAD fault kind=%d result=%d\n",int(e.kind),int(e.memory_result));record(__LINE__);std::abort();}catch(const std::exception& e){std::fprintf(stderr,"THREAD %s\n",e.what());record(__LINE__);std::abort();}
    return 0;
   }));check(threads[id]->Create()==0);}
   for(auto& thread:threads)check(thread->Resume()==0);
   for(auto& thread:threads)check(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);
   // The POSIX SDK Wait post_execution already joins the native thread.
   check(errors==0&&pcr[0]!=pcr[1]&&tls[0]!=tls[1]&&stacks[0]!=stacks[1]);
   unsigned retained=0;
   for(auto& thread:threads){rex::memory::HeapAllocationInfo info{};auto pcr_address=thread->saved_pcr,tls_address=thread->saved_tls;
    check(sdk->LookupHeap(thread->saved_stack)->QueryRegionInfo(thread->saved_stack,&info)&&info.state==0);check(thread->ReleaseHandle());thread.reset();
    for(auto address:{pcr_address,tls_address}){check(sdk->LookupHeap(address)->QueryRegionInfo(address,&info));if(info.state)++retained;}
   }
   std::printf("{\"diagnostic\":\"thread_teardown\",\"thread_destructors\":%u,\"exit_scope_destructors\":%u,\"retained_pcr_tls_regions\":%u,\"thread_observations\":%u}\n",thread_destructors.load(),exit_scope_destructors.load(),retained,thread_checks.load());
   check(retained==0&&thread_destructors==2&&exit_scope_destructors==2);
  }else if(mode=="loans"){
   invalid([&]{memory.allocate(0);});invalid([&]{memory.allocate(0xffffffff);});
   auto h=memory.allocate(0x4000);auto b=h.guest_address();Scope scope(memory,*sdk);int wrong_owner=0;
   scope.lend(b,&wrong_owner,b,4);fault([&]{brutal::instrumentation::load<uint32_t>(b);},Kind::memory,Result::ownership);
   invalid([&]{Module m(memory,*sdk,{},{});});
   fault([&]{scope.lend(b,nullptr,b,4);},Kind::memory);fault([&]{scope.lend(b,&wrong_owner,b,0);},Kind::memory);
   fault([&]{scope.lend(b,&wrong_owner,b,4);},Kind::memory);fault([&]{scope.bind(h,0x4000);},Kind::memory);
   {Scope nested(memory,*sdk);nested.bind(h,0x4000);fault([&]{nested.lend(b,&wrong_owner,b,4);},Kind::memory);}
   int owner=0,next_owner=0;auto system=sdk->SystemHeapAlloc(4096,4096,0,&owner);check(system!=0);
   {Scope loan(memory,*sdk);loan.lend(system,&owner,system+16,16);
    brutal::instrumentation::store<uint32_t>(system+16,0x12345678);check(brutal::instrumentation::load<uint32_t>(system+16)==0x12345678);
    fault([&]{brutal::instrumentation::load<uint32_t>(system+32);},Kind::memory,Result::ownership);
    check(sdk->SystemHeapFreeOwned(system,&owner));
    auto* heap=sdk->LookupHeap(system);check(heap->AllocFixed(system,4096,4096,3,3));heap->SetAllocationOwner(system,&next_owner);
    fault([&]{brutal::instrumentation::load<uint32_t>(system+16);},Kind::memory,Result::ownership);
    check(heap->ReleaseOwned(system,&next_owner));
   }
   // Two native 64-KiB virtual pages, checked before any crossing write.
   auto* heap=sdk->LookupHeap(0x60000000);uint32_t large=0;
   check(heap->AllocRange(0x60000000,0x60ffffff,0x20000,65536,3,3,false,&large));heap->SetAllocationOwner(large,&owner);
   {Scope loan(memory,*sdk);loan.lend(large,&owner,large,0x20000);
    brutal::instrumentation::store<uint64_t>(large+65532,0x1122334455667788ull);
    check(brutal::instrumentation::load<uint64_t>(large+65532)==0x1122334455667788ull);
    check(heap->Protect(large+65536,65536,1));
    fault([&]{brutal::instrumentation::store<uint64_t>(large+65532,0);},Kind::memory,Result::protection);
    check(brutal::instrumentation::load<uint64_t>(large+65532)==0x1122334455667788ull);
    check(heap->Decommit(large+65536,65536));
    fault([&]{brutal::instrumentation::load<uint64_t>(large+65532);},Kind::memory,Result::uncommitted);
    check(heap->ReleaseOwned(large,&owner));fault([&]{brutal::instrumentation::load<uint32_t>(large);},Kind::memory,Result::ownership);
   }
   for(auto address:{0x82010000u,0x90010000u,0xa0010000u}){
    Scope blocked(memory,*sdk);blocked.lend(address,&owner,address,4);
    fault([&]{brutal::instrumentation::load<uint32_t>(address);},Kind::memory,Result::unsupported_heap);
   }
  }else return 2;
  std::printf("{\"group\":\"synthetic_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"game_loaded\":false}\n",mode.c_str(),checks,thread_checks.load());return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
