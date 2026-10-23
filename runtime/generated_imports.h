// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <rex/ppc.h>
#include "generated_access.h"
#include "import_admission.h"
namespace brutal::imports {
// Explicit host-authored relocation metadata for ORIGINAL raw-test fixtures.
// This does not parse/infer an import, authorize arbitrary host callbacks, or
// change the generated instruction bodies. Admission rechecks the real XEX.
class GeneratedImports {
 struct Gate { PPCFunc* symbol;uint32_t thunk,ordinal;Signature signature;PointerBinding* binding; };
 Admission& admission_;
 uint8_t* base_;
 PPCContext& context_;
 std::vector<Gate> gates_;
 GeneratedImports* previous_;
 inline static thread_local GeneratedImports* current_=nullptr;
 void invoke(const Gate& g,bool indirect,PPCContext& c,uint8_t* base){
  if(base!=base_||&c!=&context_||rex::runtime::current_ppc_context()!=&c)
   throw std::logic_error("generated import context mismatch");
  ++attempts;
  if(g.binding){
   if(indirect)admission_.indirect(g.thunk,g.signature,c,*g.binding);
   else admission_.direct("xboxkrnl.exe",g.ordinal,g.signature,c,*g.binding);
  }else{
   if(indirect)admission_.indirect(g.thunk,g.signature,c);
   else admission_.direct("xboxkrnl.exe",g.ordinal,g.signature,c);
  }
  ++returns;
 }
 public:
 uint64_t attempts=0,returns=0;
 GeneratedImports(Admission& a,PPCContext& c,uint8_t* base):admission_(a),base_(base),context_(c),previous_(current_){current_=this;}
 GeneratedImports(const GeneratedImports&)=delete;GeneratedImports& operator=(const GeneratedImports&)=delete;
 ~GeneratedImports(){current_=previous_;}
 void bind(PPCFunc* symbol,uint32_t thunk,uint32_t ordinal,Signature signature,PointerBinding& binding){
  if(!symbol||!thunk||(thunk&3)||gates_.size()>=9)throw std::invalid_argument("generated gate");
  for(auto g:gates_)if((symbol&&g.symbol==symbol)||g.thunk==thunk||g.ordinal==ordinal)throw std::invalid_argument("duplicate generated gate");
  gates_.push_back({symbol,thunk,ordinal,signature,&binding});
 }
 void bind_indirect(uint32_t thunk,uint32_t ordinal,Signature signature,PointerBinding& binding){
  if(!thunk||(thunk&3)||gates_.size()>=9)throw std::invalid_argument("generated indirect gate");
  for(auto g:gates_)if(g.thunk==thunk||g.ordinal==ordinal)throw std::invalid_argument("duplicate generated gate");
  gates_.push_back({nullptr,thunk,ordinal,signature,&binding});
 }
 // A caller that has classified a target as an import must not reach the
 // general direct-call API's explicitly allowed native-function branch.
 static void direct_import(PPCFunc* f,PPCContext& c,uint8_t* base){
  if(f&&current_)for(auto g:current_->gates_)if(g.symbol==f){current_->invoke(g,false,c,base);return;}
  throw instrumentation::Fault(instrumentation::Kind::unsupported_call);
 }
 static void direct(PPCFunc* f,PPCContext& c,uint8_t* base){
  if(current_)for(auto g:current_->gates_)if(g.symbol==f){current_->invoke(g,false,c,base);return;}
  instrumentation::Scope::current().direct(f,c,base);
 }
 static void indirect(uint32_t target,PPCContext& c,uint8_t* base){
  if(current_){
   for(auto g:current_->gates_)if(!g.symbol&&g.thunk==target){current_->invoke(g,true,c,base);return;}
   // Generated bctrl sites are import-only in this scope. Do not admit a
   // synthetic entry/callee, a direct-only gate, or resolver fallback here.
   throw instrumentation::Fault(instrumentation::Kind::unsupported_call);
  }
  throw instrumentation::Fault(instrumentation::Kind::unsupported_call);
 }
};
}
#undef REX_CALL_FUNC
#undef REX_CALL_INDIRECT_FUNC
#define REX_CALL_FUNC(f) brutal::imports::GeneratedImports::direct(static_cast<PPCFunc*>(f),ctx,base)
#define REX_CALL_INDIRECT_FUNC(a) brutal::imports::GeneratedImports::indirect(uint32_t(a),ctx,base)
