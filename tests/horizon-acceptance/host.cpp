// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
#include <cstring>
#include "runtime/horizon/native.h"
#include "runtime/horizon/probe_control.h"
#include "supervisor.h"
#include <chrono>
#include <thread>
int main(int argc,char**argv){if(argc>2||(argc==2&&std::strcmp(argv[1],"--stress")&&std::strcmp(argv[1],"--no-trace")&&std::strcmp(argv[1],"--controller")))return 2;RuntimeIIResult r;
if(argc==2&&!std::strcmp(argv[1],"--controller")){
 brutal::horizon::ProbeControl control;brutal::horizon::ProbeControl::Activation active(control);brutal::horizon::NativeThread task;std::atomic<bool> produced{};
 if(!task.start([&]{r=acceptance_probe();produced.store(true,std::memory_order_release);},256*1024))return 3;
 struct Ui {std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();bool running(){return true;}void update_input(){}bool plus_pressed(){return false;}uint64_t elapsed_ms(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();}void progress(unsigned,bool){}void present(){}void yield_frame(){std::this_thread::sleep_for(std::chrono::milliseconds(1));}} ui;
 brutal::acceptance::supervise(ui,task,control);if(!produced.load(std::memory_order_acquire)||task.callback_error()||control.cancelled()||control.stage()!=5)return 3;
}else r=acceptance_probe(argc==2&&!std::strcmp(argv[1],"--stress")?8:1,argc!=2||std::strcmp(argv[1],"--no-trace"));std::printf("{\"passed\":%u,\"failed\":%u,\"result\":%u,\"sdk_calls\":%u,\"sdk_entered\":%u,\"imports\":%u,\"guest_entries\":%u,\"workers\":%u,\"remaining\":%u,\"leases\":%u,\"refusals\":%u,\"diagnostics\":%u,\"dropped\":%u,\"groups\":[",r.passed,r.failed,r.result,r.sdk_calls,r.sdk_entered,r.imports,r.guest_entries,r.workers,r.remaining,r.leases,r.refusals,r.diagnostics,r.dropped);for(unsigned i=0;i<4;++i)std::printf("%s{\"passed\":%u,\"failed\":%u}",i?",":"",r.group_pass[i],r.group_fail[i]);std::puts("]}");return r.failed?1:0;}
