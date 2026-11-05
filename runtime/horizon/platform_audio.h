// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <cstring>
#include <limits>
#include <optional>

namespace brutal::horizon::platform {
enum class AudioEncoding : uint32_t { signed16_le=1 };
struct AudioFormat {uint32_t rate{48000},channels{2};AudioEncoding encoding{AudioEncoding::signed16_le};};
enum class AudioOperation { initialize,start,queue,poll,stop };
class AudioError : public std::runtime_error {
 public:
 AudioOperation operation;uint32_t native_code;
 AudioError(AudioOperation op,uint32_t code):std::runtime_error("audio backend failure"),operation(op),native_code(code){}
};
inline constexpr size_t audio_slots=4,audio_max_bytes=65536,audio_alignment=4096;
// Backends retain only the service's aligned staging storage. close() must
// release the device session before returning, even after stop() fails.
class AudioBackend {
 friend class AudioService;
 std::atomic<bool> attached_{};
 public:
 virtual ~AudioBackend()=default;
 virtual void initialize(const AudioFormat&)=0;
 virtual void start()=0;
 virtual void append(size_t,std::span<const std::byte>)=0;
 virtual std::optional<size_t> released()=0;
 virtual void stop()=0;
 virtual void close()noexcept=0;
};
// One explicit, bounded stream; this synthetic API has no Xbox ABI claim.
// Calls (including backend stepping) are serialized by the owning application.
// The backend must outlive its sole AudioService.
class AudioService {
 public:
 struct Token {uint64_t owner{},generation{};};
 struct Submission {Token stream;uint64_t sequence{};uint32_t slot{};};
 enum class BufferState { queued,complete };
 struct State {uint32_t queued{};uint64_t submitted{},completed{};};
 private:
 struct alignas(audio_alignment) Slot {std::array<std::byte,audio_max_bytes> data{};uint64_t sequence{};bool queued{},complete{};};
 AudioBackend& backend_;std::array<Slot,audio_slots> slots_{};
 inline static std::atomic<uint64_t> serial_{1};
 uint64_t owner_{serial_.fetch_add(1)},generation_{},sequence_{},submitted_{},completed_{};bool running_{};
 void owned(Token t)const{
  if(t.owner!=owner_)throw Error(Fault::ownership,"foreign audio stream");
  if(!running_||!t.generation||t.generation!=generation_)throw Error(Fault::stale,"stale audio stream");
 }
 void poll(){
  for(size_t n=0;n<audio_slots;++n){auto i=backend_.released();if(!i)return;
   if(*i>=slots_.size()||!slots_[*i].queued)throw Error(Fault::ownership,"unexpected audio completion");
   auto& s=slots_[*i];s.queued=false;s.complete=true;++completed_;
  }
 }
 void clear()noexcept{for(auto&s:slots_){s.queued=false;s.complete=false;s.sequence=0;}running_=false;}
 public:
 explicit AudioService(AudioBackend& backend):backend_(backend){bool expected=false;if(!backend_.attached_.compare_exchange_strong(expected,true))throw Error(Fault::ownership,"audio backend already attached");}
 AudioService(const AudioService&)=delete;AudioService&operator=(const AudioService&)=delete;
 ~AudioService(){shutdown();backend_.attached_=false;}
 Token start(AudioFormat format={}){
  if(running_)throw Error(Fault::ownership,"audio stream already started");
  if(format.rate!=48000||format.channels!=2||format.encoding!=AudioEncoding::signed16_le)throw Error(Fault::unsupported,"audio requires 48kHz signed16 LE stereo");
  if(generation_==std::numeric_limits<uint64_t>::max())throw Error(Fault::exhausted,"audio generation exhausted");
  try{backend_.initialize(format);backend_.start();}catch(...){backend_.close();clear();throw;}
  ++generation_;submitted_=completed_=0;running_=true;return {owner_,generation_};
 }
 Submission submit(Token token,Space& space,Space::Handle allocation,uint32_t address,uint32_t length){
  owned(token);
  if(!length||length>audio_max_bytes||(length%4))throw Error(Fault::argument,"audio PCM length must contain bounded stereo frames");
  if(sequence_==std::numeric_limits<uint64_t>::max())throw Error(Fault::exhausted,"audio sequence exhausted");
  poll();size_t index=slots_.size();for(size_t i=0;i<slots_.size();++i)if(!slots_[i].queued){index=i;break;}
  if(index==slots_.size())throw Error(Fault::exhausted,"audio queue full");
  // Exact allocation admission plus a pin closes release/reallocation races.
  // Space rechecks permissions during the actual read. No guest pointer survives.
  auto pin=Space::Lease(space,allocation);
  space.validate_owned_window(allocation,address,length,Access::read);
  auto bytes=space.read(space.view(address,length,Access::read));
  auto&s=slots_[index];std::memcpy(s.data.data(),bytes.data(),length);
  backend_.append(index,std::span<const std::byte>(s.data.data(),length));
  s.sequence=++sequence_;s.queued=true;s.complete=false;++submitted_;
  return {token,s.sequence,uint32_t(index)};
 }
 State query(Token token){owned(token);poll();State result{0,submitted_,completed_};for(auto&s:slots_)result.queued+=s.queued;return result;}
 BufferState query(Token token,Submission submission){
  owned(token);
  if(submission.stream.owner!=token.owner)throw Error(Fault::ownership,"foreign audio submission");
  if(submission.stream.generation!=token.generation||submission.slot>=slots_.size()||!submission.sequence||slots_[submission.slot].sequence!=submission.sequence)throw Error(Fault::stale,"stale audio submission");
  poll();return slots_[submission.slot].queued?BufferState::queued:BufferState::complete;
 }
 void stop(Token token){
  owned(token);try{backend_.stop();}catch(...){backend_.close();clear();throw;}
  backend_.close();clear();
 }
 // Explicit stop propagates the native error; teardown always closes ownership
 // before the staging storage is destroyed and is safe to repeat.
 void shutdown()noexcept{if(running_){try{backend_.stop();}catch(...){}backend_.close();clear();}}
};

// A real deterministic queue/completion backend, requiring no output device.
// step() is the host clock: admission never pretends playback completed.
class HostAudioBackend final:public AudioBackend {
 struct Item {std::span<const std::byte> bytes;uint64_t order{};bool queued{},complete{};};
 std::array<Item,audio_slots> items_{};bool initialized_{},running_{};uint64_t order_{};
 std::optional<AudioOperation> failure_;uint32_t failure_code_{};
 void fail(AudioOperation operation){if(failure_==operation){failure_.reset();throw AudioError(operation,failure_code_);}}
 public:
 unsigned initializations{},starts{},stops{},closes{};
 void fail_next(AudioOperation operation,uint32_t code=0x1234){failure_=operation;failure_code_=code;}
 void initialize(const AudioFormat&)override{fail(AudioOperation::initialize);if(initialized_)throw Error(Fault::ownership,"host audio already initialized");initialized_=true;++initializations;}
 void start()override{fail(AudioOperation::start);if(!initialized_)throw Error(Fault::stale,"host audio not initialized");running_=true;++starts;}
 void append(size_t slot,std::span<const std::byte> bytes)override{
  fail(AudioOperation::queue);if(!running_||slot>=items_.size()||items_[slot].queued)throw Error(Fault::ownership,"host audio queue state");
  items_[slot]={bytes,++order_,true,false};
 }
 std::optional<size_t> released()override{fail(AudioOperation::poll);for(size_t i=0;i<items_.size();++i)if(items_[i].queued&&items_[i].complete){items_[i]={};return i;}return {};}
 void step(size_t count=1){while(count--){size_t first=items_.size();for(size_t i=0;i<items_.size();++i)if(items_[i].queued&&!items_[i].complete&&(first==items_.size()||items_[i].order<items_[first].order))first=i;if(first==items_.size())return;items_[first].complete=true;}}
 std::span<const std::byte> queued_bytes(size_t slot)const{if(slot>=items_.size()||!items_[slot].queued)throw Error(Fault::stale,"host audio slot not queued");return items_[slot].bytes;}
 void stop()override{fail(AudioOperation::stop);running_=false;++stops;}
 void close()noexcept override{if(initialized_)++closes;initialized_=running_=false;for(auto&i:items_)i={};}
 bool active()const{return initialized_||running_;}
};
}
