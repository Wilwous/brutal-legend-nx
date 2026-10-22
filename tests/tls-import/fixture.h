// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "../image-runtime/xex-fixture.h"
#include "catalog.h"
namespace tls_fixture {
inline void be(std::vector<uint8_t>&b,size_t p,uint32_t v){image_fixture::put(b,p,v,4,true);}
inline std::vector<uint8_t> image(uint32_t base=0x82000000,uint8_t seed=0x31,bool all_imports=false){
 auto b=image_fixture::xex(base);be(b,20,5);
 be(b,48,0x20104);be(b,52,0x380);be(b,56,0x103ff);be(b,60,0x400);
 be(b,0x380,64);be(b,0x384,base+0x10000);be(b,0x388,32);be(b,0x38c,16);
 for(unsigned i=0;i<16;++i)b[0x11000+i]=seed+i;
 // Independent import-table records, not manually created TLS runtime state.
 be(b,0x404,24);be(b,0x408,2);
 std::memcpy(b.data()+0x40c,"xboxkrnl.exe",13);std::memcpy(b.data()+0x41c,"xam.xex",8);
 unsigned index=0,pos=0x424;
 const uint32_t ordinals[]{0x154,0x155,0x135,0x83,0x3cb,0x3cd};
 for(unsigned lib=0;lib<2;++lib){std::vector<uint32_t> selected;
  if(all_imports){for(auto entry:matrix_imports)if(std::strcmp(entry.library,lib?"xam.xex":"xboxkrnl.exe")==0)selected.push_back(entry.ordinal);}
  else for(unsigned n=lib?4:0;n<(lib?6:4);++n)selected.push_back(ordinals[n]);
  be(b,pos,40+selected.size()*8);image_fixture::put(b,pos+36,lib,2,true);image_fixture::put(b,pos+38,selected.size()*2,2,true);
  for(unsigned n=0;n<selected.size();++n,++index){be(b,pos+40+n*8,base+0x20000+index*8);be(b,pos+44+n*8,base+0x20004+index*8);
   be(b,0x21000+index*8,selected[n]);be(b,0x21004+index*8,0x01000000|selected[n]);}
  pos+=40+selected.size()*8;
 }
 be(b,0x400,pos-0x400);
 return b;
}
}
