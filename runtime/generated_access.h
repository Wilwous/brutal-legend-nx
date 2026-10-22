// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Opt-in from the pinned generated PCH, never a permissive fallback.
#include "sdk_checked_memory.h"
#include <algorithm>
#include <concepts>
#include <string_view>
#include <type_traits>

namespace brutal::instrumentation {
using Memory=foundation::SdkCheckedMemory;
using Result=Memory::Result;
enum class Kind { unbound, memory, unsupported_raw_atomic, unsupported_string,
                  unsupported_call, mmio };
struct Fault : std::runtime_error {
  Kind kind;
  Result memory_result;
  explicit Fault(Kind k,Result result=Result::invalid_range)
      :std::runtime_error("checked generated operation rejected"),kind(k),memory_result(result){}
};
class Scope {
  struct Binding { Memory::Allocation handle; uint32_t size; };
  struct Call { uint32_t address; PPCFunc* function; };
  struct Loan { uint32_t base,address,size; const void* owner; };
  Memory& memory_;
  rex::memory::Memory& sdk_;
  std::vector<Binding> bindings_;
  std::vector<Call> calls_;
  std::vector<Loan> loans_;
  Scope* previous_;
  inline static thread_local Scope* current_=nullptr;
 public:
  Scope(Memory& memory,rex::memory::Memory& sdk):memory_(memory),sdk_(sdk),previous_(current_){current_=this;}
  Scope(const Scope&)=delete;Scope& operator=(const Scope&)=delete;
  ~Scope(){current_=previous_;}
  void bind(const Memory::Allocation& allocation,uint32_t size){
    if(!size||bindings_.size()>=64||uint64_t(allocation.guest_address())+size>0x100000000ull)
      throw Fault(Kind::memory);
    const uint64_t low=allocation.guest_address(),high=low+size;
    for(const auto& b:bindings_)if(low<uint64_t(b.handle.guest_address())+b.size&&b.handle.guest_address()<high)
      throw Fault(Kind::memory);
    for(auto b:loans_)if(low<uint64_t(b.address)+b.size&&b.address<high)throw Fault(Kind::memory);
    bindings_.push_back({allocation,size});
  }
  // Host-authored allowlist of original context/base fixtures ONLY. This does
  // not infer a guest signature or authorize an imported/game function.
  void allow_synthetic(uint32_t address,PPCFunc* function){
    if(!address||(address&3)||!function||calls_.size()>=64)throw Fault(Kind::unsupported_call);
    for(auto c:calls_)if(c.address==address||c.function==function)throw Fault(Kind::unsupported_call);
    calls_.push_back({address,function});
  }
  // Trusted, non-owning host loan. Caller pins the allocation/owner identity and
  // revokes before Exit/Release/Reset/Restore. Never accepts guest-supplied
  // metadata. Used only for fresh original XThread fixtures, not title memory.
  void lend(uint32_t allocation_base,const void* owner,uint32_t address,uint32_t size){
    if(!owner||!allocation_base||!sdk_.LookupHeap(allocation_base)||address<allocation_base||!size||
       uint64_t(address)+size>0x100000000ull||loans_.size()>=8)
      throw Fault(Kind::memory);
    const uint64_t end=uint64_t(address)+size;
    for(auto b:loans_)if(address<uint64_t(b.address)+b.size&&b.address<end)
      throw Fault(Kind::memory);
    for(auto b:bindings_)if(address<uint64_t(b.handle.guest_address())+b.size&&b.handle.guest_address()<end)
      throw Fault(Kind::memory);
    loans_.push_back({allocation_base,address,size,owner});
  }
  static Scope& current(){if(!current_)throw Fault(Kind::unbound);return *current_;}
  void copy(uint32_t address,std::span<std::byte> bytes,bool write){
    if(!address||bytes.empty()||bytes.size()>0x100000000ull-address)throw Fault(Kind::memory);
    for(uint64_t a=address;a<uint64_t(address)+bytes.size();++a)
      if((a>=0x7f000000ull&&a<0x80000000ull)||sdk_.LookupVirtualMappedRange(uint32_t(a)))
        throw Fault(Kind::mmio,Result::mmio);
    for(const auto& b:bindings_){
      auto start=b.handle.guest_address();
      if(address>=start&&uint64_t(address)+bytes.size()<=uint64_t(start)+b.size){
        auto result=write?memory_.write(b.handle,address-start,bytes):memory_.read(b.handle,address-start,bytes);
        if(result!=Result::ok)throw Fault(Kind::memory,result);
        return;
      }
    }
    for(auto b:loans_)if(address>=b.address&&uint64_t(address)+bytes.size()<=uint64_t(b.address)+b.size){
      auto result=sdk_.LookupHeap(b.base)->CopyOwnedMemory(b.base,b.owner,address,
          bytes.data(),bytes.size(),write?Memory::Access::write:Memory::Access::read);
      if(result!=Result::ok)throw Fault(Kind::memory,result);
      return;
    }
    throw Fault(Kind::memory,Result::ownership);
  }
  void direct(PPCFunc* function,PPCContext& context,uint8_t* base){
    for(auto c:calls_)if(c.function==function){function(context,base);return;}
    throw Fault(Kind::unsupported_call);
  }
  void indirect(uint32_t address,PPCContext& context,uint8_t* base){
    for(auto c:calls_)if(c.address==address){c.function(context,base);return;}
    throw Fault(Kind::unsupported_call);
  }
};
template<std::unsigned_integral T> T load(uint32_t address){
  std::array<std::byte,sizeof(T)> bytes{};Scope::current().copy(address,bytes,false);
  T value=0;for(auto b:bytes)value=T((value<<8)|std::to_integer<unsigned>(b));return value;
}
template<std::unsigned_integral T> void store(uint32_t address,T value){
  std::array<std::byte,sizeof(T)> bytes{};
  for(size_t i=0;i<sizeof(T);++i){bytes[sizeof(T)-i-1]=std::byte(value&255);value=T(value>>8);}
  Scope::current().copy(address,bytes,true);
}
inline void load_vector(PPCVRegister& value,uint32_t address){
  std::array<std::byte,16> bytes{};Scope::current().copy(address,bytes,false);
  for(size_t i=0;i<16;++i)value.u8[15-i]=std::to_integer<uint8_t>(bytes[i]);
}
inline void store_vector(uint32_t address,const PPCVRegister& value){
  std::array<std::byte,16> bytes{};for(size_t i=0;i<16;++i)bytes[i]=std::byte(value.u8[15-i]);
  Scope::current().copy(address,bytes,true);
}
[[noreturn]] inline uint8_t* raw(uint32_t){Scope::current();throw Fault(Kind::unsupported_raw_atomic);}
[[noreturn]] inline std::string_view string(uint32_t,size_t){Scope::current();throw Fault(Kind::unsupported_string);}
[[noreturn]] inline PPCFunc* lookup(){Scope::current();throw Fault(Kind::unsupported_call);}
// Deliberately only the context/base signature; typed guest ABI is not invented.
template<class F> concept ContextFunction=std::same_as<F,PPCFunc*>;
template<ContextFunction F> void direct(F f,PPCContext& ctx,uint8_t* base){Scope::current().direct(f,ctx,base);}
} // namespace brutal::instrumentation

