// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
namespace {
void worker(void* result){*static_cast<LeafResult*>(result)=run_leaf_probe();}
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
  std::puts("\n BRUTAL LEGEND NX - PPC RUNTIME PROBE\n Original four-instruction PPC fixture. NOT the game.\n");
  LeafResult report;unsigned passed=1,failed=0;
  auto check=[&](const char* name,bool ok){std::printf(" %s: %s\n",name,ok?"PASS":"FAIL");if(ok)++passed;else ++failed;};
  std::puts(" Platform initialization: PASS (console ready)");consoleUpdate(nullptr);
  Thread thread{};Result rc=threadCreate(&thread,worker,&report,nullptr,64*1024,0x3b,-2);
  check("Native worker creation",R_SUCCEEDED(rc));
  if(R_SUCCEEDED(rc)){
    rc=threadStart(&thread);check("Native worker start",R_SUCCEEDED(rc));
    if(R_SUCCEEDED(rc)){
      s32 index=-1;rc=svcWaitSynchronization(&index,&thread.handle,1,2000000000ULL);
      if(R_FAILED(rc)){
        std::printf(" Worker wait FAILED: %08x. Fail-stop; no live-state cleanup.\n",rc);
        consoleUpdate(nullptr);diagAbortWithResult(rc);
      }
      rc=threadWaitForExit(&thread);
      if(R_FAILED(rc)){std::printf(" Join FAILED: %08x\n",rc);consoleUpdate(nullptr);diagAbortWithResult(rc);}
      check("Native worker joined",true);
      check("Guest-memory initialization",report.memory_initialized);
      check("Generated PPC entry invoked",report.entry_invoked);
      check("Expected PPC result: r3=42, stored=42",report.expected_result);
      check("Guest memory cleanup",report.cleanup);
      passed+=report.passed;failed+=report.failed;
      std::printf(" Actual generated entries=%u checked loads=%u stores=%u\n",report.invocations,report.reads,report.writes);
      std::printf(" Leaf contract checks: PASS=%u FAIL=%u\n",report.passed,report.failed);
    }
    rc=threadClose(&thread);check("Native worker cleanup",R_SUCCEEDED(rc));
  }
  std::printf("\n PASS=%u FAIL=%u\n",passed,failed);
  std::puts(" Actual execution checks above; compile/link are separate.\n No Xbox XThread/PCR/TLS or game compatibility claimed.\n Press PLUS (+) to exit.");
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);
  consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return failed?1:0;
}
