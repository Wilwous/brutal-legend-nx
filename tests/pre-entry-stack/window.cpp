// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/space.h"
#include <iostream>
using namespace brutal::horizon;
unsigned positive{},negative{};
void check(bool v){if(!v)throw std::runtime_error("window assertion");++positive;}
template<class F>void no(F fn){try{fn();}catch(const Error&){++negative;return;}throw std::runtime_error("missing window refusal");}
int main(){try{
 Space s(65536),foreign(4096);auto a=s.allocate(8192,0x10000),b=s.allocate(4096,0x12000);auto alien=foreign.allocate(4096);
 s.validate_owned_window(a,a.address+4090,32,Access::write);check(true);
 s.protect(b,Permission::read);s.validate_owned_window(a,a.address+8192-32,32,Access::write);check(true);
 no([&]{s.validate_owned_window(a,a.address+8192-32,64,Access::write);});
 no([&]{s.validate_owned_window(b,b.address,4096,Access::write);});
 s.protect(b,Permission::none);no([&]{s.validate_owned_window(b,b.address,1,Access::write);});
 s.protect(a,Permission::read);no([&]{s.validate_owned_window(a,a.address,8192,Access::write);});
 s.protect(a,Permission::read_write);s.decommit(a);no([&]{s.validate_owned_window(a,a.address,8192,Access::write);});s.commit(a);
 no([&]{s.validate_owned_window(alien,a.address,1,Access::write);});
 auto forged=a;forged.owner=0;no([&]{s.validate_owned_window(forged,a.address,1,Access::write);});
 no([&]{s.validate_owned_window(a,UINT32_MAX-1,16,Access::write);});no([&]{s.validate_owned_window(a,a.address,0,Access::write);});
 no([&]{s.validate_owned_window(a,a.address,1,Access(99));});
 {auto lease=s.pin(a.address,8192);no([&]{s.release(a);});no([&]{s.decommit(a);});s.protect(a,Permission::read);no([&]{s.validate_owned_window(a,a.address,8192,Access::write);});}
 s.release(a);no([&]{s.validate_owned_window(a,a.address,8192,Access::write);});
 auto unissued=a;unissued.id=UINT64_MAX;no([&]{s.release_survivors(std::span(&unissued,1));});
 std::array<Space::Handle,2>registered{a,b};{auto lease=s.pin(b.address,1);no([&]{s.release_survivors(registered);});check(s.live()==1);}
 s.release_survivors(registered);check(s.live()==0);foreign.release(alien);check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"byte_comparisons\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
