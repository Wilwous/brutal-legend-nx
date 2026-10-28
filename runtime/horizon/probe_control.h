// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/checked_memory.h"
#include <cstdio>
#include <exception>
namespace brutal::horizon {
// Original synthetic-probe UI control, not guest exceptions or an Xbox API.
// The owner outlives all users; deactivate only after the native controller joins.
class ProbeControl {
 inline static std::atomic<ProbeControl*> current_{};
 inline static thread_local ProbeControl* local_{};
 std::atomic<bool> cancel_{};std::atomic<unsigned> stage_{};
 std::mutex mutex_;std::array<std::array<char,96>,16> failures_{};size_t used_{},dropped_{};
 public:
 class Activation {ProbeControl*owner_;public:explicit Activation(ProbeControl&c):owner_(&c){ProbeControl*empty=nullptr;if(!current_.compare_exchange_strong(empty,&c))throw foundation::Error(foundation::Fault::ownership,"probe controller already active");}~Activation(){auto*value=owner_;if(!current_.compare_exchange_strong(value,nullptr))std::terminate();}Activation(const Activation&)=delete;};
 class Local {ProbeControl*previous_;public:explicit Local(ProbeControl&c):previous_(local_){local_=&c;}~Local(){local_=previous_;}Local(const Local&)=delete;};
 static ProbeControl* current(){return current_.load();}
 bool request_cancel(){return !cancel_.exchange(true);}
 bool cancelled()const{return cancel_.load();}
 unsigned stage()const{return stage_.load();}
 static void mark(unsigned stage){if(auto*c=current())c->stage_=stage;}
 static void poll(){for(auto*c:{current(),local_})if(c&&c->cancelled()){diagnostic::emit(diagnostic::Kind::stop_observed,0x50524f42);throw foundation::Error(foundation::Fault::unsupported,"synthetic probe cancelled at generated safe point");}}
 void failure(const char*text){std::lock_guard lock(mutex_);if(used_==failures_.size()){++dropped_;return;}auto&row=failures_[used_++];int n=std::snprintf(row.data(),row.size(),"%s",text);if(n<0||size_t(n)>=row.size()){row[92]='.';row[93]='.';row[94]='.';row[95]=0;++dropped_;}}
 struct Failures {decltype(failures_) rows;size_t used,dropped;};
 Failures failures(){std::lock_guard lock(mutex_);return {failures_,used_,dropped_};}
};
inline void report_failure(const char*text){
#ifdef __SWITCH__
 if(auto*c=ProbeControl::current()){c->failure(text);return;}
#endif
 std::fprintf(stderr,"synthetic probe failure: %s\n",text);
}
}
