// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "tests/platform-demo/demo.h"
#include <iostream>
namespace h=brutal::horizon;namespace p=h::platform;namespace r=h::resources;using brutal::platform_demo::Demo;
int main(int argc,char**argv){if(argc!=2)return 2;try{
 unsigned positive=0,refusals=0;auto check=[&](bool value){if(!value)throw std::runtime_error("integrated platform assertion");++positive;};
 auto no=[&](auto fn){try{fn();}catch(const h::Error&){++refusals;return;}throw std::runtime_error("missing platform refusal");};
 auto declared=brutal::platform_demo::manifest();r::Service resources(argv[1],declared);p::Graphics graphics;p::Input input;p::HostAudioBackend backend;p::AudioService audio(backend);brutal::platform_demo::Stats normal;
 {
  Demo demo(graphics,input,audio,resources);demo.initialize();check(demo.stats().result==36&&demo.stats().resource_bytes==36&&resources.live()==0);check(demo.stats().audio_available&&backend.active());
  auto pixel=[&](unsigned x,unsigned y,uint32_t expected){auto bytes=graphics.pixels();auto offset=(y*graphics.width()+x)*4;uint32_t actual=0;for(unsigned i=0;i<4;++i)actual=(actual<<8)|std::to_integer<uint8_t>(bytes[offset+i]);check(actual==expected);};
  auto frame=[&](uint64_t n,int32_t x,int32_t y,uint64_t buttons,bool connected){p::Input::RawController raw{0,1,connected,buttons,x,y,0,0};input.update(n,std::span(&raw,1));backend.step(4);demo.frame();check(demo.stats().result==n&&demo.stats().frames==n&&graphics.last_frame()==n);};
  frame(1,0,0,0,true);check(demo.memory().load<uint32_t>(demo.shared().address+64)==81);pixel(81,60,0x42525554);pixel(0,0,0x102030ff);pixel(280,140,0xf0b41eff);check(backend.queued_bytes(0).size()==19200);check(backend.queued_bytes(0)[0]==std::byte{0}&&backend.queued_bytes(0)[4]!=std::byte{0});
  frame(2,32767,0,p::Input::a,true);check(demo.memory().load<uint32_t>(demo.shared().address+64)==85);pixel(85,60,0x42725554);
  frame(3,-32768,32767,p::Input::a,true);check(demo.memory().load<uint32_t>(demo.shared().address+64)==83&&demo.memory().load<uint32_t>(demo.shared().address+68)==57);pixel(83,57,0x42725554);
  frame(4,0,0,0,false);check(demo.memory().load<uint32_t>(demo.shared().address+12)==0);check(demo.memory().load<uint32_t>(demo.shared().address+128)==p::Input::magic);
  frame(5,0,0,p::Input::a,true);pixel(85,57,0x42925554);check(input.snapshot().epoch==2);check(demo.stats().audio_submitted==3&&demo.stats().audio_completed==2);
  backend.step(4);normal=demo.stats();demo.shutdown();check(demo.stats().closed&&!backend.active()&&demo.stats().cleanup_errors==0);check(demo.stats().audio_completed==3);check(h::Space::totals()==std::pair<size_t,size_t>{0,0});demo.shutdown();no([&]{demo.frame();});
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);auto before=h::Scope::guest_entries.load();demo.context().r13.u64=1;no([&]{demo.initialize();});check(h::Scope::guest_entries==before&&demo.stats().poisoned);demo.shutdown();check(h::Space::totals()==std::pair<size_t,size_t>{0,0});
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.memory().store<uint32_t>(demo.shared().address+256,0x2e2e2f78);no([&]{demo.initialize();});check(resources.live()==0&&demo.stats().poisoned);demo.shutdown();
 }
 {
  p::Graphics g;p::Input i;backend.fail_next(p::AudioOperation::initialize,0x77);Demo demo(g,i,audio,resources);demo.initialize();check(!demo.stats().audio_available&&demo.stats().audio_native_error==0x77);p::Input::RawController raw{0,1,true,0,0,0,0,0};i.update(1,std::span(&raw,1));demo.frame();check(g.last_frame()==1&&demo.memory().load<uint32_t>(demo.shared().address+28)==3);demo.shutdown();check(!backend.active());
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.initialize();auto cap=demo.shared();demo.memory().protect(cap,h::Permission::read);i.update(1,{});no([&]{demo.frame();});check(demo.stats().poisoned&&g.last_frame()==0);no([&]{demo.frame();});demo.shutdown();
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.initialize();no([&]{demo.frame();});check(demo.stats().poisoned&&g.last_frame()==0);demo.shutdown();
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.initialize();demo.memory().store<uint32_t>(demo.shared().address+64,255);demo.memory().store<uint32_t>(demo.shared().address+68,127);p::Input::RawController raw{0,1,true,0,0,-32768,0,0};i.update(1,std::span(&raw,1));demo.frame();check(demo.memory().load<uint32_t>(demo.shared().address+64)==0&&demo.memory().load<uint32_t>(demo.shared().address+68)==2);check(g.pixels()[size_t(2)*320*4]==std::byte{0x42});demo.shutdown();
 }
 {
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.initialize();i.update(1,{});demo.observer([](uint32_t op,h::Space&s,h::Space::Handle cap){if(op==7)s.store<uint32_t>(cap.address+1024+6*32,999);});no([&]{demo.frame();});check(demo.stats().poisoned&&g.last_frame()==0);check(demo.stats().service_entered==demo.stats().service_completed+1);demo.shutdown();
 }
 for(auto operation:{p::AudioOperation::poll,p::AudioOperation::queue}){
  p::Graphics g;p::Input i;Demo demo(g,i,audio,resources);demo.initialize();i.update(1,{});backend.fail_next(operation,0x88);
  bool observed=false;try{demo.frame();}catch(const p::AudioError&e){check(e.operation==operation&&e.native_code==0x88);observed=true;}check(observed);++refusals;
  check(demo.stats().poisoned&&!demo.stats().audio_available&&demo.stats().audio_native_error==0x88);check(demo.stats().service_entered==demo.stats().service_completed+1);no([&]{demo.frame();});demo.shutdown();check(demo.stats().closed&&!backend.active()&&demo.stats().cleanup_errors==0);
 }
 resources.shutdown();check(r::Service::totals()==std::pair<size_t,size_t>{0,0});check(h::Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"failures\":0,\"frames\":"<<normal.frames<<",\"result\":"<<normal.result<<",\"service_entered\":"<<normal.service_entered<<",\"service_completed\":"<<normal.service_completed<<",\"generated_dispatches\":"<<normal.guest_calls<<",\"checked_reads\":"<<normal.reads<<",\"checked_writes\":"<<normal.writes<<",\"hook_polls\":"<<normal.hooks<<",\"audio_submitted\":"<<normal.audio_submitted<<",\"allocations\":0,\"leases\":0,\"resource_handles\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
