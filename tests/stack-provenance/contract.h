// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "provenance-data.h"
namespace provenance_test {
using namespace brutal::horizon;
inline constexpr uint32_t exit_cookie=0x8201fff0, cell_address=0x20000000;
struct Inputs {
 uint64_t thread,module,source,sp,payload;uint32_t marker,tls_value;
 Space::Handle stack,pcr,tls,cell;bool require_equivalence{true};
};
class Bound;
class Contract {
 friend class Bound;
 GuestThread&g_;ModuleLife&m_;const Program&p_;const Inputs in_;
 ModuleLife::Lease module_;Space::Lease stack_,pcr_,tls_,cell_;
 const Scope*bound_{};unsigned entries_{};
 inline static thread_local Contract*current_{};
 static bool same(Space::Handle a,Space::Handle b){return a.owner==b.owner&&a.id==b.id&&a.address==b.address&&a.size==b.size;}
 public:
 Contract(GuestThread&g,ModuleLife&m,const Program&p,Inputs in)
  :g_(g),m_(m),p_(p),in_(in),module_(m),stack_(g.space(),in.stack),pcr_(g.space(),in.pcr),tls_(g.space(),in.tls),cell_(g.space(),in.cell){
  if(current_||!g.belongs_to(m)||m.identity!=p.module||in.thread!=g.identity||in.module!=g.module_instance()||in.source!=p.source)
   throw Error(Fault::ownership,"unqualified synthetic context");
  if(p.mode==Mode::indirect||(in.require_equivalence&&!p.relocatable))throw Error(Fault::unsupported,"unproved pointer observer/control");
  if(!same(in.stack,g.stack)||!same(in.pcr,g.pcr)||!same(in.tls,g.tls)||in.cell.address!=cell_address||in.cell.size!=4)
   throw Error(Fault::ownership,"wrong synthetic object roles");
  validate(p.entry);current_=this;
 }
 Contract(const Contract&)=delete;
 ~Contract(){if(current_!=this||bound_)std::terminate();current_=nullptr;}
 void validate(uint32_t pc){
  ModuleLife::Lease fresh(m_);auto&c=g_.context;auto&s=g_.space();
  if(entries_>=3||in_.sp>UINT32_MAX||in_.sp<192||(in_.sp&15))throw Error(Fault::argument,"stack/order proof missing");
  const auto expected_pc=entries_==0?p_.entry:entries_==1?p_.helper:p_.leaf;
  const uint64_t expected_sp=in_.sp-(entries_==0?0:entries_==1?64:160);
  const uint64_t expected_lr=entries_==0?exit_cookie:entries_==1?p_.root_return:p_.helper_return;
  if(pc!=expected_pc||c.r1.u64!=expected_sp||c.lr!=expected_lr||c.r22.u64!=in_.marker||c.r23.u64!=in_.payload)
   throw Error(Fault::argument,"changed entry dependency/return");
  if(entries_&&(c.r15.u64!=exit_cookie||c.r18.u64!=in_.sp||c.r20.u64!=in_.sp-64))throw Error(Fault::argument,"wrong parent frame");
  if(entries_==2&&(c.r16.u64!=p_.root_return||c.r21.u64!=in_.sp-160))throw Error(Fault::argument,"wrong nested frame");
  s.validate_owned_window(in_.stack,in_.stack.address,in_.stack.size,Access::write,true);
  s.validate_owned_window(in_.stack,uint32_t(in_.sp)-192,192,Access::write,true);
  s.validate_owned_window(in_.cell,cell_address,4,Access::write,true);
  if(c.r4.u64!=cell_address||c.r13.u64!=in_.pcr.address||s.load<uint32_t>(in_.pcr.address)!=in_.tls.address||s.load<uint64_t>(in_.pcr.address+0x30)!=in_.pcr.address||g_.get_tls()!=in_.tls_value)
   throw Error(Fault::slot,"wrong native TLS association");
 }
 void before(uint32_t pc){validate(pc);++entries_;}
 unsigned entries()const{return entries_;}
 static void prologue(PPCContext&ctx,uint8_t*base,std::string_view name){
  auto&s=Scope::current(ctx);
  if(base||!current_||current_->bound_!=&s||&current_->g_.context!=&ctx)throw Error(Fault::ownership,"missing bound provenance contract");
  if(name.starts_with("__imp__"))name.remove_prefix(7);
  for(auto f:current_->p_.functions)if(f.name==name){Scope::prologue(ctx,base,name);return;}
  throw Error(Fault::unsupported,"unqualified source function");
 }
};
class Bound {
 Contract&c_;Scope scope_;
 public:
 Bound(GuestThread&g,ModuleLife&m,const Program&p,Contract&c,brutal::synthetic::CooperativeStop*stop=nullptr,std::function<void(uint32_t)>controller={})
  :c_(c),scope_(g,p.functions,m.identity,stop,[&c,controller=std::move(controller)](uint32_t pc){if(controller)controller(pc);c.before(pc);}){
  if(&g!=&c.g_||&m!=&c.m_||&p!=&c.p_||c.bound_)throw Error(Fault::ownership,"wrong bound source/context");
  scope_.lend(c.in_.cell);c.bound_=&scope_;
 }
 ~Bound(){c_.bound_=nullptr;}
 Bound(const Bound&)=delete;
 void invoke(){scope_.invoke(c_.p_.entry);if(c_.g_.context.r1.u64!=c_.in_.sp||c_.g_.context.lr!=exit_cookie||c_.entries_!=3)throw Error(Fault::unsupported,"invalid native return boundary");}
 uint64_t calls()const{return scope_.calls;}
 uint64_t reads()const{return scope_.reads;}
 uint64_t writes()const{return scope_.writes;}
 bool owns(const Scope*s)const{return s==&scope_;}
};
}
