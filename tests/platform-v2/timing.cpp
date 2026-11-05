// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/frame_timing.h"
#include <iostream>
#include <source_location>
using namespace brutal::horizon;using platform::FrameTiming;
namespace {unsigned checks{},refusals{};void check(bool v,std::source_location at=std::source_location::current()){if(!v)throw std::runtime_error("timing assertion line "+std::to_string(at.line()));++checks;}
template<class F>void no(F f,Fault expected){try{f();}catch(const Error&e){check(e.fault==expected);++refusals;return;}throw std::runtime_error("missing timing refusal");}}
int main(){try{
 no([]{FrameTiming timing(0);},Fault::argument);no([]{FrameTiming timing(1000000001);},Fault::argument);no([]{FrameTiming timing(10,0);},Fault::argument);no([]{FrameTiming timing(10,33);},Fault::argument);
 FrameTiming timing(10,3);no([&]{timing.record_update(0,1);},Fault::stale);no([&]{timing.record_render(0,1);},Fault::stale);
 auto tick=timing.begin_frame(100);check(tick.first&&tick.updates==0&&tick.frame_ns==0&&tick.remainder_ns==0&&tick.step_ns==10);
 tick=timing.begin_frame(106);check(!tick.first&&tick.updates==0&&tick.frame_ns==6&&tick.remainder_ns==6);
 tick=timing.begin_frame(121);check(tick.updates==2&&tick.remainder_ns==1&&tick.frame_ns==15&&tick.dropped_updates==0);
 tick=timing.begin_frame(166);check(tick.updates==3&&tick.remainder_ns==6&&tick.frame_ns==45&&tick.dropped_updates==1);
 auto before=timing.stats();no([&]{timing.begin_frame(165);},Fault::stale);check(timing.stats().frames==before.frames&&timing.stats().updates==before.updates);
 no([&]{timing.record_update(165,169);},Fault::stale);no([&]{timing.record_update(169,168);},Fault::stale);timing.record_update(166,169);
 no([&]{timing.record_update(169,170);},Fault::ownership);no([&]{timing.record_render(168,170);},Fault::stale);timing.record_render(169,174);
 no([&]{timing.record_render(174,175);},Fault::ownership);no([&]{timing.begin_frame(173);},Fault::stale);
 auto measured=timing.stats();check(measured.update_ns==3&&measured.render_ns==5&&measured.total_update_ns==3&&measured.total_render_ns==5&&measured.measured_updates==1&&measured.measured_renders==1);
 tick=timing.begin_frame(180);check(tick.updates==2&&tick.remainder_ns==0&&tick.frame_ns==14&&tick.dropped_updates==0);
 auto stats=timing.stats();check(stats.frames==5&&stats.updates==7&&stats.missed_updates==1&&stats.missed_frames==6);check(stats.min_frame_ns==6&&stats.max_frame_ns==45&&stats.jitter_ns==31&&stats.max_jitter_ns==31);check(stats.update_ns==0&&stats.render_ns==0&&stats.total_update_ns==3&&stats.total_render_ns==5);
 timing.record_render(180,182);no([&]{timing.record_update(182,183);},Fault::ownership);
 // Independent quotient/remainder oracle over irregular intervals. This uses a
 // small elapsed scalar, not the implementation's incremental phase arithmetic.
 FrameTiming irregular(7,2);uint64_t now=1000,total=0,scheduled=0,dropped=0;irregular.begin_frame(now);
 for(uint64_t delta:{1,2,3,20,1,100,6,7,0,28}){auto previous_whole=total/7;total+=delta;now+=delta;auto whole=total/7-previous_whole;auto run=std::min<uint64_t>(whole,2);scheduled+=run;dropped+=whole-run;auto p=irregular.begin_frame(now);check(p.updates==run&&p.remainder_ns==total%7&&p.dropped_updates==whole-run);}
 check(irregular.stats().updates==scheduled&&irregular.stats().missed_updates==dropped);
 // Very large gaps never cause unbounded catchup or overflow an accumulator.
 FrameTiming huge(1,3);huge.begin_frame(0);auto h=huge.begin_frame(UINT64_MAX);check(h.updates==3&&h.dropped_updates==UINT64_MAX-3&&h.remainder_ns==0);check(huge.stats().missed_frames==UINT64_MAX-1);no([&]{huge.begin_frame(UINT64_MAX-1);},Fault::stale);check(huge.begin_frame(UINT64_MAX).updates==0);
 FrameTiming durations(10,1);durations.begin_frame(0);durations.record_update(0,UINT64_MAX-1);no([&]{durations.begin_frame(UINT64_MAX-2);},Fault::stale);durations.begin_frame(UINT64_MAX-1);auto saved=durations.stats();no([&]{durations.record_update(UINT64_MAX-1,UINT64_MAX-3);},Fault::stale);check(durations.stats().total_update_ns==saved.total_update_ns&&durations.stats().measured_updates==saved.measured_updates);
 std::cout<<"{\"suite\":\"frame-timing\",\"checks\":"<<checks<<",\"refusals\":"<<refusals<<",\"fixed_step_ns\":10,\"scheduled_updates\":7,\"dropped_updates\":1,\"measured_update_ns\":3,\"measured_render_ns\":5}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
