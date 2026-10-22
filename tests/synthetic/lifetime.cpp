// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original lifecycle probes. No title image or proprietary input.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <atomic>
#include <array>
#include <cerrno>
#include <barrier>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
#include <dlfcn.h>
#include <functional>
#include <mutex>
#include <memory>
#include <pthread.h>
#include <stdexcept>
#include <vector>
using namespace rex::system;
REX_EXTERN(__imp__ExTerminateThread);
static unsigned checks=0;
static void check(bool value){++checks;if(!value)throw std::runtime_error("lifetime oracle "+std::to_string(checks));}
struct State {
 std::atomic<unsigned> entered{0},ended{0},destructors{0},frames{0},callbacks{0},callback_frames{0},errors{0};
 std::atomic<bool> release{false},at_gate{false};std::mutex mutex;std::condition_variable condition;
 uint32_t stack=0,low=0,high=0,pcr=0,tls=0,id=0;
};
class LifetimeThread final:public XThread {
 State& state_;std::function<void(LifetimeThread&)> body_;
 public:
 LifetimeThread(KernelState* k,State& state,std::function<void(LifetimeThread&)> body,uint32_t stack=65536)
  :XThread(k,stack,0,0,0,1,true),state_(state),body_(std::move(body)){}
 ~LifetimeThread()override{if(state_.entered&&(!state_.ended||state_.callbacks))++state_.errors;++state_.destructors;}
 void snapshot(){state_.stack=stack_alloc_base_;state_.low=stack_limit_;state_.high=stack_base_;state_.pcr=pcr_ptr();state_.tls=tls_ptr();state_.id=thread_id();}
 struct Frame {
  LifetimeThread& thread;~Frame(){auto& s=thread.state_;auto* m=thread.memory();
   if(!m->IsRangeAccessible(s.low,s.high-s.low,false)||!m->IsRangeAccessible(s.pcr,0x2d8,false)||!m->IsRangeAccessible(s.tls,4096,false))++s.errors;
   ++s.frames;++s.ended;
  }
 };
 struct CallbackFrame {State& state;~CallbackFrame(){--state.callbacks;++state.callback_frames;}};
 void callback(const std::function<void()>& body){++state_.callbacks;CallbackFrame guard{state_};body();}
 void Execute()override{kernel_state()->OnThreadExecute(this);++state_.entered;Frame frame{*this};body_(*this);}
};
using Thread=rex::system::object_ref<LifetimeThread>;
static bool live(rex::memory::Memory& memory,uint32_t address){rex::memory::HeapAllocationInfo info{};return address&&memory.LookupHeap(address)->QueryRegionInfo(address,&info)&&info.state!=0;}
static void wait_for(State& state){std::unique_lock lock(state.mutex);check(state.condition.wait_for(lock,std::chrono::seconds(5),[&]{return state.at_gate.load();}));}
static void release(State& state){state.release=true;state.condition.notify_all();}
static void hold(State& state){state.at_gate=true;state.condition.notify_all();std::unique_lock lock(state.mutex);if(!state.condition.wait_for(lock,std::chrono::seconds(5),[&]{return state.release.load();}))throw std::runtime_error("fixture gate timeout");}
static void join(Thread& thread){check(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess);}
static void freed(rex::memory::Memory& memory,KernelState& kernel,State& state,bool executed=true){
 check(state.destructors==1&&state.errors==0&&state.callbacks==0);
 check(!live(memory,state.stack)&&!live(memory,state.pcr)&&!live(memory,state.tls));
 check(!kernel.GetThreadByID(state.id));
 if(executed)check(state.entered==1&&state.ended==1&&state.frames==1);
 else check(state.entered==0&&state.ended==0&&state.frames==0);
}
int main(int argc,char** argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path path=argv[1];std::string mode=argv[2];std::filesystem::create_directories(path/"empty");
  rex::Runtime runtime(path/"empty",path/"user",path/"updates",path/"cache",path/"metadata");
  check(!runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto& memory=*runtime.memory();auto& kernel=*runtime.kernel_state();
  auto* process=memory.TranslateVirtual<X_KPROCESS*>(kernel.GetTitleProcess());
  auto make=[&](State& s,std::function<void(LifetimeThread&)> fn,uint32_t stack=65536){return Thread(new LifetimeThread(&kernel,s,std::move(fn),stack));};
  auto prepare=[&](Thread& thread){check(thread->Create()==0);thread->snapshot();};
  auto finish=[&](Thread& thread,State& state,int code){join(thread);
   auto* guest=memory.TranslateVirtual<X_KTHREAD*>(thread->guest_object());
   check(guest->terminated==1&&guest->exit_status==uint32_t(code));
   check(!thread->is_running()&&!live(memory,state.stack));
   check(state.frames==1&&state.destructors==0&&live(memory,state.pcr)&&live(memory,state.tls));
   check(thread->ReleaseHandle());thread.reset();freed(memory,kernel,state);
  };
  auto process_count=uint32_t(process->thread_count);
  if(mode=="normal"||mode=="self-terminate"||mode=="return"||mode=="export-exit"||mode=="callback-error"||mode=="callback-exit"){
   State state;auto thread=make(state,[&](LifetimeThread& self){
    if(mode=="return")return;
    if(mode=="callback-error"){
     bool caught=false;try{self.callback([]{throw std::runtime_error("original callback");});}catch(const std::runtime_error& e){caught=std::string(e.what())=="original callback";}
     if(!caught||state.callbacks||state.callback_frames!=1)++state.errors;self.Exit(13);
    }
    if(mode=="callback-exit")self.callback([&]{self.Exit(13);});
    if(mode=="self-terminate")self.Terminate(13);
    if(mode=="export-exit"){auto* context=rex::runtime::current_ppc_context();context->r3.u64=13;__imp__ExTerminateThread(*context,memory.virtual_membase());}
    self.Exit(13);
   });prepare(thread);check(live(memory,state.pcr)&&live(memory,state.tls));check(thread->Resume()==0);finish(thread,state,mode=="return"?0:13);
   if(mode.starts_with("callback"))check(state.callback_frames==1&&state.callbacks==0);
  }else if(mode=="creation-failure"||mode=="host-creation-failure"){
   unsigned(*failure_count)()=nullptr;
   if(mode=="host-creation-failure"){
    auto arm=reinterpret_cast<void(*)()>(dlsym(RTLD_DEFAULT,"fixture_arm_create_failure"));
    failure_count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_create_failure_count"));
    check(arm&&failure_count);arm();
   }
   State state;auto thread=make(state,[](LifetimeThread&){},mode=="creation-failure"?0x10000000:65536);
   auto status=thread->Create();thread->snapshot();
   std::printf("{\"diagnostic\":\"creation_failure\",\"status\":%u,\"process_count_before\":%u,\"process_count_after\":%u}\n",status,process_count,uint32_t(process->thread_count));
   check(status==0xc0000017u);check(thread->thread()==nullptr);
   check(uint32_t(process->thread_count)==process_count);check(thread->ReleaseHandle());check(thread->handles().empty());thread.reset();freed(memory,kernel,state,false);
   if(failure_count){
   check(failure_count()==1);
   // A failed native factory must destroy the unconsumed start-data callback.
   std::atomic<unsigned> destroyed{0};
   struct Capture {std::atomic<unsigned>& destroyed;~Capture(){++destroyed;}};
   auto capture=std::shared_ptr<Capture>(new Capture{destroyed});
   auto native=rex::thread::Thread::Create({},[capture]{});check(!native);capture.reset();
   check(destroyed==1&&failure_count()==2);
   auto closes=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_sem_close_count"));check(closes&&closes()==2);
   }
  }else if(mode=="native-creation-failure"){
   auto arm=reinterpret_cast<void(*)()>(dlsym(RTLD_DEFAULT,"fixture_arm_create_failure"));
   auto failure_count=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_create_failure_count"));
   check(arm&&failure_count);arm();std::atomic<unsigned> destroyed{0};
   struct Capture {std::atomic<unsigned>& destroyed;~Capture(){++destroyed;}};
   auto capture=std::shared_ptr<Capture>(new Capture{destroyed});
   auto native=rex::thread::Thread::Create({},[capture]{});check(!native);capture.reset();
   std::printf("{\"diagnostic\":\"native_callback_ownership\",\"capture_destructors\":%u,\"injected_failures\":%u}\n",destroyed.load(),failure_count());
   check(destroyed==1&&failure_count()==1);
   auto closes=reinterpret_cast<unsigned(*)()>(dlsym(RTLD_DEFAULT,"fixture_sem_close_count"));check(closes&&closes()==1);
  }else if(mode=="owners"){
   State state;auto thread=make(state,[](LifetimeThread& self){self.Exit(13);});prepare(thread);
   Thread copied(thread);auto registry=kernel.GetThreadByID(thread->thread_id());check(registry.get()==thread.get());
   check(thread->Resume()==0);join(thread);check(thread->ReleaseHandle());thread.reset();
   check(state.destructors==0&&live(memory,state.pcr)&&live(memory,state.tls));
   copied.reset();check(state.destructors==0);registry.reset();freed(memory,kernel,state);
  }else if(mode=="release-before-exit"){
   State state;auto thread=make(state,[&](LifetimeThread& self){hold(state);self.Exit(13);});prepare(thread);
   auto native=reinterpret_cast<pthread_t>(thread->thread()->native_handle());check(thread->Resume()==0);wait_for(state);
   check(rex::thread::Wait(thread->thread(),false,std::chrono::milliseconds(1))==rex::thread::WaitResult::kTimeout);
   check(thread->ReleaseHandle());thread.reset();
   check(state.destructors==0&&state.ended==0&&live(memory,state.pcr)&&live(memory,state.tls)&&live(memory,state.stack));
   release(state);void* result=nullptr;
   // No SDK object owner remains: join the independent pthread handle exactly
   // once. Never dereference the SDK/native Thread after its lease may expire.
   check(pthread_join(native,&result)==0&&reinterpret_cast<intptr_t>(result)==13);freed(memory,kernel,state);
  }else if(mode=="foreign-termination"){
   State state;auto thread=make(state,[&](LifetimeThread& self){self.callback([&]{hold(state);});self.Exit(13);});prepare(thread);
   check(thread->Resume()==0);wait_for(state);bool rejected=false;
   auto* guest=memory.TranslateVirtual<X_KTHREAD*>(thread->guest_object());
   auto before_signal=uint32_t(guest->header.signal_state),before_status=uint32_t(guest->exit_status);auto before_terminated=uint8_t(guest->terminated);
   try{thread->Terminate(99);}catch(const std::runtime_error& e){rejected=std::string(e.what())=="External XThread termination is unsupported";}
   check(rejected&&state.destructors==0&&state.ended==0&&state.callbacks==1);
   check(uint32_t(guest->header.signal_state)==before_signal&&uint32_t(guest->exit_status)==before_status&&uint8_t(guest->terminated)==before_terminated);
   check(thread->is_running()&&live(memory,state.stack)&&live(memory,state.pcr)&&live(memory,state.tls));
   release(state);finish(thread,state,13);check(state.callback_frames==1);
  }else if(mode=="repeat"){
   for(unsigned i=0;i<16;++i){State state;auto thread=make(state,[&](LifetimeThread& self){self.Exit(13);});prepare(thread);check(thread->Resume()==0);finish(thread,state,13);check(uint32_t(process->thread_count)==process_count);}
  }else if(mode=="simultaneous"){
   std::barrier sync(4);std::array<State,4> states;std::array<Thread,4> threads;
   for(unsigned i=0;i<4;++i){threads[i]=make(states[i],[&](LifetimeThread& self){sync.arrive_and_wait();self.Exit(13);});prepare(threads[i]);}
   for(auto& thread:threads)check(thread->Resume()==0);
   for(unsigned i=0;i<4;++i)finish(threads[i],states[i],13);
  }else return 2;
  check(uint32_t(process->thread_count)==process_count);
  std::printf("{\"group\":\"lifetime_%s\",\"result\":\"PASS\",\"assertions\":%u}\n",mode.c_str(),checks);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
