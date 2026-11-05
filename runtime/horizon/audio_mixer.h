// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "platform_audio.h"
#include <memory>

namespace brutal::horizon::platform {
// Fixed 48kHz stereo signed16 LE sources; no resampling or encoded audio.
// One token owns one immutable source and one playback cursor. The mutex
// serializes mixer calls; the supplied AudioService is exclusively borrowed
// until close(), and must outlive this object. No worker or device wait occurs.
class Mixer {
 public:
 static constexpr size_t max_voices=8,max_source_bytes=65536;
 static constexpr uint32_t max_quantum_frames=2048,default_quantum_frames=800,unity=32768;
 struct Voice {uint64_t owner{},generation{};uint32_t slot{};};
 // Counters persist across restart. Underflow is a zero backlog with active
 // voices after the stream's first successful quantum; initial_fills records
 // cold startup separately. cancelled counts queued buffers discarded by
 // close; stops counts explicit active-voice stop/unload operations. errors
 // counts propagated backend failures, and native_error preserves the latest.
 struct State {
  uint32_t active_voices{},loaded_sources{},queued{},native_error{};
  uint64_t source_bytes{},submitted{},completed{},underflows{},initial_fills{},backpressure{},cancelled{},stops{},released_sources{},errors{},clipped_samples{};
  bool running{};
 };
 private:
 struct Source {std::vector<std::byte> bytes;uint64_t generation{};size_t cursor{};uint32_t gain{unity};bool active{},loop{};};
 AudioService& audio_;std::mutex mutex_;std::array<Source,max_voices> voices_{};
 std::unique_ptr<Space> staging_;Space::Handle scratch_{};AudioService::Token stream_{};
 inline static std::atomic<uint64_t> serial_{1};
 const uint64_t owner_{serial_.fetch_add(1)};uint64_t generation_{},stream_completed_{},stream_submitted_{};
 State counters_{};bool starved_{};
 void running()const{if(!counters_.running)throw Error(Fault::stale,"mixer is closed");}
 Source& owned(Voice voice){
  if(voice.owner!=owner_)throw Error(Fault::ownership,"foreign mixer voice");
  if(voice.slot>=voices_.size()||!voice.generation||voices_[voice.slot].generation!=voice.generation||voices_[voice.slot].bytes.empty())throw Error(Fault::stale,"stale mixer voice");
  return voices_[voice.slot];
 }
 static void valid_gain(uint32_t gain){if(gain>unity)throw Error(Fault::argument,"mixer gain outside Q15 unit interval");}
 void record(const AudioError& error){++counters_.errors;counters_.native_error=error.native_code;}
 AudioService::State poll(){
  try{auto state=audio_.query(stream_);counters_.completed+=state.completed-stream_completed_;stream_completed_=state.completed;counters_.queued=state.queued;if(state.queued)starved_=false;return state;}
  catch(const AudioError& error){record(error);throw;}
 }
 State snapshot()const{
  auto result=counters_;result.active_voices=result.loaded_sources=0;result.source_bytes=0;
  for(const auto&voice:voices_){result.active_voices+=voice.active;result.loaded_sources+=!voice.bytes.empty();result.source_bytes+=voice.bytes.size();}
  return result;
 }
 void release(Source& voice)noexcept{
  if(!voice.bytes.empty())++counters_.released_sources;
  std::vector<std::byte>().swap(voice.bytes);voice.active=false;voice.loop=false;voice.cursor=0;
 }
 void close_locked(){
  std::exception_ptr error;
  if(counters_.running){
   try{poll();}catch(...){error=std::current_exception();}
   counters_.cancelled+=counters_.queued;
   try{audio_.stop(stream_);}catch(const AudioError& failure){record(failure);if(!error)error=std::current_exception();}catch(...){if(!error)error=std::current_exception();}
   audio_.shutdown();
  }
  for(auto&voice:voices_)release(voice);
  counters_.running=false;counters_.queued=0;staging_.reset();scratch_={};stream_={};starved_=false;
  if(error)std::rethrow_exception(error);
 }
 public:
 explicit Mixer(AudioService& audio):audio_(audio){}
 Mixer(const Mixer&)=delete;Mixer&operator=(const Mixer&)=delete;
 ~Mixer(){shutdown();}
 void start(){
  std::lock_guard lock(mutex_);if(counters_.running)throw Error(Fault::ownership,"mixer already started");
  try{staging_=std::make_unique<Space>(max_quantum_frames*4);scratch_=staging_->allocate(max_quantum_frames*4);stream_=audio_.start();}
  catch(const AudioError& error){record(error);staging_.reset();scratch_={};throw;}
  catch(...){staging_.reset();scratch_={};throw;}
  counters_.running=true;counters_.queued=0;stream_completed_=stream_submitted_=0;starved_=false;
 }
 Voice load(Space& space,Space::Handle allocation,uint32_t address,uint32_t length,AudioFormat format={}){
  std::lock_guard lock(mutex_);running();
  if(format.rate!=48000||format.channels!=2||format.encoding!=AudioEncoding::signed16_le)throw Error(Fault::unsupported,"mixer source format unsupported");
  if(!length||length>max_source_bytes||(length%4))throw Error(Fault::argument,"mixer source must contain bounded stereo frames");
  if(generation_==UINT64_MAX)throw Error(Fault::exhausted,"mixer generation exhausted");
  size_t slot=voices_.size();for(size_t i=0;i<voices_.size();++i)if(voices_[i].bytes.empty()){slot=i;break;}
  if(slot==voices_.size())throw Error(Fault::exhausted,"mixer voice budget exhausted");
  Space::Lease pin(space,allocation);space.validate_owned_window(allocation,address,length,Access::read);
  auto bytes=space.read(space.view(address,length,Access::read));auto&voice=voices_[slot];voice.bytes=std::move(bytes);voice.generation=++generation_;voice.cursor=0;voice.gain=unity;voice.active=voice.loop=false;
  return {owner_,voice.generation,uint32_t(slot)};
 }
 void play(Voice voice,uint32_t gain=unity,bool loop=false){std::lock_guard lock(mutex_);running();valid_gain(gain);auto&v=owned(voice);v.cursor=0;v.gain=gain;v.loop=loop;v.active=true;}
 void volume(Voice voice,uint32_t gain){std::lock_guard lock(mutex_);running();valid_gain(gain);owned(voice).gain=gain;}
 // Previously submitted mixed buffers cannot be separated by voice. stop()
 // cancels future contributions; close() cancels the entire bounded queue.
 void stop(Voice voice){std::lock_guard lock(mutex_);running();auto&v=owned(voice);if(v.active){v.active=false;++counters_.stops;}}
 void unload(Voice voice){std::lock_guard lock(mutex_);running();auto&v=owned(voice);if(v.active)++counters_.stops;release(v);}
 bool pump(uint32_t frames=default_quantum_frames){
  std::lock_guard lock(mutex_);running();if(!frames||frames>max_quantum_frames)throw Error(Fault::argument,"mixer quantum exceeds bounded frames");
  const auto queue=poll();bool active=false;for(const auto&voice:voices_)active|=voice.active;if(!active)return false;
  if(queue.queued==audio_slots){++counters_.backpressure;return false;}
  if(!queue.queued&&stream_submitted_&&!starved_){++counters_.underflows;starved_=true;}
  std::array<size_t,max_voices> cursors{};std::array<bool,max_voices> playing{};
  for(size_t i=0;i<voices_.size();++i){cursors[i]=voices_[i].cursor;playing[i]=voices_[i].active;}
  std::array<std::byte,max_quantum_frames*4> mixed{};uint64_t clipped=0;
  for(uint32_t frame=0;frame<frames;++frame){
   for(unsigned channel=0;channel<2;++channel){
    int64_t sum=0;
    for(size_t i=0;i<voices_.size();++i)if(playing[i]){const auto&voice=voices_[i];const size_t offset=cursors[i]*4+channel*2;const uint32_t encoded=std::to_integer<uint32_t>(voice.bytes[offset])|(std::to_integer<uint32_t>(voice.bytes[offset+1])<<8);const int32_t sample=encoded<32768?int32_t(encoded):int32_t(encoded)-65536;sum+=int64_t(sample)*voice.gain;}
    // Sum in wide precision, round toward zero once, then saturate once.
    int64_t sample=sum/unity;if(sample>32767){sample=32767;++clipped;}else if(sample< -32768){sample= -32768;++clipped;}
    const uint16_t encoded=uint16_t(sample);const size_t offset=size_t(frame)*4+channel*2;mixed[offset]=std::byte(encoded&255);mixed[offset+1]=std::byte(encoded>>8);
   }
   for(size_t i=0;i<voices_.size();++i)if(playing[i]&&++cursors[i]==voices_[i].bytes.size()/4){if(voices_[i].loop)cursors[i]=0;else playing[i]=false;}
  }
  const uint32_t length=frames*4;staging_->write(scratch_.address,std::span(mixed).first(length));
  try{audio_.submit(stream_,*staging_,scratch_,scratch_.address,length);}catch(const AudioError& error){record(error);throw;}
  for(size_t i=0;i<voices_.size();++i){voices_[i].cursor=cursors[i];voices_[i].active=playing[i];}
  counters_.clipped_samples+=clipped;++counters_.submitted;if(!stream_submitted_)++counters_.initial_fills;++stream_submitted_;counters_.queued=queue.queued+1;starved_=false;return true;
 }
 State state(){std::lock_guard lock(mutex_);if(counters_.running)poll();return snapshot();}
 void close(){std::lock_guard lock(mutex_);close_locked();}
 void shutdown()noexcept{try{std::lock_guard lock(mutex_);close_locked();}catch(...) {}}
};
}
