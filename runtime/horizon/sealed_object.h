// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "header_buffer.h"

namespace brutal::horizon::native_module {
using header_buffer::require;using native_image::Digest;using native_image::digest;
struct Bytes {uint32_t address;std::vector<std::byte> data;};
inline std::vector<std::byte> word(uint32_t v){return {std::byte(v>>24),std::byte(v>>16),std::byte(v>>8),std::byte(v)};}
inline uint32_t be32(std::span<const std::byte>b){require(b.size()==4,Fault::bounds,"word extent");uint32_t v=0;for(auto x:b)v=(v<<8)|std::to_integer<uint8_t>(x);return v;}
// Logical read-only object with an acyclic chain of typed native retainers.
// Native Space/ModuleLife must outlive it. Raw Space pins require wrapper retention.
class Object {
 public:
 struct Ref {uint64_t object{},module{},generation{};bool operator==(const Ref&)const=default;};
 private:
 inline static std::atomic<uint64_t> next_{1};
 inline static std::mutex registry_mutex_;
 inline static std::set<std::pair<uint64_t,uint64_t>> registry_;
 struct State {
  Space& space;ModuleLife& module;uint64_t role;const uint64_t id{next_.fetch_add(1)};
  std::mutex mutex;std::unique_ptr<ModuleLife::Lease> owner;
  std::vector<Space::Handle> handles;std::vector<Space::View> views;std::vector<Space::Lease> pins;std::vector<Space::Lease*> retire;
  std::vector<std::shared_ptr<void>> dependencies;
  size_t readers{};bool published{},closing{},released{},registered{};
  State(Space&s,ModuleLife&m,uint64_t r):space(s),module(m),role(r){}
  Ref ref()const{return {id,module.identity,module.instance};}
  bool discard(){
   require(readers==0,Fault::ownership,"object readers active");
   if(pins.size()==handles.size()){
    if(!space.release_exclusive_all(retire))return false;
   }else{pins.clear();space.release_survivors(handles);} // unpublished rollback only
   retire.clear();pins.clear();views.clear();handles.clear();published=false;released=true;
   if(registered){std::lock_guard lock(registry_mutex_);registry_.erase({module.instance,role});registered=false;}
   dependencies.clear();owner.reset();return true;
  }
  void closing_request(){std::lock_guard lock(mutex);if(released)return;closing=true;if(!readers)discard();}
  ~State(){try{if(readers||(!released&&!discard()))std::terminate();}catch(...){std::terminate();}}
 };
 std::shared_ptr<State> state_;
 public:
 Object(Space&s,ModuleLife&m,std::span<const Bytes> blocks,std::vector<std::shared_ptr<void>> dependencies={},uint64_t role=0,int fail=-1){
  require(!blocks.empty()&&blocks.size()<=16,Fault::bounds,"object region budget");uint64_t total=0;
  for(size_t i=0;i<blocks.size();++i){auto&b=blocks[i];require(b.address&&!b.data.empty()&&b.data.size()<=1024*1024&&total<=1024*1024-b.data.size()&&uint64_t(b.address)+b.data.size()<=0x100000000ull,Fault::bounds,"object extent/budget");total+=b.data.size();
   for(size_t j=0;j<i;++j)require(b.address>=uint64_t(blocks[j].address)+blocks[j].data.size()||blocks[j].address>=uint64_t(b.address)+b.data.size(),Fault::ownership,"object overlap");}
  auto st=std::make_shared<State>(s,m,role);require(st->id!=0,Fault::exhausted,"object identity exhausted");st->owner=std::make_unique<ModuleLife::Lease>(m);st->dependencies=std::move(dependencies);
  if(role){std::lock_guard lock(registry_mutex_);require(registry_.insert({m.instance,role}).second,Fault::ownership,"duplicate module object role");st->registered=true;}
  st->handles.reserve(blocks.size());st->pins.reserve(blocks.size());st->views.reserve(blocks.size());st->retire.reserve(blocks.size());
  int step=0;auto point=[&]{if(step++==fail)throw Error(Fault::exhausted,"injected object failure");};
  for(auto&b:blocks){point();auto h=s.reserve_slice(uint32_t(b.data.size()),b.address);st->handles.push_back(h);point();s.commit(h);s.write(h.address,b.data);point();s.seal_read_only(h);st->pins.emplace_back(s,h);st->retire.push_back(&st->pins.back());st->views.push_back(s.view(h.address,h.size,Access::read));}
  point();st->published=true;state_=std::move(st);
 }
 Object(const Object&)=delete;Object&operator=(const Object&)=delete;
 ~Object(){if(state_)state_->closing_request();}
 Ref reference()const{std::lock_guard lock(state_->mutex);require(state_->published&&!state_->closing,Fault::stale,"object closed");return state_->ref();}
 class Reader {
  std::shared_ptr<State> state_;friend class Object;explicit Reader(std::shared_ptr<State>s):state_(std::move(s)){}
  public:
  Reader(const Reader&)=delete;Reader&operator=(const Reader&)=delete;Reader(Reader&&)=default;
  ~Reader(){if(state_){try{std::lock_guard lock(state_->mutex);if(!state_->readers)std::terminate();--state_->readers;if(!state_->readers&&state_->closing)state_->discard();}catch(...){std::terminate();}}}
  std::vector<std::byte> read(size_t index,uint32_t offset,uint32_t size)const{
   require(bool(state_),Fault::stale,"moved object reader");std::lock_guard lock(state_->mutex);
   require(state_->published&&!state_->released&&state_->readers,Fault::stale,"object retired");require(index<state_->handles.size(),Fault::bounds,"object region index");auto h=state_->handles[index];require(size&&offset<=h.size&&size<=h.size-offset,Fault::bounds,"object read extent");
   auto all=state_->space.read(state_->views[index]);return {all.begin()+offset,all.begin()+offset+size};
  }
  std::vector<Space::Handle> registrations()const{require(bool(state_),Fault::stale,"moved object reader");std::lock_guard lock(state_->mutex);require(state_->published&&!state_->released,Fault::stale,"object retired");return state_->handles;}
  bool belongs_to(const ModuleLife&m,const Space&s)const{return state_&&&state_->module==&m&&&state_->space==&s;}
 };
 Reader acquire(ModuleLife&m,Ref ref){ModuleLife::Lease admission(m);std::lock_guard lock(state_->mutex);require(&m==&state_->module&&ref==state_->ref(),Fault::ownership,"foreign/stale object");require(state_->published&&!state_->closing,Fault::stale,"object admission closed");for(auto h:state_->handles)state_->space.validate_owned_window(h,h.address,h.size,Access::read);++state_->readers;return Reader(state_);}
 bool close(){std::lock_guard lock(state_->mutex);require(!state_->released,Fault::stale,"object already closed");require(!state_->closing||!state_->readers,Fault::ownership,"close pending readers");state_->closing=true;if(state_->readers)return false;return state_->discard();}
 bool closed()const{std::lock_guard lock(state_->mutex);return state_->released;}
 static size_t registrations(){std::lock_guard lock(registry_mutex_);return registry_.size();}
};
class Metadata {
 uint32_t base_;header_buffer::Buffer::Ref header_ref_;Space::Handle header_handle_;
 std::weak_ptr<header_buffer::Buffer::Reader> header_;
 std::unique_ptr<Object> object_;
 public:
 using Ref=Object::Ref;
 Metadata(Space&s,ModuleLife&m,header_buffer::Buffer&header,header_buffer::Buffer::Ref ref,uint64_t base,int fail=-1):header_ref_(ref){
  require(base&&(base%4)==0&&base<=0x100000000ull-0x5c,Fault::bounds,"module base/word overflow");base_=uint32_t(base);
  auto retained=std::make_shared<header_buffer::Buffer::Reader>(header.acquire(m,ref));header_=retained;
  // Header and metadata must share the same checked address-space authority.
  auto handle=header.registration(m,ref);s.validate_owned_window(handle,ref.guest,ref.length,Access::read);header_handle_=handle;
  std::array<Bytes,1> field{{{base_+0x58,word(ref.guest)}}};
  object_=std::make_unique<Object>(s,m,field,std::vector<std::shared_ptr<void>>{retained},0x4d455441ull,fail);
 }
 Ref reference()const{return object_->reference();}
 class Reader {
  Object::Reader object_;std::shared_ptr<header_buffer::Buffer::Reader> header_;header_buffer::Buffer::Ref ref_;uint32_t base_;Space::Handle header_handle_;
  friend class Metadata;
  Reader(Object::Reader o,std::shared_ptr<header_buffer::Buffer::Reader>h,header_buffer::Buffer::Ref r,uint32_t b,Space::Handle handle):object_(std::move(o)),header_(std::move(h)),ref_(r),base_(b),header_handle_(handle){}
  public:
  Reader(const Reader&)=delete;Reader(Reader&&)=default;
  uint32_t base()const{require(be32(object_.read(0,0,4))==ref_.guest,Fault::ownership,"header pointer association");return base_;}
  uint32_t header_address()const{base();return ref_.guest;}
  std::vector<std::byte> header_bytes(uint32_t offset,uint32_t size)const{base();return header_->read(offset,size);}
  uint32_t header_size()const{return ref_.length;}
  std::vector<Space::Handle> registrations()const{auto h=object_.registrations();h.push_back(header_handle_);return h;}
  bool belongs_to(const ModuleLife&m,const Space&s)const{return object_.belongs_to(m,s);}
 };
 Reader acquire(ModuleLife&m,Ref ref){auto own=object_->acquire(m,ref);auto retained=header_.lock();require(bool(retained),Fault::stale,"header retainer lost");return Reader(std::move(own),std::move(retained),header_ref_,base_,header_handle_);}
 bool close(){return object_->close();}
 bool closed()const{return object_->closed();}
};
}
