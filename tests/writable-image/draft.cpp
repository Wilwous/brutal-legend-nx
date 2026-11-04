// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Data-only provenance refusal; no PPC context, function map or image loader.
#include "runtime/horizon/writable_image.h"
#include <fstream>
#include <iostream>
using namespace brutal::horizon;namespace rw=writable_image;
int main(int argc,char**argv){if(argc!=5)return 2;try{std::ifstream f(argv[1],std::ios::binary|std::ios::ate);if(!f||f.tellg()<0||uint64_t(f.tellg())>64*1024*1024)return 2;std::vector<uint8_t>b(size_t(f.tellg()));f.seekg(0);if(!f.read(reinterpret_cast<char*>(b.data()),b.size()))return 2;auto offset=std::stoull(argv[2]),length=std::stoull(argv[3]),guest=std::stoull(argv[4]);if(offset>b.size()||!length||length>b.size()-offset)return 2;Space s(65536);ModuleLife m(77);rw::Source p{m.identity,m.instance,offset,length,guest,rw::digest(b),rw::digest(std::span(b).subspan(offset,length)),rw::Authority::provenance_only};bool refused=false;try{rw::Region region(s,m,p,b,rw::identity(p));}catch(const Error&e){if(e.fault!=Fault::unsupported)throw;refused=true;}if(!refused||s.live()||s.leases()||rw::Region::registrations()||!m.close())return 1;std::cout<<"{\"provenance_only_refused\":true,\"mapped_RW_bytes\":0,\"allocations\":0,\"leases\":0,\"PPC_entries\":0}\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
