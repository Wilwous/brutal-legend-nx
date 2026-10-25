// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original SDK state observer. No generated function is linked or invoked.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/system/user_module.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <filesystem>
#include <cstdio>
#include <atomic>
#include <array>
#include <algorithm>
using namespace rex::system;
static unsigned assertions=0;static std::atomic<unsigned>errors{0},destroyed{0};
static void check(bool ok){++assertions;if(!ok)throw std::runtime_error("original context contract "+std::to_string(assertions));}
class Observer final:public XThread{
 public:uint32_t stack=0;uint64_t sp=0,pcr=0;bool zero=false,aligned=false,bound=false;
 explicit Observer(KernelState*k):XThread(k,65536,0,0,0,1,true){}
 ~Observer()override{++destroyed;}
 void Execute()override{
  kernel_state()->OnThreadExecute(this);stack=stack_alloc_base_;const auto&c=*rex::runtime::current_ppc_context();sp=c.r1.u64;pcr=c.r13.u64;
  const std::array<uint64_t,30> other_gprs{c.r0.u64,c.r2.u64,c.r3.u64,c.r4.u64,c.r5.u64,c.r6.u64,c.r7.u64,c.r8.u64,c.r9.u64,c.r10.u64,c.r11.u64,c.r12.u64,c.r14.u64,c.r15.u64,c.r16.u64,c.r17.u64,c.r18.u64,c.r19.u64,c.r20.u64,c.r21.u64,c.r22.u64,c.r23.u64,c.r24.u64,c.r25.u64,c.r26.u64,c.r27.u64,c.r28.u64,c.r29.u64,c.r30.u64,c.r31.u64};
  zero=c.lr==0&&std::all_of(other_gprs.begin(),other_gprs.end(),[](uint64_t value){return value==0;});
  for(size_t i=0;i<other_gprs.size();++i)if(other_gprs[i])std::fprintf(stderr,"PRE_GUEST_NONZERO slot=%zu value=%016llx\n",i,(unsigned long long)other_gprs[i]);
  if(c.lr)std::fprintf(stderr,"PRE_GUEST_NONZERO LR=%016llx\n",(unsigned long long)c.lr);
  aligned=!(sp&15)&&sp==stack_base_&&sp-stack_limit_==65536;bound=GetCurrentThread()==this&&pcr==pcr_ptr();if(!zero||!aligned||!bound)++errors;
  Exit(0); // Never call XThread::Execute's guest dispatcher or DeliverAPCs.
 }
};
static bool live(rex::memory::Memory&m,uint32_t a){rex::memory::HeapAllocationInfo i{};return a&&m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state;}
int main(int argc,char**argv){if(argc!=3)return 2;rex::InitLoggingEarly();unsigned active=0;
 struct Guard{unsigned&v;~Guard(){if(v)std::_Exit(70);}}guard{active};
 try{
  const std::filesystem::path input=argv[1],work=argv[2];std::filesystem::create_directories(work);
  rex::Runtime runtime(input,work/"user",work/"updates",work/"cache",work/"metadata");check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));auto&memory=*runtime.memory();auto*ih=memory.LookupHeap(0x83000000);auto*vh=memory.LookupHeapByType(false,4096);auto ib=ih->GetUnreservedPageCount(),vb=vh->GetUnreservedPageCount();
  auto module=object_ref<UserModule>(new UserModule(runtime.kernel_state()));check(module->LoadFromFile("game:\\fixture.xex")==0);
  auto thread=object_ref<Observer>(new Observer(runtime.kernel_state()));check(thread->CreateWithModule(module)==0);active=1;check(thread->Resume()==0);
  if(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(15))!=rex::thread::WaitResult::kSuccess)std::_Exit(70);active=0;if(errors||!thread->zero||!thread->aligned||!thread->bound)std::fprintf(stderr,"snapshot zero=%u aligned=%u bound=%u errors=%u\n",thread->zero,thread->aligned,thread->bound,errors.load());check(!errors&&thread->zero&&thread->aligned&&thread->bound);
  auto tls=thread->tls_ptr(),pcr=thread->pcr_ptr();check(!live(memory,thread->stack)&&live(memory,tls)&&live(memory,pcr));check(module->Unload()==0xc0000001u);check(thread->ReleaseHandle());thread.reset();check(destroyed==1&&!live(memory,tls)&&!live(memory,pcr));check(module->Unload()==0);check(module->ReleaseHandle());module.reset();check(ih->GetUnreservedPageCount()==ib&&vh->GetUnreservedPageCount()==vb);
  std::printf("{\"group\":\"sdk_pre_guest_context\",\"result\":\"PASS\",\"assertions\":%u,\"sampled_stage\":\"original observer before guest argument/APC dispatch\",\"sdk_zero_defaults\":true,\"stack_tls_pcr_reclaimed\":1,\"image_reclaimed\":1,\"guest_functions_invoked\":0,\"title_contract_verified\":false}\n",assertions);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"%s\n",e.what());return 1;}
}
