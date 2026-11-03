// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/header_buffer.h"
#include <iostream>
#include <thread>
#include <latch>
using namespace brutal::horizon;
namespace hb=brutal::horizon::header_buffer;
unsigned positive{},negative{},bulk{};
void check(bool b){if(!b)throw std::runtime_error("header lifecycle assertion");++positive;}
template<class F>void no(F fn){try{fn();}catch(const Error&){++negative;return;}throw std::runtime_error("missing header refusal");}
void bytes_equal(std::span<const std::byte>a,std::span<const uint8_t>b){if(a.size()!=b.size())throw std::runtime_error("oracle length");for(size_t i=0;i<a.size();++i){if(a[i]!=std::byte(b[i]))throw std::runtime_error("independent header byte oracle");++bulk;}}
std::vector<uint8_t> fixture(){return {0x58,0x45,0x58,0x32,0,0,0,1,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,1,0,2,4,1,0x12,0x34,0x56,0x78};}
hb::Source spec(ModuleLife&m,const std::vector<uint8_t>&b,uint64_t guest=0x40000000){return {m.identity,m.instance,0,b.size(),guest,hb::digest(b),hb::digest(b)};}
void empty(Space&s){check(s.live()==0&&s.leases()==0&&hb::Buffer::registrations()==0);}
int main(){try{
 auto b=fixture();Space s(2*1024*1024);ModuleLife m(7),other(7);auto p=spec(m,b);auto trust=hb::identity(p);
 auto bad=[&](auto modify){auto q=p;modify(q);no([&]{hb::Buffer x(s,m,q,b,hb::identity(q));});empty(s);};
 bad([](auto&q){q.source[0]^=1;});bad([](auto&q){q.header[0]^=1;});bad([](auto&q){q.length=0;});bad([](auto&q){q.length=UINT64_MAX;});bad([](auto&q){q.offset=UINT64_MAX;});bad([](auto&q){q.offset=4;});bad([](auto&q){q.guest++;});bad([](auto&q){q.guest=0xfffff000;q.length=8192;});bad([](auto&q){q.guest=0x100000000ull;});bad([](auto&q){q.module++;});bad([](auto&q){q.generation++;});
 auto wrong=trust;wrong[0]^=1;no([&]{hb::Buffer x(s,m,p,b,wrong);});auto truncated=b;truncated.pop_back();no([&]{hb::Buffer x(s,m,p,truncated,trust);});
 for(auto i:{0,8,20}){auto corrupt=b;corrupt[size_t(i)]^=0xff;auto q=spec(m,corrupt);no([&]{hb::Buffer x(s,m,q,corrupt,hb::identity(q));});empty(s);}
 for(int stage=0;stage<6;++stage){no([&]{hb::Buffer x(s,m,p,b,trust,stage);});empty(s);}
 {Space small(16);no([&]{hb::Buffer x(small,m,p,b,trust);});empty(small);}
 auto collision=s.allocate(32,uint32_t(p.guest));s.store<uint32_t>(collision.address,0xaabbccdd);no([&]{hb::Buffer x(s,m,p,b,trust);});check(s.live()==1&&s.load<uint32_t>(collision.address)==0xaabbccdd);s.release(collision);
 {Space isolated(65536);hb::Buffer x(isolated,m,p,b,trust);auto ref=x.reference();auto h=x.registration(m,ref);auto info=isolated.info(h);
  check(info.committed&&info.permission==Permission::read&&info.read_only_sealed);
  auto r=x.acquire(m,ref);bytes_equal(r.read(0,b.size()),b);check(isolated.load<uint32_t>(ref.guest+28)==0x12345678);
  no([&]{isolated.write(ref.guest,std::as_bytes(std::span(b)));});no([&]{isolated.view(ref.guest,4,Access(2));});
  no([&]{isolated.protect(h,Permission::read_write);});no([&]{isolated.protect(h,Permission::write);});no([&]{isolated.protect(h,Permission(99));});no([&]{isolated.commit(h);});
  no([&]{isolated.read(ref.guest+ref.length,1);});no([&]{r.read(UINT64_MAX,4);});no([&]{r.read(31,2);});no([&]{r.read(0,0);});
  no([&]{isolated.release(h);});no([&]{isolated.decommit(h);});auto forged=h;forged.owner++;no([&]{isolated.protect(forged,Permission::read);});
  no([&]{x.acquire(other,ref);});auto stale=ref;stale.buffer++;no([&]{x.acquire(m,stale);});no([&]{hb::Buffer duplicate(isolated,m,p,b,trust);});
  isolated.protect(h,Permission::none);no([&]{r.read(0,4);});no([&]{x.acquire(m,ref);});isolated.protect(h,Permission::read);bytes_equal(r.read(0,b.size()),b);
  {auto moved=std::move(r);no([&]{r.read(0,4);});check(!x.close());no([&]{x.acquire(m,ref);});no([&]{x.close();});bytes_equal(moved.read(0,b.size()),b);}
  check(x.closed()&&isolated.live()==0&&isolated.leases()==0);no([&]{x.reference();});no([&]{x.acquire(m,ref);});no([&]{x.close();});
 }
 {Space isolated(65536);hb::Buffer x(isolated,m,p,b,trust);auto ref=x.reference();auto h=x.registration(m,ref);auto v=isolated.view(ref.guest,4,Access::read);check(x.close());no([&]{isolated.read(v);});
  hb::Buffer replacement(isolated,m,p,b,trust);auto fresh=replacement.reference();check(fresh.guest==ref.guest&&fresh.buffer!=ref.buffer);no([&]{replacement.acquire(m,ref);});no([&]{isolated.release(h);});check(replacement.close());empty(isolated);
 }
 {Space isolated(65536);auto q=spec(other,b,0x40001000);hb::Buffer x(isolated,m,p,b,trust),y(isolated,other,q,b,hb::identity(q));auto a=x.acquire(m,x.reference()),c=y.acquire(other,y.reference());bytes_equal(a.read(0,32),b);bytes_equal(c.read(0,32),b);no([&]{y.acquire(m,x.reference());});}
 {Space isolated(65536);ModuleLife module(9);auto q=spec(module,b);auto x=std::make_unique<hb::Buffer>(isolated,module,q,b,hb::identity(q));auto ref=x->reference();auto h=x->registration(module,ref);
  std::latch ready(1),resume(1);std::vector<std::byte> observed;std::exception_ptr error;
  std::thread worker([&]{bool announced=false;try{auto reader=x->acquire(module,ref);announced=true;ready.count_down();resume.wait();observed=reader.read(0,32);}catch(...){error=std::current_exception();if(!announced)ready.count_down();}});
  ready.wait();std::exception_ptr main_error;try{check(!module.close());check(!x->close());no([&]{x->acquire(module,ref);});no([&]{isolated.release(h);});no([&]{isolated.decommit(h);});x.reset();check(isolated.live()==1&&isolated.leases()==1);}catch(...){main_error=std::current_exception();}resume.count_down();worker.join();if(main_error)std::rethrow_exception(main_error);if(error)std::rethrow_exception(error);bytes_equal(observed,b);empty(isolated);check(module.close());
 }
 {Space isolated(512*1024);for(unsigned i=0;i<32;++i){hb::Buffer x(isolated,m,p,b,trust);{auto r=x.acquire(m,x.reference());bytes_equal(r.read(0,32),b);}check(x.close());empty(isolated);}}
 {Space isolated(8192);hb::Buffer x(isolated,m,p,b,trust);auto ref=x.reference();std::optional<hb::Buffer::Reader> reader;reader.emplace(x.acquire(m,ref));
  {auto external=isolated.pin(ref.guest,32);check(!x.close());reader.reset();check(!x.closed()&&isolated.live()==1&&isolated.leases()==2);no([&]{x.acquire(m,ref);});check(!x.close());check(isolated.load<uint32_t>(ref.guest+28)==0x12345678);}
  check(x.close());empty(isolated);
 }
 {Space isolated(8192),foreign(4096);auto h=isolated.allocate(32);Space::Lease first(isolated,h);no([&]{foreign.release_exclusive(first);});
  {Space::Lease second(isolated,h);check(!isolated.release_exclusive(first));check(isolated.live()==1&&isolated.leases()==2);}
  check(isolated.release_exclusive(first));no([&]{isolated.release_exclusive(first);});empty(isolated);
 }
 {Space isolated(8192);ModuleLife retired(11);auto q=spec(retired,b);check(retired.close());no([&]{hb::Buffer closed(isolated,retired,q,b,hb::identity(q));});empty(isolated);}
 // Central authority independently denies unsupported access and irrevocable RW.
 {Space isolated(8192);auto h=isolated.reserve(32,0x60000000);no([&]{isolated.seal_read_only(h);});no([&]{isolated.view(h.address,4,Access::read);});isolated.commit(h);auto v=isolated.view(h.address,4,Access::read);auto old_write=isolated.view(h.address,4,Access::write);isolated.seal_read_only(h);std::array<std::byte,4> zeros{};no([&]{isolated.write(old_write,zeros);});no([&]{isolated.seal_read_only(h);});no([&]{isolated.view(h.address,4,Access(99));});isolated.decommit(h);no([&]{isolated.commit(h,Permission::read_write);});isolated.commit(h,Permission::read);no([&]{isolated.read(v);});isolated.release(h);empty(isolated);}
 empty(s);check(Space::totals()==std::pair<size_t,size_t>{0,0});check(m.close()&&other.close());
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"byte_comparisons\":"<<bulk<<",\"header_registrations\":"<<hb::Buffer::registrations()<<",\"PPC_entries\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
