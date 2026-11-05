// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "native_image_plan.h"
#include <memory>
#include <optional>

// Standalone logical guest metadata; no loader/provider/import/entry registration.
namespace brutal::horizon::header_buffer {
using native_image::Digest;
using native_image::digest;
struct Source {
 uint64_t module{},generation{},offset{},length{},guest{};
 Digest source{},header{};
};
inline Digest identity(const Source& p){
 std::vector<uint8_t>b;for(auto v:{uint64_t(0x4845414445523031),p.module,p.generation,p.offset,p.length,p.guest})
  for(int i=7;i>=0;--i)b.push_back(uint8_t(v>>(8*i)));
 for (const auto& d : {p.source, p.header}) {
  b.insert(b.end(), d.begin(), d.end());
 }
 return digest(b);
}
inline void require(bool ok,Fault f,const char* message){if(!ok)throw Error(f,message);}
inline void validate(const Source&p,std::span<const uint8_t>source,const Digest&trusted,const ModuleLife&m){
 require(source.size()<=64*1024*1024&&p.length>=24&&p.length<=1024*1024&&(p.length%4)==0,Fault::bounds,"header size/budget");
 require(p.offset<=source.size()&&p.length<=source.size()-p.offset,Fault::bounds,"header source extent");
 require(p.guest&&p.guest<0x100000000ull&&(p.guest%4096)==0&&p.length<=0x100000000ull-p.guest,Fault::bounds,"header guest alignment/overflow");
 require(p.module==m.identity&&p.generation==m.instance,Fault::ownership,"header module instance");
 require(p.source!=Digest{}&&p.header!=Digest{}&&identity(p)==trusted,Fault::argument,"untrusted header descriptor");
 require(digest(source)==p.source,Fault::argument,"header source fingerprint");
 auto bytes=source.subspan(size_t(p.offset),size_t(p.length));
 require(digest(bytes)==p.header,Fault::argument,"header interval fingerprint");
 auto word=[&](size_t i){uint32_t w=0;for(unsigned n=0;n<4;++n)w=(w<<8)|bytes[i+n];return w;};
 require(word(0)==0x58455832&&word(8)==p.length,Fault::argument,"header magic/declared length");
 require(word(20)<=(p.length-24)/8,Fault::bounds,"header catalog bounds");
}
class Buffer {
 public:
 struct Ref {uint64_t buffer{},module{},generation{};uint32_t guest{},length{};bool operator==(const Ref&)const=default;};
 private:
 inline static std::mutex registry_mutex_;
 inline static std::set<uint64_t> registry_;
 inline static std::atomic<uint64_t> next_{1};
 struct State {
  Space& space;ModuleLife& module;const Source source;const uint64_t instance{next_.fetch_add(1)};
  std::mutex mutex;Space::Handle handle{};std::optional<Space::View> view;
  std::optional<Space::Lease> pin;std::unique_ptr<ModuleLife::Lease> owner_pin;
  size_t readers{};bool registered{},allocated{},published{},closing{},released{};
  State(Space&s,ModuleLife&m,Source p):space(s),module(m),source(p){}
  Ref ref()const{return {instance,source.module,source.generation,handle.address,handle.size};}
  void match(ModuleLife&m,Ref r)const{
   require(&m==&module&&r==ref(),Fault::ownership,"foreign/stale header reference");
  }
  // State -> Space -> Memory. Registry is never held while acquiring State.
  bool discard_locked(){
   require(readers==0,Fault::ownership,"header readers still active");
   if(allocated){
    if(pin){if(!space.release_exclusive(*pin))return false;}
    else space.release_survivors(std::span(&handle,1));
    allocated=false;
   }
   pin.reset();view.reset();
   if(registered){std::lock_guard lock(registry_mutex_);registry_.erase(source.generation);registered=false;}
   owner_pin.reset();published=false;released=true;return true;
  }
  void begin_close(){std::lock_guard lock(mutex);if(released||closing)return;closing=true;if(!readers)discard_locked();}
  ~State(){try{if(readers)std::terminate();if(!released&&!discard_locked())std::terminate();}catch(...){std::terminate();}}
 };
 std::shared_ptr<State> state_;
 public:
 // Source is copied synchronously; caller must not mutate its input span during
 // validation/construction. Trusted digest provenance is an external obligation.
 Buffer(Space&s,ModuleLife&m,Source p,std::span<const uint8_t>source,Digest trusted,int fail=-1){
  validate(p,source,trusted,m);auto st=std::make_shared<State>(s,m,p);
  require(st->instance!=0,Fault::exhausted,"header instance exhausted");
  st->owner_pin=std::make_unique<ModuleLife::Lease>(m);
  {std::lock_guard lock(registry_mutex_);require(registry_.insert(p.generation).second,Fault::ownership,"module already owns a header");st->registered=true;}
  int step=0;auto point=[&]{if(step++==fail)throw Error(Fault::exhausted,"injected header initialization failure");};
  point();st->handle=s.reserve(uint32_t(p.length),uint32_t(p.guest));st->allocated=true;
  point();s.commit(st->handle);
  point();auto bytes=source.subspan(size_t(p.offset),size_t(p.length));s.write(st->handle.address,std::as_bytes(bytes));
  point();s.seal_read_only(st->handle);
  point();st->pin.emplace(s,st->handle);st->view.emplace(s.view(st->handle.address,st->handle.size,Access::read));
  auto copied=s.read(*st->view);require(digest(std::span(reinterpret_cast<const uint8_t*>(copied.data()),copied.size()))==p.header,Fault::argument,"copied header fingerprint");
  point();st->published=true;state_=std::move(st);
 }
 Buffer(const Buffer&)=delete;Buffer&operator=(const Buffer&)=delete;
 ~Buffer(){if(state_)state_->begin_close();}
 Ref reference()const{std::lock_guard lock(state_->mutex);require(state_->published&&!state_->closing,Fault::stale,"header publication closed");return state_->ref();}
 // Trusted native management capability, never a guest/host pointer. Mutations
 // remain subject to Space leases and Memory's irreversible read-only seal.
 Space::Handle registration(ModuleLife&m,Ref r)const{
  std::lock_guard lock(state_->mutex);state_->match(m,r);
  require(state_->published&&!state_->closing,Fault::stale,"header publication closed");return state_->handle;
 }
 class Reader {
  std::shared_ptr<State> state_;friend class Buffer;
  explicit Reader(std::shared_ptr<State> s):state_(std::move(s)){}
  public:
  Reader(const Reader&)=delete;Reader&operator=(const Reader&)=delete;
  Reader(Reader&& other)noexcept:state_(std::move(other.state_)){}
  ~Reader(){if(state_){try{std::lock_guard lock(state_->mutex);if(!state_->readers)std::terminate();--state_->readers;if(!state_->readers&&state_->closing)state_->discard_locked();}catch(...){std::terminate();}}}
  std::vector<std::byte> read(uint64_t offset,uint64_t length)const{
   require(bool(state_),Fault::stale,"moved header reader");std::lock_guard lock(state_->mutex);
   require(state_->published&&!state_->released&&state_->readers,Fault::stale,"closed header reader");
   require(length&&offset<=state_->source.length&&length<=state_->source.length-offset,Fault::bounds,"header read bounds");
   auto all=state_->space.read(*state_->view);return {all.begin()+size_t(offset),all.begin()+size_t(offset+length)};
  }
 };
 Reader acquire(ModuleLife&m,Ref r){
  // Module close prevents new readers; an admitted Reader retains the owning
  // State and its allocation/module pins through graceful close/teardown.
  ModuleLife::Lease admission(m);std::lock_guard lock(state_->mutex);state_->match(m,r);
  require(state_->published&&!state_->closing,Fault::stale,"header admission closed");
  state_->space.validate_owned_window(state_->handle,r.guest,r.length,Access::read);
  ++state_->readers;return Reader(state_);
 }
 // A raw Space pin is trusted management, not a Reader: retain this wrapper and
 // retry close after that external pin drains. Wrapper destruction while such
 // unmanaged pins remain is a native owner-lifetime precondition violation.
 bool close(){std::lock_guard lock(state_->mutex);require(!state_->released,Fault::stale,"header already closed");require(!state_->closing||!state_->readers,Fault::ownership,"header close still has readers");state_->closing=true;if(state_->readers)return false;return state_->discard_locked();}
 bool closed()const{std::lock_guard lock(state_->mutex);return state_->released;}
 static size_t registrations(){std::lock_guard lock(registry_mutex_);return registry_.size();}
};
}
