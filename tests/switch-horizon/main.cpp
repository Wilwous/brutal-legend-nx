// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
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
}
int main(){
  consoleInit(nullptr);
  std::puts("Original Horizon feasibility probe; no game code");
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
  std::printf("PASS=%u FAIL=%u\n",result.passed,result.failed);
  std::puts("Record this screen. These are synthetic component checks only.");
  consoleUpdate(nullptr);svcSleepThread(2000000000LL);consoleExit(nullptr);
  return result.failed?1:0;
}
