// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original XEX bytes -> SDK loader -> real XThread TLS -> real export shims.
#include "fixture.h"
#include "runtime/import_admission.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/thread.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <atomic>
#include <barrier>
#include <cstdio>
#include <dlfcn.h>
using namespace rex::system;
using namespace brutal::imports;
static unsigned checks=0,tls_allocations=0,tls_reclaims=0;
static std::atomic<unsigned> observations{0},errors{0},destroyed{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("TLS admission oracle "+std::to_string(checks));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
template<class F>static void rejected(F f){bool caught=false;try{f();}catch(const std::logic_error&){caught=true;}check(caught);}
static void save(const std::filesystem::path&p,const std::vector<uint8_t>&b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());check(bool(f));}
class Thread:public XThread {
 std::function<void(Thread&)> body_;
 public:
 Thread(KernelState*k,std::function<void(Thread&)>body,uint32_t stack=65536):XThread(k,stack,0,0,0,1,true),body_(std::move(body)){}
 ~Thread()override{++destroyed;}
 uint32_t total()const{return tls_total_size_;}uint32_t dynamic()const{return tls_dynamic_address_;}
 uint32_t stack()const{return stack_alloc_base_;}
 void Execute()override{kernel_state()->OnThreadExecute(this);try{body_(*this);}catch(const std::exception&e){std::fprintf(stderr,"worker %s\n",e.what());++errors;}Exit(0);}
};
int main(int argc,char**argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 std::filesystem::path p=argv[1];std::string mode=argv[2];std::filesystem::create_directories(p/"empty");
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto&m=*r.memory();auto*k=r.kernel_state();auto*system=m.LookupHeapByType(false,4096);auto*heap=m.LookupHeap(0x82000000);
 auto sysbaseline=system->GetUnreservedPageCount(),baseline=heap->GetUnreservedPageCount();
 auto load=[&](std::string name,uint32_t base,uint8_t seed){save(p/"empty"/name,tls_fixture::image(base,seed));auto a=object_ref<UserModule>(new UserModule(k));check(a->LoadFromFile("game:\\"+name)==0);return a;};
 auto release=[&](object_ref<UserModule>&a){check(a->Unload()==0);check(a->ReleaseHandle());a.reset();};
 if(mode=="malformed"){
  for(unsigned n=0;n<13;++n){auto b=tls_fixture::image();switch(n){
   case 0:tls_fixture::be(b,0x380,0);break;case 1:tls_fixture::be(b,0x380,0xffffffff);break;
   case 2:tls_fixture::be(b,0x388,0xfffffffc);break;case 3:tls_fixture::be(b,0x38c,33);break;
   case 4:tls_fixture::be(b,0x384,0);break;case 5:tls_fixture::be(b,0x384,0xfffffffc);break;
   case 6:tls_fixture::be(b,0x384,0x82030000);break;case 7:tls_fixture::be(b,0x384,0x82010001);break;
   case 8:tls_fixture::be(b,0x388,31);break;case 9:tls_fixture::be(b,0x380,257);break;
   case 10:tls_fixture::be(b,0x38c,0);break;case 11:tls_fixture::be(b,0x384,0x8202fffc);break;case 12:tls_fixture::be(b,0x380,63);break;}
   rex::runtime::XexModule image(k->function_dispatcher(),k);check(image.Load("bad.xex","bad.xex",b.data(),b.size()));check(!image.LoadContinue());
   check(!image.loaded()&&!live(m,0x82000000)&&heap->GetUnreservedPageCount()==baseline);check(image.Unload());
  }
  // Explicit BSS-only layout is supported; default/no-header is a separate case.
  auto b=tls_fixture::image();tls_fixture::be(b,0x384,0);tls_fixture::be(b,0x38c,0);
  rex::runtime::XexModule image(k->function_dispatcher(),k);check(image.Load("bss.xex","bss.xex",b.data(),b.size())&&image.LoadContinue());
  rex::xex2_opt_tls_info info{};check(image.ReadTlsInfo(&info)&&info.data_size==32&&info.raw_data_size==0);check(image.Unload());
 }else if(mode=="lifetime"||mode=="repeat"){
  unsigned cycles=mode=="repeat"?8:1;
  for(unsigned cycle=0;cycle<cycles;++cycle){auto a=load("a.xex",0x82000000,0x31),b=load("b.xex",0x83000000,0x91);
   std::array<object_ref<Thread>,4> threads;std::array<uint32_t,4> tls{},pcr{},stack{};std::barrier gate(4);
   for(unsigned id=0;id<4;++id){auto module=id<2?a:b;threads[id]=object_ref<Thread>(new Thread(k,[&,id,module](Thread&self){
    auto test=[&](bool v){++observations;if(!v)throw std::runtime_error("thread TLS observation");};
    auto&c=*rex::runtime::current_ppc_context();auto*base=m.virtual_membase();
    tls[id]=self.tls_ptr();pcr[id]=self.pcr_ptr();stack[id]=self.stack();
    test(self.total()==288&&self.dynamic()==tls[id]+32);test(c.r13.u64==pcr[id]&&!(c.r1.u32&15));
    test(rex::memory::load_and_swap<uint32_t>(base+pcr[id])==tls[id]);
    for(unsigned i=0;i<32;++i)test(base[tls[id]+i]==(i<16?uint8_t((id<2?0x31:0x91)+i):0));
    Admission admission(module,*r.export_resolver(),m);uint64_t sp=c.r1.u64,toc=c.r2.u64;
    c.r3.u64=63;c.r4.u64=0x11220000+id;admission.direct("xboxkrnl.exe",0x155,Signature::u32_u32_u32,c);test(c.r3.u32==1);
    base[tls[id]+20]=uint8_t(id+1);gate.arrive_and_wait();
    c.r3.u64=63;admission.indirect((id<2?0x82000000:0x83000000)+0x20004,Signature::u32_u32,c);test(c.r3.u32==0x11220000+id&&base[tls[id]+20]==id+1);
    c.r3.u64=64;admission.direct("xboxkrnl.exe",0x154,Signature::u32_u32,c);test(c.r3.u32==0);
    c.r3.u64=0xffffffff;c.r4.u64=7;admission.direct("xboxkrnl.exe",0x155,Signature::u32_u32_u32,c);test(c.r3.u32==0);
    test(c.r1.u64==sp&&c.r2.u64==toc&&c.r13.u64==pcr[id]);
    PPCContext fake=c;bool refused=false;try{admission.direct("xboxkrnl.exe",0x154,Signature::u32_u32,fake);}catch(const std::logic_error&){refused=true;}test(refused);
    c.r13.u64=1;refused=false;try{admission.direct("xboxkrnl.exe",0x154,Signature::u32_u32,c);}catch(const std::logic_error&){refused=true;}c.r13.u64=pcr[id];test(refused);
   }));check(threads[id]->CreateWithModule(module)==0);}
   for(auto&t:threads)check(t->Resume()==0);
   for(auto&t:threads)check(rex::thread::Wait(t->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);
   check(!errors);check(a->Unload()==0xc0000001u&&b->Unload()==0xc0000001u);
   rejected([&]{PPCContext c{};Admission ad(a,*r.export_resolver(),m);ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);});
   for(unsigned id=0;id<4;++id){check(live(m,tls[id])&&live(m,pcr[id])&&!live(m,stack[id]));
    for(unsigned j=0;j<id;++j)check(tls[id]!=tls[j]&&pcr[id]!=pcr[j]);
    check(threads[id]->ReleaseHandle());threads[id].reset();check(!live(m,tls[id])&&!live(m,pcr[id]));}
   release(a);release(b);check(heap->GetUnreservedPageCount()==baseline&&system->GetUnreservedPageCount()==sysbaseline);
  }
  check(destroyed==4*cycles);tls_allocations=tls_reclaims=4*cycles;
 }else if(mode=="failure"){
  auto a=load("a.xex",0x82000000,0x31);auto t=object_ref<Thread>(new Thread(k,[](Thread&){},0x10000000));
  check(t->CreateWithModule(a)==0xc0000017u);check(t->tls_ptr()==0&&t->pcr_ptr()==0&&t->stack()==0);check(a->Unload()==0);
  check(t->ReleaseHandle());t.reset();check(destroyed==1);check(a->ReleaseHandle());a.reset();
 }else if(mode=="tls-zero"||mode=="tls-throw"||mode=="pcr-zero"||mode=="pcr-throw"||mode=="scratch-zero"||mode=="native-failure"){
  auto a=load("a.xex",0x82000000,0x31);auto t=object_ref<Thread>(new Thread(k,[](Thread&){}));
  unsigned(*count)()=nullptr;
  auto allocation_arm=reinterpret_cast<void(*)(unsigned,unsigned)>(dlsym(RTLD_DEFAULT,"fixture_arm_tls_failure"));
  auto tls_address=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_tls_address"));check(allocation_arm&&tls_address);allocation_arm(0,288);
  if(mode=="native-failure"){
   auto arm=reinterpret_cast<void(*)()>(dlsym(RTLD_DEFAULT,"fixture_arm_create_failure"));
   count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_create_failure_count"));check(arm&&count);arm();
  }else{
   auto arm=reinterpret_cast<void(*)(unsigned,unsigned)>(dlsym(RTLD_DEFAULT,"fixture_arm_tls_failure"));
   count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_tls_failure_count"));check(arm&&count);
   arm(mode.ends_with("throw")?2:1,mode.starts_with("pcr")?0x2d8:mode.starts_with("scratch")?64:288);
  }
  auto process=m.TranslateVirtual<X_KPROCESS*>(k->GetTitleProcess());auto before=uint32_t(process->thread_count);
  check(t->CreateWithModule(a)==0xc0000017u);check(count()==1&&uint32_t(process->thread_count)==before);
  check(t->tls_ptr()==0&&t->pcr_ptr()==0&&t->stack()==0&&t->thread()==nullptr);
  auto address=tls_address();bool allocated=mode.starts_with("pcr")||mode=="native-failure";check(bool(address)==allocated);check(!live(m,address));
  tls_allocations=tls_reclaims=allocated?1:0;check(a->Unload()==0);
  check(t->ReleaseHandle());t.reset();check(destroyed==1);check(a->ReleaseHandle());a.reset();
 }else if(mode=="denied-matrix"){
  save(p/"empty/all.xex",tls_fixture::image(0x82000000,0x31,true));auto a=object_ref<UserModule>(new UserModule(k));
  check(a->LoadFromFile("game:\\all.xex")==0);Admission ad(a,*r.export_resolver(),m);PPCContext c{};unsigned denied=0;
  for(const auto&entry:tls_fixture::matrix_imports){
   bool supported=(std::string_view(entry.library)=="xboxkrnl.exe"&&(entry.ordinal==0x154||entry.ordinal==0x155||entry.ordinal==0x135||entry.ordinal==0x83))||
     (std::string_view(entry.library)=="xam.xex"&&(entry.ordinal==0x3cb||entry.ordinal==0x3cd));
   if(supported)continue;++denied;
   for(auto sig:{Signature::u32_void,Signature::u32_u32,Signature::u32_u32_u32}){auto before=c;rejected([&]{ad.direct(entry.library,entry.ordinal,sig,c);});check(std::memcmp(&before,&c,sizeof c)==0);}
  }
  check(denied==27);release(a);
 }else if(mode=="admission"){
  auto a=load("a.xex",0x82000000,0x31);Admission ad(a,*r.export_resolver(),m);PPCContext c{};c.r1.u64=0x1230;c.r2.u64=0x123456789abcdef0;c.r13.u64=0x5555;c.r31.u64=0x8877665544332211;
  auto saved=c;rejected([&]{ad.direct("xboxkrnl.exe",0x154,Signature::u32_u32,c);});check(std::memcmp(&saved,&c,sizeof c)==0);
  for(auto sig:{Signature::unverified,Signature::typed_vector_argument,Signature::typed_vector_return,Signature::mixed_vector,Signature::variadic,Signature::u32_void}){
   rejected([&]{ad.direct("xboxkrnl.exe",0x135,sig,c);});check(std::memcmp(&saved,&c,sizeof c)==0);}
  for(auto name:{"missing","xboxkrnl.exe.extra","XBOXKRNL.EXE"})rejected([&]{ad.direct(name,0x135,Signature::u32_u32,c);});
  rejected([&]{ad.direct("xboxkrnl.exe",0xffff,Signature::u32_u32,c);});rejected([&]{ad.direct("xboxkrnl.exe",0x52,Signature::u32_u32,c);});
  for(uint32_t target:{0u,0x82001000u,0x82020000u,0x82020005u,0xffffffffu})rejected([&]{ad.indirect(target,Signature::u32_u32,c);});
  for(auto pair:{std::pair{0u,0u},std::pair{0xc0000008u,6u},std::pair{0x12345678u,317u}}){c.r3.u64=pair.first;ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);check(c.r3.u32==pair.second);c.r3.u64=pair.first;ad.indirect(0x82020014,Signature::u32_u32,c);check(c.r3.u32==pair.second);}
  ad.direct("xam.xex",0x3cb,Signature::u32_void,c);check(c.r3.u32==6);ad.direct("xam.xex",0x3cd,Signature::u32_void,c);check(c.r3.u32==1);
  ad.direct("xboxkrnl.exe",0x83,Signature::u32_void,c);check(c.r3.u32>0);
  check(c.r1.u64==saved.r1.u64&&c.r2.u64==saved.r2.u64&&c.r13.u64==saved.r13.u64&&c.r31.u64==saved.r31.u64);
  auto*e=r.export_resolver()->GetExportByOrdinal("xboxkrnl.exe",0x135);char old=e->name[0];e->name[0]='?';
  rejected([&]{ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);});e->name[0]=old;
  auto old_ordinal=e->ordinal;e->ordinal=0xffff;rejected([&]{ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);});e->ordinal=old_ordinal;
  auto old_type=e->type;e->type=rex::runtime::Export::Type::kVariable;rejected([&]{ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);});e->type=old_type;
  check(a->Unload()==0);rejected([&]{ad.direct("xboxkrnl.exe",0x135,Signature::u32_u32,c);});check(a->ReleaseHandle());a.reset();
 }else return 2;
 check(!errors&&heap->GetUnreservedPageCount()==baseline&&system->GetUnreservedPageCount()==sysbaseline);
 std::printf("{\"group\":\"tls_import_%s\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"thread_destructors\":%u,\"tls_allocations\":%u,\"tls_reclaims\":%u}\n",mode.c_str(),checks,observations.load(),destroyed.load(),tls_allocations,tls_reclaims);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
