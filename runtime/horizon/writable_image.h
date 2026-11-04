// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "header_buffer.h"

namespace brutal::horizon::writable_image {
using header_buffer::require;using native_image::Digest;using native_image::digest;
// This is explicit native policy, never inferred from PE flags or source bytes.
// Provenance-only descriptors cannot authorize RW publication.
enum class Authority : uint64_t {original_synthetic=1,provenance_only=2};
struct Source {
 uint64_t module{},generation{},offset{},length{},guest{};
 Digest source{},interval{};Authority authority{Authority::provenance_only};
};
inline Digest identity(const Source&p){
 std::vector<uint8_t>b;for(auto n:{uint64_t(0x5257494d41474531),p.module,p.generation,p.offset,p.length,p.guest,uint64_t(p.authority)})for(int i=7;i>=0;--i)b.push_back(uint8_t(n>>(8*i)));
 for(auto d:{p.source,p.interval})b.insert(b.end(),d.begin(),d.end());return digest(b);
}
inline void validate(const Source&p,ModuleLife&m,std::span<const uint8_t>b,const Digest&trusted){
 require(p.module==m.identity&&p.generation==m.instance,Fault::ownership,"RW module instance");
 require(p.authority==Authority::original_synthetic,Fault::unsupported,"no admitted native RW authority");
 require(!b.empty()&&b.size()<=64*1024*1024&&p.length&&p.length<=1024*1024,Fault::bounds,"RW source budget");
 require(p.offset<=b.size()&&p.length<=b.size()-p.offset,Fault::bounds,"RW source extent");
 require(p.guest&&p.guest<0x100000000ull&&p.length<=0x100000000ull-p.guest,Fault::bounds,"RW guest span");
 require(p.source!=Digest{}&&p.interval!=Digest{}&&identity(p)==trusted,Fault::argument,"RW trusted descriptor");
 require(digest(b)==p.source&&digest(b.subspan(p.offset,p.length))==p.interval,Fault::argument,"RW source fingerprint");
}
// Trusted native Space/ModuleLife must outlive all borrows. No raw pointer escapes.
// Raw management pins require retaining this wrapper and retrying close.
class Region {
 inline static std::atomic<uint64_t> next_{1};inline static std::atomic<size_t> registered_{};
 public:
 struct Ref {uint64_t object{},module{},generation{},owner{},allocation{};uint32_t guest{},length{};bool operator==(const Ref&)const=default;};
 private:
 struct State {
  Space&space;ModuleLife&module;std::mutex mutex;std::unique_ptr<ModuleLife::Lease> life;
  Space::Handle handle{};std::unique_ptr<Space::Lease> pin;const uint64_t id{next_.fetch_add(1)};
  bool published{},closing{},released{};size_t readers{};
  State(Space&s,ModuleLife&m):space(s),module(m){}
  Ref ref()const{return {id,module.identity,module.instance,handle.owner,handle.id,handle.address,handle.size};}
  bool discard(){
   require(!readers,Fault::ownership,"active RW borrows");
   if(pin){if(!space.release_exclusive(*pin))return false;pin.reset();}else if(handle.id)space.release(handle);
   if(published)--registered_;published=false;released=true;life.reset();return true;
  }
  void request_close(){std::lock_guard lock(mutex);if(released)return;closing=true;if(!readers)discard();}
  ~State(){try{if(readers||(!released&&!discard()))std::terminate();}catch(...){std::terminate();}}
 };
 std::shared_ptr<State> state_;
 public:
 Region(Space&s,ModuleLife&m,const Source&p,std::span<const uint8_t>b,const Digest&trusted,int fail=-1){
  auto st=std::make_shared<State>(s,m);require(st->id!=0,Fault::exhausted,"RW identity exhaustion");st->life=std::make_unique<ModuleLife::Lease>(m);validate(p,m,b,trusted);
  int step=0;auto point=[&]{if(step++==fail)throw Error(Fault::exhausted,"injected RW publication failure");};
  point();st->handle=s.reserve_slice(uint32_t(p.length),uint32_t(p.guest));point();s.commit(st->handle,Permission::write);point();
  auto bytes=b.subspan(p.offset,p.length);s.write(st->handle.address,{reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()});point();s.protect(st->handle,Permission::read);
  auto copied=s.read(st->handle.address,st->handle.size);require(digest({reinterpret_cast<const uint8_t*>(copied.data()),copied.size()})==p.interval,Fault::argument,"RW copied fingerprint");point();
  st->pin=std::make_unique<Space::Lease>(s,st->handle);point();s.protect(st->handle,Permission::read_write);point();
  st->published=true;++registered_;state_=std::move(st);
 }
 Region(const Region&)=delete;Region&operator=(const Region&)=delete;
 ~Region(){if(state_)state_->request_close();}
 Ref reference()const{std::lock_guard lock(state_->mutex);require(state_->published&&!state_->closing,Fault::stale,"RW publication closed");return state_->ref();}
 class Borrow {
  std::shared_ptr<State> state_;Access mode_;friend class Region;
  Borrow(std::shared_ptr<State>s,Access mode):state_(std::move(s)),mode_(mode){}
  Space::Handle window(uint32_t offset,uint32_t length,Access operation)const{
   require(bool(state_),Fault::stale,"moved RW borrow");auto h=state_->handle;
   require(state_->published&&!state_->released,Fault::stale,"RW region retired");
   require(length&&offset<=h.size&&length<=h.size-offset,Fault::bounds,"RW logical span");
   require(operation!=Access::write||mode_==Access::write,Fault::protection,"read borrow cannot write");
   state_->space.validate_owned_window(h,h.address+offset,length,operation);return h;
  }
  public:
  Borrow(const Borrow&)=delete;Borrow(Borrow&&)=default;
  ~Borrow(){if(state_)try{std::lock_guard lock(state_->mutex);if(!state_->readers)std::terminate();--state_->readers;if(!state_->readers&&state_->closing)state_->discard();}catch(...){std::terminate();}}
  std::vector<std::byte> read(uint32_t offset,uint32_t length)const{require(bool(state_),Fault::stale,"moved RW borrow");std::lock_guard lock(state_->mutex);auto h=window(offset,length,Access::read);return state_->space.read(h.address+offset,length);}
  void write(uint32_t offset,std::span<const std::byte> bytes){require(bool(state_)&&bytes.size()<=UINT32_MAX,Fault::bounds,"RW write size");std::lock_guard lock(state_->mutex);auto h=window(offset,uint32_t(bytes.size()),Access::write);state_->space.write(h.address+offset,bytes);}
  Space::Handle registration(Access operation=Access::write)const{require(bool(state_),Fault::stale,"moved RW borrow");std::lock_guard lock(state_->mutex);require(operation==Access::read||operation==Access::write,Fault::unsupported,"RW execute/atomic unsupported");return window(0,state_->handle.size,operation);}
  template<class ScopeType>void lend(ScopeType&scope)const{require(bool(state_)&&scope.belongs_to(state_->module,state_->space),Fault::ownership,"foreign RW execution scope");scope.lend(registration());}
  bool belongs_to(const ModuleLife&m,const Space&s)const{return state_&&&state_->module==&m&&&state_->space==&s;}
 };
 Borrow acquire(ModuleLife&m,Ref ref,Access mode=Access::write){
  ModuleLife::Lease admission(m);std::lock_guard lock(state_->mutex);require(&m==&state_->module&&ref==state_->ref(),Fault::ownership,"foreign/stale RW capability");require(state_->published&&!state_->closing,Fault::stale,"RW admission closed");
  require(mode==Access::read||mode==Access::write,Fault::unsupported,"RW acquire mode");state_->space.validate_owned_window(state_->handle,state_->handle.address,state_->handle.size,mode,mode==Access::write);++state_->readers;return Borrow(state_,mode);
 }
 bool close(){std::lock_guard lock(state_->mutex);require(!state_->released,Fault::stale,"RW already released");require(!state_->closing||!state_->readers,Fault::ownership,"RW close pending readers");state_->closing=true;if(state_->readers)return false;return state_->discard();}
 bool closed()const{std::lock_guard lock(state_->mutex);return state_->released;}
 static size_t registrations(){return registered_.load();}
};
}
