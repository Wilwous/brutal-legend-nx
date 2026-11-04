// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/data_projection.h"
#include <iostream>
using namespace brutal::horizon;namespace nm=brutal::horizon::native_module;namespace off=brutal::offline;
unsigned pos{},neg{},bulk{};void check(bool v){if(!v)throw std::runtime_error("projection assertion");++pos;}template<class F>void no(F f){try{f();}catch(const Error&){++neg;return;}throw std::runtime_error("missing projection refusal");}
int main(){try{std::vector<uint8_t>b(8192);for(size_t i=0;i<b.size();++i)b[i]=uint8_t(i*13+7);Space s(256*1024);ModuleLife m(77);off::Plan p;p.base=0x60000000;p.module=m.identity;p.generation=m.instance;p.original.fill(1);p.source=off::digest(b);
 p.intervals={{p.base,0,3704,1,off::Declared::read_write,off::Provenance::verified_derived,off::digest(std::span(b).first(3704))},{p.base+3704,3704,392,0,off::Declared::unknown,off::Provenance::verified_derived,off::digest(std::span(b).subspan(3704,392))},{p.base+4096,4096,4096,2,off::Declared::read_execute,off::Provenance::verified_derived,off::digest(std::span(b).subspan(4096))}};auto trusted=off::identity(p);
 for(int stage=0;stage<7;++stage){no([&]{nm::DataProjection fail(s,m,p,b,trusted,stage);});check(s.live()==0&&s.leases()==0);}
 {nm::DataProjection projection(s,m,p,b,trusted);check(projection.mapped().size()==2&&projection.excluded().size()==1);auto r=projection.acquire(m,projection.reference());for(size_t i=0;i<projection.mapped().size();++i){auto&v=projection.mapped()[i];auto copy=r.read(i,0,v.size);for(size_t j=0;j<copy.size();++j){if(copy[j]!=std::byte(b[v.offset+j]))throw std::runtime_error("separate byte oracle");++bulk;}}
  check(s.load<uint32_t>(p.base+3700)==(uint32_t(b[3700])<<24|uint32_t(b[3701])<<16|uint32_t(b[3702])<<8|b[3703]));
  no([&]{s.read(p.base+3704,1);});no([&]{s.read(p.base+4095,1);});no([&]{s.read(p.base+3702,4);});no([&]{s.store<uint32_t>(p.base,1);});no([&]{s.view(p.base+4096,4,Access(2));});no([&]{s.query(p.base);});check(!projection.close());}
 check(s.live()==0&&s.leases()==0);check(m.close());check(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<pos<<",\"refusals\":"<<neg<<",\"byte_comparisons\":"<<bulk<<",\"excluded_bytes\":392,\"allocations\":0,\"leases\":0,\"title_RW_admission\":false}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
