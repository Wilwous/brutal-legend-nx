// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/space.h"
#include "runtime/horizon/native.h"
#include "runtime/cooperative_stop.h"
#include <cstdio>
using namespace brutal::horizon;
int main(){
 unsigned passed=0,failed=0;auto check=[&](bool ok){ok?++passed:++failed;};
 auto refusal=[&](Fault fault,auto fn){try{fn();check(false);}catch(const Error&e){check(e.fault==fault);}};
 Space memory(64*1024);auto a=memory.allocate(4096),b=memory.allocate(4096);
 {Space::Lease pin(memory,b);std::array<Space::Handle,2> group{a,b};refusal(Fault::ownership,[&]{memory.release_all(group);});check(memory.live()==2);memory.store<uint32_t>(a.address,7);check(memory.load<uint32_t>(a.address)==7);}
 std::array<Space::Handle,2> group{a,b};memory.release_all(group);check(memory.live()==0);
 auto count=memory.allocate(4096);std::mutex mutex;NativeThread one,two;
 auto work=[&]{for(unsigned i=0;i<16;++i){std::lock_guard lock(mutex);memory.store<uint32_t>(count.address,memory.load<uint32_t>(count.address)+1);}};
 check(one.start(work));check(two.start(work));check(one.join());check(two.join());check(memory.load<uint32_t>(count.address)==32);
 refusal(Fault::argument,[&]{one.join();});memory.release(count);
 NativeThread injected;check(!injected.start([]{},64*1024,NativeThread::Injection::creation_failure));
 Event entered,exit;NativeThread active;check(active.start([&]{entered.signal();if(!exit.wait())throw Error(Fault::unsupported,"fixture wait");}));
 check(entered.wait());check(!active.join(0));exit.signal();check(active.join()&&!active.callback_error());
 brutal::synthetic::CooperativeStop stop;check(stop.request());check(!stop.request());check(stop.safe_boundary());stop.exited();stop.joined();stop.reclaimed();stop.unloaded();check(stop.state()==brutal::synthetic::CooperativeStop::State::unloaded);
 check(memory.live()==0);std::printf("{\"passed\":%u,\"failed\":%u}\n",passed,failed);return failed?1:0;
}
