// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
namespace brutal::horizon::platform {
// Nanosecond timestamps supplied by one serialized monotonic native clock.
// This schedules fixed simulation steps; it does not assert a measured FPS.
// Excess catchup steps are explicitly dropped, preserving only the substep phase.
class FrameTiming {
 public:
  struct Tick {uint32_t updates{};uint64_t step_ns{},remainder_ns{},frame_ns{},dropped_updates{};bool first{};};
  struct Stats {
    uint64_t frames{},updates{},missed_updates{},missed_frames{};
    uint64_t frame_ns{},min_frame_ns{},max_frame_ns{},jitter_ns{},max_jitter_ns{};
    uint64_t update_ns{},render_ns{},total_update_ns{},total_render_ns{},measured_updates{},measured_renders{};
  };
 private:
  const uint64_t step_;const uint32_t catchup_;uint64_t last_{},event_end_{},remainder_{};
  bool started_{},updated_{},rendered_{};Stats stats_{};
  static void require(bool ok,Fault f,const char*why){if(!ok)throw Error(f,why);}
  static uint64_t add(uint64_t a,uint64_t b){require(b<=UINT64_MAX-a,Fault::exhausted,"timing counter overflow");return a+b;}
  void interval(uint64_t begin,uint64_t end){
    require(started_,Fault::stale,"timing frame not begun");
    require(begin>=last_&&begin>=event_end_&&end>=begin,Fault::stale,"timing measurement timestamp regression");
  }
 public:
  explicit FrameTiming(uint64_t step_ns=16666667,uint32_t max_catchup=4):step_(step_ns),catchup_(max_catchup){
    require(step_ns&&step_ns<=1000000000&&max_catchup&&max_catchup<=32,Fault::argument,"fixed-step timing configuration");
  }
  Tick begin_frame(uint64_t now){
    require(!started_||now>=event_end_,Fault::stale,"frame timestamp regression");
    auto next=stats_;next.frames=add(next.frames,1);next.update_ns=next.render_ns=0;
    if(!started_){stats_=next;started_=true;last_=event_end_=now;return {0,step_,0,0,0,true};}
    auto delta=now-last_;auto phase=remainder_+delta%step_;
    auto due=add(delta/step_,phase/step_);auto updates=uint32_t(std::min<uint64_t>(due,catchup_));auto dropped=due-updates;
    next.updates=add(next.updates,updates);next.missed_updates=add(next.missed_updates,dropped);
    // A frame gap of (k*step, (k+1)*step] misses k presentation deadlines.
    next.missed_frames=add(next.missed_frames,delta?(delta-1)/step_:0);
    next.jitter_ns=next.frame_ns>delta?next.frame_ns-delta:delta-next.frame_ns;
    if(stats_.frames<=1)next.jitter_ns=0;
    next.max_jitter_ns=std::max(next.max_jitter_ns,next.jitter_ns);next.frame_ns=delta;
    if(delta){if(!next.min_frame_ns||delta<next.min_frame_ns)next.min_frame_ns=delta;next.max_frame_ns=std::max(next.max_frame_ns,delta);}
    stats_=next;last_=event_end_=now;remainder_=phase%step_;updated_=rendered_=false;
    return {updates,step_,remainder_,delta,dropped,false};
  }
  // One measured batch per frame, update before render; skipped measurements
  // remain zero and do not contribute a sample. Updates counts scheduled steps.
  void record_update(uint64_t begin,uint64_t end){
    interval(begin,end);require(!updated_&&!rendered_,Fault::ownership,"duplicate or late update measurement");
    auto next=stats_;next.update_ns=end-begin;next.total_update_ns=add(next.total_update_ns,next.update_ns);next.measured_updates=add(next.measured_updates,1);
    stats_=next;event_end_=end;updated_=true;
  }
  void record_render(uint64_t begin,uint64_t end){
    interval(begin,end);require(!rendered_,Fault::ownership,"duplicate render measurement");
    auto next=stats_;next.render_ns=end-begin;next.total_render_ns=add(next.total_render_ns,next.render_ns);next.measured_renders=add(next.measured_renders,1);
    stats_=next;event_end_=end;rendered_=true;
  }
  Stats stats()const{return stats_;}uint64_t step_ns()const{return step_;}
};
}
