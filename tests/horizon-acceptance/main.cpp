// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "ii-metadata.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
namespace {struct Ui {PadState pad{};bool running(){return appletMainLoop();}void update_input(){padUpdate(&pad);}bool plus_pressed(){return padGetButtonsDown(&pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16666667);}};}
int main(){
 if(!consoleInit(nullptr))return 2;
 std::puts("\n BRUTAL LEGEND NX - HARDWARE ACCEPTANCE\n Original synthetic PPC only. NOT THE GAME.\n Platform initialization: PASS\n Running bounded smoke checks...");consoleUpdate(nullptr);
 std::printf(" Build 0.15.0 / %s / stages %u / GCC %s\n",ii_build_id,unsigned(STAGE_MAX),__VERSION__);
 auto r=acceptance_probe();
 const char* groups[]{"Memory","ABI/TLS/threads/stop","SDK/pointers","Diagnostics/lifetime"};
 for(unsigned i=0;i<4;++i)if(r.group_pass[i]||r.group_fail[i])std::printf(" %s: PASS=%u FAIL=%u\n",groups[i],r.group_pass[i],r.group_fail[i]);
 std::printf(" Generated PPC result=%u; imports=%u\n SDK entered=%u completed=%u\n Peak guest workers=%u; allocations=%u leases=%u\n",r.result,r.imports,r.sdk_entered,r.sdk_calls,r.workers,r.remaining,r.leases);
 std::printf(" Generated guest entries=%u; controlled refusals=%u\n",r.guest_entries,r.refusals);
 std::printf("\n PASS=%u FAIL=%u\n",r.passed+1,r.failed);
 std::printf(" Diagnostic events=%u; dropped=%u\n Stop / cleanup: %s\n",r.diagnostics,r.dropped,r.failed?"CHECK FAILURES":"PASS");
 std::printf(" Image %02x%02x%02x%02x; codegen %02x%02x%02x%02x; SDK %02x%02x%02x%02x\n",ii_metadata.image[0],ii_metadata.image[1],ii_metadata.image[2],ii_metadata.image[3],ii_metadata.codegen[0],ii_metadata.codegen[1],ii_metadata.codegen[2],ii_metadata.codegen[3],ii_metadata.sdk[0],ii_metadata.sdk[1],ii_metadata.sdk[2],ii_metadata.sdk[3]);
 std::puts(" Unsupported ABI/imports remain denied.\n No game compatibility claim.\n Press PLUS (+) to exit.");
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed?1:0;
}
