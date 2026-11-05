// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "tests/platform-demo/demo.h"
#include "program.h"
#include <cmath>
#include <optional>
using namespace brutal::horizon;
namespace p=brutal::horizon::platform;
namespace r=brutal::horizon::resources;
namespace ni=brutal::horizon::native_image;
namespace brutal::platform_demo {
std::array<r::Manifest,1> manifest(){return {{{"demo.txt",36,ni::Digest{0x36,0x4d,0xdc,0x10,0xa1,0xf9,0x71,0x52,0xa9,0xff,0xb5,0xa8,0x75,0xee,0x8f,0xfa,0xf0,0x13,0x27,0xa2,0xf5,0x63,0xff,0x23,0xb4,0xa9,0xc0,0x7d,0xdc,0xd6,0xa5,0x41}}}};}
struct Demo::Impl {
 p::Graphics& graphics;p::Input& input;p::AudioService& audio;r::Service& resources;
 Space space{2*1024*1024};ModuleLife module{0x504c4154};Slots slots;Slots::Token slot{slots.allocate()};
 ni::Image image{space,platform_plan(),platform_bytes,platform_identity.plan};
 ni::Binding code{image,space,module,platform_identity,platform_identity,{demo_functions,demo_function_count},platform_bounds,platform_control,{demo_functions,demo_function_count},ni::ImportPolicy::no_function_imports};
 GuestThread guest{space,module,slots,slot,3,space.read(platform_plan().tls_address,16)};
 Space::Handle shared{},pcm{};std::optional<r::Service::Handle> file;std::optional<p::AudioService::Token> stream;Stats counts;std::function<void(uint32_t,Space&,Space::Handle)> observe;bool initialized{};uint64_t initial_sp{},r31{};
 struct Frame {Impl& demo;Scope& scope;inline static thread_local Frame* current{};Frame(Impl&d,Scope&s):demo(d),scope(s){if(current)throw Error(Fault::no_thread,"nested platform bridge");current=this;}~Frame(){current=nullptr;}};
 Impl(p::Graphics&g,p::Input&i,p::AudioService&a,r::Service&res):graphics(g),input(i),audio(a),resources(res){
  try{
   shared=space.allocate(16384,0x60000000);pcm=space.allocate(19200,0x61000000);
   const std::string path="demo:/demo.txt";space.write(shared.address+256,{reinterpret_cast<const std::byte*>(path.data()),path.size()});
   space.store<uint32_t>(shared.address+64,80);space.store<uint32_t>(shared.address+68,60);
   std::array<std::byte,64> checker{};for(unsigned y=0;y<4;++y)for(unsigned x=0;x<4;++x){auto offset=(y*4+x)*4;bool light=(x+y)%2==0;checker[offset]=std::byte(light?240:30);checker[offset+1]=std::byte(light?180:80);checker[offset+2]=std::byte(light?30:180);checker[offset+3]=std::byte{255};}space.write(shared.address+2048,checker);
   std::vector<std::byte> tone(19200);for(unsigned n=0;n<4800;++n){int16_t sample=int16_t(2048*std::sin(6.283185307179586*440.0*double(n)/48000.0));for(unsigned channel=0;channel<2;++channel){tone[4*n+2*channel]=std::byte(uint16_t(sample)&255);tone[4*n+2*channel+1]=std::byte(uint16_t(sample)>>8);}}space.write(pcm.address,tone);
   initial_sp=guest.context.r1.u64;r31=0x1122334455667788ull;guest.context.r31.u64=r31;
  }catch(...){if(pcm.id)space.release(pcm);if(shared.id)space.release(shared);throw;}
 }
 void bridge(Scope& scope){
  auto& c=guest.context;scope.import_target(platform_gate);if(!scope.belongs_to(module,space))throw Error(Fault::ownership,"platform module boundary");++counts.service_entered;
  const uint32_t op=c.r3.u32;uint32_t result=0;
  if(observe)observe(op,space,shared);
  auto exact=[&](uint32_t address,uint32_t size,Access access){scope.admit_range(address,size);space.validate_owned_window(shared,address,size,access);};
  switch(op){
   case 1:{
    if(c.r4.u32!=shared.address)throw Error(Fault::argument,"input target identity");
    auto state=input.snapshot();if(state.frame!=counts.frames+1)throw Error(Fault::stale,"input frame association");input.write_snapshot(space,shared,shared.address+128);
    space.store<uint32_t>(shared.address,uint32_t(state.left_x/8192));space.store<uint32_t>(shared.address+4,uint32_t(-state.left_y/8192));space.store<uint32_t>(shared.address+8,(state.pressed&p::Input::a)?0x00200000:0);space.store<uint32_t>(shared.address+12,state.connected?1:0);space.store<uint32_t>(shared.address+32,((state.pressed&p::Input::a)||counts.frames==0)?1:0);result=state.connected?1:0;break;
   }
   case 2:{if(file)throw Error(Fault::ownership,"resource already open");auto address=c.r4.u32;auto length=c.r5.u32;if(!length||length>240)throw Error(Fault::bounds,"resource guest path size");exact(address,length,Access::read);auto bytes=space.read(address,length);std::string path(reinterpret_cast<const char*>(bytes.data()),bytes.size());file=resources.open(path);result=1;break;}
   case 3:{if(c.r4.u32!=1||!file)throw Error(Fault::stale,"unknown synthetic resource handle");exact(c.r5.u32,c.r6.u32,Access::write);result=resources.read(*file,space,shared,c.r5.u32,c.r6.u32);counts.resource_bytes+=result;break;}
   case 4:{if(c.r4.u32!=1||!file)throw Error(Fault::stale,"unknown resource close");resources.close(*file);file.reset();result=1;break;}
   case 5:{try{stream=audio.start();counts.audio_available=true;result=1;}catch(const p::AudioError&e){counts.audio_native_error=e.native_code;counts.audio_available=false;result=0;}break;}
   case 6:{
    try{
    if(!stream){result=3;break;}auto state=audio.query(*stream);counts.audio_completed=state.completed;
    if(!c.r6.u32){result=0;break;}if(state.queued==p::audio_slots){result=2;break;}
    scope.admit_range(c.r4.u32,c.r5.u32);audio.submit(*stream,space,pcm,c.r4.u32,c.r5.u32);++counts.audio_submitted;result=1;break;
    }catch(const p::AudioError&e){counts.audio_native_error=e.native_code;counts.audio_available=false;throw;}
   }
   case 7:{exact(c.r4.u32,c.r5.u32,Access::read);std::array<p::Graphics::Upload,1> uploads{{{shared,shared.address+2048,64}}};graphics.submit(space,shared,c.r4.u32,c.r5.u32,uploads);result=uint32_t(graphics.last_frame());break;}
   default:throw Error(Fault::unsupported,"unknown original platform service");
  }
  c.r3.u64=result;++counts.service_completed;
 }
 void execute(bool first){
  if(counts.closed||counts.poisoned||first==initialized)throw Error(Fault::stale,"platform guest lifecycle");
  guest.context.r3.u64=shared.address;guest.context.lr=platform_control.exit;
  try{
   code.startup(guest);Scope scope(guest,{demo_functions,demo_function_count},module.identity,nullptr,{},&platform_control);code.lend(scope);scope.lend(shared);scope.lend(pcm);Frame frame(*this,scope);
   if(first)code.invoke(scope,platform_initialize);else scope.invoke(platform_update);
   if(guest.context.r1.u64!=initial_sp||guest.context.lr!=platform_control.exit||guest.context.r31.u64!=r31)throw Error(Fault::unsupported,"synthetic platform return contract");
   counts.guest_calls+=scope.calls;counts.reads+=scope.reads;counts.writes+=scope.writes;counts.hooks+=scope.polls;counts.result=guest.context.r3.u32;
   if(first)initialized=true;else ++counts.frames;
  }catch(...){counts.poisoned=true;throw;}
 }
 void close(){
  if(counts.closed)return;
  if(file){resources.close(*file);file.reset();}
  if(stream){try{auto state=audio.query(*stream);counts.audio_completed=state.completed;audio.stop(*stream);}catch(const p::AudioError&e){counts.audio_native_error=e.native_code;++counts.cleanup_errors;audio.shutdown();}stream.reset();}
  guest.cleanup();slots.release(slot);if(pcm.id){space.release(pcm);pcm={};}if(shared.id){space.release(shared);shared={};}
  if(!module.close())throw Error(Fault::ownership,"platform module still leased");
  image.unload();counts.closed=true;
 }
 ~Impl(){try{close();}catch(...){std::terminate();}}
 static void point(uint32_t pc){auto*f=Frame::current;if(!f)throw Error(Fault::no_thread,"missing platform frame");f->scope.point(pc);if(pc==platform_gate)f->demo.bridge(f->scope);}
};
Demo::Demo(p::Graphics&g,p::Input&i,p::AudioService&a,r::Service&res):impl_(std::make_unique<Impl>(g,i,a,res)){}
Demo::~Demo()=default;void Demo::observer(std::function<void(uint32_t,Space&,Space::Handle)> fn){impl_->observe=std::move(fn);}void Demo::initialize(){impl_->execute(true);}void Demo::frame(){impl_->execute(false);}void Demo::shutdown(){impl_->close();}Stats Demo::stats()const{return impl_->counts;}
Space& Demo::memory(){return impl_->space;}Space::Handle Demo::shared(){return impl_->shared;}PPCContext& Demo::context(){return impl_->guest.context;}
void poll(uint32_t pc){Demo::Impl::point(pc);}
}
void platform_poll(uint32_t pc){brutal::platform_demo::poll(pc);}
