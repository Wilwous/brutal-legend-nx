// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "sealed_object.h"
#include <tuple>
namespace brutal::horizon::native_module {
// Selected native inline-word lookup. Absence is not invalid pointer/owner data.
class Field {
 std::shared_ptr<Metadata::Reader> owner_;uint32_t offset_{},address_{};
 friend Field lookup(std::shared_ptr<Metadata::Reader>,uint32_t,uint32_t);
 Field(std::shared_ptr<Metadata::Reader> owner,uint32_t offset,uint32_t address):owner_(std::move(owner)),offset_(offset),address_(address){}
 public:
 uint32_t address()const{require(bool(owner_),Fault::stale,"moved field borrow");owner_->header_address();return address_;}
 uint32_t value()const{require(bool(owner_),Fault::stale,"moved field borrow");require(address_!=0,Fault::bounds,"absent header field has no word");return be32(owner_->header_bytes(offset_,4));}
};
inline Field lookup(std::shared_ptr<Metadata::Reader> module,uint32_t header,uint32_t key){
 require(bool(module)&&header==module->header_address(),Fault::ownership,"header/module pointer mismatch");
 require(key==0x20401,Fault::unsupported,"unsupported native field selector");auto b=module->header_bytes(0,module->header_size());
 auto n=[&](size_t o){return be32(std::span(b).subspan(o,4));};
 require(b.size()>=24&&n(0)==0x58455832&&n(8)==b.size(),Fault::bounds,"invalid header backing");
 require(n(20)<=(b.size()-24)/8,Fault::bounds,"invalid optional catalog");uint32_t offset=0;
 for(uint32_t i=0;i<n(20);++i)if(n(24+8*i)==key){require(!offset,Fault::argument,"duplicate selected field");offset=28+8*i;}
 if(!offset)return Field(std::move(module),0,0);
 require(uint64_t(header)+offset+4<=0x100000000ull,Fault::bounds,"field ptr32 overflow");return Field(std::move(module),offset,header+offset);
}
enum class ImportKind {variable,function};
struct VariableKey {std::string library;uint32_t ordinal{};ImportKind kind{ImportKind::variable};auto operator<=>(const VariableKey&)const=default;};
class Registry;class Binding;
class Provider {
 struct State {
  Space&space;ModuleLife&module;std::mutex mutex;bool open{true};VariableKey key;
  std::unique_ptr<Object> cell;std::weak_ptr<Metadata::Reader> metadata;uint32_t address;
  State(Space&s,ModuleLife&m,VariableKey k,uint32_t a):space(s),module(m),key(std::move(k)),address(a){}
 };
 std::shared_ptr<State> state_;friend class Registry;friend class Binding;
 public:
 using Ref=Object::Ref;
 Provider(Space&s,ModuleLife&m,Metadata&metadata,Metadata::Ref ref,VariableKey key,uint64_t address,int fail=-1){
  require(key.kind==ImportKind::variable&&!key.library.empty()&&key.ordinal&&address&&(address%4)==0&&address<=UINT32_MAX-3,Fault::argument,"variable provider identity/address");
  require(key.library=="xboxkrnl.exe"&&key.ordinal==0x193,Fault::unsupported,"no contract for this variable provider");
  auto graph=std::make_shared<Metadata::Reader>(metadata.acquire(m,ref));require(graph->belongs_to(m,s),Fault::ownership,"provider address space");
  auto st=std::make_shared<State>(s,m,std::move(key),uint32_t(address));st->metadata=graph;std::array<Bytes,1>b{{{uint32_t(address),word(graph->base())}}};st->cell=std::make_unique<Object>(s,m,b,std::vector<std::shared_ptr<void>>{graph},0,fail);state_=std::move(st);
 }
 Provider(const Provider&)=delete;~Provider(){if(state_){std::lock_guard lock(state_->mutex);if(state_->open){state_->open=false;state_->cell->close();}}}
 Ref reference()const{return state_->cell->reference();}
 bool close(){std::lock_guard lock(state_->mutex);require(state_->open,Fault::stale,"provider withdrawn");state_->open=false;return state_->cell->close();}
 class Reader {
  Object::Reader cell_;std::shared_ptr<Metadata::Reader> metadata_;std::shared_ptr<State> state_;friend class Registry;friend class Binding;
  Reader(Object::Reader c,std::shared_ptr<Metadata::Reader>m,std::shared_ptr<State>s):cell_(std::move(c)),metadata_(std::move(m)),state_(std::move(s)){}
  public:
  Reader(const Reader&)=delete;Reader(Reader&&)=default;
  uint32_t cell_address()const{require(be32(cell_.read(0,0,4))==metadata_->base(),Fault::ownership,"variable object pointer");return state_->address;}
  uint32_t module_base()const{cell_address();return metadata_->base();}
  Field field(uint32_t header,uint32_t key)const{cell_address();return lookup(metadata_,header,key);}
  uint32_t header_address()const{cell_address();return metadata_->header_address();}
  std::vector<Space::Handle> registrations()const{auto h=cell_.registrations();auto m=metadata_->registrations();h.insert(h.end(),m.begin(),m.end());return h;}
 };
};
class Registry {
 struct Entry {std::weak_ptr<Provider::State> state;Provider::Ref identity;};
 std::mutex mutex_;std::map<VariableKey,Entry> entries_;friend class Binding;
 public:
 void publish(Provider&p){std::lock_guard lock(mutex_);auto s=p.state_;std::lock_guard owner(s->mutex);require(s->open,Fault::stale,"closed provider publication");auto i=entries_.find(s->key);
  if(i!=entries_.end()){auto old=i->second.state.lock();if(old&&old!=s){std::lock_guard previous(old->mutex);require(!old->open,Fault::ownership,"variable already registered");}else require(!old,Fault::ownership,"duplicate variable publication");}
  entries_[s->key]={s,s->cell->reference()};
 }
 void withdraw(const VariableKey&key,Provider::Ref ref){std::lock_guard lock(mutex_);auto i=entries_.find(key);require(i!=entries_.end()&&i->second.identity==ref,Fault::ownership,"stale provider withdrawal");auto s=i->second.state.lock();entries_.erase(i);if(s){std::lock_guard owner(s->mutex);if(s->open){s->open=false;s->cell->close();}}}
 size_t active(){std::lock_guard lock(mutex_);size_t n=0;for(auto&[key,e]:entries_){(void)key;auto s=e.state.lock();if(s){std::lock_guard owner(s->mutex);n+=s->open;}}return n;}
};
// Owns a new synthetic import slot S storing P, not M. Self-module policy only;
// original PE import records and general DLL/process resolution remain unadmitted.
class Binding {
 Registry&registry_;ModuleLife&module_;VariableKey key_;Provider::Ref provider_ref_;
 std::weak_ptr<Provider::Reader> provider_;std::unique_ptr<Object> slot_;
 public:
 using Ref=Object::Ref;
 Binding(Registry&r,Space&s,ModuleLife&m,VariableKey key,Provider::Ref expected,uint64_t address,int fail=-1):registry_(r),module_(m),key_(std::move(key)),provider_ref_(expected){
  require(key_.kind==ImportKind::variable&&address&&(address%4)==0&&address<=UINT32_MAX-3,Fault::argument,"variable binding kind/address");
  std::lock_guard lock(r.mutex_);auto i=r.entries_.find(key_);require(i!=r.entries_.end()&&i->second.identity==expected,Fault::ownership,"unknown/stale variable provider");auto state=i->second.state.lock();require(bool(state),Fault::stale,"retired provider");std::lock_guard owner(state->mutex);
  require(state->open&&&state->module==&m&&&state->space==&s,Fault::ownership,"variable module/space/admission");
  auto cell=state->cell->acquire(m,expected);auto graph=state->metadata.lock();require(bool(graph),Fault::stale,"lost metadata owner");
  auto retained=std::shared_ptr<Provider::Reader>(new Provider::Reader(std::move(cell),std::move(graph),state));provider_=retained;
  std::array<Bytes,1>bytes{{{uint32_t(address),word(retained->cell_address())}}};slot_=std::make_unique<Object>(s,m,bytes,std::vector<std::shared_ptr<void>>{retained},0,fail);
 }
 Ref reference()const{return slot_->reference();}
 class Reader {
  Object::Reader slot_;std::shared_ptr<Provider::Reader> provider_;friend class Binding;
  Reader(Object::Reader s,std::shared_ptr<Provider::Reader>p):slot_(std::move(s)),provider_(std::move(p)){}
  public:
  Reader(const Reader&)=delete;Reader(Reader&&)=default;
  uint32_t cell_address()const{auto p=be32(slot_.read(0,0,4));require(p==provider_->cell_address(),Fault::ownership,"import/export cell depth");return p;}
  uint32_t module_base()const{cell_address();return provider_->module_base();}
  uint32_t header_address()const{cell_address();return provider_->header_address();}
  Field field(uint32_t h,uint32_t k)const{cell_address();return provider_->field(h,k);}
  std::vector<Space::Handle> registrations()const{auto h=slot_.registrations();auto p=provider_->registrations();h.insert(h.end(),p.begin(),p.end());return h;}
 };
 Reader acquire(ModuleLife&m,Ref ref){
  std::lock_guard lock(registry_.mutex_);auto i=registry_.entries_.find(key_);require(i!=registry_.entries_.end()&&i->second.identity==provider_ref_,Fault::ownership,"withdrawn/rebound import");auto state=i->second.state.lock();require(bool(state),Fault::stale,"retired import provider");std::lock_guard owner(state->mutex);require(state->open&&&m==&module_&&&state->module==&m,Fault::ownership,"import owner/admission");
  auto own=slot_->acquire(m,ref);auto retained=provider_.lock();require(bool(retained),Fault::stale,"lost import retainer");return Reader(std::move(own),std::move(retained));
 }
 bool close(){return slot_->close();}
 bool closed()const{return slot_->closed();}
};
}
