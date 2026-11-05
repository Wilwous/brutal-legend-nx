// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "tests/platform-v2/demo.h"
#include "tests/platform-v2/ui.h"
#include "runtime/horizon/scene3d_gl.h"
#include "runtime/horizon/platform_audio_libnx.h"
#include "runtime/horizon/platform_lifecycle.h"
#include "runtime/horizon/frame_timing.h"
#include "tests/switch-horizon/result_ui.h"
#include "assets_contract.h"
#include <cstdio>
namespace h=brutal::horizon;namespace p=h::platform;namespace r=h::resources;namespace demo=brutal::platform_v2;
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=0;}
namespace {
uint64_t now(){return armTicksToNs(armGetSystemTick());}
class Application {
 std::unique_ptr<r::Service> resources_;std::unique_ptr<p::Scene3DGl> gl_;std::unique_ptr<p::Scene3D> scene_;p::Input input_;
 p::LibnxAudioBackend backend_;std::unique_ptr<p::AudioService> audio_;std::unique_ptr<p::Mixer> mixer_;std::unique_ptr<demo::Demo> guest_;
 p::FrameTiming timing_;std::vector<std::byte> hud_;uint64_t hud_time_{},sim_tick_{},last_render_ns_{};bool romfs_{};p::Lifecycle life_;
 void hud(uint64_t time){
  if(hud_time_&&time-hud_time_<250000000)return;
  hud_.assign(640*90*4,std::byte{0});for(size_t i=3;i<hud_.size();i+=4)hud_[i]=std::byte{200};
  auto status=guest_->stats();auto audio=mixer_->state();auto pace=timing_.stats();auto memory=h::Space::totals();char line[110];
  auto text=[&](unsigned row,const char*value){demo::ui::text(hud_,640,90,6,3+row*10,value);};text(0,"BRUTAL NX - SYNTHETIC 3D V2 - T105");
  std::snprintf(line,sizeof(line),"FRAME %llu TICK %llu PPC %llu",(unsigned long long)frames,(unsigned long long)status.ticks,(unsigned long long)status.guest_calls);text(1,line);
  std::snprintf(line,sizeof(line),"FRAME US %llu UPDATE US %llu LAST RENDER US %llu",(unsigned long long)(pace.frame_ns/1000),(unsigned long long)(pace.update_ns/1000),(unsigned long long)(last_render_ns_/1000));text(2,line);
  std::snprintf(line,sizeof(line),"DRAW %llu INPUT %s RES %u OF 39240 READY %u OF 4",(unsigned long long)gl_->draws(),input_.snapshot().connected?"CONNECTED":"DISCONNECTED",status.resource_bytes,status.assets_ready);text(3,line);
  std::snprintf(line,sizeof(line),"AUDIO %s VOICES %u QUEUED %u UNDERFLOW %llu",!status.audio_available?"UNAVAILABLE":status.assets_ready<4?"LOADING":"READY",audio.active_voices,audio.queued,(unsigned long long)audio.underflows);text(4,line);
  std::snprintf(line,sizeof(line),"ALLOC %zu LEASES %zu CPU ASSET %zu GPU PAYLOAD %zu",memory.first,memory.second,scene_->mesh_bytes()+scene_->texture_bytes()+size_t(audio.source_bytes),gl_->mesh_bytes()+gl_->texture_bytes());text(5,line);
  std::snprintf(line,sizeof(line),"MISSED STEPS %llu ERRORS %llu",(unsigned long long)pace.missed_updates,(unsigned long long)audio.errors);text(6,line);text(7,"L MOVE - R CAMERA - A ACTION - B AMBIENT - X Y VOLUME");text(8,"PLUS STOPS - ORIGINAL SYNTHETIC PPC ONLY");gl_->set_overlay(hud_,640,90);hud_time_=time;
 }
 public:
 demo::Stats final{};p::Mixer::State final_audio{};p::FrameTiming::Stats final_time{};uint64_t frames{},gpu_draws{},gpu_uploads{};unsigned gl_cleanup{};uint32_t observed_audio_error{};bool failed{},closed{};char error[160]{},driver[128]{};
 void failure(const std::exception&e){failed=true;std::snprintf(error,sizeof(error),"%s%s%s",e.what(),gl_&&!gl_->last_error().empty()?": ":"",gl_?gl_->last_error().c_str():"");}
 void initialize(){
  life_.activate("romfs",true,[&]{if(R_FAILED(romfsInit()))throw std::runtime_error("V2 ROMFS initialization failed");romfs_=true;return true;},[&]{if(romfs_){romfsExit();romfs_=false;}});
  life_.activate("resources",true,[&]{auto declared=v2_manifest();resources_=std::make_unique<r::Service>("romfs:/resources",declared);return true;},[&]{if(resources_)resources_->shutdown();resources_.reset();});
  life_.activate("gpu",true,[&]{gl_=std::make_unique<p::Scene3DGl>();gl_->initialize();std::snprintf(driver,sizeof(driver),"%s",gl_->driver().c_str());return true;},[&]{if(gl_){gpu_draws=gl_->draws();gpu_uploads=gl_->uploads();gl_->shutdown();gl_cleanup=gl_->cleanup_errors();}gl_.reset();});
  life_.activate("scene",true,[&]{scene_=std::make_unique<p::Scene3D>();return true;},[&]{if(scene_)scene_->shutdown();scene_.reset();});
  life_.activate("audio",true,[&]{audio_=std::make_unique<p::AudioService>(backend_);return true;},[&]{if(audio_)audio_->shutdown();audio_.reset();});
  life_.activate("mixer",true,[&]{mixer_=std::make_unique<p::Mixer>(*audio_);return true;},[&]{if(mixer_)mixer_->shutdown();mixer_.reset();});
  life_.activate("module",true,[&]{guest_=std::make_unique<demo::Demo>(*scene_,input_,*mixer_,*resources_);guest_->initialize();return true;},[&]{if(guest_){guest_->request_stop();guest_->shutdown();final=guest_->stats();final_audio=mixer_->state();}guest_.reset();});
 }
 void frame(PadState&pad){
  auto start=now();auto tick=timing_.begin_frame(start);auto left=padGetStickPos(&pad,0);auto right=padGetStickPos(&pad,1);p::Input::RawController raw{0,1,padIsConnected(&pad),demo::ui::buttons(padGetButtons(&pad)),left.x,left.y,right.x,right.y};
  for(uint32_t i=0;i<tick.updates;++i){if(sim_tick_==UINT32_MAX)throw std::runtime_error("V2 simulation counter exhausted");input_.update(++sim_tick_,std::span(&raw,1));guest_->tick();}
  timing_.record_update(start,now());auto render_begin=now();if(guest_->stats().scene_ready)gl_->render(*scene_->snapshot());else gl_->clear_frame(0x142030ff);hud(render_begin);gl_->draw_overlay();gl_->present();timing_.record_render(render_begin,now());last_render_ns_=timing_.stats().render_ns;++frames;
 }
 void shutdown(){if(closed)return;final_time=timing_.stats();life_.shutdown();hud_.clear();closed=true;if(life_.cleanup_failures()||gl_cleanup||final.cleanup_errors)failed=true;auto memory=h::Space::totals();auto resources=r::Service::totals();if(memory.first||memory.second||resources.first||resources.second)failed=true;}
 ~Application(){shutdown();}
};
struct ExitUi {PadState*pad;bool running(){return appletMainLoop();}void update_input(){padUpdate(pad);}bool plus_pressed(){return padGetButtonsDown(pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16000000);}};
}
int main(){
 Application app;PadState pad{};padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);bool applet_exit=false;
 try{app.initialize();while(true){if(!appletMainLoop()){applet_exit=true;break;}padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;app.frame(pad);}}
 catch(const p::AudioError&e){app.observed_audio_error=e.native_code;app.failure(e);}
 catch(const std::exception&e){app.failure(e);}
 app.shutdown();if(app.observed_audio_error){app.final.audio_native_error=app.observed_audio_error;app.final.audio_available=false;}if(applet_exit)return app.failed?1:0;
 auto*console=consoleInit(nullptr);if(!console||!console->consoleInitialised)return 2;
 std::puts("BRUTAL NX - SYNTHETIC 3D PLATFORM DEMO V2 / T105\nOriginal PPC/assets only. NOT BRUTAL LEGEND.");
 std::printf("Renderer: %s\nFrames=%llu ticks=%llu result=%u\nPPC calls=%llu hooks=%llu checked R/W=%llu/%llu\nServices entered/completed=%llu/%llu\nGPU draws=%llu uploads=%llu\nResources delivered=%u assets=%u/4\n",app.driver,(unsigned long long)app.frames,(unsigned long long)app.final.ticks,app.final.result,(unsigned long long)app.final.guest_calls,(unsigned long long)app.final.hooks,(unsigned long long)app.final.reads,(unsigned long long)app.final.writes,(unsigned long long)app.final.service_entered,(unsigned long long)app.final.service_completed,(unsigned long long)app.gpu_draws,(unsigned long long)app.gpu_uploads,app.final.resource_bytes,app.final.assets_ready);
 std::printf("Audio available=%s native_error=%08x\nSent/done/cancelled=%llu/%llu/%llu underflows=%llu\nVoices=%u sources=%u queued=%u\nFrame/update/render us=%llu/%llu/%llu missed_steps=%llu\nGPU cleanup errors=%u\n",app.final.audio_available?"yes":"NO",app.final.audio_native_error,(unsigned long long)app.final_audio.submitted,(unsigned long long)app.final_audio.completed,(unsigned long long)app.final_audio.cancelled,(unsigned long long)app.final_audio.underflows,app.final_audio.active_voices,app.final_audio.loaded_sources,app.final_audio.queued,(unsigned long long)(app.final_time.frame_ns/1000),(unsigned long long)(app.final_time.update_ns/1000),(unsigned long long)(app.final_time.render_ns/1000),(unsigned long long)app.final_time.missed_updates,app.gl_cleanup);
 auto memory=h::Space::totals();auto resources=r::Service::totals();std::printf("Cleanup allocations=%zu leases=%zu handles=%zu resource_bytes=%zu\nStatus=%s\n",memory.first,memory.second,resources.first,resources.second,app.failed?"FAIL":"COMPLETED");if(app.error[0])std::printf("Error: %s\n",app.error);
 std::puts("Photograph results. Release PLUS then press PLUS to hbmenu.\nVisual/input/audio hardware acceptance needs actual observation.");consoleUpdate(nullptr);ExitUi ui{&pad};brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return app.failed?1:0;
}
