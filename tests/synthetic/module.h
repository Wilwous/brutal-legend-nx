// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Original FIXTURE loader: not XEX/ELF, no dynamic instruction fetch or general
// relocation engine. Trusted host descriptors; raw PPC bytes remain read-only.
#include "runtime/generated_access.h"
#include <utility>
namespace synthetic {
using namespace brutal::instrumentation;
struct Section { uint32_t offset,size; bool write,code; std::vector<std::byte> bytes; };
struct Relocation { uint32_t offset,target; };
class Module {
  Memory& memory_; rex::memory::Memory& sdk_;
  Memory::Allocation image_;
  bool live_=false;
  std::vector<Section> sections_;
  std::vector<std::pair<uint32_t,PPCFunc*>> functions_;
 public:
  static constexpr uint32_t image_size=0xc000;
  Module(Memory& memory,rex::memory::Memory& sdk,std::vector<Section> sections,
         std::vector<Relocation> relocations,bool needs_guest_exceptions=false):memory_(memory),sdk_(sdk),sections_(std::move(sections)){
    if(needs_guest_exceptions)throw std::invalid_argument("guest exceptions unsupported");
    if(sections_.empty()||sections_.size()>8||relocations.size()>32)throw std::invalid_argument("module counts");
    for(size_t i=0;i<sections_.size();++i){const auto& s=sections_[i];
      if(!s.size||(s.offset&0x3fff)||(s.size&4095)||s.offset>image_size||s.size>image_size-s.offset||
         s.bytes.size()>s.size||(s.write&&s.code))throw std::invalid_argument("module section");
      for(size_t j=0;j<i;++j){const auto& p=sections_[j];if(s.offset<p.offset+p.size&&p.offset<s.offset+s.size)throw std::invalid_argument("module overlap");}
    }
    for(size_t i=0;i<relocations.size();++i){auto r=relocations[i];bool destination=false,target=false;
      for(const auto& s:sections_){destination|=s.write&&r.offset>=s.offset&&uint64_t(r.offset)+4<=uint64_t(s.offset)+s.size;target|=r.target>=s.offset&&r.target-s.offset<s.size;}
      if((r.offset&3)||!destination||!target)throw std::invalid_argument("module relocation");
      for(size_t j=0;j<i;++j)if(relocations[j].offset==r.offset)throw std::invalid_argument("duplicate relocation");
    }
    image_=memory_.allocate(image_size,false);live_=true;
    try{
      auto* heap=sdk_.LookupHeap(base());
      for(const auto& s:sections_){
        if(!heap->AllocFixed(base()+s.offset,s.size,4096,rex::memory::kMemoryAllocationCommit,3))throw std::runtime_error("module commit");
        std::vector<std::byte> zero(s.size);
        if(memory_.write(image_,s.offset,zero)!=Result::ok)throw std::runtime_error("module zero initialization");
        if(!s.bytes.empty()&&memory_.write(image_,s.offset,s.bytes)!=Result::ok)throw std::runtime_error("module initialization");
      }
      for(auto r:relocations)if(memory_.store32be(image_,r.offset,base()+r.target)!=Result::ok)throw std::runtime_error("module relocation write");
      for(const auto& s:sections_)if(!heap->Protect(base()+s.offset,s.size,s.write?3:1))throw std::runtime_error("module protect");
    }catch(...){memory_.release(image_);live_=false;throw;}
  }
  Module(const Module&)=delete;Module& operator=(const Module&)=delete;
  ~Module(){if(live_)memory_.release(image_);}
  uint32_t base()const{return image_.guest_address();}
  const Memory::Allocation& allocation()const{return image_;}
  void function(uint32_t offset,PPCFunc* function){
    bool valid=false;for(const auto& s:sections_)valid|=s.code&&offset>=s.offset&&uint64_t(offset)+4<=uint64_t(s.offset)+s.bytes.size();
    if(!live_||!valid||(offset&3)||!function||functions_.size()>=32)throw std::invalid_argument("module function");
    for(auto [old,fn]:functions_)if(old==offset||fn==function)throw std::invalid_argument("module duplicate function");
    functions_.push_back({offset,function});
  }
  void admit(Scope& scope){if(!live_)throw std::invalid_argument("unloaded module");scope.bind(image_,image_size);for(auto [off,fn]:functions_)scope.allow_synthetic(base()+off,fn);}
  void enter(uint32_t address,PPCContext& context){
    if(!live_)throw std::invalid_argument("unloaded module");
    for(auto [off,fn]:functions_)if(address==base()+off){Scope::current().indirect(address,context,nullptr);return;}
    throw std::invalid_argument("module entry");
  }
  void unload(){if(!live_||memory_.release(image_)!=Result::ok)throw std::runtime_error("module unload");live_=false;}
};
}
