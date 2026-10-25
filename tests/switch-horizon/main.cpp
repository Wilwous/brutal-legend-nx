// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "result_ui.h"
#include <atomic>
#include <cstdio>

// Fixed budgets for this original fixture, not a title runtime memory budget.
extern "C" {
  u32 __nx_applet_type=AppletType_Default;
  size_t __nx_heap_size=16*1024*1024;
}
namespace {
struct Work {std::atomic<bool> stop{false};std::atomic<unsigned> steps{0};};
void worker(void* raw){
  auto& work=*static_cast<Work*>(raw);
  for(unsigned i=0;i<100000 && !work.stop.load(std::memory_order_acquire);++i)
    work.steps.fetch_add(1,std::memory_order_relaxed);
}
struct HorizonUi {
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
  std::puts("\n  BRUTAL LEGEND NX PROBE\n  Original runtime test - NOT the game\n");
  std::puts("  Running 21 portable + 2 native checks...\n");
  consoleUpdate(nullptr);
  auto result=portable_probe();
  Thread thread{};Work work;
  Result rc=threadCreate(&thread,worker,&work,nullptr,64*1024,0x3b,-2);
  if(R_FAILED(rc)){++result.failed;std::printf("threadCreate: %08x\n",rc);}
  else {
    rc=threadStart(&thread);
    if(R_FAILED(rc)){++result.failed;std::printf("threadStart: %08x\n",rc);}
    else {
      work.stop.store(true,std::memory_order_release);
      s32 index=-1;
      rc=svcWaitSynchronization(&index,&thread.handle,1,2000000000ULL);
      if(R_FAILED(rc)) {
        // Do not destroy stack-owned worker state or close a still-live thread.
        std::printf("thread wait failed: %08x; aborting process\n",rc);
        consoleUpdate(nullptr);diagAbortWithResult(rc);
      }
      rc=threadWaitForExit(&thread);
      if(R_FAILED(rc)){std::printf("threadWaitForExit: %08x\n",rc);diagAbortWithResult(rc);}
      ++result.passed;
    }
    rc=threadClose(&thread);
    if(R_FAILED(rc)){++result.failed;std::printf("threadClose: %08x\n",rc);}
    else ++result.passed;
  }
  std::printf("\n  %sPASS=%u FAIL=%u\x1b[0m\n\n",result.failed?"\x1b[31m":"\x1b[32m",result.passed,result.failed);
  std::puts("  Record this screen and your hbmenu launch mode.\n");
  std::puts("  Synthetic checks only; no PPC or game code executed.\n");
  std::puts("  Press PLUS (+) to exit to hbmenu.");
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);
  HorizonUi ui;padInitializeDefault(&ui.pad);
  consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);
  return result.failed?1:0;
}
