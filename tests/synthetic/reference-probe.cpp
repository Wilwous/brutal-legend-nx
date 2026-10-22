// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original Debug-only object-ownership checkpoints for read-only LLDB inspection.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <atomic>
#include <filesystem>
#include <cstdio>
using namespace rex::system;
static std::atomic<unsigned> destroyed{0};
extern "C" __attribute__((noinline)) void checkpoint(XThread* thread,const char* phase){std::printf("CHECKPOINT %s\n",phase);asm volatile(""::"r"(thread):"memory");}
class Probe final:public XThread {
 public:
 explicit Probe(KernelState* kernel):XThread(kernel,65536,0,0,0,1,true){}
 void Execute()override{kernel_state()->OnThreadExecute(this);checkpoint(this,"executing");Exit(7);}
 ~Probe()override{++destroyed;std::puts("DESTROYED");}
};
int main(int argc,char** argv){
 if(argc!=2)return 2;rex::InitLoggingEarly();std::filesystem::path path=argv[1];std::filesystem::create_directories(path/"empty");
 rex::Runtime runtime(path/"empty",path/"user",path/"updates",path/"cache",path/"metadata");
 if(runtime.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}))return 3;
 object_ref<Probe> thread(new Probe(runtime.kernel_state()));checkpoint(thread.get(),"constructed");
 if(thread->Create())return 4;checkpoint(thread.get(),"created");if(thread->Resume())return 5;
 if(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(5))!=rex::thread::WaitResult::kSuccess)return 6;
 checkpoint(thread.get(),"joined");if(!thread->ReleaseHandle())return 7;checkpoint(thread.get(),"handle-closed");thread.reset();
 return destroyed==1?0:1;
}
