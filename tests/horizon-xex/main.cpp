// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
namespace {
struct Ui {
 PadState pad{};
 bool running(){return appletMainLoop();}
 void update_input(){padUpdate(&pad);}
 bool plus_pressed(){return (padGetButtonsDown(&pad)&HidNpadButton_Plus)!=0;}
 void present(){consoleUpdate(nullptr);}
 void yield_frame(){svcSleepThread(16666667);}
};
}
int main(){
 if(!consoleInit(nullptr))return 2;
 std::puts("\n BRUTAL LEGEND NX - SYNTHETIC XEX RUNTIME\n Original synthetic PPC. NOT the game.\n Platform init: PASS\n Loading original synthetic XEX and running bounded tests...\n");consoleUpdate(nullptr);
 auto r=xex_probe();
 std::printf(" Synthetic XEX / metadata / refusal checks: PASS=%u FAIL=%u\n",r.passed,r.failed);
 std::printf(" Observed primary PPC r3=%u; primary SDK calls=%u\n",r.observed_result,r.sdk_calls);
 std::printf(" Production stop hook hits: %u / %u; live guest allocations=%u\n",r.hook_hits[0],r.hook_hits[1],r.remaining);
 std::printf("\n PASS=%u FAIL=%u\n",r.passed+1,r.failed);
 std::puts(" Original flat XEX; one admitted real SDK leaf.\n NtQueryVirtualMemory / critical-section import blocked.\n No game compatibility or full Xbox runtime.\n Press PLUS (+) to exit.");
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed?1:0;
}
