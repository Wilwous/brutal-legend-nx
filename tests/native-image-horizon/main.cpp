// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "tests/native-image-binding/probe.h"
#include "runtime/horizon/native.h"
#include "runtime/horizon/probe_control.h"
#include "tests/horizon-acceptance/supervisor.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
struct Ui {PadState pad{};uint64_t start=armGetSystemTick();bool shutdown{};
 bool running(){if(!appletMainLoop())shutdown=true;return !shutdown;}void update_input(){padUpdate(&pad);}bool plus_pressed(){return padGetButtonsDown(&pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16666667);}uint64_t elapsed_ms(){return armTicksToNs(armGetSystemTick()-start)/1000000;}
 void progress(unsigned,bool cancel){if(cancel)std::printf("\x1b[6;1HCANCEL requested. Waiting for owned threads to join.\n");}
};
int main(){
 if(!consoleInit(nullptr)){return 2;}
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);
 std::puts("BRUTAL LEGEND NX - NATIVE IMAGE PROBE\nOriginal synthetic PPC only. NOT THE GAME.\nInitializing validated plan, guest memory and runtime...\nPLUS requests cooperative cancellation during startup.");consoleUpdate(nullptr);
 brutal::horizon::ProbeControl control;brutal::horizon::ProbeControl::Activation activation(control);brutal::horizon::NativeThread controller;NativeResult r;std::atomic<bool>produced{};bool started=false;
 try{started=controller.start([&]{r=native_probe();produced.store(true,std::memory_order_release);},256*1024);}catch(const std::exception&e){control.failure(e.what());}
 if(started)brutal::acceptance::supervise(ui,controller,control);
 if(ui.shutdown){consoleExit(nullptr);return 1;}
 auto platform=brutal::acceptance::platform_checks(true,started,controller.joined(),controller.joined()&&!controller.callback_error()&&produced.load(std::memory_order_acquire));if(control.cancelled())++platform.failed;
 consoleClear();std::puts("BRUTAL LEGEND NX - NATIVE IMAGE PROBE\nSynthetic native image contract / N3\nHardware results require external observation.");
 std::printf("Platform init: %s\n",platform.failed?"FAIL/INCOMPLETE":"PASS");
 if(produced.load(std::memory_order_acquire))std::printf("Runtime checks completed; actual PPC result=%u\nSDK entered=%u completed=%u\nRuntime dispatches/entry=%u observed PPC functions=%u\nDiagnostics=%u allocations=%u leases=%u\nPositive=%u expected refusals=%u failures=%u\nPASS=%u FAIL=%u\n",r.actual,r.sdk_entered,r.sdk_completed,r.functions,r.observed_functions,r.diagnostics,r.remaining,r.leases,r.positive,r.negative,r.failed,r.positive+r.negative+platform.passed,r.failed+platform.failed);
 else std::puts("PPC/result/cleanup UNAVAILABLE; acceptance FAIL.");
 if(control.cancelled()){std::puts("CANCELLED: acceptance invalid.");}
 std::puts("No game code. Guest continuation unsupported.\nPhotograph full screen; PLUS (+) returns to hbmenu.");consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed||platform.failed?1:0;
}
