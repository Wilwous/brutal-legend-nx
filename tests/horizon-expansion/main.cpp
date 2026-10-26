// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
namespace {struct Ui {PadState pad{};bool running(){return appletMainLoop();}void update_input(){padUpdate(&pad);}bool plus_pressed(){return padGetButtonsDown(&pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16666667);}};}
int main(){
 if(!consoleInit(nullptr))return 2;
 std::puts("\n BRUTAL LEGEND NX - INTEGRATED RUNTIME\n Original synthetic PPC only. NOT THE GAME.\n Platform initialization: PASS\n Running bounded smoke checks...");consoleUpdate(nullptr);
 std::printf(" Build 0.5.0 / stages through %u / GCC %s\n",unsigned(STAGE_MAX),__VERSION__);
 auto r=expansion_probe();
 for(unsigned i=0;i<5;++i)if(r.stage_pass[i]||r.stage_fail[i])std::printf(" Stage %u: PASS=%u FAIL=%u\n",i+4,r.stage_pass[i],r.stage_fail[i]);
 std::printf(" Generated PPC result=%u; actual SDK calls=%u\n Peak concurrent guest workers=%u; allocations=%u leases=%u\n",r.result,r.sdk_calls,r.workers,r.remaining,r.leases);
 std::printf("\n PASS=%u FAIL=%u\n",r.passed+1,r.failed);
 std::puts(" Unsupported ABI/imports remain denied.\n No game compatibility claim.\n Press PLUS (+) to exit.");
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed?1:0;
}
