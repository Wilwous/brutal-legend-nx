// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Generic bounded DATA-only reader: no instruction generator, loader or dispatch.
#include "runtime/horizon/header_buffer.h"
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace brutal::horizon;
namespace hb=brutal::horizon::header_buffer;
unsigned positive{},negative{},bulk{};
void check(bool b){if(!b)throw std::runtime_error("header data assertion");++positive;}
template<class F>void no(F fn){try{fn();}catch(const Error&){++negative;return;}throw std::runtime_error("header data missing refusal");}
std::vector<uint8_t> file(const char* path,size_t max){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||uint64_t(f.tellg())>max)throw std::runtime_error("data input extent");std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),std::streamsize(b.size())))throw std::runtime_error("data read");return b;}
int main(int argc,char**argv){if(argc!=4)return 2;try{
 auto source=file(argv[1],64*1024*1024),metadata=file(argv[2],112),trusted=file(argv[3],32);check(metadata.size()==112&&trusted.size()==32);
 auto n=[&](size_t o){uint64_t v=0;for(unsigned i=0;i<8;++i)v=(v<<8)|metadata[o+i];return v;};check(n(0)==0x4845414445523031ull);
 hb::Source p{n(8),n(16),n(24),n(32),n(40)};std::copy_n(metadata.begin()+48,32,p.source.begin());std::copy_n(metadata.begin()+80,32,p.header.begin());hb::Digest expected;std::copy(trusted.begin(),trusted.end(),expected.begin());
 ModuleLife module(p.module);Space space(1024*1024);hb::Buffer buffer(space,module,p,source,expected);auto ref=buffer.reference();auto h=buffer.registration(module,ref);auto info=space.info(h);check(info.committed&&info.read_only_sealed&&info.permission==Permission::read);hb::Digest observed{};
 {auto reader=buffer.acquire(module,ref);auto copy=reader.read(0,p.length);check(copy.size()==p.length);
  for(size_t i=0;i<copy.size();++i){if(copy[i]!=std::byte(source[size_t(p.offset)+i]))throw std::runtime_error("source byte mismatch");++bulk;}
  observed=hb::digest(std::span(reinterpret_cast<const uint8_t*>(copy.data()),copy.size()));check(observed==p.header);
  std::array<std::byte,1> change{std::byte{}};no([&]{space.write(ref.guest,change);});no([&]{space.view(ref.guest,4,Access(2));});no([&]{reader.read(p.length,1);});no([&]{space.release(h);});no([&]{space.decommit(h);});
  check(space.read(space.view(ref.guest,uint32_t(p.length),Access::read))==copy);
 }
 check(buffer.close());check(space.live()==0&&space.leases()==0&&hb::Buffer::registrations()==0);check(module.close());check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"byte_comparisons\":"<<bulk<<",\"header_sha256\":\"";
 for(auto v:observed)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(v);
 std::cout<<std::dec<<"\",\"guest\":"<<ref.guest<<",\"allocations\":0,\"leases\":0,\"executable_images\":0,\"PPC_entries\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
