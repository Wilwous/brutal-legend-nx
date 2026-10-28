// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Independently authored load-only corpus. No title bytes or generated PPC.
#include "tests/image-runtime/xex-fixture.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <array>
int main(int argc,char**argv){
 if(argc!=2)return 2;std::filesystem::path out=argv[1];std::filesystem::create_directories(out);
 const std::array names{"plain","raw-zero","raw-short","pe-overhang","owned-tail","basic-zero","basic-gap","none-short","security-larger","no-read","overlap","invalid-entry","bad-pe","rounded-owned"};
 for(auto name:names){auto b=image_fixture::xex();std::string n=name;
  auto be=[&](size_t p,uint32_t v){image_fixture::put(b,p,v,4,true);};auto le=[&](size_t p,uint32_t v){image_fixture::put(b,0x1000+p,v,4,false);};
  constexpr size_t third=0x98+224+80;
  // Entry writes a marker then traps if a future executor erroneously invokes it.
  // No CPU executor is linked here. The marker is checked, not the PPC executed.
  for(auto [p,v]:std::array<std::pair<size_t,uint32_t>,5>{{{0x2000,0x3c808202},{0x2004,0x60840040},{0x2008,0x3860006e},{0x200c,0x90640000},{0x2010,0x7fe00008}}})be(p,v);
  if(n=="raw-zero")le(third+16,0);
  if(n=="raw-short")le(third+16,0x200);
  if(n=="pe-overhang")le(third+8,0x10001);
  if(n=="owned-tail"){b.resize(0x41000);be(0x104,0x40000);be(0x284+48,0x22);le(third+8,0x20000);le(0x98+56,0x40000);}
  if(n=="rounded-owned"){b.resize(0x31001);b.back()=0x7b;le(third+8,0x10001);le(0x98+56,0x40000);}
  if(n=="basic-zero"||n=="basic-gap"){
   be(0x300,16);image_fixture::put(b,0x306,1,2,true);be(0x308,0x20000);be(0x30c,n=="basic-zero"?0x10000:0);b.resize(0x21000);
  }
  if(n=="none-short")b.resize(0x21000);
  if(n=="security-larger")be(0x104,0x40000);
  if(n=="no-read")le(third+36,0);
  if(n=="overlap")le(third+12,0x10000);
  if(n=="invalid-entry")be(44,0x82040000);
  if(n=="bad-pe")le(0x80,0);
  std::ofstream f(out/(n+".xex"),std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),b.size());if(!f)return 3;
 }
 return 0;
}
