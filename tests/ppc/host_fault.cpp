// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original fault fixture. No guest executable, generated code or assets.
#if defined(__APPLE__)
#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE
#endif
#include <rex/exception_handler.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>
#include <ucontext.h>
static volatile sig_atomic_t faults = 0;
extern "C" void fixture_store(uint8_t*, uint32_t);
extern "C" void fixture_resume();
#if defined(__APPLE__)
#define ASM_NAME(n) "_" n
#else
#define ASM_NAME(n) n
#endif
#if defined(__aarch64__)
asm(".text\n.p2align 2\n.globl " ASM_NAME("fixture_store") "\n" ASM_NAME("fixture_store") ":\nstrb w1,[x0]\n.globl " ASM_NAME("fixture_resume") "\n" ASM_NAME("fixture_resume") ":\nret\n");
#elif defined(__x86_64__)
asm(".text\n.globl " ASM_NAME("fixture_store") "\n" ASM_NAME("fixture_store") ":\nmovb %sil,(%rdi)\n.globl " ASM_NAME("fixture_resume") "\n" ASM_NAME("fixture_resume") ":\nret\n");
#else
#error Fault fixture requires ARM64 or x86-64
#endif
struct Event { int signal, code; uintptr_t address, pc; };
static void signal_exit(int number, siginfo_t* info, void* raw) {
  Event event{number, info->si_code, reinterpret_cast<uintptr_t>(info->si_addr), 0};
#if defined(__APPLE__) && defined(__aarch64__)
  mcontext_t context;
  std::memcpy(&context,static_cast<const char*>(raw)+offsetof(ucontext_t,uc_mcontext),sizeof(context));
  event.pc = context->__ss.__pc;
#elif defined(__linux__) && defined(__x86_64__)
  event.pc = reinterpret_cast<ucontext_t*>(raw)->uc_mcontext.gregs[REG_RIP];
#endif
  // Binary record uses only async-signal-safe write and _exit.
  (void)!write(STDERR_FILENO, &event, sizeof(event));
  _exit(80);
}
static bool observer(rex::arch::Exception* ex, void* data) {
  if (ex->fault_address() != reinterpret_cast<uintptr_t>(data)) return false;
  faults = faults + 1;
  if (faults <= 3) {
    uintptr_t record[3]{ex->fault_address(), ex->pc(), static_cast<uintptr_t>(ex->access_violation_operation())};
    (void)!write(STDERR_FILENO, record, sizeof(record));
  }
  return false;  // Reproduces SDK's unclaimed-fault path.
}
static bool recover(rex::arch::Exception* ex, void* data) {
  if (ex->fault_address() != reinterpret_cast<uintptr_t>(data)) return false;
  faults = faults + 1;
  ex->set_resume_pc(reinterpret_cast<uintptr_t>(&fixture_resume));
  return true;
}
int main(int argc, char** argv) {
  if (argc!=2) return 2;
  std::setbuf(stdout, nullptr);
  std::string mode=argv[1];
  auto page=static_cast<uint8_t*>(mmap(nullptr, sysconf(_SC_PAGESIZE), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, -1, 0));
  if (page==MAP_FAILED) return 3;
  page[0]=0x4a;
  if (mprotect(page, sysconf(_SC_PAGESIZE), PROT_READ)) return 4;
  if(mode=="posix-capture") {
    struct sigaction action{}; action.sa_sigaction=signal_exit; action.sa_flags=SA_SIGINFO;
    sigaction(SIGBUS,&action,nullptr); sigaction(SIGSEGV,&action,nullptr);
  }
  rex::memory::Memory* memory=nullptr;
  if(mode=="sdk-memory") {
    rex::InitLoggingEarly(); memory=new rex::memory::Memory;
    if(!memory->Initialize()) return 5;
    auto address=memory->SystemHeapAlloc(4096, sysconf(_SC_PAGESIZE));
    page=memory->TranslateVirtual(address);
    auto heap=memory->LookupHeap(address); uint32_t size=0;
    if(!heap->QuerySize(address,&size)||!heap->Protect(address,size,rex::memory::kMemoryProtectRead))return 6;
  }
  if(mode=="sdk-decline"||mode=="sdk-memory") rex::arch::ExceptionHandler::Install(observer,page);
  if(mode=="sdk-recover") rex::arch::ExceptionHandler::Install(recover,page);
  std::printf("protected_address=%p mode=%s\n",page,argv[1]);
  fixture_store(page,0xff);
  if(mode=="sdk-recover") {
    rex::arch::ExceptionHandler::Uninstall(recover,page);
    if(faults!=1||page[0]!=0x4a)return 7;
    munmap(page,sysconf(_SC_PAGESIZE));
    puts("RECOVERED: one fault, write skipped, page remains read-only"); return 0;
  }
  return 86; // A protected write must never succeed.
}
