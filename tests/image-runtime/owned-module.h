// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "tests/synthetic/module.h"
#include <mutex>
#include <memory>
#include <string>
namespace image_fixture {
using namespace synthetic;
// Custom fixture ownership. NOT an SDK XModule/HMODULE or XEX TLS implementation.
// Memory and SDK must outlive every handle/lease. No Reset/Restore/MMIO mutation.
class OwnedModule {
 struct State {
  std::mutex mutex;
  std::unique_ptr<Module> image;
  std::vector<std::byte> tls;
  unsigned active=0;
  bool closing=false;
 };
 std::shared_ptr<State> state_;
 public:
 enum class Close { closed,busy,already_closed };
 struct Import {std::string name;uint32_t offset;};
 struct Export {std::string name;PPCFunc* function;bool context_signature=true;};
 class Lease {
  friend class OwnedModule;
  std::shared_ptr<State> state_;
  explicit Lease(std::shared_ptr<State> s):state_(std::move(s)){}
  State& state()const{if(!state_)throw std::logic_error("empty module lease");return *state_;}
  public:
  Lease(const Lease&)=delete;Lease& operator=(const Lease&)=delete;
  Lease(Lease&& other)noexcept:state_(std::move(other.state_)){}
  Lease& operator=(Lease&&)=delete;
  ~Lease(){if(state_){std::lock_guard lock(state_->mutex);--state_->active;}}
  uint32_t base()const{return state().image->base();}
  Memory::Allocation allocation()const{return state().image->allocation();}
  std::vector<std::byte> tls_template()const{return state().tls;}
  void admit(Scope& scope){state().image->admit(scope);}
  void enter(uint32_t address,PPCContext& c){state().image->enter(address,c);}
 };
 static OwnedModule load(Memory& memory,rex::memory::Memory& sdk,std::vector<Section> sections,
   std::vector<Relocation> relocations,const std::vector<Import>& imports,
   const std::vector<Export>& exports,std::vector<std::byte> tls={},int fail_stage=0){
  if(tls.size()>4096||imports.size()>32||exports.size()>32)throw std::invalid_argument("fixture metadata limit");
  auto state=std::make_shared<State>();state->tls=std::move(tls);
  state->image=std::make_unique<Module>(memory,sdk,std::move(sections),std::move(relocations));
  if(fail_stage==1)throw std::runtime_error("injected after mapping");
  for(size_t i=0;i<imports.size();++i){
   const auto& request=imports[i];PPCFunc* found=nullptr;
   for(size_t j=0;j<i;++j)if(imports[j].name==request.name)throw std::invalid_argument("duplicate fixture import");
   for(const auto& symbol:exports)if(symbol.name==request.name){
    if(found||!symbol.context_signature||!symbol.function)throw std::invalid_argument("ambiguous or unsupported fixture signature");
    found=symbol.function;
   }
   if(!found)throw std::invalid_argument("unresolved fixture import");
   state->image->function(request.offset,found);
  }
  if(fail_stage==2)throw std::runtime_error("injected after import resolution");
  OwnedModule result;result.state_=std::move(state);return result;
 }
 Lease acquire()const{
  if(!state_)throw std::logic_error("empty module handle");
  std::lock_guard lock(state_->mutex);
  if(state_->closing||!state_->image)throw std::logic_error("module admission closed");
  ++state_->active;return Lease(state_);
 }
 Close close(){
  if(!state_)throw std::logic_error("empty module handle");
  std::lock_guard lock(state_->mutex);state_->closing=true;
  if(!state_->image)return Close::already_closed;
  if(state_->active)return Close::busy;
  state_->image->unload();state_->image.reset();return Close::closed;
 }
};
}
