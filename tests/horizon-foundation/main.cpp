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
 std::puts("\n BRUTAL LEGEND NX - HORIZON RUNTIME FOUNDATION\n Original synthetic PPC. NOT the game.\n Platform init: PASS\n Running bounded native/guest tests...\n");consoleUpdate(nullptr);
 auto r=foundation_probe();const char* names[]{"Checked memory","Native threads/join","Guest PCR/TLS/context","Generated PPC multicall","Cooperative stop","Ownership/rollback/cleanup"};
 for(unsigned i=0;i<6;++i)std::printf(" %s: PASS=%u FAIL=%u\n",names[i],r.stage_passed[i],r.stage_failed[i]);
 std::printf(" Observed primary PPC r3=%u; remaining allocations=%u\n",r.observed_ppc_result,r.remaining_allocations);
 std::printf(" Selected generated calls=%llu reads=%llu writes=%llu polls=%llu\n",(unsigned long long)r.generated_calls,(unsigned long long)r.checked_reads,(unsigned long long)r.checked_writes,(unsigned long long)r.safe_points);
 std::printf("\n PASS=%u FAIL=%u\n",r.passed+1,r.failed);
 std::puts(" Runtime results above are actual checks.\n Synthetic XThread/PCR/TLS subset; no Xbox scheduler.\n Creation-failure control is injected, not kernel OOM.\n Press PLUS (+) to exit.");
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed?1:0;
}
