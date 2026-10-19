// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "checked_memory.h"
#include <thread>

namespace brutal::foundation {
// Process slot allocation + guest-addressed per-host-thread values. The layout
// follows the inspected SDK: static bytes first, then 4-byte dynamic slots.
// PCR/r13, fibers, guest APCs and XThread binding are not implemented here.
class Tls {
  struct Thread { Allocation allocation; };
  Memory& memory_;
  uint32_t static_size_;
  std::vector<std::byte> initial_;
  std::vector<bool> used_;
  std::map<std::thread::id,Thread> threads_;
  std::mutex mutex_;
  Thread& current() {
    auto i=threads_.find(std::this_thread::get_id());
    if(i==threads_.end())throw Error(Fault::no_thread,"host thread is not attached");
    return i->second;
  }
  void valid_slot(uint32_t slot) {
    if(slot>=used_.size() || !used_[slot])throw Error(Fault::slot,"TLS slot is invalid or free");
  }
 public:
  Tls(Memory& memory,uint32_t slots,uint32_t static_size,std::span<const std::byte> initial)
    :memory_(memory),static_size_(static_size) {
    if (!slots || slots>256 || static_size>4096 || initial.size()>static_size)
      throw Error(Fault::argument,"invalid TLS layout/template");
    initial_.assign(initial.begin(),initial.end());used_.resize(slots);
  }
  Tls(const Tls&)=delete;
  void attach() {
    std::lock_guard lock(mutex_);
    if(threads_.contains(std::this_thread::get_id()))throw Error(Fault::argument,"host thread already attached");
    auto a=memory_.allocate(static_size_+uint32_t(used_.size())*4);
    try {
      if(!initial_.empty())memory_.write(memory_.translate(a.guest,uint32_t(initial_.size()),Access::write),initial_);
      threads_.emplace(std::this_thread::get_id(),Thread{a});
    } catch(...) { memory_.release(a);throw; }
  }
  void detach() {
    std::lock_guard lock(mutex_);auto a=current().allocation;
    memory_.release(a);threads_.erase(std::this_thread::get_id());
  }
  class Scope {
    Tls& tls_;
    const std::thread::id owner_=std::this_thread::get_id();
   public:
    explicit Scope(Tls& tls):tls_(tls){tls_.attach();}
    Scope(const Scope&)=delete;
    ~Scope(){
      if(std::this_thread::get_id()!=owner_)std::terminate();
      tls_.detach();
    }
  };
  uint32_t allocate_slot() {
    std::lock_guard lock(mutex_);current();
    for(uint32_t i=0;i<used_.size();++i)if(!used_[i]){used_[i]=true;return i;}
    throw Error(Fault::exhausted,"TLS slots exhausted");
  }
  void free_slot(uint32_t slot) {
    std::lock_guard lock(mutex_);current();valid_slot(slot);
    for(auto& [id,t]:threads_){(void)id;memory_.store32(t.allocation.guest+static_size_+slot*4,0);}
    used_[slot]=false;
  }
  uint32_t get(uint32_t slot) {
    std::lock_guard lock(mutex_);auto& t=current();valid_slot(slot);
    return memory_.load32(t.allocation.guest+static_size_+slot*4);
  }
  void set(uint32_t slot,uint32_t value) {
    std::lock_guard lock(mutex_);auto& t=current();valid_slot(slot);
    memory_.store32(t.allocation.guest+static_size_+slot*4,value);
  }
  uint32_t static_address(){std::lock_guard lock(mutex_);return current().allocation.guest;}
  uint32_t dynamic_address(){return static_address()+static_size_;}
};
} // namespace brutal::foundation
