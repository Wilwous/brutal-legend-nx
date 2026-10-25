// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Real pinned SDK context; no desktop Runtime, XThread or Xbox exports linked.
#include <rex/ppc/context.h>
#include "runtime/checked_memory.h"
#include <array>

namespace brutal::horizon {
using foundation::Memory;
using foundation::Error;
using foundation::Fault;
// A native thread-local capability, NOT an Xbox PCR, guest TLS or XThread.
// The four-instruction leaf does not consume r1/r2/r13 or any guest import.
class LeafScope {
  Memory& memory_;
  PPCContext& context_;
  LeafScope* previous_;
  inline static thread_local LeafScope* current_=nullptr;
 public:
  unsigned entries{},reads{},writes{};
  LeafScope(Memory& memory,PPCContext& context):memory_(memory),context_(context),previous_(current_){
    if(previous_)throw Error(Fault::unsupported,"nested leaf scope is unsupported");
    current_=this;
  }
  LeafScope(const LeafScope&)=delete;
  LeafScope& operator=(const LeafScope&)=delete;
  ~LeafScope(){current_=previous_;}
  static LeafScope& current(PPCContext& context){
    if(!current_ || &current_->context_!=&context)throw Error(Fault::no_thread,"missing or foreign native context binding");
    return *current_;
  }
  static void enter(PPCContext& context,uint8_t* base){
    auto& s=current(context);
    if(base!=nullptr)throw Error(Fault::argument,"raw guest base forbidden");
    ++s.entries;
  }
  static uint32_t load(PPCContext& context,uint32_t address){
    auto& s=current(context);auto value=s.memory_.load32(address);++s.reads;return value;
  }
  static void store(PPCContext& context,uint32_t address,uint32_t value){
    auto& s=current(context);s.memory_.store32(address,value);++s.writes;
  }
};
// No generic imports, indirect resolver, raw pointers, atomics, typed-vector ABI,
// PCR or guest exception fallback exists in this leaf-only interface.
}
