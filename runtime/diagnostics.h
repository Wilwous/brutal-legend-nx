// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
#include <map>
#include <span>
#include <stdexcept>
namespace brutal::diagnostic {
enum class Kind : uint32_t {instruction,function_enter,function_exit,refusal,branch_rejected,abi_rejected,import_admitted,import_enter,import_return,import_commit,import_rejected,stop_request,stop_observed,thread_create,thread_start,thread_exit,callback_failure,join,cleanup,module_close,module_unload,resources,exceptional_rejected,instruction_rejected,lifecycle};
struct Metadata {std::array<uint8_t,32> image{},codegen{},sdk{};};
struct Event {Kind kind;uint32_t pc,detail,auxiliary;uint64_t module,thread;uint32_t phase{};uint64_t allocation_owner{},allocation_id{};bool operator==(const Event&)const=default;};
class Trace {
 mutable std::mutex mutex_;std::array<Event,2048> events_{};size_t used_{},dropped_{};std::atomic<bool> broken_{};
 public:
 static constexpr size_t capacity=2048;
 const Metadata metadata;const bool instruction_sites;
 explicit Trace(Metadata m={},bool sites=false):metadata(m),instruction_sites(sites){}
 void write(Event event)noexcept{try{std::lock_guard lock(mutex_);if(used_<capacity)events_[used_++]=event;else ++dropped_;}catch(...){broken_=true;}}
 std::vector<Event> snapshot()const{std::lock_guard lock(mutex_);return {events_.begin(),events_.begin()+used_};}
 size_t dropped()const{std::lock_guard lock(mutex_);return dropped_;}
 bool broken()const{return broken_.load();}
 void clear(){std::lock_guard lock(mutex_);used_=0;dropped_=0;broken_=false;}
};
struct Context {Trace*trace{};uint64_t module{},thread{};uint32_t pc{},phase{};uint64_t allocation_owner{},allocation_id{};};
inline thread_local Context context;
inline Trace* current_trace(){return context.trace;}
class Binding {
 Context previous_;
 public:
 Binding(Trace*trace,uint64_t module,uint64_t thread):previous_(context){context={trace,module,thread,0,previous_.phase,0,0};}
 ~Binding(){context=previous_;}
 Binding(const Binding&)=delete;
};
class PhaseBinding {
 uint32_t previous_;
 public:explicit PhaseBinding(uint32_t phase):previous_(context.phase){context.phase=phase;}~PhaseBinding(){context.phase=previous_;}PhaseBinding(const PhaseBinding&)=delete;
};
class AllocationBinding {
 uint64_t owner_,id_;
 public:AllocationBinding(uint64_t owner,uint64_t id):owner_(context.allocation_owner),id_(context.allocation_id){context.allocation_owner=owner;context.allocation_id=id;}~AllocationBinding(){context.allocation_owner=owner_;context.allocation_id=id_;}AllocationBinding(const AllocationBinding&)=delete;
};
// Normalize process-local numeric owners for controlled serial trace comparison.
// Preserve order; this does not impose an order on concurrently interleaved events.
inline std::vector<Event> canonical(std::span<const Event> events){
 if(events.size()>Trace::capacity)throw std::invalid_argument("trace normalization budget");
 std::map<uint64_t,uint64_t> modules,threads,owners;std::map<std::pair<uint64_t,uint64_t>,uint64_t> allocations;std::vector<Event> out(events.begin(),events.end());
 auto number=[](auto& map,auto key){if(key==0)return uint64_t(0);auto [it,added]=map.emplace(key,map.size()+1);(void)added;return it->second;};
 for(auto&e:out){auto key=std::pair(e.allocation_owner,e.allocation_id);e.module=number(modules,e.module);e.thread=number(threads,e.thread);e.allocation_owner=number(owners,e.allocation_owner);if(e.allocation_id){auto [it,added]=allocations.emplace(key,allocations.size()+1);(void)added;e.allocation_id=it->second;}}
 return out;
}
inline void emit(Kind kind,uint32_t detail=0,uint32_t auxiliary=0)noexcept{if(auto*t=context.trace)t->write({kind,context.pc,detail,auxiliary,context.module,context.thread,context.phase,context.allocation_owner,context.allocation_id});}
inline void emit_for(Trace*trace,Kind kind,uint64_t module,uint64_t thread,uint32_t detail=0,uint32_t auxiliary=0)noexcept{if(trace)trace->write({kind,0,detail,auxiliary,module,thread,context.phase,context.allocation_owner,context.allocation_id});}
inline void site(uint32_t pc)noexcept{if(context.trace){context.pc=pc;if(context.trace->instruction_sites)emit(Kind::instruction);}}
}
