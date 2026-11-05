// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "program.h"
#include "runtime/horizon/sdk_services.h"
#include "runtime/horizon/format_translation.h"
#include "runtime/horizon/scene3d.h"
#include "runtime/horizon/audio_mixer.h"
#include <iostream>
namespace h=brutal::horizon;namespace c=h::compatibility;namespace p=h::platform;namespace ni=h::native_image;
struct Workload {
 h::Space space{4*1024*1024};h::ModuleLife module{0x504c4154};h::Slots slots;h::Slots::Token slot{slots.allocate()};
 ni::Image image{space,platform_plan(),platform_bytes,platform_identity.plan};
 ni::Binding code{image,space,module,platform_identity,platform_identity,{demo_functions,demo_function_count},platform_bounds,platform_control,{demo_functions,demo_function_count},ni::ImportPolicy::no_function_imports};
 h::GuestThread guest{space,module,slots,slot,3,space.read(platform_plan().tls_address,16)};
 c::SdkServices sdk{space,module,slots};c::SdkServices::Thread thread{sdk.register_thread(guest)};
 h::Space::Handle shared{space.allocate(16384,0x60000000)};p::Scene3D scene;p::HostAudioBackend backend;p::AudioService audio{backend};p::Mixer mixer{audio};h::ProbeControl control;
 uint64_t sp{guest.context.r1.u64},reads{},writes{},polls{},dispatches{},native_calls{};bool closed{};std::function<void(uint32_t)> observe;
 inline static thread_local Workload* current{};h::Scope* scope{};
 Workload(){
  guest.context.r31.u64=0x1122334455667788ull;guest.context.r20.u64=0x8877665544332211ull;guest.context.f14.f64=3.25;guest.context.v20.u64[0]=0x0102030405060708ull;guest.context.v20.u64[1]=0x090a0b0c0d0e0f00ull;
  std::array<std::byte,12> indices{};indices[4]=std::byte{1};indices[8]=std::byte{2};space.write(shared.address+2048,indices);
  const std::array<float,15> vertices{0,0,0,0,0,1,0,0,1,0,0,1,0,0,1};std::vector<std::byte> data(60);for(unsigned i=0;i<15;++i){uint32_t bits=std::bit_cast<uint32_t>(vertices[i]);for(unsigned j=0;j<4;++j)data[4*i+j]=std::byte(bits>>(8*j));}space.write(shared.address+2176,data);
  for(unsigned i=0;i<256;++i){space.store<uint32_t>(shared.address+8192+i*4,0x3f000000);space.store<uint32_t>(shared.address+8192+1024+i*4,0xbf000000);}
 }
 void graphics(){
  auto&ctx=guest.context;if(ctx.r3.u32!=shared.address+2048||ctx.r4.u32!=12)throw h::Error(h::Fault::argument,"explicit synthetic graphics span");scope->admit_range(ctx.r3.u32,ctx.r4.u32);scope->admit_range(shared.address+2176,60);
  auto index=c::indices_be16(space,{shared,ctx.r3.u32,ctx.r4.u32},3,4,3,c::Endian::none);auto vertex=c::vertices_be(space,{shared,shared.address+2176,60},3,c::Endian::none);
  space.write(shared.address+3072,vertex);space.write(shared.address+3200,index);space.store<uint32_t>(shared.address+3328,0xffffffff);
  auto mesh=scene.upload_mesh(space,{shared,shared.address+3072,60},3,{shared,shared.address+3200,6},3);auto texture=scene.upload_texture(space,{shared,shared.address+3328,4},1,1,4);
  auto identity=p::scene_math::identity();for(unsigned i=0;i<16;++i)space.store<uint32_t>(shared.address+4096+i*4,std::bit_cast<uint32_t>(identity[i]));
  const std::array<std::array<uint32_t,8>,7> commands{{{1,0,1,0},{3,0,0},{2,uint32_t(mesh.owner>>32),uint32_t(mesh.owner),mesh.slot,mesh.generation},{5,uint32_t(texture.owner>>32),uint32_t(texture.owner),texture.slot,texture.generation},{4,0},{6,0,3},{7}}};for(unsigned i=0;i<7;++i)for(unsigned j=0;j<8;++j)space.store<uint32_t>(shared.address+4608+i*32+j*4,commands[i][j]);
  std::array<p::Scene3D::GuestSpan,1> matrices{{{shared,shared.address+4096,64}}};scene.submit(space,{shared,shared.address+4608,224},matrices);ctx.r3.u64=scene.snapshot()->draws.size();
 }
 void sound(){auto&ctx=guest.context;if(ctx.r3.u32!=shared.address+8192||ctx.r4.u32!=6144)throw h::Error(h::Fault::argument,"explicit synthetic audio span");scope->admit_range(ctx.r3.u32,ctx.r4.u32);auto pcm=c::front_pcm_s16(space,{shared,ctx.r3.u32,ctx.r4.u32});space.write(shared.address+12288,pcm);mixer.start();auto voice=mixer.load(space,shared,shared.address+12288,1024);mixer.play(voice);if(!mixer.pump(256))throw h::Error(h::Fault::unsupported,"PCM queue failed");ctx.r3.u64=mixer.state().submitted;}
 void point(uint32_t pc){scope->point(pc);if(pc!=platform_gate)return;scope->import_target(pc);scope->validate_context();if(observe)observe(guest.context.r11.u32);switch(guest.context.r11.u32){case 0x10000:graphics();++native_calls;break;case 0x10001:sound();++native_calls;break;default:sdk.invoke(thread,guest.context,shared);}}
 void execute(bool first){
  if(current){throw h::Error(h::Fault::no_thread,"nested compatibility workload");}guest.context.r3.u64=shared.address;guest.context.lr=platform_control.exit;
  h::ProbeControl::Local stop(control);code.startup(guest);h::Scope active(guest,{demo_functions,demo_function_count},module.identity,nullptr,{},&platform_control);code.lend(active);active.lend(shared);scope=&active;current=this;
  try{if(first)code.invoke(active,platform_initialize);else active.invoke(platform_update);reads+=active.reads;writes+=active.writes;polls+=active.polls;dispatches+=active.calls;current=nullptr;scope=nullptr;}
  catch(...){current=nullptr;scope=nullptr;throw;}
 }
 void close(){if(closed)return;mixer.close();scene.shutdown();sdk.close();guest.cleanup();slots.release(slot);space.release(shared);if(!module.close())throw h::Error(h::Fault::ownership,"compatibility module still leased");image.unload();closed=true;}
 ~Workload(){try{close();}catch(...){std::terminate();}}
};
void platform_poll(uint32_t pc){if(!Workload::current)throw h::Error(h::Fault::no_thread,"no compatibility workload");Workload::current->point(pc);}
int main(){try{
 unsigned positive=0,refusals=0,bytes=0;uint64_t reads=0,writes=0,polls=0,dispatches=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("integrated compatibility assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const h::Error&){++refusals;return;}throw std::runtime_error("missing integration refusal");};
 {Workload w;w.execute(true);yes(w.guest.context.r3.u64==1);w.execute(false);yes(w.guest.context.r3.u64==0x89abcdef);const std::array<uint32_t,7> expected{1,1,0x89abcdef,w.shared.address+1024,1,1,1};for(unsigned i=0;i<7;++i)yes(w.space.load<uint32_t>(w.shared.address+i*4)==expected[i]);for(unsigned i=0;i<16;++i){if(w.space.read(w.shared.address+1024+i,1)[0]!=std::byte(0x12345678u>>(24-8*(i%4))))throw std::runtime_error("fill byte oracle");++bytes;}
  yes(w.guest.context.r1.u64==w.sp&&w.guest.context.lr==platform_control.exit);yes(w.guest.context.r31.u64==0x1122334455667788ull&&w.guest.context.r20.u64==0x8877665544332211ull);yes(w.guest.context.f14.f64==3.25&&w.guest.context.v20.u64[0]==0x0102030405060708ull&&w.guest.context.v20.u64[1]==0x090a0b0c0d0e0f00ull);yes(w.guest.get_tls()==3&&w.space.load<uint32_t>(w.guest.tls.address+36)==0);
  yes(w.sdk.entered==5&&w.sdk.completed==5&&w.native_calls==2);auto frame=w.scene.snapshot();yes(frame->draws.size()==1&&frame->draws[0].mesh->vertices[1].x==1&&frame->draws[0].mesh->vertices[2].y==1&&frame->draws[0].mesh->indices==std::vector<uint16_t>({0,1,2}));frame.reset();auto queued=w.backend.queued_bytes(0);for(unsigned i=0;i<256;++i){if(queued[i*4]!=std::byte{0}||queued[i*4+1]!=std::byte{0x40}||queued[i*4+2]!=std::byte{0}||queued[i*4+3]!=std::byte{0xc0})throw std::runtime_error("PCM byte oracle");bytes+=4;}w.backend.step();yes(w.mixer.state().completed==1);reads=w.reads;writes=w.writes;polls=w.polls;dispatches=w.dispatches;w.close();yes(!w.backend.active()&&h::Space::totals()==std::pair<size_t,size_t>{0,0});}
 {Workload w;auto entries=h::Scope::guest_entries.load();w.guest.context.r13.u64=1;no([&]{w.execute(true);});yes(h::Scope::guest_entries==entries&&w.sdk.entered==0);w.guest.context.r13.u64=w.guest.pcr.address;w.close();}
 {Workload w;w.execute(true);auto before=w.space.read(w.shared.address+1024,16);w.space.protect(w.shared,h::Permission::read);no([&]{w.execute(false);});yes(w.space.read(w.shared.address+1024,16)==before);w.close();}
 {Workload w;w.observe=[&](uint32_t){w.guest.context.r11.u64=0xffffffff;};no([&]{w.execute(true);});yes(w.sdk.entered==1&&w.sdk.completed==0&&w.space.load<uint32_t>(w.shared.address)==0);w.close();}
 {Workload w;w.control.request_cancel();auto entries=h::Scope::guest_entries.load();no([&]{w.execute(true);});yes(h::Scope::guest_entries==entries&&w.sdk.entered==0);w.close();}
 {Workload w;w.execute(true);w.space.store<uint32_t>(w.shared.address+2048,0x000000ff);no([&]{w.execute(false);});yes(w.scene.last_frame()==0&&!w.backend.active());w.close();}
 yes(h::Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<bytes<<",\"failures\":0,\"result\":2309737967,\"sdk_entered\":5,\"sdk_completed\":5,\"native_translation_calls\":2,\"checked_reads\":"<<reads<<",\"checked_writes\":"<<writes<<",\"hook_polls\":"<<polls<<",\"dispatches\":"<<dispatches<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
