// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Generic RO forensic projection; no PPC context, function table or image admission.
#include "runtime/horizon/data_projection.h"
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
 Digest trusted;std::copy(trust.begin(),trust.end(),trusted.begin());
 brutal::horizon::Space space(1024*1024);brutal::horizon::ModuleLife module(p.module);
 brutal::horizon::native_module::DataProjection projection(space,module,p,source,trusted);uint64_t copied=0,excluded=0;unsigned read_refusals=0,write_refusals=0;std::vector<uint8_t> observed;
 {auto reader=projection.acquire(module,projection.reference());for(size_t i=0;i<projection.mapped().size();++i){auto&r=projection.mapped()[i];auto bytes=reader.read(i,0,r.size);for(size_t j=0;j<bytes.size();++j){if(bytes[j]!=std::byte(source[r.offset+j]))throw std::runtime_error("projection byte oracle");observed.push_back(std::to_integer<uint8_t>(bytes[j]));++copied;}
  try{space.store<uint32_t>(r.address,0);throw std::runtime_error("read-only data allowed write");}catch(const Error&){++write_refusals;}}
  for(auto&r:projection.excluded()){excluded+=r.size;for(auto address:{r.address,r.address+r.size-1}){try{space.read(address,1);throw std::runtime_error("unknown region accessible");}catch(const Error&){++read_refusals;}}}
 }
 require(projection.close()&&space.live()==0&&space.leases()==0&&module.close(),"projection cleanup");auto hash=digest(observed);
 std::cout<<"{\"readable_bytes\":"<<copied<<",\"excluded_bytes\":"<<excluded<<",\"unknown_read_refusals\":"<<read_refusals<<",\"write_refusals\":"<<write_refusals<<",\"copy_sha256\":\"";
 for(auto b:hash)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);
 std::cout<<"\",\"allocations\":0,\"leases\":0,\"PPC_entries\":0,\"original_RW_admission\":false}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
