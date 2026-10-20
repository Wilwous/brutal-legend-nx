// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Entirely original synthetic resource bytes; no game-derived corpus.
#include <rex/system/util/xdbf_utils.h>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>
using namespace rex::system;
using namespace rex::system::util;
using Bytes=std::vector<uint8_t>;
static unsigned checks=0;
static void require(bool ok){++checks;if(!ok)throw std::runtime_error("synthetic XDBF oracle mismatch");}
static void put(Bytes& b,size_t p,uint64_t v,unsigned n){for(unsigned i=0;i<n;++i)b.at(p+n-1-i)=uint8_t(v>>(8*i));}
static void magic(Bytes& b,size_t p,const char* s){std::memcpy(b.data()+p,s,4);}
static Bytes valid(){
 Bytes b(177);magic(b,0,"XDBF");put(b,4,0x10000,4);put(b,8,3,4);put(b,12,3,4);put(b,16,1,4);put(b,20,1,4);
 auto entry=[&](size_t i,uint16_t section,uint64_t id,uint32_t offset,uint32_t size){size_t p=24+18*i;put(b,p,section,2);put(b,p+2,id,8);put(b,p+10,offset,4);put(b,p+14,size,4);};
 entry(0,3,1,0,25);entry(1,1,0x58535443,25,16);entry(2,1,0x58414348,41,50);put(b,78,91,4);put(b,82,0xffffffffu-91,4);
 magic(b,86,"XSTR");put(b,90,1,4);put(b,94,21,4);put(b,98,1,2);put(b,100,0x8000,2);put(b,102,7,2);std::memcpy(b.data()+104,"SYNTHET",7);
 magic(b,111,"XSTC");put(b,115,1,4);put(b,119,12,4);put(b,123,1,4);
 magic(b,127,"XACH");put(b,131,1,4);put(b,135,46,4);put(b,139,1,2);put(b,141,0x1234,2);put(b,149,0x76543210,4);put(b,173,0x11223344,4);
 return b;
}
static void exercise(const uint8_t* p,size_t n){
 XdbfGameData db(p,n);if(!db.is_valid()){require(db.status()!=XdbfWrapper::Status::kOk);require(!db.icon());require(db.title().empty());require(db.GetAchievements().empty());return;}
 require(db.status()==XdbfWrapper::Status::kOk);auto icon=db.icon();if(icon)require(icon.buffer>=p&&size_t(icon.buffer-p)<=n&&icon.size<=n-size_t(icon.buffer-p));
 (void)db.title();(void)db.title(XLanguage::kEnglish);(void)db.GetAchievements();
}
int main(){try{
 auto good=valid();
 for(size_t alignment=0;alignment<16;++alignment){Bytes b(alignment+good.size());std::memcpy(b.data()+alignment,good.data(),good.size());XdbfGameData db(b.data()+alignment,good.size());require(db.is_valid());require(db.title()=="SYNTHET");auto a=db.GetAchievements();require(a.size()==1&&a[0].id==0x1234&&a[0].image_id==0x76543210&&a[0].unk20==0x11223344);exercise(b.data()+alignment,good.size());}
 for(size_t n=0;n<good.size();++n){XdbfGameData db(good.data(),n);require(!db.is_valid());exercise(good.data(),n);}
 struct Corrupt {size_t offset;uint64_t value;unsigned bytes;};
 Corrupt corrupt[]{{0,0,4},{4,2,4},{8,0xffffffff,4},{12,4,4},{16,0xffffffff,4},{20,2,4},{34,0xffffffff,4},{38,0xffffffff,4},{78,0xfffffffe,4},{82,4,4},{86,0,4},{90,2,4},{94,13,4},{94,26,4},{98,65535,2},{102,65535,2},{111,0,4},{119,15,4},{127,0,4},{135,1000,4},{139,2,2}};
 for(auto c:corrupt){auto b=good;put(b,c.offset,c.value,c.bytes);XdbfGameData db(b.data(),b.size());require(!db.is_valid());exercise(b.data(),b.size());}
 Bytes empty(24);magic(empty,0,"XDBF");put(empty,4,0x10000,4);XdbfGameData e(empty.data(),empty.size());require(e.is_valid()&&e.title().empty());exercise(nullptr,0);
 uint32_t random=0x83f102d9;for(unsigned i=0;i<2000;++i){auto b=good;random=random*1664525+1013904223;size_t p=random%b.size();random=random*1664525+1013904223;b[p]^=uint8_t(random>>24);if(i&1)b.resize(random%(b.size()+1));exercise(b.data(),b.size());}
 std::printf("{\"group\":\"bounded_XDBF_corpus\",\"result\":\"PASS\",\"assertions\":%u,\"alignments\":16,\"truncations\":177,\"malformed\":21,\"mutations\":2000}\n",checks);return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s after %u checks\n",e.what(),checks);return 1;}}
