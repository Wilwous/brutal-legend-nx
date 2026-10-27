// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Portable metadata contracts only; this does not call an SDK export.
#include "runtime/horizon/space.h"
#include <cstdio>
using namespace brutal::horizon;
int main(){unsigned passed=0,failed=0;auto check=[&](bool b){b?++passed:++failed;};auto refusal=[&](Fault f,auto fn){try{fn();check(false);}catch(const Error&e){check(e.fault==f);}};
 Space s(65536);auto h=s.reserve(8192,0x90000000);auto q=s.query(h.address);check(!q.committed&&q.current==Permission::none);check(q.base==h.address&&q.allocation_base==h.address&&q.allocation_size==8192&&q.region_size==8192);
 s.commit(h);q=s.query(h.address);check(q.committed&&q.current==Permission::read_write&&q.initial==Permission::read_write);s.protect(h,Permission::read);q=s.query(h.address+4096);check(q.base==h.address+4096&&q.region_size==4096&&q.initial==Permission::read_write&&q.current==Permission::read);
 refusal(Fault::unsupported,[&]{s.query(h.address+1);});refusal(Fault::bounds,[&]{s.query(0xdead0000);});
 {auto pin=s.pin(h.address+4092,8);check(s.leases()==1);refusal(Fault::ownership,[&]{s.release(h);});refusal(Fault::ownership,[&]{s.decommit(h);});}check(s.leases()==0);
 s.decommit(h);q=s.query(h.address);check(!q.committed&&q.current==Permission::read&&q.initial==Permission::read_write);s.commit(h);check(s.load<uint32_t>(h.address)==0);s.release(h);refusal(Fault::bounds,[&]{s.query(h.address);});
 auto small=s.allocate(728);refusal(Fault::unsupported,[&]{s.query(small.address);});s.release(small);check(s.live()==0&&Space::totals()==std::pair<size_t,size_t>{0,0});
 std::printf("{\"passed\":%u,\"failed\":%u}\n",passed,failed);return failed?1:0;}
