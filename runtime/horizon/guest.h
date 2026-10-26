// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include "native.h"
#include <rex/ppc/context.h>
#include <memory>
namespace brutal::horizon {
class Slots {
  std::mutex mutex_;std::array<uint64_t,64> generations_{};std::array<bool,64> used_{};std::array<unsigned,64> pins_{};
  inline static std::atomic<uint64_t> next_owner_{1};const uint64_t owner_{next_owner_.fetch_add(1)};
 public:
  struct Token {uint32_t index;uint64_t generation;uint64_t owner;};
  Token allocate(){std::lock_guard lock(mutex_);for(uint32_t i=0;i<64;++i)if(!used_[i]){used_[i]=true;return {i,++generations_[i],owner_};}throw Error(Fault::exhausted,"synthetic process TLS slots exhausted");}
  void validate(Token t){std::lock_guard lock(mutex_);if(t.owner!=owner_||t.index>=64||!used_[t.index]||t.generation!=generations_[t.index])throw Error(Fault::slot,"stale or invalid TLS registration");}
  void release(Token t){std::lock_guard lock(mutex_);if(t.owner!=owner_||t.index>=64||!used_[t.index]||t.generation!=generations_[t.index])throw Error(Fault::slot,"stale TLS release");if(pins_[t.index])throw Error(Fault::ownership,"TLS registration is pinned by live guest threads");used_[t.index]=false;}
  class Pin {
    Slots& slots_;Token token_;
   public:
    Pin(Slots& slots,Token token):slots_(slots),token_(token){std::lock_guard lock(slots.mutex_);if(token.owner!=slots.owner_||token.index>=64||!slots.used_[token.index]||token.generation!=slots.generations_[token.index])throw Error(Fault::slot,"invalid TLS pin");++slots.pins_[token.index];}
    ~Pin(){std::lock_guard lock(slots_.mutex_);--slots_.pins_[token_.index];}
    Pin(const Pin&)=delete;
  };
};
// SDK-observed X_KPCR subset: size 0x2d8, TLS pointer +0, self pointer +0x30.
// Static TLS: 32 bytes followed by 64 four-byte dynamic slots. No KTHREAD/APC,
// host_stash, PRCB scheduling, fibers or arbitrary Xbox TLS exports are provided.
class GuestThread {
  Space& space_;ModuleLife& module_;Slots& slots_;Slots::Token slot_;
  std::unique_ptr<ModuleLife::Lease> module_lease_;std::unique_ptr<Slots::Pin> slot_lease_;
  std::vector<Space::Handle> allocations_;
  bool closed_{},active_{},native_pending_{};
  std::mutex mutex_;
 public:
  PPCContext context{};
  Space::Handle stack{},pcr{},tls{};
  static constexpr uint32_t static_tls_size=32;
  GuestThread(Space& space,ModuleLife& module,Slots& slots,Slots::Token slot,uint32_t seed=7,
              std::span<const std::byte> initial={}):space_(space),module_(module),slots_(slots),slot_(slot),module_lease_(std::make_unique<ModuleLife::Lease>(module)),slot_lease_(std::make_unique<Slots::Pin>(slots,slot)){
    allocations_.reserve(3);slots_.validate(slot_);if(slot_.index!=0)throw Error(Fault::unsupported,"selected generated TLS fixture requires slot zero");if(initial.size()>static_tls_size)throw Error(Fault::argument,"TLS template extent");
    try{
      stack=space_.allocate(16*1024);allocations_.push_back(stack);
      pcr=space_.allocate(0x2d8);allocations_.push_back(pcr);
      tls=space_.allocate(static_tls_size+64*4);allocations_.push_back(tls);
      if(!initial.empty())space_.write(tls.address,initial);
      space_.store<uint32_t>(pcr.address,tls.address);space_.store<uint64_t>(pcr.address+0x30,pcr.address);
      space_.store<uint32_t>(tls.address+static_tls_size+slot_.index*4,seed);
      context.r1.u64=(stack.address+stack.size-256)&~15u;context.r2.u64=0x82020000;context.r13.u64=pcr.address;
    }catch(...){for(auto h:allocations_)space_.release(h);throw;}
  }
  GuestThread(const GuestThread&)=delete;
  ~GuestThread(){if(!closed_)try{cleanup();}catch(...){std::terminate();}}
  uint64_t module_identity()const{return module_.identity;}
  bool belongs_to(const ModuleLife& owner)const{return &module_==&owner;}
  Space& space(){return space_;}
  void set_tls(uint32_t value){slots_.validate(slot_);space_.store<uint32_t>(tls.address+static_tls_size+slot_.index*4,value);}
  uint32_t get_tls(){slots_.validate(slot_);return space_.load<uint32_t>(tls.address+static_tls_size+slot_.index*4);}
 private:
  friend class GuestTask;
  void native_pending(bool pending){std::lock_guard lock(mutex_);if(closed_||active_||native_pending_==pending)throw Error(Fault::ownership,"native lifecycle transition during call");native_pending_=pending;}
 public:
  class Execution {
    GuestThread& thread_;ModuleLife::Lease module_;
    std::vector<Space::Lease> pins_;
   public:
    explicit Execution(GuestThread& t):thread_(t),module_(t.module_){
      std::lock_guard lock(t.mutex_);t.slots_.validate(t.slot_);
      if(t.closed_||t.active_||t.context.r13.u64!=t.pcr.address||t.context.r1.u64<t.stack.address+64||
         t.context.r1.u64>uint64_t(t.stack.address)+t.stack.size-128||(t.context.r1.u64&15))throw Error(Fault::argument,"guest startup/context admission");
      for(auto h:t.allocations_){pins_.emplace_back(t.space_,h);}
      t.active_=true;
    }
    Execution(const Execution&)=delete;
    ~Execution(){std::lock_guard lock(thread_.mutex_);pins_.clear();thread_.active_=false;}
  };
  void cleanup(){
    std::lock_guard lock(mutex_);if(closed_)return;
    if(active_||native_pending_)throw Error(Fault::ownership,"guest cleanup before native join or active call return");
    space_.release_all(allocations_);
    allocations_.clear();slot_lease_.reset();module_lease_.reset();closed_=true;
  }
};
// Native ownership is kept until a successful join; a timeout returns failure
// and preserves resources. Destruction of a live native worker fail-stops.
class GuestTask {
  GuestThread& guest_;NativeThread native_;bool pending_{};
 public:
  explicit GuestTask(GuestThread& guest):guest_(guest){}
  bool start(std::function<void()> callback,NativeThread::Injection injection=NativeThread::Injection::none){
    if(pending_||native_.joined())throw Error(Fault::argument,"guest task start lifecycle");
    guest_.native_pending(true);
    try{if(!native_.start(std::move(callback),64*1024,injection)){guest_.native_pending(false);return false;}pending_=true;return true;}
    catch(...){guest_.native_pending(false);throw;}
  }
  bool join(uint64_t timeout=2000){if(!native_.join(timeout))return false;guest_.native_pending(false);pending_=false;return true;}
  bool callback_error()const{return native_.callback_error();}
  ~GuestTask(){if(pending_)std::terminate();}
};
}
