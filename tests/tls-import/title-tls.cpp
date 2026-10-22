// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Header-driven *default* title-process path, in its own watchdog child.
#include "fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/user_module.h>
#include <rex/system/xthread.h>
#include <rex/system/kernel_state.h>
#include <rex/thread.h>
#include <filesystem>
#include <fstream>
#include <atomic>
#include <cstdio>
REX_EXTERN(__imp__KeTlsAlloc);REX_EXTERN(__imp__KeTlsFree);
REX_EXTERN(__imp__KeTlsGetValue);REX_EXTERN(__imp__KeTlsSetValue);
using namespace rex::system;
static unsigned checks=0;static std::atomic<unsigned> errors{0},observed{0},destructors{0},modules{0};
static void check(bool v){++checks;if(!v)throw std::runtime_error("title TLS oracle "+std::to_string(checks));}
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
static uint32_t tls=0,pcr=0,dispatch_tls=0,dispatch_pcr=0;
class Module:public UserModule{public:using UserModule::UserModule;~Module()override{if(live(*memory(),tls)||live(*memory(),pcr)||live(*memory(),dispatch_tls)||live(*memory(),dispatch_pcr))++errors;++modules;}};
class Thread:public XThread{public:using XThread::XThread;~Thread()override{++destructors;}
 void Execute()override{kernel_state()->OnThreadExecute(this);auto&c=*rex::runtime::current_ppc_context();auto*base=memory()->virtual_membase();
  auto test=[](bool v){++observed;if(!v)++errors;};tls=tls_ptr();pcr=pcr_ptr();
  test(base[tls]==0x51&&base[tls+15]==0x60&&base[tls+16]==0&&c.r13.u64==pcr);
  for(unsigned i=0;i<64;++i){__imp__KeTlsAlloc(c,base);test(c.r3.u32==i);c.r3.u64=i;c.r4.u64=i+1;__imp__KeTlsSetValue(c,base);test(c.r3.u32==1);}
  __imp__KeTlsAlloc(c,base);test(c.r3.u32==0xffffffff);
  c.r3.u64=64;__imp__KeTlsFree(c,base);test(c.r3.u32==0);
  for(unsigned i=0;i<64;++i){c.r3.u64=i;__imp__KeTlsGetValue(c,base);test(c.r3.u32==i+1);c.r3.u64=i;__imp__KeTlsFree(c,base);test(c.r3.u32==1);c.r3.u64=i;__imp__KeTlsGetValue(c,base);test(c.r3.u32==0);}
  c.r3.u64=0;__imp__KeTlsFree(c,base);test(c.r3.u32==0);Exit(0);
 }
};
int main(int argc,char**argv){if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();try{
 std::filesystem::path p=argv[1];std::filesystem::create_directories(p/"empty");auto bytes=tls_fixture::image(0x82000000,0x51);
 {std::ofstream f(p/"empty/title.xex",std::ios::binary);f.write(reinterpret_cast<char*>(bytes.data()),bytes.size());check(bool(f));}
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto a=object_ref<UserModule>(new Module(r.kernel_state()));check(a->LoadFromFile("game:\\title.xex")==0);
 r.kernel_state()->SetExecutableModule(a);
 auto*process=r.memory()->TranslateVirtual<X_KPROCESS*>(r.kernel_state()->GetTitleProcess());
 check(process->tls_slot_size==256&&process->tls_data_size==32&&process->tls_raw_data_size==16&&process->tls_static_data_address==0x82010000);
 check(process->tls_slot_bitmap[0]==0xffffffff&&process->tls_slot_bitmap[1]==0xffffffff&&process->tls_slot_bitmap[2]==0);
 auto t=object_ref<Thread>(new Thread(r.kernel_state(),65536,0,0,0,1,true));check(t->Create()==0);check(t->Resume()==0);
 check(rex::thread::Wait(t->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);std::printf("{\"diagnostic\":\"title_tls\",\"observations\":%u,\"errors\":%u}\n",observed.load(),errors.load());check(!errors&&observed==324);
 check(a->Unload()==0xc0000001u);check(t->ReleaseHandle());t.reset();check(destructors==1&&!live(*r.memory(),tls)&&!live(*r.memory(),pcr));
 {
  auto objects=r.kernel_state()->object_table()->GetObjectsByType<XThread>();unsigned host=0;
  for(auto&thread:objects)if(!thread->is_guest_thread()){++host;dispatch_tls=thread->tls_ptr();dispatch_pcr=thread->pcr_ptr();}
  check(host==1&&live(*r.memory(),dispatch_tls)&&live(*r.memory(),dispatch_pcr));
 }
 check(a->ReleaseHandle());a.reset();
 // Runtime owns the dispatch worker and executable reference. This exercises
 // real shutdown, never forces counters or bypasses the SDK thread join.
 r.Shutdown();check(modules==1&&!errors);
 std::printf("{\"group\":\"header_title_tls\",\"result\":\"PASS\",\"assertions\":%u,\"thread_observations\":%u,\"tls_allocations\":2,\"tls_reclaims\":2}\n",checks,observed.load());return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
