// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original MMIO/fault fixtures, one mode per externally supervised child.
#include <rex/system/xmemory.h>
#include <rex/system/mmio_handler.h>
#include <rex/exception_handler.h>
#include <rex/logging.h>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
extern "C" void fault_store(uint32_t*,uint32_t);
extern "C" uint32_t fault_load(uint32_t*);
#if defined(__APPLE__)
#define SYM(n) "_" n
#else
#define SYM(n) n
#endif
#if defined(__aarch64__)
asm(".text\n.p2align 2\n.globl " SYM("fault_store") "\n" SYM("fault_store") ":\nstr w1,[x0]\nret\n.globl " SYM("fault_load") "\n" SYM("fault_load") ":\nldr w0,[x0]\nret\n");
#elif defined(__x86_64__)
asm(".text\n.globl " SYM("fault_store") "\n" SYM("fault_store") ":\nmovl %esi,(%rdi)\nret\n.globl " SYM("fault_load") "\n" SYM("fault_load") ":\nmovl (%rdi),%eax\nret\n");
#else
#error Unsupported fixture host
#endif
static volatile sig_atomic_t reads=0,writes=0;
static volatile sig_atomic_t stored=0;
static uint32_t read_mmio(void*,void*,uint32_t address){if(address!=0x7f000000)_exit(90);reads=reads+1;return 0x89abcdef;}
static void write_mmio(void*,void*,uint32_t address,uint32_t value){if(address!=0x7f000000)_exit(91);writes=writes+1;stored=static_cast<sig_atomic_t>(value);}
static bool observe(rex::arch::Exception* ex,void*){
  uint64_t record[3]{0x50484337,ex->fault_address(),ex->pc()};(void)!write(STDERR_FILENO,record,sizeof(record));return false;
}
static bool nested(rex::arch::Exception* ex,void*){
  uint64_t record[3]{0x4e455354,ex->fault_address(),ex->pc()};(void)!write(STDERR_FILENO,record,sizeof(record));
  fault_store(nullptr,0x1234);return false;
}
int main(int argc,char** argv){
  if(argc!=2)return 2;std::setbuf(stdout,nullptr);std::string mode=argv[1];rex::InitLoggingEarly();
  rex::memory::Memory memory;if(!memory.Initialize())return 3;
  auto* mmio=rex::runtime::MMIOHandler::global_handler();if(!mmio)return 4;
  if(mode=="mmio"||mode=="nested"){
    if(!mmio->RegisterRange(0x7f000000,0xfffff000,0x1000,nullptr,read_mmio,write_mmio))return 5;
    if(mode=="mmio"){
      uint32_t value=0;
      if(!mmio->CheckStore(0x7f000000,0x11223344)||stored!=0x11223344||!mmio->CheckLoad(0x7f000000,&value)||value!=0x89abcdef)return 6;
      if(mmio->CheckLoad(0x7e000000,&value)||mmio->CheckStore(0x7e000000,7))return 7;
      auto* address=memory.TranslateVirtual<uint32_t*>(0x7f000000);
      if(!rex::memory::Protect(address,4096,rex::memory::PageAccess::kNoAccess))return 14;
      fault_store(address,0x78563412);if(stored!=0x12345678)return 8;
      if(fault_load(address)!=0xefcdab89||reads!=2||writes!=2)return 9;
      puts("PASS legitimate MMIO: direct helpers and decoded native load/store faults");return 0;
    }
  }
  if(mode=="hole-probe"){
    auto* hole=memory.TranslateVirtual<uint32_t*>(0x7f000000);
    if(memory.LookupHeap(0x7f000000)!=nullptr)return 15;
    fault_store(hole,0x12345678);
    if(fault_load(hole)!=0x12345678)return 16;
    puts("{\"finding\":\"unallocated_guest_hole_writable\",\"status\":\"BLOCKED\"}");return 79;
  }
  auto address=memory.SystemHeapAlloc(4096,4096);if(!address)return 10;
  auto* pointer=memory.TranslateVirtual<uint32_t*>(address);
  *pointer=0x4a;
  auto* heap=memory.LookupHeap(address);uint32_t size=0;if(!heap->QuerySize(address,&size))return 11;
  if(!heap->Protect(address,size,mode=="readonly"||mode=="nested"?rex::memory::kMemoryProtectRead:rex::memory::kMemoryProtectNoAccess))return 12;
  rex::arch::ExceptionHandler::Install(observe,nullptr);
  if(mode=="nested")rex::arch::ExceptionHandler::Install(nested,nullptr);
  if(mode=="sigsegv")fault_store(nullptr,7);
  else if(mode=="invalid-guest")fault_store(memory.TranslateVirtual<uint32_t*>(0),7);
  else if(mode=="noaccess-read")(void)fault_load(pointer);
  else if(mode=="readonly"||mode=="nested")fault_store(pointer,7);
  else return 13;
  return 86;
}