[[noreturn]] inline void brutal_reject_partial_vector(){brutal::instrumentation::Scope::current();throw brutal::instrumentation::Fault(brutal::instrumentation::Kind::unsupported_raw_atomic);}
#undef REX_PARTIAL_VECTOR_GUARD
#define REX_PARTIAL_VECTOR_GUARD() brutal_reject_partial_vector()
#undef REX_LOAD_U8
#undef REX_LOAD_U16
#undef REX_LOAD_U32
#undef REX_LOAD_U64
#undef REX_STORE_U8
#undef REX_STORE_U16
#undef REX_STORE_U32
#undef REX_STORE_U64
#undef REX_RAW_ADDR
#undef REX_LOAD_STRING
#undef REX_MM_LOAD_U8
#undef REX_MM_LOAD_U16
#undef REX_MM_LOAD_U32
#undef REX_MM_LOAD_U64
#undef REX_MM_STORE_U8
#undef REX_MM_STORE_U16
#undef REX_MM_STORE_U32
#undef REX_MM_STORE_U64
#undef REX_LOAD_VECTOR
#undef REX_STORE_VECTOR
#undef REX_CALL_FUNC
#undef REX_LOOKUP_FUNC
#undef REX_CALL_INDIRECT_FUNC
#define REX_LOAD_U8(a) brutal::instrumentation::load<uint8_t>(uint32_t(a))
#define REX_LOAD_U16(a) brutal::instrumentation::load<uint16_t>(uint32_t(a))
#define REX_LOAD_U32(a) brutal::instrumentation::load<uint32_t>(uint32_t(a))
#define REX_LOAD_U64(a) brutal::instrumentation::load<uint64_t>(uint32_t(a))
#define REX_STORE_U8(a,v) brutal::instrumentation::store<uint8_t>(uint32_t(a),uint8_t(v))
#define REX_STORE_U16(a,v) brutal::instrumentation::store<uint16_t>(uint32_t(a),uint16_t(v))
#define REX_STORE_U32(a,v) brutal::instrumentation::store<uint32_t>(uint32_t(a),uint32_t(v))
#define REX_STORE_U64(a,v) brutal::instrumentation::store<uint64_t>(uint32_t(a),uint64_t(v))
#define REX_RAW_ADDR(a) brutal::instrumentation::raw(uint32_t(a))
#define REX_LOAD_STRING(a,n) brutal::instrumentation::string(uint32_t(a),size_t(n))
#define REX_MM_LOAD_U8(a) REX_LOAD_U8(a)
#define REX_MM_LOAD_U16(a) REX_LOAD_U16(a)
#define REX_MM_LOAD_U32(a) REX_LOAD_U32(a)
#define REX_MM_LOAD_U64(a) REX_LOAD_U64(a)
#define REX_MM_STORE_U8(a,v) REX_STORE_U8(a,v)
#define REX_MM_STORE_U16(a,v) REX_STORE_U16(a,v)
#define REX_MM_STORE_U32(a,v) REX_STORE_U32(a,v)
#define REX_MM_STORE_U64(a,v) REX_STORE_U64(a,v)
#define REX_LOAD_VECTOR(v,a) brutal::instrumentation::load_vector(v,uint32_t(a))
#define REX_STORE_VECTOR(a,v) brutal::instrumentation::store_vector(uint32_t(a),v)
#define REX_CALL_FUNC(f) brutal::instrumentation::direct(static_cast<PPCFunc*>(f),ctx,base)
#define REX_LOOKUP_FUNC(base,a) brutal::instrumentation::lookup()
#define REX_CALL_INDIRECT_FUNC(a) brutal::instrumentation::Scope::current().indirect(uint32_t(a),ctx,base)
