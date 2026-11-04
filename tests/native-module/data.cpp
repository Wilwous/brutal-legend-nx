// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/sealed_object.h"
#include <fstream>
#include <iostream>
using namespace brutal::horizon;namespace hb=brutal::horizon::header_buffer;namespace nm=brutal::horizon::native_module;
std::vector<uint8_t> load(const char*p,size_t max){std::ifstream f(p,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||uint64_t(f.tellg())>max)throw std::runtime_error("data extent");std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw std::runtime_error("data read");return b;}
int main(int argc,char**argv){if(argc!=5)return 2;try{
 auto source=load(argv[1],64*1024*1024),plan=load(argv[2],112),trusted=load(argv[3],32);if(plan.size()!=112||trusted.size()!=32)throw std::runtime_error("metadata extent");
 auto n=[&](size_t o){uint64_t v=0;for(unsigned i=0;i<8;++i)v=(v<<8)|plan[o+i];return v;};if(n(0)!=0x4845414445523031ull)throw std::runtime_error("metadata schema");
 hb::Source p{n(8),n(16),n(24),n(32),n(40)};std::copy_n(plan.begin()+48,32,p.source.begin());std::copy_n(plan.begin()+80,32,p.header.begin());hb::Digest expected;std::copy(trusted.begin(),trusted.end(),expected.begin());
 Space s(1024*1024);ModuleLife m(p.module);hb::Buffer header(s,m,p,source,expected);nm::Metadata meta(s,m,header,header.reference(),std::stoull(argv[4],nullptr,0));unsigned refusals=0;
 {auto r=meta.acquire(m,meta.reference());if(s.load<uint32_t>(r.base()+0x58)!=p.guest)throw std::runtime_error("pointer association");auto copy=r.header_bytes(0,uint32_t(p.length));for(size_t i=0;i<copy.size();++i)if(copy[i]!=std::byte(source[size_t(p.offset)+i]))throw std::runtime_error("header bytes");
  for(auto address:{r.base(),r.base()+0x57,r.base()+0x5c}){try{s.read(address,1);throw std::runtime_error("adjacent object field exposed");}catch(const Error&){++refusals;}}
 }
 if(!meta.close()||!header.close()||!m.close()||s.live()||s.leases()||nm::Object::registrations())throw std::runtime_error("cleanup");
 std::cout<<"{\"header_bytes\":"<<p.length<<",\"field_bytes\":4,\"adjacent_refusals\":"<<refusals<<",\"allocations\":0,\"leases\":0,\"PPC_entries\":0,\"imports_bound\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
