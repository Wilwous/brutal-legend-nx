// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/space.h"
#include <iostream>
#include <thread>
#include <condition_variable>
using namespace brutal::horizon;
int main(){try{unsigned positive=0,refusals=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("transaction assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing transaction refusal");};
 Space s,foreign;auto a=s.allocate(16),b=s.reserve(16),alien=foreign.allocate(16);s.store<uint32_t>(a.address,0x11111111);std::array<Space::WordWrite,2> group{{{a,a.address,1},{b,b.address,2}}};
 no([&]{s.write_words(group);});yes(s.load<uint32_t>(a.address)==0x11111111&&!s.info(b).committed);s.commit(b);s.store<uint32_t>(b.address,0x22222222);s.protect(b,Permission::read);no([&]{s.write_words(group);});yes(s.load<uint32_t>(a.address)==0x11111111&&s.load<uint32_t>(b.address)==0x22222222);s.protect(b,Permission::read_write);
 group[1]={alien,alien.address,2};no([&]{s.write_words(group);});yes(s.load<uint32_t>(a.address)==0x11111111);group[1]={b,b.address,2};
 auto old=b;s.release(b);b=s.allocate(16,old.address);s.store<uint32_t>(b.address,0x33333333);no([&]{s.write_words(group);});yes(s.load<uint32_t>(b.address)==0x33333333);group[1]={b,b.address,2};s.write_words(group);yes(s.load<uint32_t>(a.address)==1&&s.load<uint32_t>(b.address)==2);
 auto stale=s.view(b.address,4,Access::read);s.decommit(b);s.commit(b);no([&]{s.read(stale);});s.store<uint32_t>(b.address,4);
 // Controlled overlap with real teardown, no timing-based race or broad sleep.
 std::mutex mutex;std::condition_variable changed;bool ready=false,proceed=false;std::exception_ptr failure;
 std::thread worker([&]{try{Space::Lease pa(s,a),pb(s,b);{std::unique_lock lock(mutex);ready=true;changed.notify_one();changed.wait(lock,[&]{return proceed;});}s.write_words(group);}catch(...){failure=std::current_exception();}});
 {std::unique_lock lock(mutex);changed.wait(lock,[&]{return ready;});}no([&]{s.release_all(std::array{a,b});});yes(s.load<uint32_t>(a.address)==1&&s.load<uint32_t>(b.address)==4);{std::lock_guard lock(mutex);proceed=true;}changed.notify_one();worker.join();if(failure)std::rethrow_exception(failure);yes(s.load<uint32_t>(b.address)==2);
 ModuleLife module(88);yes(module.close());no([&]{ModuleLife::Lease admission(module);s.write_words(group);});yes(s.load<uint32_t>(b.address)==2); // Module admission is caller's obligation, not guessed by Space.
 auto x=s.reserve_slice(4,0x70000000),y=s.reserve_slice(4,0x70000004);s.commit(x);s.commit(y,Permission::read);std::array<Space::WordWrite,2> adjacent{{{x,x.address,7},{y,y.address,9}}};no([&]{s.write_words(adjacent);});yes(s.load<uint32_t>(x.address)==0&&s.load<uint32_t>(y.address)==0);s.release(x);s.release(y);
 s.release_all(std::array{a,b});no([&]{s.release(a);});foreign.release(alien);yes(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
