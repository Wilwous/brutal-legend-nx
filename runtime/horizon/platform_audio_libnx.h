// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "platform_audio.h"
#include <switch.h>

namespace brutal::horizon::platform {
// Checked against libnx 4.12.0 commit
// 7644c9b26099aa2d2145bc72a21ee24190e92085: services/audout.h,
// audio/audio.h, arm/cache.h and audout.c. The application must reserve
// libnx's global audout session exclusively for this adapter.
class LibnxAudioBackend final:public AudioBackend {
 inline static std::atomic<bool> claimed_{};
 std::array<AudioOutBuffer,audio_slots> buffers_{};
 std::array<bool,audio_slots> queued_{};
 bool claimed_here_{},initialized_{},started_{};
 static void check(AudioOperation op,Result result){if(R_FAILED(result))throw AudioError(op,result);}
 public:
 LibnxAudioBackend()=default;
 LibnxAudioBackend(const LibnxAudioBackend&)=delete;LibnxAudioBackend&operator=(const LibnxAudioBackend&)=delete;
 ~LibnxAudioBackend()override{close();}
 void initialize(const AudioFormat&)override{
  bool expected=false;if(!claimed_.compare_exchange_strong(expected,true))throw Error(Fault::ownership,"audout already owned");
  claimed_here_=true;
  // libnx's service guard cleans a failed initialization itself.
  check(AudioOperation::initialize,audoutInitialize());initialized_=true;
  if(audoutGetSampleRate()!=48000||audoutGetChannelCount()!=2||audoutGetPcmFormat()!=PcmFormat_Int16)throw Error(Fault::unsupported,"audout negotiated unsupported PCM");
 }
 void start()override{if(!initialized_)throw Error(Fault::stale,"audout not initialized");check(AudioOperation::start,audoutStartAudioOut());started_=true;}
 void append(size_t slot,std::span<const std::byte> bytes)override{
  if(!started_||slot>=buffers_.size()||queued_[slot]||bytes.empty()||bytes.size()>audio_max_bytes||(bytes.size()%4)||uintptr_t(bytes.data())%audio_alignment)throw Error(Fault::argument,"audout staging contract");
  auto&b=buffers_[slot];b={};b.buffer=const_cast<std::byte*>(bytes.data());
  b.buffer_size=(bytes.size()+audio_alignment-1)&~(audio_alignment-1);b.data_size=bytes.size();
  armDCacheFlush(b.buffer,b.buffer_size);
  check(AudioOperation::queue,audoutAppendAudioOutBuffer(&b));queued_[slot]=true;
 }
 std::optional<size_t> released()override{
  if(!initialized_)throw Error(Fault::stale,"audout not initialized");
  AudioOutBuffer* buffer=nullptr;u32 count=0;
  check(AudioOperation::poll,audoutGetReleasedAudioOutBuffer(&buffer,&count));
  if(!count)return {};
  if(count!=1)throw Error(Fault::ownership,"unexpected audout release count");
  for(size_t i=0;i<buffers_.size();++i)if(buffer==&buffers_[i]&&queued_[i]){queued_[i]=false;return i;}
  throw Error(Fault::ownership,"foreign audout released buffer");
 }
 void stop()override{if(!initialized_)throw Error(Fault::stale,"audout not initialized");check(AudioOperation::stop,audoutStopAudioOut());started_=false;}
 void close()noexcept override{
  // A failed stop still closes the output session before descriptors or the
  // AudioService staging storage can be freed. Never reuse queued storage.
  if(initialized_){if(started_)(void)audoutStopAudioOut();audoutExit();}
  initialized_=started_=false;buffers_={};queued_={};
  if(claimed_here_){claimed_=false;claimed_here_=false;}
 }
};
}
