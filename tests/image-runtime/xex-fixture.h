// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Original, unencrypted XEX2/PE32 loader-only fixture. No game bytes or codegen.
#include <vector>
#include <cstdint>
#include <cstring>
namespace image_fixture {
inline void put(std::vector<uint8_t>& b,size_t p,uint32_t v,unsigned n,bool big){
 for(unsigned i=0;i<n;++i)b.at(p+i)=uint8_t(v>>(8*(big?n-1-i:i)));
}
inline std::vector<uint8_t> xex(uint32_t base=0x82000000){
 constexpr uint32_t h=0x1000,size=0x30000,sec=0x100,fmt=0x300;
 std::vector<uint8_t>b(h+size);
 auto be=[&](size_t p,uint32_t v){put(b,p,v,4,true);};
 auto le=[&](size_t p,uint32_t v,unsigned n=4){put(b,h+p,v,n,false);};
 be(0,0x58455832);be(8,h);be(16,sec);be(20,3);
 be(24,0x000003ff);be(28,fmt);be(32,0x00010201);be(36,base);
 be(40,0x00010100);be(44,base+0x1000);
 be(sec,0x1cc);be(sec+4,size);be(sec+0x110,base);be(sec+0x180,3);
 // Descriptor word: count in upper 28 bits, type in low nibble.
 for(unsigned i=0;i<3;++i)be(sec+0x184+24*i,0x10+(i==0?1:i==1?3:2));
 be(fmt,8); // encryption/compression u16 both zero.
 le(0,0x00905a4d);le(0x3c,0x80);le(0x80,0x4550);
 le(0x84,0x1f2,2);le(0x86,3,2);le(0x94,224,2);le(0x96,0x100,2);
 constexpr unsigned opt=0x98;le(opt,0x10b,2);le(opt+16,0x1000);le(opt+28,base);
 le(opt+32,0x10000);le(opt+36,0x200);le(opt+56,size);le(opt+60,0x1000);le(opt+68,14,2);
 const char* names[]={".text",".rdata",".data"};
 for(unsigned i=0;i<3;++i){unsigned p=opt+224+40*i;std::memcpy(b.data()+h+p,names[i],std::strlen(names[i]));
  le(p+8,0x10000);le(p+12,i*0x10000);le(p+16,0x10000);le(p+20,i*0x10000);
  le(p+36,i==0?0x60000020:i==1?0x40000040:0xc0000040);
 }
 b[h+0x10000]=0x5a;b[h+0x20000]=0xa5;return b;
}
}
