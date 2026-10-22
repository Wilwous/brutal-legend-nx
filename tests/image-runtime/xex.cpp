// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "xex-fixture.h"
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/system/xex_module.h>
#include <rex/system/user_module.h>
#include <fstream>
#include <filesystem>
#include <cstdio>
#include <stdexcept>
static unsigned checks=0;
static void check(bool v){++checks;if(!v)throw std::runtime_error("XEX ownership oracle "+std::to_string(checks));}
static bool live(rex::memory::Memory& m,uint32_t a){rex::memory::HeapAllocationInfo i{};return m.LookupHeap(a)->QueryRegionInfo(a,&i)&&i.state!=0;}
int main(int argc,char**argv){if(argc!=3)return 2;std::setbuf(stdout,nullptr);rex::InitLoggingEarly();
 try{std::filesystem::path p=argv[1];std::string mode=argv[2];std::filesystem::create_directories(p/"empty");
 rex::Runtime r(p/"empty",p/"user",p/"updates",p/"cache",p/"metadata");check(!r.Setup({.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true}));
 auto& m=*r.memory();auto bytes=image_fixture::xex();auto* heap=m.LookupHeap(0x82000000);
 if(mode=="foreign-allocation"){
  int owner=0;check(heap->AllocFixed(0x83000000,65536,65536,3,3));heap->SetAllocationOwner(0x83000000,&owner);
  rex::runtime::XexModule image(r.kernel_state()->function_dispatcher(),r.kernel_state());
  check(image.Load("original.xex","original.xex",bytes.data(),bytes.size()));
  bool survived=live(m,0x83000000);std::printf("{\"diagnostic\":\"foreign_allocation_after_load\",\"live\":%s}\n",survived?"true":"false");
  check(survived);check(heap->ReleaseOwned(0x83000000,&owner));check(image.Unload());
 }else if(mode=="load-unload"){
  rex::runtime::XexModule image(r.kernel_state()->function_dispatcher(),r.kernel_state());
  check(image.Load("original.xex","original.xex",bytes.data(),bytes.size()));check(image.LoadContinue());
  check(image.pe_sections().size()==3&&image.image_size()==0x30000&&image.entry_point()==0x82001000);
  check(m.IsRangeAccessible(0x82010000,1,false)&&!m.IsRangeAccessible(0x82010000,1,true));
  check(m.IsRangeAccessible(0x82020000,1,true));check(image.Unload());check(!live(m,0x82000000));check(image.Unload());
 }else if(mode=="failed-load-rollback"){
  // Valid container lengths, deliberately invalid PE magic; no truncated reads.
  bytes[0x1000]=0;rex::runtime::XexModule image(r.kernel_state()->function_dispatcher(),r.kernel_state());
  check(!image.Load("invalid.xex","invalid.xex",bytes.data(),bytes.size()));
  bool retained=live(m,0x82000000);std::printf("{\"diagnostic\":\"failed_load_pages\",\"retained\":%s}\n",retained?"true":"false");check(!retained);
 }else if(mode=="user-module-teardown"){
  {std::ofstream file(p/"empty/original.xex",std::ios::binary);file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());check(bool(file));}
  auto module=rex::system::object_ref<rex::system::UserModule>(new rex::system::UserModule(r.kernel_state()));
  check(module->LoadFromFile("game:\\original.xex")==0);auto header=module->guest_xex_header(),handle=module->hmodule_ptr();
  check(header&&handle&&live(m,header)&&live(m,handle));check(module->Unload()==0);check(module->ReleaseHandle());module.reset();
  check(!live(m,handle));bool retained=live(m,header);
  std::printf("{\"diagnostic\":\"user_module_header_after_final_owner\",\"retained\":%s}\n",retained?"true":"false");check(!retained);
 }else return 2;
 std::printf("{\"group\":\"xex_%s\",\"result\":\"PASS\",\"assertions\":%u}\n",mode.c_str(),checks);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
