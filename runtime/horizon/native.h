// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/checked_memory.h"
#include <functional>
#ifdef __SWITCH__
#include <switch.h>
#else
#include <thread>
#include <condition_variable>
#include <chrono>
#endif
namespace brutal::horizon {
// Native synchronization only. Neither class implements an Xbox wait/APC API.
class Event {
#ifdef __SWITCH__
  Mutex mutex_{};CondVar cond_{};
#else
  std::mutex mutex_;std::condition_variable cond_;
#endif
  bool set_{};
 public:
  void signal(){
#ifdef __SWITCH__
    mutexLock(&mutex_);set_=true;auto rc=condvarWakeAll(&cond_);mutexUnlock(&mutex_);if(R_FAILED(rc))throw foundation::Error(foundation::Fault::unsupported,"native condition wake failure");
#else
    {std::lock_guard lock(mutex_);set_=true;}cond_.notify_all();
#endif
  }
  bool wait(uint64_t milliseconds=2000){
    if(milliseconds>2000)throw foundation::Error(foundation::Fault::argument,"event wait budget");
#ifdef __SWITCH__
    const uint64_t deadline=armGetSystemTick()+armNsToTicks(milliseconds*1000000);
    mutexLock(&mutex_);
    while(!set_){
      auto now=armGetSystemTick();if(now>=deadline){mutexUnlock(&mutex_);return false;}
      auto rc=condvarWaitTimeout(&cond_,&mutex_,armTicksToNs(deadline-now));
      if(R_FAILED(rc)&&!set_){mutexUnlock(&mutex_);return false;}
    }
    mutexUnlock(&mutex_);return true;
#else
    std::unique_lock lock(mutex_);return cond_.wait_for(lock,std::chrono::milliseconds(milliseconds),[&]{return set_;});
#endif
  }
};
class NativeThread {
 public:
  enum class State { empty,starting,running,exited,joined,failed };
 private:
  std::atomic<State> state_{State::empty};
#ifdef __SWITCH__
  Thread thread_{};
#else
  std::thread thread_;
#endif
  std::function<void()> work_;
  Event done_;
  bool started_{},joined_{};
  std::atomic<bool> callback_error_{};
  static void entry(void* raw) noexcept {
    auto& self=*static_cast<NativeThread*>(raw);
    self.state_.store(State::running);
    try{self.work_();}catch(...){self.callback_error_=true;}
    self.state_.store(State::exited);
    self.done_.signal();
  }
 public:
  enum class Injection { none,creation_failure }; // Original deterministic failure fixture, not OS evidence.
  NativeThread()=default;
  NativeThread(const NativeThread&)=delete;
  ~NativeThread(){if(started_&&!joined_)std::terminate();}
  bool start(std::function<void()> work,size_t stack=64*1024,Injection injection=Injection::none){
    if(started_||!work||stack<32*1024||stack>256*1024||(stack&4095))
      throw foundation::Error(foundation::Fault::argument,"native thread start admission");
    state_=State::starting;
    if(injection==Injection::creation_failure){state_=State::failed;return false;}
    work_=std::move(work);
#ifdef __SWITCH__
    auto rc=threadCreate(&thread_,entry,this,nullptr,stack,0x3b,-2);
    if(R_FAILED(rc)){work_={};state_=State::failed;return false;}
    rc=threadStart(&thread_);
    if(R_FAILED(rc)){auto closed=threadClose(&thread_);if(R_FAILED(closed))std::terminate();work_={};state_=State::failed;return false;}
#else
    try{thread_=std::thread([this]{entry(this);});}catch(const std::system_error&){work_={};state_=State::failed;return false;}
#endif
    started_=true;return true;
  }
  bool join(uint64_t milliseconds=2000){
    if(!started_||joined_||milliseconds>2000)throw foundation::Error(foundation::Fault::argument,"native join lifecycle");
#ifdef __SWITCH__
    s32 index=-1;auto rc=svcWaitSynchronization(&index,&thread_.handle,1,milliseconds*1000000);
    if(R_FAILED(rc))return false;
    if(R_FAILED(threadWaitForExit(&thread_)))return false;
    // Pair with entry's exited release before publishing callback-owned data.
    if(state_.load(std::memory_order_acquire)!=State::exited)return false;
    if(R_FAILED(threadClose(&thread_)))std::terminate();
#else
    if(!done_.wait(milliseconds))return false;
    thread_.join();
#endif
    joined_=true;state_=State::joined;work_={};return true;
  }
  State state()const{return state_.load();}
  bool callback_error()const{return callback_error_.load();}
  bool joined()const{return joined_;}
};
}
