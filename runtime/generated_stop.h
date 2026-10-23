// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "generated_imports.h"
#include "cooperative_stop.h"
#include <functional>
namespace brutal::synthetic {
// Hooks are emitted by production midasm_hook configuration, outside complete
// guest instructions/imports. No guest unwind or asynchronous cancellation.
class GeneratedStop {
 public:
  class Stopped final {
    friend class GeneratedStop;
    const GeneratedStop* owner_;
    explicit Stopped(const GeneratedStop* owner):owner_(owner){}
  };
  using Observer=std::function<void(uint32_t)>;
 private:
  CooperativeStop& stop_;PPCContext& context_;
  std::span<const uint32_t> sites_;Observer observer_;
  GeneratedStop* previous_;
  static inline thread_local GeneratedStop* current_=nullptr;
 public:
  std::vector<uint64_t> hits;
  GeneratedStop(CooperativeStop& stop,PPCContext& context,std::span<const uint32_t> sites,Observer observer={})
   :stop_(stop),context_(context),sites_(sites),observer_(std::move(observer)),previous_(current_),hits(sites.size()){
    if(sites.empty()||sites.size()>256||!std::is_sorted(sites.begin(),sites.end())||std::adjacent_find(sites.begin(),sites.end())!=sites.end())throw std::invalid_argument("stop-site policy");
    if(rex::runtime::current_ppc_context()!=&context)throw std::logic_error("stop context");
    current_=this;
  }
  GeneratedStop(const GeneratedStop&)=delete;GeneratedStop&operator=(const GeneratedStop&)=delete;
  ~GeneratedStop(){current_=previous_;}
  bool owns(const Stopped& signal)const{return signal.owner_==this;}
  static void poll(uint32_t pc){
    auto* self=current_;
    if(!self||rex::runtime::current_ppc_context()!=&self->context_)throw std::logic_error("unbound generated stop");
    const auto found=std::lower_bound(self->sites_.begin(),self->sites_.end(),pc);
    if(found==self->sites_.end()||*found!=pc)throw std::logic_error("unadmitted generated stop site");
    ++self->hits[size_t(found-self->sites_.begin())];
    if(self->observer_)self->observer_(pc);
    if(self->stop_.state()==CooperativeStop::State::requested){
      if(!self->stop_.safe_boundary())throw std::logic_error("stop transition race");
      throw Stopped(self);
    }
    if(self->stop_.state()!=CooperativeStop::State::running)throw std::logic_error("generated execution after stop boundary");
  }
};
}
