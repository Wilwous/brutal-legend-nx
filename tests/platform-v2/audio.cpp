// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/audio_mixer.h"
#include <cmath>
#include <iostream>
#include <thread>
using namespace brutal::horizon;
using namespace brutal::horizon::platform;
namespace {
unsigned semantic{},refusals{},bytewise{};
void require(bool value){if(!value)throw std::runtime_error("mixer semantic assertion failed");++semantic;}
template<class F>void refuse(F operation,Fault fault){try{operation();}catch(const Error&e){if(e.fault!=fault)throw std::runtime_error("wrong mixer refusal");++refusals;return;}throw std::runtime_error("missing mixer refusal");}
template<class F>void native_failure(F operation,AudioOperation expected,uint32_t code){try{operation();}catch(const AudioError&e){require(e.operation==expected&&e.native_code==code);return;}throw std::runtime_error("missing mixer native failure");}
std::vector<std::byte> encode(const std::vector<int32_t>& samples){std::vector<std::byte> bytes;for(auto sample:samples){const auto encoded=uint16_t(sample);bytes.push_back(std::byte(encoded&255));bytes.push_back(std::byte(encoded>>8));}return bytes;}
Mixer::Voice load(Mixer& mixer,Space& space,const std::vector<int32_t>& samples){const auto bytes=encode(samples);auto h=space.allocate(uint32_t(bytes.size()));space.write(h.address,bytes);auto result=mixer.load(space,h,h.address,uint32_t(bytes.size()));space.release(h);return result;}
void compare(std::span<const std::byte> actual,const std::vector<int32_t>& expected){const auto bytes=encode(expected);if(actual.size()!=bytes.size()||!std::equal(actual.begin(),actual.end(),bytes.begin()))throw std::runtime_error("independent PCM oracle mismatch");++bytewise;}
// Independent numerical oracle: explicit source tables and floating point
// rational gains (all these inputs are represented exactly), no mixer cursor,
// integer decoder, accumulator or saturation helper is shared with production.
std::vector<int32_t> oracle(const std::vector<std::vector<int32_t>>& sources,const std::vector<uint32_t>& gains,const std::vector<bool>& loops,size_t offset,size_t frames){
 std::vector<int32_t> result;
 for(size_t frame=offset;frame<offset+frames;++frame)for(size_t channel=0;channel<2;++channel){double value=0;for(size_t voice=0;voice<sources.size();++voice){const auto count=sources[voice].size()/2;if(loops[voice]||frame<count)value+=double(sources[voice][(frame%count)*2+channel])*double(gains[voice])/32768.0;}result.push_back(int32_t(std::clamp(std::trunc(value),-32768.0,32767.0)));}
 return result;
}
void mixing(){
 HostAudioBackend backend;AudioService audio(backend);Mixer mixer(audio);Space space;
 mixer.start();
 const std::vector<std::vector<int32_t>> samples{{30000,-30000,10001,-10001,-32768,32767},{30000,-30000,-9001,9001,32767,-32768},{10001,-10001,20003,-20003}};
 auto a=load(mixer,space,samples[0]);auto b=load(mixer,space,samples[1]);auto c=load(mixer,space,samples[2]);
 require(space.live()==0&&space.leases()==0);mixer.play(a,Mixer::unity,true);mixer.play(b,Mixer::unity,true);mixer.play(c,Mixer::unity/2,false);
 require(mixer.pump(8));compare(backend.queued_bytes(0),oracle(samples,{32768,32768,16384},{true,true,false},0,8));
 auto state=mixer.state();require(state.active_voices==2&&state.loaded_sources==3&&state.source_bytes==32);require(state.clipped_samples==6&&state.initial_fills==1&&state.underflows==0);
 mixer.volume(a,8192);mixer.stop(b);require(mixer.pump(2));compare(backend.queued_bytes(1),oracle({samples[0]},{8192},{true},8,2));
 mixer.play(c,32768,false);require(mixer.pump(4));
 std::vector<int32_t> shifted;for(size_t frame=10;frame<14;++frame)for(size_t channel=0;channel<2;++channel)shifted.push_back(samples[0][(frame%3)*2+channel]);
 compare(backend.queued_bytes(2),oracle({shifted,samples[2]},{8192,32768},{false,false},0,4));mixer.stop(a);require(!mixer.pump(1));
 state=mixer.state();require(state.active_voices==0&&state.stops==2&&state.queued==3);mixer.close();state=mixer.state();require(!state.running&&state.loaded_sources==0&&state.source_bytes==0&&state.cancelled==3&&state.released_sources==3);require(!backend.active());
}
void scheduling(){
 HostAudioBackend backend;AudioService audio(backend);Mixer mixer(audio);Space space;mixer.start();auto voice=load(mixer,space,{1000,-1000,2000,-2000,3000,-3000});mixer.play(voice,32768,true);
 for(size_t i=0;i<audio_slots;++i)require(mixer.pump(1));
 require(!mixer.pump(1));auto state=mixer.state();require(state.queued==4&&state.backpressure==1&&state.active_voices==1&&state.underflows==0);
 backend.step(4);require(mixer.pump(1));compare(backend.queued_bytes(0),{2000,-2000});state=mixer.state();require(state.completed==4&&state.submitted==5&&state.underflows==1&&state.initial_fills==1);
 backend.step();mixer.stop(voice);require(!mixer.pump(1));require(mixer.state().underflows==1);
 mixer.play(voice,0,true);require(mixer.pump(2));compare(backend.queued_bytes(0),{0,0,0,0});require(mixer.state().underflows==2);
 mixer.close();auto closed=mixer.state();mixer.close();require(mixer.state().cancelled==closed.cancelled);require(closed.cancelled==1&&closed.completed==5);
 mixer.start();refuse([&]{mixer.play(voice);},Fault::stale);require(mixer.state().loaded_sources==0);auto fresh=load(mixer,space,{777,-777});require(fresh.generation!=voice.generation);mixer.play(fresh);require(mixer.pump(2));compare(backend.queued_bytes(0),{777,-777,0,0});require(mixer.state().active_voices==0);mixer.close();
 mixer.start();for(size_t i=0;i<Mixer::max_voices;++i){auto loud=load(mixer,space,{32767,-32768});mixer.play(loud,32768,true);}require(mixer.pump(Mixer::max_quantum_frames));std::vector<int32_t> saturated;for(size_t i=0;i<Mixer::max_quantum_frames;++i){saturated.push_back(32767);saturated.push_back(-32768);}compare(backend.queued_bytes(0),saturated);require(mixer.state().active_voices==8&&mixer.state().clipped_samples==Mixer::max_quantum_frames*2);mixer.close();
}
void validation(){
 HostAudioBackend backend;AudioService audio(backend);Mixer mixer(audio);Space space,foreign;auto h=space.allocate(65536);auto other=foreign.allocate(4096);
 refuse([&]{mixer.pump();},Fault::stale);mixer.start();refuse([&]{mixer.start();},Fault::ownership);
 auto submit=[&](Space::Handle allocation,uint32_t address,uint32_t length,AudioFormat format=AudioFormat{}){return mixer.load(space,allocation,address,length,format);};
 refuse([&]{submit(h,h.address,0);},Fault::argument);refuse([&]{submit(h,h.address,6);},Fault::argument);refuse([&]{submit(h,h.address,65540);},Fault::argument);
 refuse([&]{submit(h,h.address,4,{44100,2,AudioEncoding::signed16_le});},Fault::unsupported);refuse([&]{submit(h,h.address,4,{48000,1,AudioEncoding::signed16_le});},Fault::unsupported);refuse([&]{submit(h,h.address,4,{48000,2,AudioEncoding(9)});},Fault::unsupported);
 refuse([&]{submit(other,h.address,4);},Fault::ownership);auto forged=h;++forged.size;refuse([&]{submit(forged,h.address,4);},Fault::ownership);
 refuse([&]{submit(h,0,4);},Fault::bounds);refuse([&]{submit(h,0xfffffffc,8);},Fault::bounds);refuse([&]{submit(h,h.address+65534,4);},Fault::bounds);
 space.protect(h,Permission::write);refuse([&]{submit(h,h.address,4);},Fault::protection);space.protect(h,Permission::read_write);space.decommit(h);refuse([&]{submit(h,h.address,4);},Fault::uncommitted);space.commit(h);
 require(space.leases()==0&&mixer.state().loaded_sources==0);
 space.protect(h,Permission::read);auto voice=submit(h,h.address,65536);space.protect(h,Permission::read_write);require(mixer.state().source_bytes==65536);refuse([&]{mixer.play(voice,32769);},Fault::argument);refuse([&]{mixer.volume(voice,UINT32_MAX);},Fault::argument);
 auto foreign_voice=voice;++foreign_voice.owner;refuse([&]{mixer.play(foreign_voice);},Fault::ownership);auto stale=voice;++stale.generation;refuse([&]{mixer.play(stale);},Fault::stale);stale=voice;stale.slot=99;refuse([&]{mixer.stop(stale);},Fault::stale);
 refuse([&]{mixer.pump(0);},Fault::argument);refuse([&]{mixer.pump(2049);},Fault::argument);
 for(size_t i=1;i<Mixer::max_voices;++i)submit(h,h.address,4);refuse([&]{submit(h,h.address,4);},Fault::exhausted);require(mixer.state().loaded_sources==8);
 mixer.unload(voice);refuse([&]{mixer.play(voice);},Fault::stale);auto replacement=submit(h,h.address,4);require(replacement.slot==voice.slot&&replacement.generation!=voice.generation);
 mixer.close();space.release(h);foreign.release(other);mixer.start();refuse([&]{submit(h,h.address,4);},Fault::stale);mixer.close();
}
void failures(){
 HostAudioBackend backend;AudioService audio(backend);Mixer mixer(audio);Space space;
 backend.fail_next(AudioOperation::initialize,0x4101);native_failure([&]{mixer.start();},AudioOperation::initialize,0x4101);auto state=mixer.state();require(!state.running&&state.errors==1&&state.native_error==0x4101&&!backend.active());
 backend.fail_next(AudioOperation::start,0x4102);native_failure([&]{mixer.start();},AudioOperation::start,0x4102);require(!mixer.state().running&&!backend.active());
 mixer.start();auto voice=load(mixer,space,{1000,-1000,2000,-2000,3000,-3000});mixer.play(voice,32768,true);
 backend.fail_next(AudioOperation::queue,0x4103);native_failure([&]{mixer.pump(1);},AudioOperation::queue,0x4103);require(mixer.state().submitted==0&&mixer.state().underflows==0);require(mixer.pump(1));compare(backend.queued_bytes(0),{1000,-1000});
 backend.step();backend.fail_next(AudioOperation::queue,0x4104);native_failure([&]{mixer.pump(1);},AudioOperation::queue,0x4104);require(mixer.state().underflows==1);require(mixer.pump(1));compare(backend.queued_bytes(0),{2000,-2000});require(mixer.state().underflows==1);
 backend.fail_next(AudioOperation::poll,0x4105);native_failure([&]{mixer.state();},AudioOperation::poll,0x4105);require(mixer.state().native_error==0x4105&&mixer.state().errors==5);
 backend.fail_next(AudioOperation::stop,0x4106);native_failure([&]{mixer.close();},AudioOperation::stop,0x4106);state=mixer.state();require(state.errors==6&&state.native_error==0x4106&&!state.running&&state.loaded_sources==0&&state.source_bytes==0&&state.cancelled==1&&!backend.active());mixer.shutdown();require(mixer.state().errors==6);
 mixer.start();voice=load(mixer,space,{1,-1});mixer.play(voice,32768,true);mixer.pump(1);backend.fail_next(AudioOperation::poll,0x4107);native_failure([&]{mixer.close();},AudioOperation::poll,0x4107);require(!mixer.state().running&&!backend.active());
 const auto before=Space::totals();{Mixer scoped(audio);scoped.start();auto v=load(scoped,space,{12,-12});scoped.play(v,32768,true);scoped.pump(1);}require(Space::totals()==before&&!backend.active());
}
void serialized_calls(){
 HostAudioBackend backend;AudioService audio(backend);Mixer mixer(audio);Space space;mixer.start();auto voice=load(mixer,space,{321,-321});mixer.play(voice,32768,true);
 std::thread writer([&]{for(unsigned i=0;i<200;++i)mixer.volume(voice,(i%2)?32768:16384);});std::thread reader([&]{for(unsigned i=0;i<200;++i)(void)mixer.state();});writer.join();reader.join();mixer.volume(voice,32768);require(mixer.pump(1));compare(backend.queued_bytes(0),{321,-321});mixer.close();
}
}
int main(){try{const auto before=Space::totals();mixing();scheduling();validation();failures();serialized_calls();require(Space::totals()==before);std::cout<<"{\"result\":\"PASS\",\"suite\":\"audio-mixer\",\"checks\":"<<semantic+refusals+bytewise<<",\"semantic\":"<<semantic<<",\"expected_refusals\":"<<refusals<<",\"bytewise\":"<<bytewise<<"}\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
