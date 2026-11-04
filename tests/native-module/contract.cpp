// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/sealed_object.h"
#include <iostream>
#include <latch>
#include <thread>
using namespace brutal::horizon;namespace nm=brutal::horizon::native_module;namespace hb=brutal::horizon::header_buffer;
unsigned pos{},neg{},bytes{};void check(bool x){if(!x)throw std::runtime_error("module assertion");++pos;}template<class F>void no(F f){try{f();}catch(const Error&){++neg;return;}throw std::runtime_error("missing module refusal");}
std::vector<uint8_t> input(){return {0x58,0x45,0x58,0x32,0,0,0,1,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,1,0,2,4,1,0x12,0x34,0x56,0x78};}
hb::Source plan(ModuleLife&m,std::span<const uint8_t>b,uint64_t addr){return {m.identity,m.instance,0,b.size(),addr,hb::digest(b),hb::digest(b)};}
int main(){try{
 Space s(2*1024*1024);ModuleLife m(7),other(7);auto data=input();auto p=plan(m,data,0x40000000);hb::Buffer header(s,m,p,data,hb::identity(p));auto href=header.reference();
 for(int stage=0;stage<4;++stage){no([&]{nm::Metadata fail(s,m,header,href,0x41000000,stage);});check(s.live()==1&&s.leases()==1&&nm::Object::registrations()==0);}
 no([&]{nm::Metadata wrong(s,other,header,href,0x41000000);});no([&]{nm::Metadata wrong(s,m,header,href,0x100000000ull);});
 {nm::Metadata meta(s,m,header,href,0x41000000);auto ref=meta.reference();auto reader=meta.acquire(m,ref);check(reader.base()==0x41000000&&reader.header_address()==href.guest);auto actual=s.read(0x41000058,4);auto expected=nm::word(href.guest);for(unsigned i=0;i<4;++i){check(actual[i]==expected[i]);++bytes;}
  no([&]{s.read(0x41000000,1);});no([&]{s.read(0x41000057,1);});no([&]{s.read(0x4100005c,1);});no([&]{s.read(0x41000056,8);});no([&]{s.store<uint32_t>(0x41000058,0);});no([&]{s.view(0x41000058,4,Access(2));});
  no([&]{nm::Metadata duplicate(s,m,header,href,0x41001000);});no([&]{meta.acquire(other,ref);});auto stale=ref;++stale.object;no([&]{meta.acquire(m,stale);});
  auto h=reader.registrations()[0];no([&]{s.release(h);});no([&]{s.decommit(h);});s.protect(h,Permission::none);no([&]{reader.base();});s.protect(h,Permission::read);check(reader.header_bytes(28,4)==nm::word(0x12345678));
  check(!meta.close());no([&]{meta.acquire(m,ref);});check(!header.close());check(reader.header_bytes(0,4)==std::vector<std::byte>({std::byte{0x58},std::byte{0x45},std::byte{0x58},std::byte{0x32}}));
 }
 check(header.closed()&&s.live()==0&&s.leases()==0&&nm::Object::registrations()==0);
 // Group retirement must refuse without freeing an unpinned sibling.
 {std::array<nm::Bytes,2>b{{{0x50000001,nm::word(1)},{0x50000010,nm::word(2)}}};nm::Object object(s,m,b);std::optional<nm::Object::Reader> r(object.acquire(m,object.reference()));auto handles=r->registrations();
  {auto external=s.pin(handles[1].address,4);check(!object.close());r.reset();check(s.live()==2&&!object.closed());
   check(nm::be32(s.read(handles[0].address,4))==1&&nm::be32(s.read(handles[1].address,4))==2);check(!object.close());}
  no([&]{s.read(0x50000005,1);});check(object.close());
 }
 check(s.live()==0&&s.leases()==0);
 {auto q=plan(other,data,0x40001000);hb::Buffer a(s,m,p,data,hb::identity(p)),b(s,other,q,data,hb::identity(q));nm::Metadata x(s,m,a,a.reference(),0x42000000),y(s,other,b,b.reference(),0x42001000);auto r=x.acquire(m,x.reference());no([&]{y.acquire(m,x.reference());});check(r.header_address()==0x40000000);}
 for(unsigned i=0;i<24;++i){hb::Buffer h(s,m,p,data,hb::identity(p));nm::Metadata x(s,m,h,h.reference(),0x43000000);{auto r=x.acquire(m,x.reference());check(r.header_bytes(28,4)==nm::word(0x12345678));}check(x.close());check(h.close());}
 // An admitted reader survives parent wrapper teardown on a deterministic worker.
 {auto h=std::make_unique<hb::Buffer>(s,m,p,data,hb::identity(p));auto x=std::make_unique<nm::Metadata>(s,m,*h,h->reference(),0x44000000);auto ref=x->reference();std::latch ready(1),resume(1);std::exception_ptr error;uint32_t result=0;
  std::thread worker([&]{bool announced=false;try{auto r=x->acquire(m,ref);announced=true;ready.count_down();resume.wait();result=nm::be32(r.header_bytes(28,4));}catch(...){error=std::current_exception();if(!announced)ready.count_down();}});
  ready.wait();x.reset();h.reset();resume.count_down();worker.join();if(error)std::rethrow_exception(error);check(result==0x12345678);}
 check(s.live()==0&&s.leases()==0&&nm::Object::registrations()==0);check(m.close()&&other.close());check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<pos<<",\"refusals\":"<<neg<<",\"byte_comparisons\":"<<bytes<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
