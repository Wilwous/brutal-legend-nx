// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "tests/platform-demo/demo.h"
#include "runtime/horizon/platform_audio_libnx.h"
#include "runtime/horizon/platform_lifecycle.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
#include <cstring>
namespace p=brutal::horizon::platform;namespace r=brutal::horizon::resources;
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=32*1024*1024;}
namespace {
// Independently authored compact glyphs for the live native overlay.
const uint8_t* glyph(char c){
 static constexpr uint8_t letters[][7]={{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
 static constexpr uint8_t digits[][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
 static constexpr uint8_t blank[7]{},dash[7]{0,0,0,31,0,0,0},colon[7]{0,4,4,0,4,4,0};
 if(c>='A'&&c<='Z')return letters[c-'A'];
 if(c>='0'&&c<='9')return digits[c-'0'];
 return c=='-'?dash:c==':'?colon:blank;
}
void text(uint8_t*fb,uint32_t stride,uint32_t x,uint32_t y,const char*message){
 for(;*message;++message,x+=18){if(x+15>1280)return;auto bits=glyph(*message);for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)if(bits[row]&(1u<<(4-col)))for(unsigned sy=0;sy<3;++sy)for(unsigned sx=0;sx<3;++sx){auto offset=size_t(y+row*3+sy)*stride+(x+col*3+sx)*4;fb[offset]=230;fb[offset+1]=240;fb[offset+2]=255;fb[offset+3]=255;}}
}
uint64_t buttons(uint64_t hid){uint64_t result=0;const std::pair<uint64_t,uint64_t> map[]={{HidNpadButton_A,p::Input::a},{HidNpadButton_B,p::Input::b},{HidNpadButton_X,p::Input::x},{HidNpadButton_Y,p::Input::y},{HidNpadButton_Up,p::Input::up},{HidNpadButton_Down,p::Input::down},{HidNpadButton_Left,p::Input::left},{HidNpadButton_Right,p::Input::right},{HidNpadButton_L,p::Input::l},{HidNpadButton_R,p::Input::r},{HidNpadButton_ZL,p::Input::zl},{HidNpadButton_ZR,p::Input::zr},{HidNpadButton_Plus,p::Input::plus},{HidNpadButton_Minus,p::Input::minus},{HidNpadButton_StickL,p::Input::stick_l},{HidNpadButton_StickR,p::Input::stick_r}};for(auto [native,portable]:map)if(hid&native)result|=portable;return result;}
class Application {
 std::unique_ptr<r::Service> resources_;std::unique_ptr<p::Graphics> graphics_;p::Input input_;
 p::LibnxAudioBackend backend_;std::unique_ptr<p::AudioService> audio_;std::unique_ptr<brutal::platform_demo::Demo> demo_;
 Framebuffer framebuffer_{};bool framebuffer_live_{},romfs_live_{};p::Lifecycle lifecycle_; // destroyed before captured members
 public:
 brutal::platform_demo::Stats final{};bool stopped{},failed{};char error[128]{};uint64_t last_frame_ns{};
 void initialize(){
  lifecycle_.activate("romfs",true,[&]{Result rc=romfsInit();if(R_FAILED(rc))throw std::runtime_error("romfs initialization failed");romfs_live_=true;return true;},[&]{if(romfs_live_){romfsExit();romfs_live_=false;}});
  lifecycle_.activate("resources",true,[&]{auto declared=brutal::platform_demo::manifest();resources_=std::make_unique<r::Service>("romfs:/resources",declared);return true;},[&]{if(resources_)resources_->shutdown();resources_.reset();});
  lifecycle_.activate("framebuffer",true,[&]{Result rc=framebufferCreate(&framebuffer_,nwindowGetDefault(),1280,720,PIXEL_FORMAT_RGBA_8888,2);if(R_FAILED(rc))throw std::runtime_error("framebuffer create failed");framebuffer_live_=true;rc=framebufferMakeLinear(&framebuffer_);if(R_FAILED(rc))throw std::runtime_error("linear framebuffer failed");return true;},[&]{if(framebuffer_live_){framebufferClose(&framebuffer_);framebuffer_live_=false;}});
  lifecycle_.activate("graphics",true,[&]{graphics_=std::make_unique<p::Graphics>();return true;},[&]{graphics_.reset();});
  lifecycle_.activate("audio",true,[&]{audio_=std::make_unique<p::AudioService>(backend_);return true;},[&]{if(audio_)audio_->shutdown();audio_.reset();});
  lifecycle_.activate("module",true,[&]{demo_=std::make_unique<brutal::platform_demo::Demo>(*graphics_,input_,*audio_,*resources_);demo_->initialize();return true;},[&]{if(demo_){demo_->shutdown();final=demo_->stats();}demo_.reset();});
 }
 void frame(PadState&pad,uint64_t number){
  auto start=armGetSystemTick();auto left=padGetStickPos(&pad,0);auto right=padGetStickPos(&pad,1);p::Input::RawController raw{0,1,padIsConnected(&pad),buttons(padGetButtons(&pad)),left.x,left.y,right.x,right.y};input_.update(number,std::span(&raw,1));demo_->frame();
  u32 stride=0;auto*fb=static_cast<uint8_t*>(framebufferBegin(&framebuffer_,&stride));if(!fb)throw std::runtime_error("frame acquisition failed");
  if(stride<1280*4){framebufferEnd(&framebuffer_);throw std::runtime_error("framebuffer stride refused");}
  auto source=graphics_->pixels();for(unsigned y=0;y<720;++y)for(unsigned x=0;x<1280;++x){size_t from=((y/4)*320+x/4)*4;std::memcpy(fb+size_t(y)*stride+x*4,source.data()+from,4);}
  auto stats=demo_->stats();char line[96];text(fb,stride,24,18,"BRUTAL NX SYNTHETIC PLATFORM - T85");std::snprintf(line,sizeof(line),"FRAME %llu PPC CALLS %llu",static_cast<unsigned long long>(stats.frames),static_cast<unsigned long long>(stats.guest_calls));text(fb,stride,24,54,line);
  std::snprintf(line,sizeof(line),"INPUT %s RESOURCE %u BYTES",raw.connected?"CONNECTED":"DISCONNECTED",stats.resource_bytes);text(fb,stride,24,90,line);
  std::snprintf(line,sizeof(line),"AUDIO %s SENT %llu DONE %llu",stats.audio_available?"READY":"UNAVAILABLE",static_cast<unsigned long long>(stats.audio_submitted),static_cast<unsigned long long>(stats.audio_completed));text(fb,stride,24,126,line);text(fb,stride,24,630,"STICK MOVES - A COLOR AND TONE - PLUS STOPS");text(fb,stride,24,666,"ORIGINAL SYNTHETIC PPC - NOT THE GAME");
  framebufferEnd(&framebuffer_);last_frame_ns=armTicksToNs(armGetSystemTick()-start);
 }
 void shutdown(){if(stopped)return;lifecycle_.shutdown();stopped=true;if(lifecycle_.cleanup_failures()||final.cleanup_errors)failed=true;auto counts=brutal::horizon::Space::totals();auto files=r::Service::totals();if(counts.first||counts.second||files.first||files.second)failed=true;}
 ~Application(){shutdown();}
};
struct ExitUi {PadState*pad;bool running(){return appletMainLoop();}void update_input(){padUpdate(pad);}bool plus_pressed(){return padGetButtonsDown(pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16000000);}};
}
int main(){
 auto app=std::make_unique<Application>();PadState pad{};padConfigureInput(1,HidNpadStyleSet_NpadStandard);padInitializeDefault(&pad);bool applet_exit=false;uint64_t frame=0;
 try{app->initialize();while(true){if(!appletMainLoop()){applet_exit=true;break;}padUpdate(&pad);if(padGetButtonsDown(&pad)&HidNpadButton_Plus)break;if(frame==UINT32_MAX)break;app->frame(pad,++frame);}}
 catch(const std::exception&e){app->failed=true;std::snprintf(app->error,sizeof(app->error),"%s",e.what());}
 app->shutdown();if(applet_exit)return app->failed?1:0;
 auto* console=consoleInit(nullptr);if(!console||!console->consoleInitialised)return 2;
 std::puts("BRUTAL LEGEND NX - SYNTHETIC PLATFORM DEMO\nT66-T85 / Original PPC only / NOT THE GAME");
 std::printf("Actual frames=%llu result=%u\nService entered=%llu completed=%llu\nAudio available=%s submitted=%llu completed=%llu native_error=%08x\nLast frame CPU+present ns=%llu (not FPS guarantee)\n",static_cast<unsigned long long>(app->final.frames),app->final.result,static_cast<unsigned long long>(app->final.service_entered),static_cast<unsigned long long>(app->final.service_completed),app->final.audio_available?"yes":"NO",static_cast<unsigned long long>(app->final.audio_submitted),static_cast<unsigned long long>(app->final.audio_completed),app->final.audio_native_error,static_cast<unsigned long long>(app->last_frame_ns));
 auto memory=brutal::horizon::Space::totals();auto files=r::Service::totals();std::printf("Cleanup: allocations=%zu leases=%zu handles=%zu resource_bytes=%zu\nExecution/cleanup status=%s\n",memory.first,memory.second,files.first,files.second,app->failed?"FAIL":"COMPLETED");if(app->error[0])std::printf("Failure: %s\n",app->error);
 std::puts("Hardware visual/audio/input acceptance requires observation.\nPhotograph this screen. Release PLUS, then press PLUS to hbmenu.");consoleUpdate(nullptr);ExitUi ui{&pad};brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return app->failed?1:0;
}
