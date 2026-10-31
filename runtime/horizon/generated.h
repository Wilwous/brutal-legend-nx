// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "guest.h"
#include "probe_control.h"
#include "runtime/cooperative_stop.h"
#include <string_view>
#include <concepts>
#include <cmath>
namespace brutal::horizon {
struct Function {uint32_t address;PPCFunc* implementation;std::string_view name;bool strict{},finite_fpr{};};
enum class SiteKind {read,write,direct,indirect,ret,branch};
struct Site {uint32_t pc,owner;SiteKind kind;uint32_t width,target;};
struct ControlPlan {std::span<const Site> sites;std::span<const uint32_t> roots;uint32_t exit;};
class Scope {
 public:
  struct Stopped {const Scope* owner;};
 private:
  GuestThread& guest_;diagnostic::Binding trace_binding_;diagnostic::PhaseBinding entry_phase_{4};GuestThread::Execution execution_;
  std::span<const Function> functions_;std::vector<Space::Handle> loans_;std::vector<Space::Lease> pins_;
  synthetic::CooperativeStop* stop_{};std::function<void(uint32_t)> observer_;
  inline static thread_local Scope* current_{};
  const ControlPlan* plan_{};const Site* pending_{};
  std::vector<uint32_t> returns_,active_;
  uint32_t host_csr_{PPCFPSCRRegister::Platform::getcsr()};
  void memory_site(Access access,uint32_t width){
    if(!plan_)return;
    const Site* site=std::exchange(pending_,nullptr);
    if(!site||site->width!=width||site->kind!=(access==Access::read?SiteKind::read:SiteKind::write))
      throw Error(Fault::unsupported,"unadmitted generated memory site");
  }
  void dispatch(Function f,bool indirect=false){
    execution_.validate_entry();
    diagnostic::PhaseBinding execution_phase{5};
    if(f.finite_fpr&&(!std::isfinite(guest_.context.f1.f64)||!std::isfinite(guest_.context.f2.f64)))throw Error(Fault::unsupported,"unadmitted nonfinite FPR arguments");
    if(!plan_){++calls;diagnostic::emit(diagnostic::Kind::function_enter,f.address);f.implementation(guest_.context,nullptr);return;}
    uint32_t expected=plan_->exit;
    if(active_.empty()){
      if(pending_||std::find(plan_->roots.begin(),plan_->roots.end(),f.address)==plan_->roots.end())throw Error(Fault::unsupported,"unadmitted root");
    }else{
      const Site* site=std::exchange(pending_,nullptr);
      if(!site||site->kind!=(indirect?SiteKind::indirect:SiteKind::direct)||site->target!=f.address)throw Error(Fault::unsupported,"unadmitted call edge");
      expected=site->pc+4;
    }
    if(guest_.context.lr!=expected||returns_.size()>=32)throw Error(Fault::unsupported,"call LR/depth admission");
    returns_.push_back(expected);active_.push_back(f.address);++calls;diagnostic::emit(f.name.starts_with("__imp__")?diagnostic::Kind::import_admitted:diagnostic::Kind::function_enter,f.address);
    try{f.implementation(guest_.context,nullptr);
      if(pending_||guest_.context.lr!=expected)throw Error(Fault::unsupported,"return target admission");
    }catch(...){pending_=nullptr;returns_.pop_back();active_.pop_back();throw;}
    diagnostic::emit(diagnostic::Kind::function_exit,f.address);returns_.pop_back();active_.pop_back();
  }
  void permitted(uint32_t address,uint32_t size){
    for(auto h:loans_)if(address>=h.address&&uint64_t(address)+size<=uint64_t(h.address)+h.size)return;
    throw Error(Fault::ownership,"generated guest range has no retained capability");
  }
 public:
  inline static std::atomic<uint64_t> guest_entries{};
  uint64_t reads{},writes{},calls{},polls{};
  bool belongs_to(const ModuleLife& module,const Space& space)const{return guest_.belongs_to(module)&&&guest_.space()==&space;}
  Scope(GuestThread& guest,std::span<const Function> functions,uint64_t module_identity,
        synthetic::CooperativeStop* stop=nullptr,std::function<void(uint32_t)> observer={},const ControlPlan* plan=nullptr,const GuestThread::StackContract* stack=nullptr)
    :guest_(guest),trace_binding_(guest.trace(),guest.module_instance(),guest.identity),execution_(guest,stack),functions_(functions),stop_(stop),observer_(std::move(observer)),plan_(plan){
    if(plan_){if(plan_->sites.empty()||plan_->roots.empty()||!plan_->exit)throw Error(Fault::argument,"empty control plan");returns_.reserve(32);active_.reserve(32);}
    if(current_||guest.module_identity()!=module_identity||functions.empty()||functions.size()>32)throw Error(Fault::argument,"scope/module admission");
    for(size_t i=0;i<functions.size();++i){auto f=functions[i];if(f.strict&&!plan_)throw Error(Fault::unsupported,"strict generated function requires control plan");if(!f.address||(f.address&3)||!f.implementation||f.name.empty())throw Error(Fault::argument,"function identity");
      for(size_t j=0;j<i;++j)if(functions[j].address==f.address||functions[j].implementation==f.implementation||functions[j].name==f.name)throw Error(Fault::argument,"duplicate function identity");}
    loans_.reserve(64);pins_.reserve(64);lend(guest.stack);lend(guest.pcr);lend(guest.tls);current_=this;
  }
  Scope(const Scope&)=delete;
  ~Scope(){if(current_!=this)std::terminate();current_=nullptr;PPCFPSCRRegister::Platform::setcsr(host_csr_);}
  void lend(Space::Handle h){
    if(loans_.size()>=64)throw Error(Fault::exhausted,"scope capability limit");
    for(auto old:loans_)if(h.address<uint64_t(old.address)+old.size&&old.address<uint64_t(h.address)+h.size)throw Error(Fault::ownership,"overlapping scope capability");
    Space::Lease lease(guest_.space(),h);loans_.push_back(h);pins_.push_back(std::move(lease));
  }
  void admit_range(uint32_t address,uint32_t size){permitted(address,size);}
  void import_target(uint32_t address){if(plan_&&(active_.empty()||active_.back()!=address))throw Error(Fault::unsupported,"import bypassed admitted call edge");}
  static void deny_exception(PPCContext& ctx){current(ctx);diagnostic::emit(diagnostic::Kind::exceptional_rejected);throw Error(Fault::unsupported,"guest exceptional transfer denied");}
  static void deny_instruction(PPCContext& ctx){current(ctx);diagnostic::emit(diagnostic::Kind::instruction_rejected);throw Error(Fault::unsupported,"unsupported guest instruction denied");}
  static Scope& current(PPCContext& context){if(!current_||&current_->guest_.context!=&context)throw Error(Fault::no_thread,"missing or foreign generated thread context");return *current_;}
  void poll(uint32_t address){
    ++polls;ProbeControl::poll();if(observer_)observer_(address);
    if(!stop_)return;
    auto state=stop_->state();
    if(state==synthetic::CooperativeStop::State::requested){if(!stop_->safe_boundary())throw Error(Fault::unsupported,"stop transition race");diagnostic::emit(diagnostic::Kind::stop_observed,address);throw Stopped{this};}
    if(state!=synthetic::CooperativeStop::State::running)throw Error(Fault::unsupported,"execution after stop");
  }
  void point(uint32_t pc){
    diagnostic::site(pc);poll(pc);if(!plan_)return;
    if(pending_||active_.empty())throw Error(Fault::unsupported,"orphan or unconsumed site");
    for(const auto&site:plan_->sites)if(site.pc==pc){
      if(site.owner!=active_.back())throw Error(Fault::unsupported,"site/function ownership");
      if(site.kind==SiteKind::ret){if(guest_.context.lr!=returns_.back()){diagnostic::emit(diagnostic::Kind::branch_rejected,1);throw Error(Fault::unsupported,"generated return target rejected");}}
      else if(site.kind!=SiteKind::branch)pending_=&site;
      return;
    }
    throw Error(Fault::unsupported,"unknown instruction site");
  }
  static void prologue(PPCContext& context,uint8_t* base,std::string_view name){
    auto&s=current(context);if(base)throw Error(Fault::argument,"raw base context admission");
    if(name.starts_with("__imp__"))name.remove_prefix(7);
    for(auto f:s.functions_)if(f.name==name){if(s.plan_&&(s.active_.empty()||s.active_.back()!=f.address))throw Error(Fault::unsupported,"prologue without admitted dispatch");diagnostic::site(f.address);s.poll(f.address);s.execution_.validate_entry();++guest_entries;return;}
    throw Error(Fault::unsupported,"unadmitted generated function prologue");
  }
  template<class T>static T load(PPCContext& context,uint32_t address){auto&s=current(context);s.memory_site(Access::read,sizeof(T));s.permitted(address,sizeof(T));auto v=s.guest_.space().load<T>(address);++s.reads;return v;}
  template<class T>static void store(PPCContext& context,uint32_t address,T value){auto&s=current(context);s.memory_site(Access::write,sizeof(T));s.permitted(address,sizeof(T));s.guest_.space().store<T>(address,value);++s.writes;}
  static void load_vector(PPCContext& context,PPCVRegister& value,uint32_t address){
    auto&s=current(context);s.memory_site(Access::read,16);s.permitted(address,16);
    if(address&15)throw Error(Fault::argument,"vector adapter requires rounded block address");
    auto bytes=s.guest_.space().read(address,16);for(unsigned i=0;i<16;++i)value.u8[15-i]=std::to_integer<uint8_t>(bytes[i]);++s.reads;
  }
  static void store_vector(PPCContext& context,uint32_t address,const PPCVRegister& value){
    auto&s=current(context);s.memory_site(Access::write,16);s.permitted(address,16);
    if(address&15)throw Error(Fault::argument,"vector adapter requires rounded block address");
    std::array<std::byte,16> bytes;for(unsigned i=0;i<16;++i)bytes[i]=std::byte(value.u8[15-i]);s.guest_.space().write(address,bytes);++s.writes;
  }
  void invoke(uint32_t address){for(auto f:functions_)if(f.address==address){dispatch(f);return;}diagnostic::emit(diagnostic::Kind::branch_rejected,2);throw Error(Fault::unsupported,"unknown generated target");}
  template<class F>requires std::same_as<F,PPCFunc*>
  static void direct(F function,PPCContext& context,uint8_t* base){
    auto&s=current(context);if(base)throw Error(Fault::argument,"raw direct-call base");
    for(auto f:s.functions_)if(f.implementation==function){s.dispatch(f);return;}
    throw Error(Fault::unsupported,"unadmitted direct call");
  }
  static void indirect(PPCContext& context,uint8_t* base,uint32_t address){auto&s=current(context);if(base)throw Error(Fault::argument,"raw indirect-call base");for(auto f:s.functions_)if(f.address==address){context.last_indirect_target=address;s.dispatch(f,true);return;}diagnostic::emit(diagnostic::Kind::branch_rejected,3);throw Error(Fault::unsupported,"unadmitted indirect target");}
};
}
