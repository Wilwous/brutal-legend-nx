// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/offline_image_data.h"
#include <iostream>
using namespace brutal::offline;
unsigned positive{},negative{},bulk{};
void check(bool b){if(!b)throw std::runtime_error("data contract assertion");++positive;}
template<class F>void refused(F fn){try{fn();}catch(const Error&){++negative;return;}throw std::runtime_error("missing data refusal");}
int main(){try{
 std::vector<uint8_t>source(64);for(unsigned i=0;i<source.size();++i)source[i]=uint8_t(i*7+3);
 Plan p;p.base=0x1000;p.module=7;p.generation=1;p.original.fill(2);p.source=digest(source);
 p.intervals={{0x1000,0,16,1,Declared::read,Provenance::verified_derived,digest(std::span(source).subspan(0,16))},
 {0x1020,32,12,2,Declared::read_write,Provenance::verified_derived,digest(std::span(source).subspan(32,12))},
 {0x102c,44,4,0,Declared::unknown,Provenance::verified_derived,digest(std::span(source).subspan(44,4))}};
 auto trusted=identity(p);Owner owner(7,1);Data::View stale{};
 auto bad=[&](auto change,bool retain_identity=false){auto q=p;change(q);refused([&]{Data d(owner,q,source,retain_identity?trusted:identity(q));});check(Data::totals()==std::pair<uint64_t,uint64_t>{0,0});};
 bad([](auto&q){q.source[0]^=1;});bad([](auto&q){q.original={};});bad([](auto&q){q.base++;});
 bad([](auto&q){q.intervals[0].offset=UINT32_MAX;});bad([](auto&q){q.intervals[0].size=UINT32_MAX;});
 bad([](auto&q){q.intervals[0].provenance=Provenance::unknown;});bad([](auto&q){q.intervals[1].address=q.base;});
 bad([](auto&q){q.intervals[0].bytes[0]^=1;});bad([](auto&q){q.module=8;});bad([](auto&q){q.generation=2;});
 bad([](auto&q){q.intervals[2].declared=Declared::read_write;});bad([](auto&q){q.schema=2;});
 bad([](auto&q){q.original[0]^=1;},true);bad([](auto&q){q.intervals[0].section=99;},true);
 auto missing=source;missing.pop_back();refused([&]{Data d(owner,p,missing,trusted);});
 for(int stage=0;stage<8;++stage){refused([&]{Data fail(owner,p,source,trusted,stage);});check(Data::totals()==std::pair<uint64_t,uint64_t>{0,0});}
 {Data d(owner,p,source,trusted);check(Data::totals()==std::pair<uint64_t,uint64_t>{1,32});
  for(size_t i=0;i<p.intervals.size();++i){auto&r=p.intervals[i];auto bytes=d.copy_bytes(d.view(i,0,r.size));for(uint32_t j=0;j<r.size;++j){if(bytes[j]!=source[r.offset+j])throw std::runtime_error("independent data byte oracle");++bulk;}}
  refused([&]{Data duplicate(owner,p,source,trusted);});refused([&]{owner.close();});
  for(size_t i=0;i<3;++i)refused([&]{d.require_guest_access(i);});
  refused([&]{d.view(1,11,2);});refused([&]{d.view(3,0,1);});
  stale=d.view(0,0,4);auto forged=stale;forged.offset=UINT32_MAX;refused([&]{d.copy_bytes(forged);});
  source[0]^=1;check(d.copy_bytes(stale)[0]!=source[0]);source[0]^=1;
  d.close();check(Data::totals()==std::pair<uint64_t,uint64_t>{0,0});refused([&]{d.copy_bytes(stale);});d.close();
 }
 {Data repeated(owner,p,source,trusted);refused([&]{repeated.copy_bytes(stale);});}
 check(Data::totals()==std::pair<uint64_t,uint64_t>{0,0});owner.close();refused([&]{Data closed(owner,p,source,trusted);});
 refused([&]{Owner missing_owner(0,1);});
 // Original synthetic guest fixture only: exact logical bounds can exclude a
 // page remainder. This does not admit a title through NativeImagePlan.
 {brutal::horizon::Space s(8192);auto a=s.allocate(3704,0x4000),neighbor=s.allocate(16,0x5000);
  s.store<uint32_t>(a.address+3700,0x12345678);s.store<uint32_t>(neighbor.address,0xabcdef01);
  refused([&]{s.read(a.address+3704,1);});refused([&]{s.read(a.address+3702,4);});
  s.protect(a,brutal::foundation::Permission::read);refused([&]{s.store<uint32_t>(a.address,0);});
  check(s.load<uint32_t>(a.address+3700)==0x12345678&&s.load<uint32_t>(neighbor.address)==0xabcdef01);
  s.release(a);s.release(neighbor);check(s.live()==0&&s.leases()==0);
 }
 check(brutal::horizon::Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"byte_comparisons\":"<<bulk<<",\"live_data\":0,\"guest_images\":0,\"guest_execution\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
