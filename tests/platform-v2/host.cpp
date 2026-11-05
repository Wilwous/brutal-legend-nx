// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "tests/platform-v2/demo.h"
#include "runtime/horizon/frame_timing.h"
#include "assets_contract.h"
#include <iostream>
namespace h=brutal::horizon;namespace p=h::platform;namespace r=h::resources;using brutal::platform_v2::Demo;
int main(int argc,char**argv){if(argc!=2)return 2;try{
 unsigned checks=0,refusals=0;auto yes=[&](bool value){if(!value)throw std::runtime_error("V2 integrated assertion");++checks;};auto no=[&](auto fn){try{fn();}catch(const h::Error&){++refusals;return;}throw std::runtime_error("missing V2 refusal");};
 auto declared=v2_manifest();r::Service resources(argv[1],declared);p::HostAudioBackend backend;p::AudioService audio(backend);p::Mixer mixer(audio);brutal::platform_v2::Stats normal;p::Mixer::State normal_audio;
 {
  p::Scene3D scene;p::Input input;Demo demo(scene,input,mixer,resources);demo.initialize();yes(demo.stats().result==4&&demo.stats().audio_available&&resources.live()==4);yes(scene.meshes()==0&&demo.stats().assets_ready==0);
  for(uint64_t tick=1;tick<=340;++tick){
   uint64_t buttons=tick<=2||tick==311||tick==312?p::Input::a:tick==320||tick==321||tick==325?p::Input::b:tick==326?p::Input::x:tick==327?p::Input::y:0;
   p::Input::RawController raw{0,1,tick!=330,buttons,tick==1?32767:0,0,tick==1?32767:0,0};input.update(tick,std::span(&raw,1));backend.step(1);demo.tick();
   if(tick==1){yes(demo.memory().load<uint32_t>(demo.shared().address+64)==527);yes(demo.memory().load<uint32_t>(demo.shared().address+72)==3);yes(demo.stats().draws==0&&!demo.stats().scene_ready);}
   if(tick==20){yes(demo.stats().scene_ready&&scene.meshes()==1&&scene.textures()==1);yes(demo.stats().assets_ready==2);}
  }
  normal=demo.stats();normal_audio=mixer.state();yes(normal.ticks==340&&normal.result==340&&normal.resource_bytes==39240&&normal.assets_ready==4);yes(normal.service_entered==normal.service_completed&&normal.draws>1200);yes(resources.live()==0&&scene.mesh_bytes()==552&&scene.texture_bytes()==256);yes(normal_audio.loaded_sources==2&&normal_audio.source_bytes==38400&&normal_audio.submitted>20);yes(normal.deferred_actions==1&&input.snapshot().epoch==2);
  auto frame=scene.snapshot();yes(frame->number==340&&frame->draws.size()==4&&frame->visible_draws>=3);yes(frame->draws[0].mesh->vertices.size()==24&&frame->draws[0].mesh->indices.size()==36&&frame->draws[0].texture->rgba.size()==256);
  // Independent exact arithmetic expectations for the executed integer update.
  yes(frame->draws[0].model[12]==15.f/256&&frame->draws[0].model[14]==0);yes(demo.memory().load<uint32_t>(demo.shared().address+80)==84);yes(demo.memory().load<uint32_t>(demo.shared().address+84)==2);yes(demo.context().r31.u64==0x1122334455667788ull);frame.reset();
  demo.shutdown();yes(demo.stats().closed&&demo.stats().cleanup_errors==0);yes(!backend.active()&&!mixer.state().running&&mixer.state().loaded_sources==0);yes(scene.mesh_bytes()==0&&scene.texture_bytes()==0&&resources.live()==0);yes(h::Space::totals()==std::pair<size_t,size_t>{0,0});demo.shutdown();no([&]{demo.tick();});
 }
 {
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);auto entries=h::Scope::guest_entries.load();d.context().r13.u64=1;no([&]{d.initialize();});yes(h::Scope::guest_entries==entries&&d.stats().poisoned);d.shutdown();yes(resources.live()==0&&h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 {
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);d.initialize();no([&]{d.tick();});yes(d.stats().poisoned&&s.last_frame()==0);d.shutdown();
 }
 {
  p::Scene3D s;p::Input i;backend.fail_next(p::AudioOperation::initialize,0x91);Demo d(s,i,mixer,resources);d.initialize();yes(!d.stats().audio_available&&d.stats().audio_native_error==0x91);
  for(uint64_t n=1;n<=12;++n){i.update(n,{});d.tick();}yes(s.last_frame()==12&&d.stats().scene_ready);d.shutdown();yes(!backend.active()&&h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 {
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);d.initialize();d.memory().protect(d.shared(),h::Permission::read);i.update(1,{});no([&]{d.tick();});yes(d.stats().poisoned&&s.last_frame()==0);no([&]{d.tick();});d.shutdown();
 }
 {
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);d.initialize();d.observer([](uint32_t op,h::Space&sp,h::Space::Handle cap){if(op==4)sp.store<uint32_t>(cap.address+1024+12*32,999);});
  bool rejected=false;for(uint64_t n=1;n<=12&&!rejected;++n){i.update(n,{});try{d.tick();}catch(const h::Error&){++refusals;rejected=true;}}
  yes(rejected&&d.stats().poisoned&&s.last_frame()==0);yes(d.stats().service_entered==d.stats().service_completed+1);d.shutdown();
 }
 {
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);d.initialize();i.update(1,{});d.observer([&](uint32_t op,h::Space&,h::Space::Handle){if(op==1)d.request_stop();});no([&]{d.tick();});yes(d.stats().stopped&&!d.stats().poisoned);no([&]{d.tick();});d.shutdown();yes(h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 for(auto operation:{p::AudioOperation::poll,p::AudioOperation::queue}){
  p::Scene3D s;p::Input i;Demo d(s,i,mixer,resources);d.initialize();backend.fail_next(operation,0x92);bool rejected=false;
  for(uint64_t n=1;n<=330&&!rejected;++n){i.update(n,{});try{d.tick();}catch(const p::AudioError&e){yes(e.native_code==0x92&&e.operation==operation);++refusals;rejected=true;}}
  yes(rejected&&d.stats().poisoned&&!d.stats().audio_available&&d.stats().audio_native_error==0x92);d.shutdown();yes(!backend.active()&&h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 {
  r::Service source(argv[1],declared);p::Scene3D s;p::Input i;Demo d(s,i,mixer,source);d.initialize();source.shutdown();i.update(1,{});no([&]{d.tick();});yes(d.stats().poisoned&&s.last_frame()==0);d.shutdown();yes(h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 resources.shutdown();yes(r::Service::totals()==std::pair<size_t,size_t>{0,0}&&h::Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<checks<<",\"refusals\":"<<refusals<<",\"failures\":0,\"ticks\":"<<normal.ticks<<",\"result\":"<<normal.result<<",\"resource_bytes\":"<<normal.resource_bytes<<",\"draws\":"<<normal.draws<<",\"service_entered\":"<<normal.service_entered<<",\"service_completed\":"<<normal.service_completed<<",\"generated_dispatches\":"<<normal.guest_calls<<",\"checked_reads\":"<<normal.reads<<",\"checked_writes\":"<<normal.writes<<",\"hook_polls\":"<<normal.hooks<<",\"audio_submitted\":"<<normal_audio.submitted<<",\"audio_completed\":"<<normal_audio.completed<<",\"audio_underflows\":"<<normal_audio.underflows<<",\"allocations\":0,\"leases\":0,\"resource_handles\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
