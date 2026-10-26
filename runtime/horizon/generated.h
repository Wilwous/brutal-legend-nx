// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "guest.h"
#include "runtime/cooperative_stop.h"
#include <string_view>
#include <concepts>
namespace brutal::horizon {
struct Function {uint32_t address;PPCFunc* implementation;std::string_view name;};
class Scope {
 public:
  struct Stopped {const Scope* owner;};
 private:
  GuestThread& guest_;GuestThread::Execution execution_;
  std::span<const Function> functions_;std::vector<Space::Handle> loans_;std::vector<Space::Lease> pins_;
  synthetic::CooperativeStop* stop_{};std::function<void(uint32_t)> observer_;
  inline static thread_local Scope* current_{};
  void permitted(uint32_t address,uint32_t size){
    for(auto h:loans_)if(address>=h.address&&uint64_t(address)+size<=uint64_t(h.address)+h.size)return;
    throw Error(Fault::ownership,"generated guest range has no retained capability");
  }
 public:
  uint64_t reads{},writes{},calls{},polls{};
  bool belongs_to(const ModuleLife& module,const Space& space)const{return guest_.belongs_to(module)&&&guest_.space()==&space;}
  Scope(GuestThread& guest,std::span<const Function> functions,uint64_t module_identity,
        synthetic::CooperativeStop* stop=nullptr,std::function<void(uint32_t)> observer={})
    :guest_(guest),execution_(guest),functions_(functions),stop_(stop),observer_(std::move(observer)){
    if(current_||guest.module_identity()!=module_identity||functions.empty()||functions.size()>32)throw Error(Fault::argument,"scope/module admission");
    for(size_t i=0;i<functions.size();++i){auto f=functions[i];if(!f.address||(f.address&3)||!f.implementation||f.name.empty())throw Error(Fault::argument,"function identity");
      for(size_t j=0;j<i;++j)if(functions[j].address==f.address||functions[j].implementation==f.implementation||functions[j].name==f.name)throw Error(Fault::argument,"duplicate function identity");}
    loans_.reserve(64);pins_.reserve(64);lend(guest.stack);lend(guest.pcr);lend(guest.tls);current_=this;
  }
  Scope(const Scope&)=delete;
  ~Scope(){if(current_!=this)std::terminate();current_=nullptr;}
  void lend(Space::Handle h){
    if(loans_.size()>=64)throw Error(Fault::exhausted,"scope capability limit");
    for(auto old:loans_)if(h.address<uint64_t(old.address)+old.size&&old.address<uint64_t(h.address)+h.size)throw Error(Fault::ownership,"overlapping scope capability");
    Space::Lease lease(guest_.space(),h);loans_.push_back(h);pins_.push_back(std::move(lease));
  }
  static Scope& current(PPCContext& context){if(!current_||&current_->guest_.context!=&context)throw Error(Fault::no_thread,"missing or foreign generated thread context");return *current_;}
  void poll(uint32_t address){
    ++polls;if(observer_)observer_(address);
    if(!stop_)return;
    auto state=stop_->state();
    if(state==synthetic::CooperativeStop::State::requested){if(!stop_->safe_boundary())throw Error(Fault::unsupported,"stop transition race");throw Stopped{this};}
    if(state!=synthetic::CooperativeStop::State::running)throw Error(Fault::unsupported,"execution after stop");
  }
  static void prologue(PPCContext& context,uint8_t* base,std::string_view name){
    auto&s=current(context);if(base||context.r13.u64!=s.guest_.pcr.address)throw Error(Fault::argument,"raw base/PCR context admission");
    if(context.r1.u64<s.guest_.stack.address+64||context.r1.u64>uint64_t(s.guest_.stack.address)+s.guest_.stack.size-128||(context.r1.u64&15))
      throw Error(Fault::argument,"generated call stack window admission");
    if(name.starts_with("__imp__"))name.remove_prefix(7);
    for(auto f:s.functions_)if(f.name==name){s.poll(f.address);return;}
    throw Error(Fault::unsupported,"unadmitted generated function prologue");
  }
  template<class T>static T load(PPCContext& context,uint32_t address){auto&s=current(context);s.permitted(address,sizeof(T));auto v=s.guest_.space().load<T>(address);++s.reads;return v;}
  template<class T>static void store(PPCContext& context,uint32_t address,T value){auto&s=current(context);s.permitted(address,sizeof(T));s.guest_.space().store<T>(address,value);++s.writes;}
  void invoke(uint32_t address){for(auto f:functions_)if(f.address==address){++calls;f.implementation(guest_.context,nullptr);return;}throw Error(Fault::unsupported,"unknown generated target");}
  template<class F>requires std::same_as<F,PPCFunc*>
  static void direct(F function,PPCContext& context,uint8_t* base){
    auto&s=current(context);if(base)throw Error(Fault::argument,"raw direct-call base");
    for(auto f:s.functions_)if(f.implementation==function){++s.calls;function(context,nullptr);return;}
    throw Error(Fault::unsupported,"unadmitted direct call");
  }
  static void indirect(PPCContext& context,uint8_t* base,uint32_t address){auto&s=current(context);if(base)throw Error(Fault::argument,"raw indirect-call base");context.last_indirect_target=address;s.invoke(address);}
};
}
