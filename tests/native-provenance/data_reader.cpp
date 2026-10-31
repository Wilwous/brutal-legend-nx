// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Generic host-data reader; no PPC context, function table or guest image.
#include "runtime/offline_image_data.h"
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace brutal::offline;
std::vector<uint8_t> file(const char*path,size_t limit){std::ifstream f(path,std::ios::binary|std::ios::ate);if(!f)throw std::runtime_error("missing data input");auto size=f.tellg();if(size<0||uint64_t(size)>limit)throw std::runtime_error("data input budget");std::vector<uint8_t>b(size);f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),size))throw std::runtime_error("data input read");return b;}
int main(int argc,char**argv){try{
 if(argc!=4)return 2;auto metadata=file(argv[1],16384),source=file(argv[2],64*1024*1024),trust=file(argv[3],32);if(trust.size()!=32)return 2;
 size_t at=0;auto n=[&](){if(metadata.size()-at<8)throw std::runtime_error("truncated data metadata");uint64_t v=0;for(unsigned j=0;j<8;++j)v=(v<<8)|metadata[at++];return v;};
 auto u=[&](){auto v=n();if(v>UINT32_MAX)throw std::runtime_error("data integer overflow");return uint32_t(v);};
 auto d=[&](){if(metadata.size()-at<32)throw std::runtime_error("truncated data digest");Digest v;std::copy_n(metadata.begin()+at,32,v.begin());at+=32;return v;};
 if(n()!=0x444154413031ull)return 2;Plan p;p.schema=u();p.base=u();p.module=n();p.generation=n();p.original=d();p.source=d();auto count=n();if(!count||count>64)return 2;
 for(uint64_t j=0;j<count;++j){Interval r;r.address=u();r.offset=u();r.size=u();r.section=n();r.declared=Declared(u());r.provenance=Provenance(u());r.bytes=d();p.intervals.push_back(r);}if(at!=metadata.size())return 2;
 Digest trusted;std::copy(trust.begin(),trust.end(),trusted.begin());Owner owner(p.module,p.generation);unsigned failures=0,guest_refusals=0;uint64_t compared=0;std::vector<uint8_t>observed;
 try{Data injected(owner,p,source,trusted,3);}catch(const Error&e){if(e.fault!=Fault::exhausted)throw;++failures;}
 require(failures==1&&Data::totals()==std::pair<uint64_t,uint64_t>{0,0},"data rollback accounting");
 {Data data(owner,p,source,trusted);
  for(size_t i=0;i<p.intervals.size();++i){auto&r=p.intervals[i];auto bytes=data.copy_bytes(data.view(i,0,r.size));for(size_t j=0;j<bytes.size();++j){if(bytes[j]!=source[r.offset+j])throw std::runtime_error("data byte oracle");++compared;}observed.insert(observed.end(),bytes.begin(),bytes.end());try{data.require_guest_access(i);}catch(const Error&){++guest_refusals;}}
  data.close();require(Data::totals()==std::pair<uint64_t,uint64_t>{0,0},"data cleanup accounting");
 }
 owner.close();require(guest_refusals==p.intervals.size()&&digest(source)==p.source,"guest denial/source immutability");
 require(brutal::horizon::Space::totals()==std::pair<size_t,size_t>{0,0},"unexpected guest allocation");
 auto copied_hash=digest(observed);std::vector<uint8_t>().swap(observed);
 std::cout<<"{\"copied_bytes\":"<<compared<<",\"intervals\":"<<p.intervals.size()<<",\"expected_guest_refusals\":"<<guest_refusals<<",\"rollback_refusals\":"<<failures<<",\"live_buffers\":0,\"live_bytes\":0,\"guest_images\":0,\"guest_execution\":0,\"copy_sha256\":\"";
 for(auto b:copied_hash)std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(b);std::cout<<"\"}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
