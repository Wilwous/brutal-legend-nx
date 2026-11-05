// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "tests/platform-v2/demo.h"
#include "program.h"
#include "assets_contract.h"
#include <bit>
#include <cmath>
#include <optional>
namespace h=brutal::horizon;namespace p=h::platform;namespace r=h::resources;namespace ni=h::native_image;namespace math=p::scene_math;
namespace brutal::platform_v2 {
struct Demo::Impl {
 p::Scene3D& scene;p::Input& input;p::Mixer& mixer;r::Service& resources;
 h::Space space{4*1024*1024};h::ModuleLife module{0x504c4154};h::Slots slots;h::Slots::Token slot{slots.allocate()};
 ni::Image image{space,platform_plan(),platform_bytes,platform_identity.plan};
 ni::Binding code{image,space,module,platform_identity,platform_identity,{demo_functions,demo_function_count},platform_bounds,platform_control,{demo_functions,demo_function_count},ni::ImportPolicy::no_function_imports};
 h::GuestThread guest{space,module,slots,slot,3,space.read(platform_plan().tls_address,16)};
 r::Stream streaming{resources,space,256*1024};h::ProbeControl control;
 h::Space::Handle shared{};std::array<std::optional<r::Stream::Token>,4> jobs{};std::array<h::Space::Handle,4> assets{};std::array<uint32_t,4> delivered{};std::array<bool,4> taken{};
 std::optional<p::Scene3D::MeshHandle> mesh;std::optional<p::Scene3D::TextureHandle> texture;std::array<std::optional<p::Mixer::Voice>,2> voices{};
 Stats counts;uint64_t initial_sp{},saved_r31{};bool ambient_playing{};std::function<void(uint32_t,h::Space&,h::Space::Handle)> observe;
 struct Frame {Impl& demo;h::Scope& scope;inline static thread_local Frame* current{};Frame(Impl&d,h::Scope&s):demo(d),scope(s){if(current)throw h::Error(h::Fault::no_thread,"nested V2 bridge");current=this;}~Frame(){current=nullptr;}};
 Impl(p::Scene3D&s,p::Input&i,p::Mixer&m,r::Service&res):scene(s),input(i),mixer(m),resources(res){
  try{shared=space.allocate(16384,0x60000000);space.store<uint32_t>(shared.address+64,512);space.store<uint32_t>(shared.address+68,512);space.store<uint32_t>(shared.address+76,73);initial_sp=guest.context.r1.u64;saved_r31=0x1122334455667788ull;guest.context.r31.u64=saved_r31;}
  catch(...){if(shared.id)space.release(shared);throw;}
 }
 template<class T>void token(uint32_t offset,T v){space.store<uint32_t>(shared.address+offset,uint32_t(v.owner>>32));space.store<uint32_t>(shared.address+offset+4,uint32_t(v.owner));space.store<uint32_t>(shared.address+offset+8,v.slot);space.store<uint32_t>(shared.address+offset+12,v.generation);}
 void release_asset(size_t i){if(assets[i].id){space.release(assets[i]);assets[i]={};}}
 void advance_resources(){
  for(size_t i=0;i<jobs.size();++i)if(jobs[i]){
   auto progress=streaming.step(*jobs[i],128);counts.resource_bytes+=progress.transferred-delivered[i];delivered[i]=progress.transferred;
   if(progress.ready){assets[i]=streaming.take(*jobs[i]);jobs[i].reset();taken[i]=true;++counts.assets_ready;}
  }
  if(!mesh&&assets[0].id&&assets[1].id){
   auto cap=assets[0];auto at=cap.address;
   if(cap.size!=584||space.load<uint32_t>(at)!=0x53434e32||space.load<uint32_t>(at+4)!=1||space.load<uint32_t>(at+8)!=24||space.load<uint32_t>(at+12)!=36||space.load<uint32_t>(at+16)!=32||space.load<uint32_t>(at+20)!=512||space.load<uint32_t>(at+24)!=584||space.load<uint32_t>(at+28)!=4)throw h::Error(h::Fault::argument,"original scene resource layout");
   mesh=scene.upload_mesh(space,{cap,at+32,480},24,{cap,at+512,72},36);texture=scene.upload_texture(space,{assets[1],assets[1].address,256},8,8,32);
   token(112,*mesh);token(128,*texture);release_asset(0);release_asset(1);counts.scene_ready=true;
  }
  for(size_t i=0;i<2;++i)if(assets[i+2].id){
   if(counts.audio_available)voices[i]=mixer.load(space,assets[i+2],assets[i+2].address,assets[i+2].size);
   release_asset(i+2);
   if(i==0&&voices[0]){mixer.play(*voices[0],8192,true);ambient_playing=true;}
  }
 }
 void transforms(){
  auto read=[&](uint32_t o){return space.load<uint32_t>(shared.address+o);};
  auto x=read(64),z=read(68),yaw=read(72),pitch=read(76),phase=read(80),action=read(84);
  if(x>1023||z>1023||yaw>255||pitch>255||phase>255)throw h::Error(h::Fault::argument,"bounded synthetic scene state");
  constexpr float tau=6.283185307179586f;float a=float(phase)*tau/256,look=float(yaw)*tau/256,tilt=(float(pitch)-128)*.0055f;
  std::array<math::Mat4,6> matrices;math::Vec3 eye{0,2.5f,7};matrices[0]=math::look_at(eye,{std::sin(look)*std::cos(tilt),2.5f+std::sin(tilt),7-std::cos(look)*std::cos(tilt)},{0,1,0});matrices[1]=math::perspective(tau/6,1280.f/720,.1f,100);
  float scale=(action&1)?1.35f:1.f;
  matrices[2]=math::multiply(math::translation(float(x)/256-2,0,float(z)/256-2),math::multiply(math::rotation_y(a),math::scale(scale,scale,scale)));
  matrices[3]=math::multiply(math::translation(-2,0,-2),math::rotation_y(-a));
  matrices[4]=math::multiply(math::translation(2,.5f,-2),math::multiply(math::rotation_x(a),math::scale(.8f,.8f,.8f)));
  matrices[5]=math::multiply(math::multiply(math::translation(0,1,-3),math::rotation_y(a)),math::multiply(math::translation(1.25f,0,0),math::scale(.45f,.45f,.45f)));
  for(size_t i=0;i<matrices.size();++i)for(size_t j=0;j<16;++j)space.store<uint32_t>(shared.address+4096+uint32_t(i*64+j*4),std::bit_cast<uint32_t>(matrices[i][j]));
 }
 void bridge(h::Scope& scope){
  auto& c=guest.context;scope.import_target(platform_gate);if(!scope.belongs_to(module,space))throw h::Error(h::Fault::ownership,"V2 module boundary");++counts.service_entered;auto op=c.r3.u32;if(observe)observe(op,space,shared);uint32_t result=0;
  switch(op){
   case 1:{
    auto state=input.snapshot();if(state.frame!=counts.ticks+1)throw h::Error(h::Fault::stale,"V2 input frame association");input.write_snapshot(space,shared,shared.address+256);
    auto pitch=space.load<uint32_t>(shared.address+76);if(pitch>255)throw h::Error(h::Fault::argument,"V2 pitch state");int32_t delta=state.right_y/8192;delta=std::clamp(int32_t(pitch)+delta,0,255)-int32_t(pitch);
    space.store<uint32_t>(shared.address,uint32_t(state.left_x/2048));space.store<uint32_t>(shared.address+4,uint32_t(-state.left_y/2048));space.store<uint32_t>(shared.address+8,uint32_t(state.right_x/8192));space.store<uint32_t>(shared.address+12,uint32_t(delta));space.store<uint32_t>(shared.address+16,(state.pressed&p::Input::a)?1:0);space.store<uint32_t>(shared.address+20,state.connected?1:0);space.store<uint32_t>(shared.address+36,uint32_t(state.pressed));result=state.connected?1:0;break;
   }
   case 2:{if(counts.initialized||streaming.live())throw h::Error(h::Fault::stale,"V2 resource startup twice");auto declared=v2_manifest();for(size_t i=0;i<declared.size();++i)jobs[i]=streaming.begin({"demo:/"+declared[i].path,uint32_t(declared[i].expected_size),*declared[i].sha256});result=4;break;}
   case 3:{advance_resources();if(counts.scene_ready)transforms();result=counts.scene_ready?1:0;break;}
   case 4:{
    if(!counts.scene_ready)throw h::Error(h::Fault::stale,"V2 draw before resources ready");
    if(c.r4.u32!=shared.address+1024||c.r5.u32!=platform_command_bytes)throw h::Error(h::Fault::argument,"V2 command identity");
    scope.admit_range(c.r4.u32,c.r5.u32);
    std::array<p::Scene3D::GuestSpan,6> matrices;for(size_t i=0;i<matrices.size();++i)matrices[i]={shared,shared.address+4096+uint32_t(i*64),64};scene.submit(space,{shared,c.r4.u32,c.r5.u32},matrices);counts.draws+=scene.snapshot()->draws.size();result=uint32_t(scene.last_frame());break;
   }
   case 5:{try{mixer.start();counts.audio_available=true;result=1;}catch(const p::AudioError&e){counts.audio_native_error=e.native_code;counts.audio_available=false;}break;}
   case 6:{
    if(!counts.audio_available){result=3;break;}
    try{auto pressed=space.load<uint32_t>(shared.address+36);
     if(pressed&p::Input::a){if(voices[1])mixer.play(*voices[1],32768,false);else ++counts.deferred_actions;}
     if((pressed&p::Input::b)&&voices[0]){if(ambient_playing)mixer.stop(*voices[0]);else mixer.play(*voices[0],8192,true);ambient_playing=!ambient_playing;}
     if((pressed&p::Input::x)&&voices[0])mixer.volume(*voices[0],16384);
     if((pressed&p::Input::y)&&voices[0])mixer.volume(*voices[0],4096);
     auto audio=mixer.state();result=mixer.pump(800)?1:0;
     if(result&&!audio.queued)mixer.pump(800); // Bounded two-quantum cold/resume fill.
    }catch(const p::AudioError&e){counts.audio_native_error=e.native_code;counts.audio_available=false;throw;}break;
   }
   default:throw h::Error(h::Fault::unsupported,"unknown original V2 service");
  }
  c.r3.u64=result;++counts.service_completed;
 }
 void execute(bool first){
  if(counts.closed||counts.poisoned||counts.stopped||first==counts.initialized)throw h::Error(h::Fault::stale,"V2 execution lifecycle");
  guest.context.r3.u64=shared.address;guest.context.lr=platform_control.exit;
  try{h::ProbeControl::Local stop(control);code.startup(guest);h::Scope scope(guest,{demo_functions,demo_function_count},module.identity,nullptr,{},&platform_control);code.lend(scope);scope.lend(shared);Frame frame(*this,scope);
   if(first)code.invoke(scope,platform_initialize);else scope.invoke(platform_update);
   if(guest.context.r1.u64!=initial_sp||guest.context.lr!=platform_control.exit||guest.context.r31.u64!=saved_r31)throw h::Error(h::Fault::unsupported,"V2 return register contract");
   counts.guest_calls+=scope.calls;counts.reads+=scope.reads;counts.writes+=scope.writes;counts.hooks+=scope.polls;counts.result=guest.context.r3.u32;if(first)counts.initialized=true;else ++counts.ticks;
  }catch(...){if(control.cancelled())counts.stopped=true;else counts.poisoned=true;throw;}
 }
 void close(){
  if(counts.closed)return;
  try{mixer.close();}catch(const p::AudioError&e){counts.audio_native_error=e.native_code;++counts.cleanup_errors;mixer.shutdown();}
  streaming.shutdown();for(size_t i=0;i<assets.size();++i)release_asset(i);scene.shutdown();guest.cleanup();slots.release(slot);if(shared.id){space.release(shared);shared={};}if(!module.close())throw h::Error(h::Fault::ownership,"V2 module still leased");image.unload();counts.closed=true;
 }
 ~Impl(){try{close();}catch(...){std::terminate();}}
 static void point(uint32_t pc){auto*f=Frame::current;if(!f)throw h::Error(h::Fault::no_thread,"V2 active frame absent");f->scope.point(pc);if(pc==platform_gate)f->demo.bridge(f->scope);}
};
Demo::Demo(p::Scene3D&s,p::Input&i,p::Mixer&m,r::Service&r):impl_(std::make_unique<Impl>(s,i,m,r)){}
Demo::~Demo()=default;void Demo::initialize(){impl_->execute(true);}void Demo::tick(){impl_->execute(false);}void Demo::shutdown(){impl_->close();}bool Demo::request_stop(){return impl_->control.request_cancel();}Stats Demo::stats()const{return impl_->counts;}void Demo::observer(std::function<void(uint32_t,h::Space&,h::Space::Handle)>fn){impl_->observe=std::move(fn);}h::Space& Demo::memory(){return impl_->space;}h::Space::Handle Demo::shared(){return impl_->shared;}PPCContext& Demo::context(){return impl_->guest.context;}void poll(uint32_t pc){Demo::Impl::point(pc);}
}
void platform_poll(uint32_t pc){brutal::platform_v2::poll(pc);}
