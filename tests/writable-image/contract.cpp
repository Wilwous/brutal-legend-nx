// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/writable_image.h"
#include "runtime/horizon/sealed_object.h"
#include <iostream>
#include <thread>
#include <latch>
using namespace brutal::horizon;namespace rw=writable_image;
unsigned pos{},neg{},bytes{};void check(bool b){if(!b)throw std::runtime_error("RW assertion");++pos;}template<class F>void no(F f){try{f();}catch(const Error&){++neg;return;}throw std::runtime_error("missing RW refusal");}
rw::Source plan(ModuleLife&m,std::span<const uint8_t>b,uint32_t address=0x60000008){return {m.identity,m.instance,4,8,address,rw::digest(b),rw::digest(b.subspan(4,8)),rw::Authority::original_synthetic};}
int main(){try{Space s(1024*1024);ModuleLife m(77),foreign(77);std::vector<uint8_t> b(32);for(unsigned i=0;i<b.size();++i)b[i]=uint8_t(i*7+3);auto p=plan(m,b);
 for(int stage=0;stage<7;++stage){no([&]{rw::Region bad(s,m,p,b,rw::identity(p),stage);});check(s.live()==0&&s.leases()==0&&rw::Region::registrations()==0);}
 {Space tight(4096);auto existing=tight.allocate(4);no([&]{rw::Region fail(tight,m,p,b,rw::identity(p));});check(tight.live()==1&&tight.leases()==0);tight.release(existing);}
 auto bad=p;bad.authority=rw::Authority::provenance_only;no([&]{rw::Region x(s,m,bad,b,rw::identity(bad));});bad=p;++bad.generation;no([&]{rw::Region x(s,m,bad,b,rw::identity(bad));});bad=p;bad.source[0]^=1;no([&]{rw::Region x(s,m,bad,b,rw::identity(bad));});bad=p;bad.offset=UINT64_MAX;no([&]{rw::Region x(s,m,bad,b,rw::identity(bad));});bad=p;bad.guest=0xffffffff;no([&]{rw::Region x(s,m,bad,b,rw::identity(bad));});no([&]{rw::Region x(s,foreign,p,b,rw::identity(p));});
 rw::Region::Ref old;Space::Handle stale{};
 {rw::Region region(s,m,p,b,rw::identity(p));old=region.reference();auto borrow=region.acquire(m,old);stale=borrow.registration();auto copy=borrow.read(0,8);for(unsigned i=0;i<8;++i){if(copy[i]!=std::byte(b[i+4]))throw std::runtime_error("source byte oracle");++bytes;}
 no([&]{region.acquire(foreign,old);});no([&]{s.release(stale);});no([&]{s.decommit(stale);});
 for(auto [a,n]:std::array<std::pair<uint32_t,uint32_t>,4>{{{p.guest-1,1},{p.guest+8,1},{p.guest-1,4},{p.guest+6,4}}}){no([&]{s.read(a,n);});std::vector<std::byte> data(n);no([&]{s.write(a,data);});}
 std::array<std::byte,1> v{std::byte{0x6a}};borrow.write(0,v);borrow.write(7,v);check(borrow.read(0,1)==std::vector<std::byte>(v.begin(),v.end()));check(borrow.read(7,1)==std::vector<std::byte>(v.begin(),v.end()));
 s.store<uint64_t>(p.guest,0x0102030405060708ull);auto observed=borrow.read(0,8);for(unsigned i=0;i<8;++i){if(observed[i]!=std::byte(i+1))throw std::runtime_error("endian oracle");++bytes;}
 auto read=region.acquire(m,old,Access::read);no([&]{read.write(0,v);});auto view=s.view(stale.address,8,Access::write);s.protect(stale,Permission::read);no([&]{borrow.write(0,v);});no([&]{s.write(view,std::vector<std::byte>(8));});check(read.read(0,1).size()==1);s.protect(stale,Permission::read_write);
 no([&]{borrow.registration(Access(2));});no([&]{region.acquire(m,old,Access(2));});check(!region.close());no([&]{region.acquire(m,old);});}
 check(s.live()==0&&s.leases()==0);{rw::Region fresh(s,m,p,b,rw::identity(p));no([&]{fresh.acquire(m,old);});no([&]{s.validate_owned_window(stale,stale.address,1,Access::write);});check(fresh.close());}
 // Mixed adjacent slices and uncommitted storage in one logical 16-KiB page.
 {std::array<native_module::Bytes,1> d{{{0x61000000,native_module::word(9)}}};native_module::Object ro(s,m,d);auto q=plan(m,b,0x61000004);rw::Region region(s,m,q,b,rw::identity(q));auto a=region.acquire(m,region.reference());auto uncommitted=s.reserve_slice(4,0x61000194);no([&]{s.read(uncommitted.address,1);});no([&]{s.store<uint32_t>(uncommitted.address,1);});s.release(uncommitted);
 no([&]{s.read(0x61000000,8);});no([&]{s.read(0x6100000a,4);});no([&]{s.store<uint32_t>(0x61000000,1);});no([&]{s.store<uint64_t>(0x61000000,1);});no([&]{s.store<uint32_t>(0x6100000a,1);});for(uint32_t offset:{12u,403u}){no([&]{s.read(0x61000000+offset,1);});no([&]{s.store<uint8_t>(0x61000000+offset,1);});}check(s.load<uint32_t>(0x61000000)==9);}
 // Deterministic reader/writer ownership through wrapper teardown.
 {auto r=std::make_unique<rw::Region>(s,m,p,b,rw::identity(p));auto ref=r->reference();std::latch ready(1),resume(1);std::exception_ptr error;std::thread t([&]{try{auto a=r->acquire(m,ref);ready.count_down();resume.wait();std::array<std::byte,1> x{std::byte{17}};a.write(0,x);if(a.read(0,1)[0]!=x[0])throw std::runtime_error("worker effect");}catch(...){error=std::current_exception();}});ready.wait();r.reset();resume.count_down();t.join();if(error)std::rethrow_exception(error);}
 check(s.live()==0&&s.leases()==0&&rw::Region::registrations()==0);check(m.close()&&foreign.close());std::cout<<"{\"semantic_positive\":"<<pos<<",\"refusals\":"<<neg<<",\"byte_comparisons\":"<<bytes<<",\"allocations\":0,\"leases\":0,\"registrations\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
