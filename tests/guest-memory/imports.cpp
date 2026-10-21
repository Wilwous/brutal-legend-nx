// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Absence/error contracts only; no loaded executable or address dispatch.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <array>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
REX_EXTERN(__imp__DbgPrint);REX_EXTERN(__imp__XexGetModuleHandle);
REX_EXTERN(__imp__XexCheckExecutablePrivilege);REX_EXTERN(__imp__NtQueryVirtualMemory);
REX_EXTERN(__imp__RtlInitAnsiString);REX_EXTERN(__imp__NtCreateEvent);
REX_EXTERN(__imp__NtWaitForSingleObjectEx);REX_EXTERN(__imp__NtClose);
static unsigned checks=0;
static void require(bool v){++checks;if(!v)throw std::runtime_error("import error oracle mismatch");}
static uint32_t call(PPCFunc* fn,PPCContext& c,uint8_t* base,std::initializer_list<uint32_t> args){size_t i=0;for(auto arg:args)rex::ppc::ArgTranslator::SetIntegerArgumentValue(c,base,i++,arg);fn(c,base);return c.r3.u32;}
int main(int argc,char** argv){
 if(argc!=2)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{
  std::filesystem::path work=argv[1];std::filesystem::create_directories(work/"empty");
  rex::Runtime r(work/"empty",work/"user",work/"updates",work/"cache",work/"metadata");
  require(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
  auto* m=r.memory();auto* b=m->virtual_membase();PPCContext c{};auto data=m->SystemHeapAlloc(4096);require(data);
  auto get=[&](unsigned offset){return rex::memory::load_and_swap<uint32_t>(b+data+offset);};
  auto sentinel=[&]{m->Fill(data,64,0x55);};
  require(call(__imp__DbgPrint,c,b,{0})==0);
  for(uint32_t bit:{0u,6u,31u})require(call(__imp__XexCheckExecutablePrivilege,c,b,{bit})==0);
  sentinel();require(call(__imp__XexGetModuleHandle,c,b,{0,data})==1168&&get(0)==0);
  const char missing[]="synthetic_missing.xex";std::memcpy(b+data+128,missing,sizeof(missing));
  sentinel();require(call(__imp__XexGetModuleHandle,c,b,{data+128,data})==1168&&get(0)==0);
  sentinel();require(call(__imp__NtQueryVirtualMemory,c,b,{0x7f000000,data,0})==0xc000000du);
  for(unsigned i=0;i<28;++i)require(b[data+i]==0x55);
  require(call(__imp__NtQueryVirtualMemory,c,b,{data,data,99})==0xc000000du);
  for(unsigned i=0;i<28;++i)require(b[data+i]==0x55);
  require(call(__imp__NtQueryVirtualMemory,c,b,{data,data,0})==0);
  require(get(20)==4&&get(16)==0x1000); // PAGE_READWRITE / MEM_COMMIT.
  sentinel();call(__imp__RtlInitAnsiString,c,b,{data,0});
  for(unsigned i=0;i<8;++i)require(b[data+i]==0);
  // A valid original host event lifecycle; never block on a guest wait.
  sentinel();require(call(__imp__NtCreateEvent,c,b,{data,0,0,0})==0);auto handle=get(0);require(handle!=0&&handle!=0x55555555);
  require(call(__imp__NtClose,c,b,{handle})==0);
  require(call(__imp__NtClose,c,b,{handle})==0xc0000008u);
  require(call(__imp__NtWaitForSingleObjectEx,c,b,{handle,0,0,0})==0xc0000008u);
  require(call(__imp__NtCreateEvent,c,b,{data,0,0,0})==0);auto contested=get(0);
  std::array<uint32_t,2> results{};std::atomic<bool> start{false};std::array<std::thread,2> threads;
  for(unsigned i=0;i<2;++i)threads[i]=std::thread([&,i]{PPCContext thread_context{};while(!start.load())std::this_thread::yield();results[i]=call(__imp__NtClose,thread_context,b,{contested});});
  start=true;for(auto& thread:threads)thread.join();
  require((results[0]==0&&results[1]==0xc0000008u)||(results[1]==0&&results[0]==0xc0000008u));
  m->SystemHeapFree(data);
  std::printf("{\"group\":\"entry_import_errors\",\"result\":\"PASS\",\"assertions\":%u,\"selected_exports\":8}\n",checks);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s after %u assertions\n",e.what(),checks);return 1;}
}
