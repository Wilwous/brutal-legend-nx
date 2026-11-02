// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "equivalence-data.h"
namespace context_test {
using namespace brutal::horizon;
inline constexpr uint32_t exit_cookie=0x8201fff0,cell_address=0x20000000;
inline constexpr std::array<GeometrySpan,6> spans{{{-40,8},{-32,8},{-24,8},{-16,8},{-8,4},{-496,4}}};
struct Inputs {
 uint64_t thread,module,source,sp;uint32_t entry,mask,tls_value;
 Space::Handle stack,pcr,tls,cell;std::array<uint64_t,4> payload;
 bool require_equivalence{true};
};
class Bound;
class Contract {
 friend class Bound;
 GuestThread& g_;ModuleLife&m_;const Program&p_;const Inputs in_;
 ModuleLife::Lease module_;Space::Lease stack_,pcr_,tls_,cell_;
 const Scope* bound_{};unsigned entries_{};
 inline static thread_local Contract* current_{};
 static bool same(Space::Handle a,Space::Handle b){return a.owner==b.owner&&a.id==b.id&&a.address==b.address&&a.size==b.size;}
 public:
 Contract(GuestThread&g,ModuleLife&m,const Program&p,Inputs in):g_(g),m_(m),p_(p),in_(in),module_(m),stack_(g.space(),in.stack),pcr_(g.space(),in.pcr),tls_(g.space(),in.tls),cell_(g.space(),in.cell){
  if(current_||!g.belongs_to(m)||m.identity!=p.module||in.module!=g.module_instance()||in.thread!=g.identity||in.entry!=p.entry||in.source!=p.source||in.mask!=15)
   throw Error(Fault::ownership,"missing native input/identity binding");
  if(p.mode==Mode::indirect||(in.require_equivalence&&!p.relocatable))throw Error(Fault::unsupported,"unproved relocation/control observer");
  if(!same(in.stack,g.stack)||!same(in.pcr,g.pcr)||!same(in.tls,g.tls)||in.cell.address!=cell_address||in.cell.size!=4)
   throw Error(Fault::ownership,"wrong native object association");
  validate(p.entry);current_=this;
 }
 Contract(const Contract&)=delete;
 ~Contract(){if(current_!=this||bound_)std::terminate();current_=nullptr;}
 void validate(uint32_t pc){
  ModuleLife::Lease fresh(m_);const auto&c=g_.context;
  if(entries_>=2||pc!=(entries_?p_.helper:p_.entry)||c.r1.u64!=in_.sp||in_.sp>UINT32_MAX||(in_.sp&15)||in_.sp<496)
   throw Error(Fault::argument,"unproved entry pointer/order");
  if(c.lr!=(entries_?uint64_t(p_.entry)+12:exit_cookie)||std::array<uint64_t,4>{c.r20.u64,c.r21.u64,c.r22.u64,c.r23.u64}!=in_.payload)
   throw Error(Fault::argument,"changed incoming dependency/return cookie");
  if(entries_&&(c.r7.u64!=exit_cookie||c.r10.u64!=in_.sp-496))throw Error(Fault::argument,"changed caller-derived helper inputs");
  auto&s=g_.space();s.validate_owned_window(in_.stack,in_.stack.address,in_.stack.size,Access::write,true);
  for(auto span:spans){auto ea=int64_t(in_.sp)+span.offset;if(ea<0||uint64_t(ea)+span.width>0x100000000ull||uint64_t(ea)%span.width)throw Error(Fault::bounds,"native footprint overflow/alignment");s.validate_owned_window(in_.stack,uint32_t(ea),span.width,Access::write,true);}
  s.validate_owned_window(in_.cell,cell_address,4,Access::write,true);
  if(s.load<uint32_t>(g_.pcr.address)!=in_.tls.address||s.load<uint64_t>(g_.pcr.address+0x30)!=in_.pcr.address||c.r13.u64!=in_.pcr.address||g_.get_tls()!=in_.tls_value||c.r4.u64!=cell_address)
   throw Error(Fault::slot,"wrong native thread/PCR/TLS contract");
 }
 void before(uint32_t pc){validate(pc);++entries_;}
 unsigned entries()const{return entries_;}
 static void prologue(PPCContext&ctx,uint8_t*base,std::string_view name){
  auto&s=Scope::current(ctx);if(base||!current_||current_->bound_!=&s||&current_->g_.context!=&ctx)throw Error(Fault::ownership,"missing bound native ABI contract");
  if(name.starts_with("__imp__"))name.remove_prefix(7);
  for(auto f:current_->p_.functions)if(f.name==name){Scope::prologue(ctx,base,name);return;}
  throw Error(Fault::unsupported,"unexpected generated program");
 }
};
class Bound {
 Contract&c_;Scope scope_;
 public:
 Bound(GuestThread&g,ModuleLife&m,const Program&p,Contract&c,brutal::synthetic::CooperativeStop*stop=nullptr,std::function<void(uint32_t)>controller={})
  :c_(c),scope_(g,p.functions,m.identity,stop,[&c,controller=std::move(controller)](uint32_t pc){if(controller)controller(pc);c.before(pc);}){
  if(&g!=&c.g_||&m!=&c.m_||&p!=&c.p_||c.bound_)throw Error(Fault::ownership,"wrong bound native scope");
  scope_.lend(c.in_.cell);c.bound_=&scope_; // publish binding after throwing setup
 }
 Bound(const Bound&)=delete;
 ~Bound(){c_.bound_=nullptr;}
 void invoke(){scope_.invoke(c_.p_.entry);if(c_.g_.context.lr!=exit_cookie||c_.g_.context.r1.u64!=c_.in_.sp||c_.entries_!=2)throw Error(Fault::unsupported,"unproved native return boundary");}
 uint64_t writes()const{return scope_.writes;}
 uint64_t reads()const{return scope_.reads;}
 uint64_t calls()const{return scope_.calls;}
 bool owns(const Scope*s)const{return s==&scope_;}
};
}
