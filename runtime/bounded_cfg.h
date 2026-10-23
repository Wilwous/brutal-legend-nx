// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "generated_stop.h"
namespace brutal::synthetic {
// Original bounded fixtures ONLY. Immutable tables come from independently
// reconciled bytes and a host-verified canonical manifest. No title admission.
class BoundedCfg {
 public:
  enum class Kind { linear,branch,conditional,call,indirect_call,indirect_branch,ret };
  enum class Action { execute,exit,refuse };
  struct Site {uint32_t index,pc,owner;Kind kind;Action action;uint32_t condition;std::span<const uint32_t> targets;};
  class Refused:public std::logic_error{public:Refused():std::logic_error("bounded control transfer refused"){};};
  class Exit {friend class BoundedCfg;const BoundedCfg*owner_;explicit Exit(const BoundedCfg*p):owner_(p){};};
  using Observer=std::function<void(uint32_t)>;
  struct Statistics {uint64_t visits=0,refusals=0,native_calls=0,imports=0,returns=0;};
 private:
  PPCContext& context_;uint8_t*base_;std::span<const Site> sites_;std::span<const uint32_t>roots_,imports_;
  std::vector<PPCFuncMapping> mappings_;std::vector<std::array<uint64_t,3>>edges_;
  struct Frame{uint32_t owner,continuation;};std::array<Frame,32>stack_{};size_t depth_=0;
  uint32_t low_,high_,root_=0;uint64_t initial_lr_,r2_,r13_;std::vector<uint32_t>pending_;
  const Site*active_=nullptr;Observer observer_;BoundedCfg*previous_;bool returned_=false,entered_=false;
  inline static thread_local BoundedCfg*current_=nullptr;
  [[noreturn]] void deny(){++statistics.refusals;throw Refused();}
  const Site*find(uint32_t pc)const{auto i=std::lower_bound(sites_.begin(),sites_.end(),pc,[](const Site&s,uint32_t p){return s.pc<p;});return i!=sites_.end()&&i->pc==pc?&*i:nullptr;}
  bool imported(uint32_t pc)const{return std::find(imports_.begin(),imports_.end(),pc)!=imports_.end();}
  PPCFunc*function(uint32_t pc)const{for(auto f:mappings_)if(f.guest==pc)return f.host;return nullptr;}
  void context(PPCContext&c,uint8_t*base){if(&c!=&context_||base!=base_||rex::runtime::current_ppc_context()!=&c||c.r2.u64!=r2_||c.r13.u64!=r13_||c.r1.u64<low_||c.r1.u64>high_||(c.r1.u64&15))deny();}
  void edge(uint32_t from,uint32_t to){for(auto&e:edges_)if(e[0]==from&&e[1]==to){++e[2];return;}deny();}
 public:
  Statistics statistics{};
  BoundedCfg(PPCContext&c,uint8_t*base,std::span<const Site>sites,std::span<const uint32_t>roots,
    std::span<const uint32_t>imports,std::span<const PPCFuncMapping>mappings,
    std::span<const std::array<uint32_t,2>>edges,uint32_t low,uint32_t high,Observer observer={})
   :context_(c),base_(base),sites_(sites),roots_(roots),imports_(imports),low_(low),high_(high),initial_lr_(c.lr),r2_(c.r2.u64),r13_(c.r13.u64),observer_(std::move(observer)),previous_(current_){
    if(sites.empty()||sites.size()>256||roots.empty()||low>=high)throw std::invalid_argument("bounded policy size");
    uint32_t last=0;
    for(size_t i=0;i<sites.size();++i){auto&s=sites[i];
     if(s.index!=i||!s.pc||(s.pc&3)||(i&&s.pc<=last)||s.owner>s.pc||static_cast<unsigned>(s.kind)>6||static_cast<unsigned>(s.action)>2||s.targets.size()>32)throw std::invalid_argument("bounded site identity");last=s.pc;
    }
    for(const auto&s:sites){if(!find(s.owner)||find(s.owner)->owner!=s.owner)throw std::invalid_argument("bounded owner");if(s.kind==Kind::indirect_branch&&s.action==Action::execute)throw std::invalid_argument("computed tail unsupported");
     for(auto target:s.targets)if(!target||(target&3)||(!find(target)&&!imported(target)))throw std::invalid_argument("bounded target");
    }
    for(const auto&s:sites){
     if(s.action!=Action::execute){if(!s.targets.empty())throw std::invalid_argument("boundary continuation");continue;}
     if(s.kind==Kind::linear&&(s.targets.size()!=1||s.targets[0]!=s.pc+4))throw std::invalid_argument("linear continuation");
     if((s.kind==Kind::branch||s.kind==Kind::call)&&s.targets.size()!=1)throw std::invalid_argument("direct target count");
     if(s.kind==Kind::conditional&&(s.targets.size()!=2||s.targets[1]!=s.pc+4||((s.condition>>5)!=4&&(s.condition>>5)!=12)||((s.condition&31)!=0&&(s.condition&31)!=2)))throw std::invalid_argument("conditional contract");
     if(s.kind==Kind::indirect_call&&s.targets.empty())throw std::invalid_argument("unknown indirect policy");
    }
    for(size_t i=0;i<imports.size();++i){if(!imports[i]||(imports[i]&3)||find(imports[i]))throw std::invalid_argument("import/native collision");for(size_t j=0;j<i;++j)if(imports[i]==imports[j])throw std::invalid_argument("duplicate import");}
    for(auto root:roots)if(!find(root)||find(root)->owner!=root)throw std::invalid_argument("bounded root");
    for(auto f:mappings){if(!f.guest&&!f.host)continue;if(!f.host||!f.guest||f.guest>0xffffffffull)throw std::invalid_argument("bounded mapping");for(auto previous:mappings_)if(previous.guest==f.guest)throw std::invalid_argument("duplicate mapping");mappings_.push_back(f);}
    for(auto e:edges){if(!find(e[0])||!find(e[1]))throw std::invalid_argument("bounded edge");for(auto old:edges_)if(old[0]==e[0]&&old[1]==e[1])throw std::invalid_argument("duplicate edge");edges_.push_back({e[0],e[1],0});}
    std::vector<std::array<uint32_t,2>>expected;
    auto expected_edge=[&](uint32_t from,uint32_t to){std::array<uint32_t,2>e{from,to};if(std::find(expected.begin(),expected.end(),e)==expected.end())expected.push_back(e);};
    for(const auto&s:sites)if(s.action==Action::execute)for(auto target:s.targets){
     if(s.kind==Kind::call||s.kind==Kind::indirect_call){if(!imported(target)&&find(target)->owner!=target)throw std::invalid_argument("callee is not entry");expected_edge(s.pc,imported(target)?s.pc+4:target);}
     else {if(imported(target))throw std::invalid_argument("non-call import transfer");expected_edge(s.pc,target);}
    }
    if(edges_.size()!=expected.size())throw std::invalid_argument("missing graph edge");
    for(auto edge:edges_)if(std::find(expected.begin(),expected.end(),std::array<uint32_t,2>{uint32_t(edge[0]),uint32_t(edge[1])})==expected.end())throw std::invalid_argument("unexpected graph edge");
    context(c,base);current_=this;
  }
  BoundedCfg(const BoundedCfg&)=delete;BoundedCfg&operator=(const BoundedCfg&)=delete;
  ~BoundedCfg(){current_=previous_;}
  bool owns(const Exit&e)const{return e.owner_==this;}
  const auto&edges()const{return edges_;}
  void enter(uint32_t pc){context(context_,base_);if(entered_||std::find(roots_.begin(),roots_.end(),pc)==roots_.end()||!function(pc))deny();entered_=true;root_=pc;pending_={pc};instrumentation::Scope::current().indirect(pc,context_,base_);if(!returned_||depth_)deny();}
  static void point(uint32_t index,uint32_t pc){
   auto*self=current_;if(!self)throw Refused();self->context(self->context_,self->base_);
   if(index>=self->sites_.size()||self->sites_[index].pc!=pc||self->sites_[index].index!=index)self->deny();
   const auto&s=self->sites_[index];if(!self->entered_||self->returned_||std::find(self->pending_.begin(),self->pending_.end(),pc)==self->pending_.end())self->deny();
   if(self->active_)self->edge(self->active_->pc,pc);self->active_=&s;++self->statistics.visits;
   if(self->observer_)self->observer_(pc);
   GeneratedStop::poll(pc); // Same complete-instruction boundary and owned stop.
   self->pending_.clear();
   if(s.action==Action::exit)throw Exit(self);
   if(s.action==Action::refuse)self->deny();
   if(s.kind==Kind::call||s.kind==Kind::indirect_call||s.kind==Kind::indirect_branch){
    uint64_t target=s.kind==Kind::call?(s.targets.empty()?0:s.targets[0]):self->context_.ctr.u64;
    if(target>0xffffffffull||std::find(s.targets.begin(),s.targets.end(),target)==s.targets.end()||!self->function(uint32_t(target)))self->deny();
    if(self->imported(uint32_t(target))){if(s.kind==Kind::indirect_branch)self->deny();self->pending_={pc+4};}
    else {if(s.kind!=Kind::indirect_branch){if(self->depth_==self->stack_.size())self->deny();self->stack_[self->depth_++]={uint32_t(target),pc+4};}self->pending_={uint32_t(target)};}
   }else if(s.kind==Kind::ret){
    ++self->statistics.returns;
    if(self->depth_){auto frame=self->stack_[self->depth_-1];if(frame.owner!=s.owner||self->context_.lr!=frame.continuation||std::find(s.targets.begin(),s.targets.end(),frame.continuation)==s.targets.end())self->deny();--self->depth_;self->pending_={frame.continuation};}
    else {if(s.owner!=self->root_||self->context_.lr!=self->initial_lr_)self->deny();self->returned_=true;}
   }else if(s.kind==Kind::conditional){
    if(s.targets.size()!=2)self->deny();const bool bit=(s.condition&31)==2?self->context_.cr0.eq:self->context_.cr0.lt;const bool taken=(s.condition>>5)==12?bit:!bit;self->pending_={s.targets[taken?0:1]};
   }else self->pending_.assign(s.targets.begin(),s.targets.end());
  }
  static void direct(PPCFunc*f,PPCContext&c,uint8_t*base){auto*self=current_;if(!self)throw Refused();self->context(c,base);auto*s=self->active_;if(!s||s->action!=Action::execute||s->kind!=Kind::call||s->targets.size()!=1||self->function(s->targets[0])!=f||c.lr!=s->pc+4)self->deny();if(self->imported(s->targets[0])){++self->statistics.imports;imports::GeneratedImports::direct_import(f,c,base);}else{++self->statistics.native_calls;instrumentation::Scope::current().direct(f,c,base);}}
  static void indirect(uint32_t target,PPCContext&c,uint8_t*base){auto*self=current_;if(!self)throw Refused();self->context(c,base);auto*s=self->active_;if(!s||s->action!=Action::execute||s->kind!=Kind::indirect_call||c.ctr.u64!=target||c.lr!=s->pc+4||std::find(s->targets.begin(),s->targets.end(),target)==s->targets.end())self->deny();if(self->imported(target)){++self->statistics.imports;imports::GeneratedImports::indirect(target,c,base);}else{++self->statistics.native_calls;instrumentation::Scope::current().indirect(target,c,base);}}
};
}
#undef REX_CALL_FUNC
#undef REX_CALL_INDIRECT_FUNC
#define REX_CALL_FUNC(f) brutal::synthetic::BoundedCfg::direct(static_cast<PPCFunc*>(f),ctx,base)
#define REX_CALL_INDIRECT_FUNC(a) brutal::synthetic::BoundedCfg::indirect(uint32_t(a),ctx,base)
