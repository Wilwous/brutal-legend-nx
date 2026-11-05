// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/platform_audio.h"
#include <iostream>
using namespace brutal::horizon;
using namespace brutal::horizon::platform;
namespace {
unsigned checks{};
void require(bool value){if(!value)throw std::runtime_error("audio assertion failed");++checks;}
template<class F>void refused(F operation,Fault fault){try{operation();}catch(const Error&e){require(e.fault==fault);return;}throw std::runtime_error("missing audio refusal");}
template<class F>void backend_error(F operation,AudioOperation expected){try{operation();}catch(const AudioError&e){require(e.operation==expected&&e.native_code==0x4321);return;}throw std::runtime_error("missing backend failure");}
void queue_and_lifetime(){
 Space space;auto allocation=space.allocate(4096);std::array<std::byte,16> pcm{};
 pcm[0]=std::byte{0x34};pcm[1]=std::byte{0x12};pcm[2]=std::byte{0xcc};pcm[3]=std::byte{0xed};space.write(allocation.address,pcm);
 HostAudioBackend backend;AudioService audio(backend);auto token=audio.start();auto buffer=audio.submit(token,space,allocation,allocation.address,pcm.size());
 refused([&]{AudioService duplicate(backend);},Fault::ownership);
 require(space.leases()==0);require(audio.query(token).queued==1);require(audio.query(token,buffer)==AudioService::BufferState::queued);
 // Queue owns the exact PCM bytes after guest storage is changed and freed.
 pcm[0]=std::byte{0};space.write(allocation.address,pcm);space.release(allocation);
 require(backend.queued_bytes(buffer.slot)[0]==std::byte{0x34});require(backend.queued_bytes(buffer.slot)[1]==std::byte{0x12});
 backend.step();require(audio.query(token,buffer)==AudioService::BufferState::complete);
 auto state=audio.query(token);require(state.queued==0&&state.submitted==1&&state.completed==1);
 audio.stop(token);require(!backend.active()&&backend.stops==1&&backend.closes==1);audio.shutdown();require(backend.closes==1);
 refused([&]{audio.query(token);},Fault::stale);refused([&]{audio.stop(token);},Fault::stale);
 auto replacement=audio.start();require(replacement.generation!=token.generation);refused([&]{audio.query(token);},Fault::stale);
 audio.stop(replacement);
}
void validation_and_capacity(){
 Space space,foreign;auto allocation=space.allocate(4096);auto other=foreign.allocate(4096);
 HostAudioBackend backend;AudioService audio(backend);
 refused([&]{audio.start({44100,2,AudioEncoding::signed16_le});},Fault::unsupported);
 refused([&]{audio.start({48000,1,AudioEncoding::signed16_le});},Fault::unsupported);
 refused([&]{audio.start({48000,2,static_cast<AudioEncoding>(99)});},Fault::unsupported);require(!backend.active());
 auto token=audio.start();refused([&]{audio.start();},Fault::ownership);
 auto submit=[&](Space::Handle h,uint32_t address,uint32_t size){audio.submit(token,space,h,address,size);};
 refused([&]{submit(allocation,allocation.address,0);},Fault::argument);
 refused([&]{submit(allocation,allocation.address,3);},Fault::argument);
 refused([&]{submit(allocation,allocation.address,uint32_t(audio_max_bytes+4));},Fault::argument);
 refused([&]{submit(allocation,0,4);},Fault::bounds);
 refused([&]{submit(allocation,0xfffffffc,8);},Fault::bounds);
 refused([&]{submit(allocation,allocation.address+4094,4);},Fault::bounds);
 refused([&]{submit(other,allocation.address,4);},Fault::ownership);
 auto forged=allocation;++forged.size;refused([&]{submit(forged,allocation.address,4);},Fault::ownership);
 space.protect(allocation,Permission::write);refused([&]{submit(allocation,allocation.address,4);},Fault::protection);space.protect(allocation,Permission::read_write);
 space.decommit(allocation);refused([&]{submit(allocation,allocation.address,4);},Fault::uncommitted);space.commit(allocation);
 require(space.leases()==0&&audio.query(token).submitted==0);
 std::array<AudioService::Submission,audio_slots> buffers{};for(auto&buffer:buffers)buffer=audio.submit(token,space,allocation,allocation.address,4);
 require(audio.query(token).queued==audio_slots);refused([&]{submit(allocation,allocation.address,4);},Fault::exhausted);
 backend.step();require(audio.query(token,buffers[0])==AudioService::BufferState::complete);require(audio.query(token,buffers[1])==AudioService::BufferState::queued);
 auto replacement=audio.submit(token,space,allocation,allocation.address,4);require(replacement.slot==buffers[0].slot&&replacement.sequence!=buffers[0].sequence);
 refused([&]{audio.query(token,buffers[0]);},Fault::stale);
 auto forged_submission=replacement;++forged_submission.stream.owner;refused([&]{audio.query(token,forged_submission);},Fault::ownership);
 ++replacement.sequence;refused([&]{audio.query(token,replacement);},Fault::stale);
 HostAudioBackend another_backend;AudioService another(another_backend);auto another_token=another.start();refused([&]{audio.query(another_token);},Fault::ownership);
 audio.stop(token);require(!backend.active());space.release(allocation);refused([&]{audio.submit(another_token,space,allocation,allocation.address,4);},Fault::ownership);
 auto restart=audio.start();refused([&]{audio.submit(restart,space,allocation,allocation.address,4);},Fault::stale);require(space.leases()==0);
 foreign.release(other);
}
void rollback(){
 Space space;auto allocation=space.allocate(4096);HostAudioBackend backend;AudioService audio(backend);
 backend.fail_next(AudioOperation::initialize,0x4321);backend_error([&]{audio.start();},AudioOperation::initialize);require(!backend.active());
 backend.fail_next(AudioOperation::start,0x4321);backend_error([&]{audio.start();},AudioOperation::start);require(!backend.active()&&backend.closes==1);
 auto token=audio.start();backend.fail_next(AudioOperation::queue,0x4321);
 backend_error([&]{audio.submit(token,space,allocation,allocation.address,4);},AudioOperation::queue);require(audio.query(token).submitted==0&&space.leases()==0);
 auto buffer=audio.submit(token,space,allocation,allocation.address,4);backend.fail_next(AudioOperation::poll,0x4321);
 backend_error([&]{audio.query(token);},AudioOperation::poll);require(audio.query(token,buffer)==AudioService::BufferState::queued);
 backend.fail_next(AudioOperation::stop,0x4321);backend_error([&]{audio.stop(token);},AudioOperation::stop);require(!backend.active());
 refused([&]{audio.query(token);},Fault::stale);auto restart=audio.start();require(audio.query(restart).submitted==0);audio.shutdown();audio.shutdown();require(!backend.active());
 {HostAudioBackend cleanup;{AudioService scoped(cleanup);auto t=scoped.start();scoped.submit(t,space,allocation,allocation.address,4);}require(!cleanup.active()&&cleanup.stops==1&&cleanup.closes==1);}
 space.release(allocation);
}
}
int main(){try{const auto before=Space::totals();queue_and_lifetime();validation_and_capacity();rollback();require(Space::totals()==before);std::cout<<"{\"result\":\"PASS\",\"suite\":\"platform-audio\",\"checks\":"<<checks<<"}\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
