// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/checked_tls.h"
#include "runtime/execution_boundary.h"
#include <array>
#include <barrier>
#include <functional>
#include <iostream>
#include <random>
#include <string>
using namespace brutal::foundation;
namespace {
void require(bool condition){if(!condition)throw std::runtime_error("assertion failed");}
template<class F> void rejects(Fault expected,F&& f){
  try{f();}catch(const Error& e){require(e.fault==expected);return;}
  throw std::runtime_error("expected an explicit failure");
}
void run_memory(){
  Memory m(16*4096);auto a=m.reserve(17,8192);require(a.guest%8192==0);
  rejects(Fault::uncommitted,[&]{m.translate(a.guest,1,Access::read);});
  rejects(Fault::uncommitted,[&]{m.protect(a,Permission::read);});
  rejects(Fault::argument,[&]{m.commit(a,static_cast<Permission>(99));});
  m.commit(a);m.store32(a.guest,0x01234567);require(m.load32(a.guest)==0x01234567);
  auto bytes=m.read(m.translate(a.guest,4,Access::read));
  require(bytes==std::vector<std::byte>({std::byte{1},std::byte{0x23},std::byte{0x45},std::byte{0x67}}));
  m.store32(a.guest+1,0x89abcdef);require(m.load32(a.guest+1)==0x89abcdef);
  rejects(Fault::bounds,[&]{m.translate(a.guest+16,2,Access::read);});
  rejects(Fault::bounds,[&]{m.translate(a.guest,UINT32_MAX,Access::read);});
  rejects(Fault::bounds,[&]{m.translate(UINT32_MAX,4,Access::read);});
  rejects(Fault::argument,[&]{m.translate(0,4,Access::read);});
  rejects(Fault::argument,[&]{m.translate(a.guest,0,Access::read);});
  auto w=m.translate(a.guest,4,Access::write);
  rejects(Fault::protection,[&]{m.read(w);});
  std::array<std::byte,3> short_data{};
  rejects(Fault::bounds,[&]{m.write(w,short_data);});
  rejects(Fault::argument,[&]{m.protect(a,static_cast<Permission>(99));});
  m.protect(a,Permission::read);
  std::array<std::byte,4> data{};
  rejects(Fault::protection,[&]{m.write(w,data);});
  require(m.load32(a.guest)!=0);m.protect(a,Permission::none);
  rejects(Fault::protection,[&]{m.load32(a.guest);});
  m.protect(a,Permission::read_write);auto old=m.translate(a.guest,4,Access::read);
  m.decommit(a);rejects(Fault::stale,[&]{m.read(old);});
  m.commit(a);require(m.load32(a.guest)==0);
  rejects(Fault::stale,[&]{m.read(old);});
  auto forged=m.translate(a.guest,4,Access::read);forged.length=UINT32_MAX;
  rejects(Fault::bounds,[&]{m.read(forged);});
  Memory other;rejects(Fault::ownership,[&]{other.release(a);});
  auto forged_a=a;++forged_a.guest;
  rejects(Fault::ownership,[&]{m.release(forged_a);});
  auto view=m.translate(a.guest,4,Access::read);m.release(a);
  rejects(Fault::stale,[&]{m.read(view);});
  rejects(Fault::stale,[&]{m.release(a);});
  rejects(Fault::bounds,[&]{m.translate(a.guest,4,Access::read);});
  require(m.live_allocations()==0);
}
void run_allocation_failures(){
  Memory m(4096);rejects(Fault::argument,[&]{m.allocate(0);});
  rejects(Fault::argument,[&]{m.allocate(1,6144);});
  rejects(Fault::argument,[&]{m.allocate(1,1);});
  rejects(Fault::exhausted,[&]{m.allocate(UINT32_MAX);});
  auto a=m.allocate(1);rejects(Fault::exhausted,[&]{m.allocate(1);});
  m.release(a);rejects(Fault::exhausted,[&]{m.allocate(1);});
  // Free releases ownership, but this bounded arena quarantines addresses.
  require(m.live_allocations()==0);
  Memory aligned(1024*1024);auto b=aligned.allocate(8,65536);require(b.guest%65536==0);
  rejects(Fault::argument,[]{Memory invalid(0);});
}
void run_random_memory(){
  Memory m(8*4096);auto a=m.allocate(256);std::array<std::byte,256> model{};std::mt19937 rng(0x42524c);
  for(unsigned n=0;n<4000;++n){
    uint32_t offset=rng()%260,size=1+rng()%20;
    if(offset>=256 || size>256-offset){rejects(Fault::bounds,[&]{m.translate(a.guest+offset,size,Access::write);});continue;}
    std::vector<std::byte> data(size);for(auto& b:data)b=std::byte(rng()&255);
    m.write(m.translate(a.guest+offset,size,Access::write),data);
    std::copy(data.begin(),data.end(),model.begin()+offset);
    require(m.read(m.translate(a.guest,256,Access::read))==std::vector<std::byte>(model.begin(),model.end()));
  }
  m.release(a);
}
void run_tls_slots(){
  Memory m;const std::byte initial[]={std::byte{0x11},std::byte{0x22},std::byte{0x33},std::byte{0x44}};
  Tls tls(m,64,28,initial);
  rejects(Fault::no_thread,[&]{tls.allocate_slot();});
  rejects(Fault::no_thread,[&]{tls.get(0);});
  {Tls::Scope scope(tls);
    require(tls.dynamic_address()==tls.static_address()+28);
    require(m.load32(tls.static_address())==0x11223344);
    require(m.load32(tls.static_address()+4)==0);
    for(uint32_t i=0;i<64;++i){require(tls.allocate_slot()==i);require(tls.get(i)==0);tls.set(i,i+17);}
    require(tls.get(63)==80);require(m.load32(tls.dynamic_address()+63*4)==80);
    rejects(Fault::bounds,[&]{m.load32(tls.dynamic_address()+64*4);});
    rejects(Fault::exhausted,[&]{tls.allocate_slot();});
    rejects(Fault::slot,[&]{tls.get(64);});
    rejects(Fault::slot,[&]{tls.set(0x40000000,1);});
    rejects(Fault::slot,[&]{tls.free_slot(UINT32_MAX);});
    tls.free_slot(3);rejects(Fault::slot,[&]{tls.get(3);});
    rejects(Fault::slot,[&]{tls.free_slot(3);});
    require(tls.allocate_slot()==3);require(tls.get(3)==0);
    rejects(Fault::argument,[&]{tls.attach();});
  }
  require(m.live_allocations()==0);
  rejects(Fault::argument,[&]{Tls invalid(m,257,28,initial);});
  rejects(Fault::argument,[&]{Tls invalid(m,64,1,initial);});
  Memory small(4096);auto owned=small.allocate(1);Tls no_room(small,64,28,initial);
  rejects(Fault::exhausted,[&]{no_room.attach();});small.release(owned);
}
void run_tls_threads(){
  Memory m;Tls tls(m,2,4,{});std::barrier phases(3);std::array<std::exception_ptr,2> errors{};
  {
    Tls::Scope main(tls);auto slot=tls.allocate_slot();tls.set(slot,111);
    std::array<std::thread,2> workers;
    for(unsigned i=0;i<2;++i)workers[i]=std::thread([&,i]{
      try{Tls::Scope scope(tls);require(tls.get(slot)==0);tls.set(slot,222+i);
        phases.arrive_and_wait();phases.arrive_and_wait();
        require(tls.get(slot)==0);tls.set(slot,333+i);phases.arrive_and_wait();require(tls.get(slot)==333+i);
      }catch(...){errors[i]=std::current_exception();phases.arrive_and_drop();}
    });
    phases.arrive_and_wait();require(tls.get(slot)==111);
    tls.free_slot(slot);require(tls.allocate_slot()==slot);tls.set(slot,444);
    phases.arrive_and_wait();phases.arrive_and_wait();
    for(auto& t:workers)t.join();for(auto& e:errors)if(e)std::rethrow_exception(e);
    require(tls.get(slot)==444);require(m.live_allocations()==1);
    std::thread later([&]{try{Tls::Scope scope(tls);require(tls.get(slot)==0);}catch(...){errors[0]=std::current_exception();}});
    later.join();if(errors[0])std::rethrow_exception(errors[0]);
  }
  require(m.live_allocations()==0);
}
void run_boundary(){
  Memory m;Tls tls(m,2,4,{});DiagnosticContext ctx;ctx.pc=0x12345678;ctx.gpr[13]=0x10001000;
  auto outcome=execute(ctx,[&]{Tls::Scope scope(tls);unsupported_exception();});
  require(!outcome.ok && outcome.fault==Fault::unsupported);
  std::cout<<"{\"event\":\"unsupported_guest_exception\",\"expected_failure\":true,\"pc\":305419896,\"r13\":268439552}\n";
  require(outcome.snapshot.pc==ctx.pc && outcome.snapshot.gpr[13]==ctx.gpr[13]);
  require(m.live_allocations()==0);
  require(execute(ctx,[]{}).ok);
  require(!execute(ctx,[]{throw std::runtime_error("host failure");}).ok);
}
}
int main(){
  const std::pair<const char*,std::function<void()>> cases[]={
    {"memory_bounds_permissions_endian_lifetime",run_memory},{"allocation_alignment_failure",run_allocation_failures},
    {"memory_random_model_4000",run_random_memory},{"tls_layout_slots_cleanup",run_tls_slots},
    {"tls_host_thread_isolation_reuse",run_tls_threads},{"fail_closed_execution_boundary",run_boundary}};
  unsigned failed=0;
  for(auto& [name,test]:cases){try{test();std::cout<<"{\"test\":\""<<name<<"\",\"result\":\"PASS\"}\n";}
    catch(const std::exception& e){++failed;std::cerr<<"failure in "<<name<<": "<<e.what()<<'\n';
      std::cout<<"{\"test\":\""<<name<<"\",\"result\":\"FAIL\"}\n";}}
  std::cout<<"{\"suite\":\"runtime_foundation\",\"cases\":6,\"failed\":"<<failed<<"}\n";
  return failed?1:0;
}
