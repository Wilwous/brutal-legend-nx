// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
namespace brutal::diagnostic {
enum class Kind : uint32_t {instruction,function_enter,function_exit,refusal,branch_rejected,abi_rejected,import_admitted,import_enter,import_return,import_commit,import_rejected,stop_request,stop_observed,thread_create,thread_start,thread_exit,callback_failure,join,cleanup,module_close,module_unload,resources,exceptional_rejected,instruction_rejected};
struct Metadata {std::array<uint8_t,32> image{},codegen{},sdk{};};
struct Event {Kind kind;uint32_t pc,detail,auxiliary;uint64_t module,thread;};
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
struct Context {Trace*trace{};uint64_t module{},thread{};uint32_t pc{};};
inline thread_local Context context;
inline Trace* current_trace(){return context.trace;}
class Binding {
 Context previous_;
 public:
 Binding(Trace*trace,uint64_t module,uint64_t thread):previous_(context){context={trace,module,thread,0};}
 ~Binding(){context=previous_;}
 Binding(const Binding&)=delete;
};
inline void emit(Kind kind,uint32_t detail=0,uint32_t auxiliary=0)noexcept{if(auto*t=context.trace)t->write({kind,context.pc,detail,auxiliary,context.module,context.thread});}
inline void emit_for(Trace*trace,Kind kind,uint64_t module,uint64_t thread,uint32_t detail=0,uint32_t auxiliary=0)noexcept{if(trace)trace->write({kind,0,detail,auxiliary,module,thread});}
inline void site(uint32_t pc)noexcept{if(context.trace){context.pc=pc;if(context.trace->instruction_sites)emit(Kind::instruction);}}
}
