// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "geometry-data.h"
namespace geometry_test {
using namespace brutal::horizon;
// Original fixture policy only. No automatic runtime or title policy change.
struct Declaration {
 uint64_t thread,module,source;uint32_t entry;Space::Handle stack;uint64_t sp;
 std::array<GeometrySpan,6> stores;int64_t begin{-496},end{-4};bool closed_calls{true};
 uint64_t root_lr{};std::array<uint64_t,4> payload{};
};
inline Declaration declare(GuestThread& g){Declaration p{g.identity,g.module_instance(),geometry_source_tag,geometry_entry,g.stack,g.context.r1.u64,geometry_stores};p.root_lr=g.context.lr;p.payload={g.context.r20.u64,g.context.r21.u64,g.context.r22.u64,g.context.r23.u64};return p;}
class BoundScope;
class Admission {
 friend class BoundScope;
 GuestThread& guest_;ModuleLife& module_;const Declaration proof_;
 ModuleLife::Lease module_pin_;Space::Lease stack_pin_;unsigned entered_{};
 inline static thread_local Admission* current_{};
 const Scope* bound_{};
 public:
 static uint32_t address(uint64_t sp,int64_t offset,uint32_t width){
  if(sp>UINT32_MAX||!width)throw Error(Fault::bounds,"invalid synthetic stack base/width");
  uint64_t ea;
  if(offset<0){const uint64_t n=uint64_t(-(offset+1))+1;if(n>sp)throw Error(Fault::bounds,"stack offset underflow");ea=sp-n;}
  else{if(uint64_t(offset)>UINT32_MAX-sp)throw Error(Fault::bounds,"stack offset overflow");ea=sp+uint64_t(offset);}
  if(ea+width>0x100000000ull)throw Error(Fault::bounds,"stack access end overflow");
  if(ea%width)throw Error(Fault::argument,"stack access alignment");return uint32_t(ea);
 }
 static void layout(Space& space,Space::Handle h,uint64_t sp){
  // Keep full-allocation RW as the strict policy. Span checks additionally prove
  // all declared instruction-relative accesses fit; never infer smaller rights.
  space.validate_owned_window(h,h.address,h.size,Access::write,true);
  for(auto s:geometry_stores)space.validate_owned_window(h,address(sp,s.offset,s.width),s.width,Access::write,true);
 }
 Admission(GuestThread& g,ModuleLife& m,Declaration p):guest_(g),module_(m),proof_(p),module_pin_(m),stack_pin_(g.space(),p.stack){
  if(!g.belongs_to(m)||p.thread!=g.identity||p.module!=g.module_instance()||p.entry!=geometry_entry||p.source!=geometry_source_tag||!p.closed_calls)
   throw Error(Fault::ownership,"unproved synthetic thread/module/entry/call closure");
  if(p.stack.owner!=g.stack.owner||p.stack.id!=g.stack.id||p.stack.address!=g.stack.address||p.stack.size!=g.stack.size||p.stores!=geometry_stores)
   throw Error(Fault::ownership,"changed synthetic stack/source footprint");
  if(p.sp!=g.context.r1.u64||(p.sp&15)||p.begin>=p.end)throw Error(Fault::argument,"synthetic incoming stack contract");
  for(auto s:geometry_stores)if(s.offset<p.begin||s.offset+int64_t(s.width)>p.end)throw Error(Fault::bounds,"declared window misses required stores");
  layout(g.space(),p.stack,p.sp);
  if(current_)throw Error(Fault::ownership,"nested synthetic footprint admission");
  current_=this;
 }
 Admission(const Admission&)=delete;
 ~Admission(){if(current_!=this||bound_)std::terminate();current_=nullptr;}
 void before(uint32_t pc){
  ModuleLife::Lease fresh(module_);
  const uint32_t expected=entered_==0?geometry_entry:geometry_helper;
  if(entered_>=2||pc!=expected||guest_.context.r1.u64!=proof_.sp||guest_.identity!=proof_.thread||guest_.module_instance()!=proof_.module)
   throw Error(Fault::argument,"unexpected synthetic entry/order/stack mutation");
  if(guest_.context.lr!=(entered_==0?proof_.root_lr:uint64_t(geometry_entry)+12)||
     std::array<uint64_t,4>{guest_.context.r20.u64,guest_.context.r21.u64,guest_.context.r22.u64,guest_.context.r23.u64}!=proof_.payload)
   throw Error(Fault::argument,"changed declared synthetic payload/LR inputs");
  layout(guest_.space(),proof_.stack,proof_.sp);++entered_;
 }
 unsigned entries()const{return entered_;}
 static void prologue(PPCContext& context,uint8_t* base,std::string_view name){
  auto&scope=Scope::current(context);
  if(base||!current_||&current_->guest_.context!=&context||current_->bound_!=&scope)throw Error(Fault::ownership,"missing bound fixture footprint admission");
  if(name.starts_with("__imp__"))name.remove_prefix(7);
  for(auto f:geometry_functions)if(f.name==name){Scope::prologue(context,base,name);return;}
  throw Error(Fault::argument,"unbound synthetic function identity");
 }
};
// The ordinary Scope cannot silently omit this declaration's validation. The
// required prologue binds this exact wrapper; validation runs AFTER the trusted
// test controller and BEFORE Scope's final T8 check/guest-entry counter.
class BoundScope {
 Admission& admission_;Scope scope_;
 public:
 BoundScope(GuestThread& g,ModuleLife& m,Admission& a,brutal::synthetic::CooperativeStop* stop=nullptr,std::function<void(uint32_t)> controller={},bool revoke_after_check=false)
  :admission_(a),scope_(g,geometry_functions,m.identity,stop,[&g,&a,controller=std::move(controller),revoke_after_check](uint32_t pc){if(controller)controller(pc);a.before(pc);if(revoke_after_check)g.space().protect(a.proof_.stack,Permission::read);}){
  if(&g!=&a.guest_||&m!=&a.module_||a.bound_)throw Error(Fault::ownership,"wrong bound synthetic context");a.bound_=&scope_;
 }
 BoundScope(const BoundScope&)=delete;
 ~BoundScope(){admission_.bound_=nullptr;}
 void invoke(uint32_t pc){scope_.invoke(pc);}
 uint64_t writes()const{return scope_.writes;}
 uint64_t reads()const{return scope_.reads;}
 bool owns(const Scope* scope)const{return scope==&scope_;}
};
}
