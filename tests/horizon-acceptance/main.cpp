// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <switch.h>
#include "probe.h"
#include "ii-metadata.h"
#include "runtime/horizon/native.h"
#include "runtime/horizon/probe_control.h"
#include "supervisor.h"
#include "tests/switch-horizon/result_ui.h"
#include <cstdio>
extern "C" {u32 __nx_applet_type=AppletType_Default;size_t __nx_heap_size=16*1024*1024;}
namespace {
struct Ui {PadState pad{};uint64_t start=armGetSystemTick();unsigned last=99;bool cancelled{},shutdown{};
 bool running(){bool live=appletMainLoop();if(!live)shutdown=true;return live;}void update_input(){padUpdate(&pad);}bool plus_pressed(){return padGetButtonsDown(&pad)&HidNpadButton_Plus;}void present(){consoleUpdate(nullptr);}void yield_frame(){svcSleepThread(16666667);}uint64_t elapsed_ms(){return armTicksToNs(armGetSystemTick()-start)/1000000;}
 void progress(unsigned stage,bool cancel){if(stage==last&&cancel==cancelled)return;last=stage;cancelled=cancel;const char*names[]{"Starting","Preserved runtime checks","PPC scalar/VMX","SDK OUT/query contracts","Lifecycle/stop/trace","Complete"};std::printf("\x1b[6;1H Stage %u: %-36s\n %-70s\n",stage,stage<6?names[stage]:"UNKNOWN",cancel?"CANCEL REQUESTED; waiting for owned threads to join.":"Running one finite smoke workload. PLUS requests cooperative cancel.");if(cancel)std::puts(" If native calls remain stuck: photograph this stage; use HOME/Close Software.");}
};
}
int main(){
 if(!consoleInit(nullptr))return 2;
 padConfigureInput(1,HidNpadStyleSet_NpadStandard);Ui ui;padInitializeDefault(&ui.pad);
 std::puts("\n BRUTAL LEGEND NX - HARDWARE ACCEPTANCE CANDIDATE\n Original synthetic PPC only. NOT THE GAME.");std::printf(" Build 0.15.0 / %s; applet type=%d\n",ii_build_id,int(appletGetAppletType()));consoleUpdate(nullptr);
 brutal::horizon::ProbeControl control;brutal::horizon::ProbeControl::Activation active(control);brutal::horizon::NativeThread controller;RuntimeIIResult r;std::atomic<bool> produced{};
 bool started=false;try{started=controller.start([&]{try{r=acceptance_probe();produced.store(true,std::memory_order_release);}catch(const std::exception&e){brutal::horizon::report_failure(e.what());throw;}},256*1024);}catch(const std::exception&e){control.failure(e.what());}
 if(!started)control.failure("Native controller start failed; runtime checks were not executed.");
 bool joined=false;if(started){brutal::acceptance::supervise(ui,controller,control);joined=controller.joined();}
 if(ui.shutdown){consoleExit(nullptr);return 1;}
 auto platform=brutal::acceptance::platform_checks(true,started,joined,joined&&!controller.callback_error()&&produced.load(std::memory_order_acquire));if(control.cancelled())++platform.failed;
 consoleClear();std::puts("\n BRUTAL LEGEND NX - HARDWARE ACCEPTANCE CANDIDATE\n Original synthetic PPC only. NOT THE GAME.");std::printf(" Build 0.15.0 / %s; libnx applet type=%d\n",ii_build_id,int(appletGetAppletType()));
 std::printf(" PPC functions=%u instructions=%u sites=%u hooks=%u\n",acceptance_functions,acceptance_instructions,acceptance_memory_sites,acceptance_hooks);
 std::printf(" Fixture SHA prefix: ");for(unsigned i=0;i<8;++i)std::printf("%02x",ii_metadata.image[i]);std::puts("");
 if(!produced.load(std::memory_order_acquire))std::puts(" Runtime execution/result UNAVAILABLE.");
 const char*groups[]{"Memory","ABI/PCR/TLS/threads/stop","SDK/pointers","Semantics/lifecycle/refusals"};for(unsigned i=0;i<4;++i)std::printf(" %s: PASS=%u FAIL=%u\n",groups[i],r.group_pass[i],r.group_fail[i]);
 std::printf(" Generated PPC result=%u; guest entries=%u\n Imports=%u; SDK entered=%u completed=%u\n Peak guest workers=%u; allocations=%u leases=%u\n",r.result,r.guest_entries,r.imports,r.sdk_entered,r.sdk_calls,r.workers,r.remaining,r.leases);
 std::printf(" Controlled refusals=%u; stop/cleanup=%s\n Diagnostic events=%u dropped=%u\n",r.refusals,produced.load(std::memory_order_acquire)&&!r.failed&&!r.remaining&&!r.leases?"PASS":"FAIL/UNAVAILABLE",r.diagnostics,r.dropped);
 std::printf(" Runtime PASS=%u FAIL=%u; platform PASS=%u FAIL=%u\n\n PASS=%u FAIL=%u\n",r.passed,r.failed,platform.passed,platform.failed,r.passed+platform.passed,r.failed+platform.failed);
 auto failures=control.failures();for(size_t i=0;i<failures.used&&i<3;++i)std::printf(" FAIL: %s\n",failures.rows[i].data());if(failures.used>3||failures.dropped)std::printf(" Failure labels: stored=%zu truncated/dropped=%zu; showing first 3\n",failures.used,failures.dropped);
 if(control.cancelled())std::puts(" CANCELLED/TIMED OUT: hardware acceptance INVALID.");
 std::puts(" Horizon results require external observation.\n Atomics/typed vectors/guest unwind remain denied.\n Photograph the complete screen. Press PLUS (+) to exit.");consoleUpdate(nullptr);brutal::probe::wait_for_exit(ui);consoleExit(nullptr);return r.failed||platform.failed?1:0;
}
