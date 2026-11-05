// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <functional>
#include <string>
#include <string_view>
namespace brutal::horizon::platform {
// Serialized application ownership. Callbacks must outlive this object and each
// cleanup must safely undo partial initialization. Names are copied before init.
// Activation cannot reenter; initialization may request shutdown, in which case
// its own partial/successful initialization is cleaned before activate returns.
// Cleanup may call shutdown idempotently. This is not an Xbox service ABI.
class Lifecycle {
 struct Entry {std::string name;std::function<void()> close;};
 std::array<Entry,12> entries_{};size_t size_{};bool closing_{},initializing_{};unsigned failures_{},closed_{};
 public:
 Lifecycle()=default;Lifecycle(const Lifecycle&)=delete;
 ~Lifecycle(){shutdown();}
 bool activate(std::string_view name,bool required,std::function<bool()> initialize,std::function<void()> close){
  if(initializing_)throw Error(Fault::ownership,"platform activation cannot reenter");
  if(closing_||name.empty()||!initialize||!close||size_==entries_.size())throw Error(Fault::argument,"platform lifecycle admission");
  for(size_t i=0;i<size_;++i)if(entries_[i].name==name)throw Error(Fault::ownership,"duplicate platform service");
  // All potentially allocating registration storage precedes initialization.
  Entry pending{std::string(name),std::move(close)};
  initializing_=true;struct Reset {bool& flag;~Reset(){flag=false;}} reset{initializing_};
  bool started=false;std::exception_ptr error;
  try{started=initialize();}catch(...){error=std::current_exception();}
  if(!started||closing_){
   try{pending.close();}catch(...){++failures_;}
   if(required){shutdown();if(error)std::rethrow_exception(error);throw Error(Fault::unsupported,"required platform service unavailable or lifecycle closed");}
   return false;
  }
  // Moving this prepared entry cannot allocate after initialization succeeds.
  entries_[size_++]=std::move(pending);return true;
 }
 void shutdown()noexcept{if(closing_)return;closing_=true;while(size_){auto&entry=entries_[--size_];try{entry.close();}catch(...){++failures_;}entry.close={};entry.name.clear();++closed_;}}
 bool active(std::string_view name)const{if(closing_)return false;for(size_t i=0;i<size_;++i)if(entries_[i].name==name)return true;return false;}
 unsigned cleanup_failures()const{return failures_;}unsigned closed()const{return closed_;}size_t live()const{return size_;}
};
}
