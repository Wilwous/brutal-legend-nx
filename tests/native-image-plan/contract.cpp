// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/native_image_plan.h"
#include "oracle.h"
#include <iostream>
using namespace brutal::horizon;using namespace brutal::horizon::native_image;
unsigned semantic{},bulk{},negative{};void check(bool v){if(!v)throw std::runtime_error("N1 assertion");++semantic;}
template<class F>void refused(F f){bool no=false;try{f();}catch(const Error&){no=true;}check(no);++negative;}
int main(){try{
 std::vector<uint8_t>source(8192);for(size_t i=0;i<source.size();++i)source[i]=uint8_t(i*17+3);
 Plan p;p.provenance=Provenance::original_synthetic;p.module=7;p.lifetime=1;p.base=0x82000000;p.limit=0x82005000;p.entry=p.base;p.tls_address=p.base+4096;p.tls_file_size=16;p.tls_total_size=32;p.tls_slots=64;
 std::copy(std::begin(expected_source),std::end(expected_source),p.source.begin());p.code.fill(1);p.sdk.fill(1);p.runtime.fill(1);
 p.regions={{1,p.base,4096,0,4096,0,4096,Rights::read_execute,Init::file_then_authored_zero,true,true},{2,p.base+4096,4096,4096,4096,0,4096,Rights::read,Init::file_then_authored_zero,true,true},{3,p.base+8192,8192,0,0,8192,4096,Rights::read_write,Init::file_then_authored_zero,true,true}};
 Digest expected;std::copy(std::begin(expected_plan),std::end(expected_plan),expected.begin());check(identity(p)==expected);check(digest(source)==p.source);
 auto bad=[&](auto modify){auto q=p;modify(q);refused([&]{validate(q,source,identity(q));});};
 bad([](auto&q){q.schema=2;});bad([](auto&q){q.provenance=Provenance::unknown;});bad([](auto&q){q.module=0;});bad([](auto&q){q.lifetime=0;});bad([](auto&q){q.limit=q.base;});bad([](auto&q){q.relocations=1;});bad([](auto&q){q.code={};});bad([](auto&q){q.sdk={};});bad([](auto&q){q.runtime={};});bad([](auto&q){q.regions.clear();});
 bad([](auto&q){q.regions[1].owner=1;});bad([](auto&q){q.regions[1].address=q.base;});bad([](auto&q){q.regions[1].address++;});bad([](auto&q){q.regions[1].size++;});bad([](auto&q){q.regions[1].alignment=1;});bad([](auto&q){q.regions[1].source_offset=UINT32_MAX;});bad([](auto&q){q.regions[1].file_size=UINT32_MAX;});bad([](auto&q){q.regions[1].init=Init::unknown;});bad([](auto&q){q.regions[2].zero_size--;});bad([](auto&q){q.regions[2].address=0xfffff000;});bad([](auto&q){q.regions[0].commit=false;});bad([](auto&q){q.regions[0].reserve=false;});bad([](auto&q){q.regions[0].rights=Rights(7);});bad([](auto&q){q.entry=q.base+0x4000;});bad([](auto&q){q.entry++;});bad([](auto&q){q.tls_address=q.base;});bad([](auto&q){q.tls_file_size=33;});bad([](auto&q){q.tls_slots=65;});bad([](auto&q){q.imports={{0x11b,4}};});bad([](auto&q){q.imports={{0x11b,1},{0x11b,1}};});
 auto wrong=expected;wrong[0]^=1;refused([&]{validate(p,source,wrong);});auto altered=source;altered[0]^=1;refused([&]{validate(p,altered,expected);});
 Space s(1024*1024);for(int stage=0;stage<9;++stage){refused([&]{Image fail(s,p,source,expected,stage);});check(s.live()==0);}
 {Space small(8192);refused([&]{Image fail(small,p,source,expected);});check(small.live()==0);}
 auto neighbor=s.allocate(8192,p.base+8192);s.store<uint32_t>(neighbor.address,0xabcdef01);refused([&]{Image fail(s,p,source,expected);});check(s.live()==1&&s.load<uint32_t>(neighbor.address)==0xabcdef01);s.release(neighbor);
 {Image im(s,p,source,expected);check(im.admitted());check(im.fingerprint()==expected);for(const auto&st:im.states())check(st.declared&&st.reserved&&st.backed&&st.initialized&&st.committed&&st.accessible&&!st.released);
  for(unsigned i=0;i<3;++i){auto&r=p.regions[i];auto bytes=s.read(r.address,r.size);for(unsigned j=0;j<r.size;++j){if(bytes[j]!=(i<2?std::byte(uint8_t((i*4096+j)*17+3)):std::byte{}))throw std::runtime_error("independent byte oracle");++bulk;}}
  check(s.load<uint32_t>(p.base)==0x03142536);refused([&]{s.store<uint32_t>(p.base,0);});refused([&]{s.read(p.base+0x4000,1);});refused([&]{s.read(p.base+0x3fff,2);});
  refused([&]{Image duplicate(s,p,source,expected);});auto stale=s.view(p.base+8192,4,Access::read);
  {auto lease=s.pin(p.base,4);refused([&]{im.unload();});check(im.admitted()&&s.live()==3);}
  im.unload();check(!im.admitted()&&s.live()==0);for(auto st:im.states())check(st.released&&!st.accessible&&!st.committed);refused([&]{s.read(stale);});
 }
 {Image repeat(s,p,source,expected);check(s.load<uint32_t>(p.base+8192)==0);}check(s.live()==0&&s.leases()==0);
 std::cout<<"{\"semantic_passed\":"<<semantic<<",\"negative_refusals\":"<<negative<<",\"semantic_failed\":0,\"byte_comparisons\":"<<bulk<<",\"remaining\":"<<s.live()<<",\"leases\":"<<s.leases()<<"}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
