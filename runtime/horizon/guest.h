// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include "native.h"
#include <rex/ppc/context.h>
#include <memory>
#include <cstring>
#include <type_traits>
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
  diagnostic::Trace* trace_{};
  std::unique_ptr<ModuleLife::Lease> module_lease_;std::unique_ptr<Slots::Pin> slot_lease_;
  std::vector<Space::Handle> allocations_;
  bool closed_{},active_{},native_pending_{};
  std::mutex mutex_;
 public:
  inline static std::atomic<uint64_t> next_identity_{1};
  const uint64_t identity=next_identity_.fetch_add(1); // Synthetic identity; not an Xbox handle.
  PPCContext context{};
  Space::Handle stack{},pcr{},tls{};
  static constexpr uint32_t static_tls_size=32;
  GuestThread(Space& space,ModuleLife& module,Slots& slots,Slots::Token slot,uint32_t seed=7,
              std::span<const std::byte> initial={},diagnostic::Trace* trace=diagnostic::current_trace()):space_(space),module_(module),slots_(slots),slot_(slot),trace_(trace),module_lease_(std::make_unique<ModuleLife::Lease>(module)),slot_lease_(std::make_unique<Slots::Pin>(slots,slot)){
    static_assert(std::is_trivially_copyable_v<PPCContext>);
    // Aggregate initialization does not guarantee all bytes of every union.
    // Zero the complete representation, then restore the verified SDK MSR default.
    std::memset(static_cast<void*>(&context),0,sizeof(context));context.msr=PPCContext{}.msr;
    allocations_.reserve(3);slots_.validate(slot_);if(slot_.index!=0)throw Error(Fault::unsupported,"selected generated TLS fixture requires slot zero");if(initial.size()>static_tls_size)throw Error(Fault::argument,"TLS template extent");
    try{
      stack=space_.allocate(16*1024);allocations_.push_back(stack);
      pcr=space_.allocate(0x2d8);allocations_.push_back(pcr);
      tls=space_.allocate(static_tls_size+64*4);allocations_.push_back(tls);
      if(!initial.empty())space_.write(tls.address,initial);
      space_.store<uint32_t>(pcr.address,tls.address);space_.store<uint64_t>(pcr.address+0x30,pcr.address);
      space_.store<uint32_t>(tls.address+static_tls_size+slot_.index*4,seed);
      diagnostic::emit_for(trace_,diagnostic::Kind::thread_create,module_.instance,identity);
      context.r1.u64=(stack.address+stack.size-256)&~15u;context.r2.u64=0x82020000;context.r13.u64=pcr.address;
    }catch(...){for(auto h:allocations_)space_.release(h);throw;}
  }
  GuestThread(const GuestThread&)=delete;
  ~GuestThread(){if(!closed_)try{cleanup();}catch(...){std::terminate();}}
  diagnostic::Trace* trace()const{return trace_;}
  uint64_t module_instance()const{return module_.instance;}
  uint64_t module_identity()const{return module_.identity;}
  bool belongs_to(const ModuleLife& owner)const{return &module_==&owner;}
  Space& space(){return space_;}
  void set_tls(uint32_t value){slots_.validate(slot_);space_.store<uint32_t>(tls.address+static_tls_size+slot_.index*4,value);}
  uint32_t get_tls(){slots_.validate(slot_);return space_.load<uint32_t>(tls.address+static_tls_size+slot_.index*4);}
 private:
  friend class GuestTask;
  void native_pending(bool pending){std::lock_guard lock(mutex_);if(closed_||active_||native_pending_==pending)throw Error(Fault::ownership,"native lifecycle transition during call");native_pending_=pending;}
 public:
  struct StackContract {
    uint64_t thread{},module_instance{};
    bool relative{};int64_t begin{},end{}; // read/write [current r1+begin,current r1+end)
  };
  StackContract whole_stack_contract()const{return {identity,module_.instance,false,0,0};}
  StackContract relative_stack_contract(int64_t begin,int64_t end)const{return {identity,module_.instance,true,begin,end};}
  class Execution {
    GuestThread& thread_;ModuleLife::Lease module_;
    std::vector<Space::Lease> pins_;
    const StackContract contract_;
    std::pair<uint32_t,uint32_t> window_{};
    static bool same(Space::Handle a,Space::Handle b){return a.owner==b.owner&&a.id==b.id&&a.address==b.address&&a.size==b.size;}
    static uint64_t offset(uint64_t base,int64_t delta){
      if(delta>=0){auto n=uint64_t(delta);if(n>0x100000000ull-base)throw Error(Fault::bounds,"stack positive offset overflow");return base+n;}
      auto n=uint64_t(-(delta+1))+1;if(n>base)throw Error(Fault::bounds,"stack negative offset overflow");return base-n;
    }
    void validate_locked(){
      auto&t=thread_;t.slots_.validate(t.slot_);
      if(t.closed_||contract_.thread!=t.identity||contract_.module_instance!=t.module_.instance||t.allocations_.size()!=3)
        throw Error(Fault::ownership,"stack thread/module/lifetime identity");
      if(!same(t.stack,t.allocations_[0])||!same(t.pcr,t.allocations_[1])||!same(t.tls,t.allocations_[2]))
        throw Error(Fault::ownership,"altered context allocation token");
      const auto h=t.allocations_[0];const uint64_t high=uint64_t(h.address)+h.size,sp=t.context.r1.u64;
      if(!h.owner||!h.id||h.size<192||high>0x100000000ull||sp>UINT32_MAX||sp<uint64_t(h.address)+64||sp>high-128||(sp&15)||
         t.context.r13.u64!=t.pcr.address||t.context.r2.u64!=0x82020000||t.context.msr!=PPCContext{}.msr||
         t.space_.load<uint64_t>(t.pcr.address+0x30)!=t.pcr.address||t.space_.load<uint32_t>(t.pcr.address)!=t.tls.address)
        throw Error(Fault::argument,"guest startup/context admission");
      uint64_t lo=h.address,hi=high;
      if(contract_.relative){if(contract_.begin>=contract_.end)throw Error(Fault::argument,"empty/reversed stack contract");lo=offset(sp,contract_.begin);hi=offset(sp,contract_.end);}
      else if(contract_.begin||contract_.end)throw Error(Fault::argument,"inconsistent whole-stack contract");
      if(lo<h.address||hi>high||hi<=lo||hi-lo>UINT32_MAX)throw Error(Fault::bounds,"required stack window is not owned");
      t.space_.validate_owned_window(h,uint32_t(lo),uint32_t(hi-lo),Access::write,true);
      window_={uint32_t(lo),uint32_t(hi-lo)};
    }
   public:
    explicit Execution(GuestThread& t,const StackContract* contract=nullptr):thread_(t),module_(t.module_),contract_(contract?*contract:t.whole_stack_contract()){
      std::lock_guard lock(t.mutex_);t.slots_.validate(t.slot_);
      if(t.active_)throw Error(Fault::argument,"guest startup/context already active");
      if(t.closed_||t.allocations_.size()!=3)throw Error(Fault::ownership,"closed/incomplete context allocations");
      for(auto h:t.allocations_){pins_.emplace_back(t.space_,h);}
      validate_locked();
      t.active_=true;
    }
    void validate_entry(){ModuleLife::Lease entry(thread_.module_);std::lock_guard lock(thread_.mutex_);validate_locked();}
    std::pair<uint32_t,uint32_t> required_window()const{return window_;}
    Execution(const Execution&)=delete;
    ~Execution(){std::lock_guard lock(thread_.mutex_);pins_.clear();thread_.active_=false;}
  };
  void cleanup(){
    diagnostic::PhaseBinding phase{8};
    diagnostic::Binding binding(trace_,module_.instance,identity);
    std::lock_guard lock(mutex_);if(closed_)return;
    if(active_||native_pending_)throw Error(Fault::ownership,"guest cleanup before native join or active call return");
    space_.release_survivors(allocations_);
    allocations_.clear();slot_lease_.reset();module_lease_.reset();closed_=true;auto counts=Space::totals();diagnostic::emit(diagnostic::Kind::cleanup,uint32_t(counts.first),uint32_t(counts.second));
  }
};
// Native ownership is kept until a successful join; a timeout returns failure
// and preserves resources. Destruction of a live native worker fail-stops.
class GuestTask {
  GuestThread& guest_;NativeThread native_;bool pending_{};
 public:
  explicit GuestTask(GuestThread& guest):guest_(guest){}
  bool start(std::function<void()> callback,NativeThread::Injection injection=NativeThread::Injection::none){
    if(!callback)throw Error(Fault::argument,"empty guest callback");
    if(pending_||native_.joined())throw Error(Fault::argument,"guest task start lifecycle");
    guest_.native_pending(true);
    try{if(!native_.start([this,fn=std::move(callback)]{diagnostic::Binding binding(guest_.trace(),guest_.module_instance(),guest_.identity);diagnostic::emit(diagnostic::Kind::thread_start);try{fn();}catch(...){diagnostic::emit(diagnostic::Kind::callback_failure);throw;}diagnostic::emit(diagnostic::Kind::thread_exit);},64*1024,injection)){guest_.native_pending(false);return false;}pending_=true;return true;}
    catch(...){guest_.native_pending(false);throw;}
  }
  bool join(uint64_t timeout=2000){diagnostic::Binding binding(guest_.trace(),guest_.module_instance(),guest_.identity);if(!native_.join(timeout)){diagnostic::emit(diagnostic::Kind::join,0);return false;}guest_.native_pending(false);pending_=false;diagnostic::emit(diagnostic::Kind::join,1);return true;}
  bool callback_error()const{return native_.callback_error();}
  ~GuestTask(){if(pending_)std::terminate();}
};
}
