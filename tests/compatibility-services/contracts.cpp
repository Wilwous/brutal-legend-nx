// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original host-only witnesses for G8 proof obligations. No generated/game code.
#include "runtime/execution_boundary.h"
#include "runtime/horizon/space.h"
#include "runtime/horizon/native.h"
#include <cstdio>
#include <memory>
using namespace brutal::foundation;
using brutal::horizon::Space;
using brutal::horizon::ModuleLife;
using brutal::horizon::NativeThread;
namespace d=brutal::diagnostic;
struct Marker {unsigned& destroyed;~Marker(){++destroyed;}};
int main(){
 unsigned passed=0,failed=0;auto check=[&](bool ok){ok?++passed:++failed;};
 auto refuse=[&](Fault fault,auto fn){try{fn();check(false);}catch(const Error&e){check(e.fault==fault);}};
 auto trace=std::make_unique<d::Trace>();d::Binding binding(trace.get(),71,9);
 DiagnosticContext ctx;ctx.pc=0x90001000;ctx.lr=0x90002000;ctx.ctr=0x90003000;
 for(unsigned i=0;i<32;++i)ctx.gpr[i]=0x1000+i;
 Space space(128*1024);auto code=space.allocate(4096,0x90000000),data=space.allocate(8192,0x90004000);
 ModuleLife module(71);const auto totals=Space::totals();unsigned destroyed=0;bool continued=false,close_refused=false;
 space.store<uint32_t>(data.address,0x12345678);space.protect(data,Permission::read);
 const auto outcome=execute(ctx,[&]{
  Marker outer{destroyed};ModuleLife::Lease active(module);Space::Lease code_pin(space,code),data_pin(space,data);
  d::PhaseBinding phase(5);d::AllocationBinding allocation(data.owner,data.id);
  ctx.pc=0x90001008;ctx.gpr[3]=0x55;d::site(ctx.pc);
  close_refused=!module.close();
  // Two real RAII frames unwind on a checked-memory refusal; no signal is raised.
  [&]{Marker inner{destroyed};space.store<uint32_t>(data.address,0xdeadbeef);continued=true;}();
 });
 check(!outcome.ok&&outcome.fault==Fault::protection);check(!continued&&destroyed==2&&close_refused);
 check(outcome.snapshot.pc==0x90001008&&outcome.snapshot.gpr[3]==0x55);
 check(outcome.snapshot.lr==0x90002000&&outcome.snapshot.ctr==0x90003000);
 check(outcome.snapshot.gpr[1]==0x1001&&outcome.snapshot.gpr[2]==0x1002&&outcome.snapshot.gpr[13]==0x100d);
 check(space.leases()==0&&Space::totals()==totals);check(space.load<uint32_t>(data.address)==0x12345678);
 auto events=trace->snapshot();check(!events.empty());auto event=events.back();
 check(event.kind==d::Kind::refusal&&event.detail==uint32_t(Fault::protection));
 check(event.pc==ctx.pc&&event.module==71&&event.thread==9&&event.phase==5);
 check(event.allocation_owner==data.owner&&event.allocation_id==data.id);
 check(d::context.phase==0&&d::context.allocation_owner==0&&d::context.allocation_id==0);
 refuse(Fault::stale,[&]{ModuleLife::Lease denied(module);});check(module.close());
 // Outcome owns numeric diagnostic values even after the caller context changes.
 ctx.pc=0;ctx.gpr.fill(0);check(outcome.snapshot.pc==0x90001008&&outcome.snapshot.gpr[3]==0x55);
 auto input=space.view(data.address,4,Access::read);space.protect(data,Permission::read_write);
 auto output=space.view(data.address,4,Access::write);
 auto failure=execute(ctx,[&]{space.update(input,output,[](auto scratch){scratch[0]=std::byte{0xff};throw Error(Fault::unsupported,"original callback refusal");});});
 check(!failure.ok&&failure.fault==Fault::unsupported);check(space.load<uint32_t>(data.address)==0x12345678);
 space.decommit(data);refuse(Fault::stale,[&]{space.read(input);});
 refuse(Fault::uncommitted,[&]{space.load<uint32_t>(data.address);});space.commit(data);
 check(space.load<uint32_t>(data.address)==0);refuse(Fault::stale,[&]{space.read(input);});
 auto bad=execute(ctx,[]{unsupported_exception();});check(!bad.ok&&bad.fault==Fault::unsupported);
 auto foreign=space.allocate(4096);auto forged=data;forged.owner=foreign.owner+1;
 refuse(Fault::ownership,[&]{space.release(forged);});check(space.live()==3);
 // Native callback failure is observable after join, not a successful guest return.
 NativeThread worker;unsigned worker_destroyed=0;check(worker.start([&]{d::Binding b(trace.get(),71,10);d::site(0x9000100c);Marker m{worker_destroyed};throw Error(Fault::unsupported,"original worker refusal");}));
 check(worker.join()&&worker.callback_error()&&worker_destroyed==1);
 check(d::context.thread==9);check(trace->snapshot().back().thread==10);
 std::array handles{code,data,foreign};space.release_all(handles);
 check(space.live()==0&&space.leases()==0&&Space::totals()==std::pair<size_t,size_t>{0,0});
 check(trace->dropped()==0&&!trace->broken());
 std::printf("{\"passed\":%u,\"failed\":%u,\"guest_continuations\":0,\"host_unwind_frames\":%u}\n",passed,failed,destroyed);
 return failed?1:0;
}
