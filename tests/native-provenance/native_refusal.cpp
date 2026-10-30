// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Generic DATA reader. Never constructs Image/Space, maps memory or calls a guest PC.
#include "runtime/horizon/native_image_plan.h"
#include <fstream>
#include <iostream>
using namespace brutal::horizon::native_image;
std::vector<uint8_t> read(const char*name){std::ifstream f(name,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("input missing");auto size=f.tellg();if(size<0||size>64*1024*1024)throw std::runtime_error("input budget");std::vector<uint8_t>b(size);f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),size))throw std::runtime_error("input read");return b;}
int main(int argc,char**argv){try{
 if(argc!=3)return 2;auto data=read(argv[1]),source=read(argv[2]);size_t at=0;
 auto n=[&](){if(at+8>data.size())throw std::runtime_error("truncated metadata");uint64_t v=0;for(unsigned i=0;i<8;++i)v=(v<<8)|data[at++];return v;};
 if(data.size()<4||std::string_view(reinterpret_cast<const char*>(data.data()),4)!="ND01")return 2;at=4;
 auto u=[&](){auto value=n();if(value>UINT32_MAX)throw std::runtime_error("metadata integer overflow");return uint32_t(value);};
 auto bit=[&](){auto value=u();if(value>1)throw std::runtime_error("metadata boolean");return value!=0;};
 Plan p;p.schema=u();p.provenance=Provenance(u());p.module=n();p.lifetime=n();p.base=u();p.limit=u();p.entry=u();p.tls_address=u();p.tls_file_size=u();p.tls_total_size=u();p.tls_slots=u();p.relocations=u();
 for(auto*d:{&p.source,&p.code,&p.sdk,&p.runtime})for(auto&b:*d){if(at>=data.size())return 2;b=data[at++];}
 auto count=n();if(count>4096)return 2;
 for(size_t i=0;i<count;++i){Region r;r.owner=n();r.address=u();r.size=u();r.source_offset=u();r.file_size=u();r.zero_size=u();r.alignment=u();r.rights=Rights(u());r.init=Init(u());r.reserve=bit();r.commit=bit();p.regions.push_back(r);}
 if(at!=data.size())return 2;unsigned refusals=0;
 try{validate(p,source,identity(p));return 3;}catch(const brutal::horizon::Error&){++refusals;}
 // Diagnostic isolation of origin gate, not a truncated/admitted title plan.
 auto origin_control=p;origin_control.regions.clear();
 try{validate(origin_control,source,identity(origin_control));return 3;}catch(const brutal::horizon::Error&){++refusals;}
 std::cout<<"{\"expected_refusals\":"<<refusals<<",\"source_identity_match\":"<<(digest(source)==p.source?"true":"false")<<",\"guest_images_constructed\":0,\"guest_instructions\":0}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
