// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/generated.h"
#include "foundation-data.h"
#include <atomic>
using namespace brutal::horizon;
using Stop=brutal::synthetic::CooperativeStop;
namespace {
class Checks {
 FoundationResult& r_;unsigned group_{};
 public:
 explicit Checks(FoundationResult& r):r_(r){}
 void group(unsigned n){group_=n;}
 void operator()(bool ok){if(ok){++r_.passed;++r_.stage_passed[group_];}else{++r_.failed;++r_.stage_failed[group_];}}
 template<class F>void refusal(Fault fault,F action){try{action();(*this)(false);}catch(const Error&e){(*this)(e.fault==fault);}catch(...){(*this)(false);}}
};
void run_generated(GuestThread& guest,FoundationResult& result,uint32_t count=4){
 guest.context.r3.u64=count;guest.context.r14.u64=0x1122334455667788;guest.context.f14.u64=0x12345678;
 guest.context.v14.u64[0]=0x87654321;guest.context.lr=0x8201dead;
 Scope scope(guest,foundation_functions,foundation_module_id);scope.invoke(foundation_entry);
 result.generated_calls+=scope.calls;result.checked_reads+=scope.reads;result.checked_writes+=scope.writes;result.safe_points+=scope.polls;
}
}
FoundationResult foundation_probe(){
 FoundationResult result;Checks check(result);
 try{
  check.group(0);
  Space space;auto a=space.reserve(8192);
  check.refusal(Fault::uncommitted,[&]{space.load<uint32_t>(a.address);});space.commit(a);
  space.store<uint32_t>(a.address+4094,0x12345678);check(space.load<uint32_t>(a.address+4094)==0x12345678);
  check(space.read(a.address+4094,4)==std::vector<std::byte>{std::byte{0x12},std::byte{0x34},std::byte{0x56},std::byte{0x78}});
  check.refusal(Fault::bounds,[&]{space.load<uint32_t>(a.address+a.size-2);});
  check.refusal(Fault::bounds,[&]{space.load<uint64_t>(UINT32_MAX-2);});
  check.refusal(Fault::bounds,[&]{space.load<uint32_t>(0x7fc80000);});
  space.protect(a,brutal::horizon::Permission::read);check.refusal(Fault::protection,[&]{space.store<uint32_t>(a.address,99);});
  check(space.load<uint32_t>(a.address+4094)==0x12345678);space.protect(a,brutal::horizon::Permission::read_write);
  {Space::Lease pin(space,a);check.refusal(Fault::ownership,[&]{space.release(a);});check.refusal(Fault::ownership,[&]{space.decommit(a);});}
  space.decommit(a);check.refusal(Fault::uncommitted,[&]{space.load<uint32_t>(a.address);});space.commit(a);check(space.load<uint32_t>(a.address)==0);
  auto old=a;space.release(a);check.refusal(Fault::bounds,[&]{space.load<uint32_t>(old.address);});check.refusal(Fault::stale,[&]{space.release(old);});
  auto mapped=space.allocate(4096,0x82020000);check.refusal(Fault::ownership,[&]{space.allocate(4096,0x82020000);});space.release(mapped);
  check.refusal(Fault::argument,[&]{space.allocate(16,0x82020001);});check.refusal(Fault::bounds,[&]{space.allocate(8192,0xfffff000);});
  Space foreign;auto alien=foreign.allocate(4);check.refusal(Fault::ownership,[&]{space.release(alien);});foreign.release(alien);
  check(space.live()==0&&foreign.live()==0);
  check.group(1);
  NativeThread injected;check(!injected.start([]{},64*1024,NativeThread::Injection::creation_failure));
  check.refusal(Fault::argument,[&]{injected.start([]{},0);});
  std::atomic<unsigned> done{};NativeThread normal;check(normal.start([&]{done.fetch_add(1);}));check(normal.join());check(done==1&&!normal.callback_error());
  check.refusal(Fault::argument,[&]{normal.join();});check.refusal(Fault::argument,[&]{normal.start([]{});});
  brutal::horizon::Event blocked,open;NativeThread recoverable;check(recoverable.start([&]{blocked.signal();if(!open.wait())throw Error(Fault::unsupported,"controlled wait failed");}));
  check(blocked.wait());check(!recoverable.join(0));open.signal();check(recoverable.join()&&!recoverable.callback_error());
  auto shared=space.allocate(4096);std::mutex increments;
  NativeThread writer_a,writer_b;auto write=[&]{for(unsigned i=0;i<32;++i){std::lock_guard lock(increments);space.store<uint32_t>(shared.address,space.load<uint32_t>(shared.address)+1);}};
  check(writer_a.start(write));check(writer_b.start(write));check(writer_a.join());check(writer_b.join());check(space.load<uint32_t>(shared.address)==64);
  std::atomic<unsigned> reads{};NativeThread reader_a,reader_b;auto read=[&]{for(unsigned i=0;i<32;++i)if(space.load<uint32_t>(shared.address)==64)++reads;};
  check(reader_a.start(read));check(reader_b.start(read));check(reader_a.join());check(reader_b.join());check(reads==64);
  space.release(shared);
  std::atomic<unsigned> balanced{};NativeThread alloc_a,alloc_b;auto allocate=[&]{for(unsigned i=0;i<16;++i){auto h=space.allocate(16);space.store<uint32_t>(h.address,i);space.release(h);++balanced;}};
  check(alloc_a.start(allocate));check(alloc_b.start(allocate));check(alloc_a.join());check(alloc_b.join());check(balanced==32&&space.live()==0);
  check.group(2);
  Slots slots;auto slot=slots.allocate();ModuleLife module(foundation_module_id);
  GuestThread guest(space,module,slots,slot,7),other(space,module,slots,slot,41);
  check(guest.pcr.address!=other.pcr.address&&guest.stack.address!=other.stack.address&&guest.tls.address!=other.tls.address);
  check(guest.context.r13.u32==guest.pcr.address&&space.load<uint32_t>(guest.pcr.address)==guest.tls.address);
  check(space.load<uint64_t>(guest.pcr.address+0x30)==guest.pcr.address);
  check((guest.context.r1.u32&15)==0&&guest.context.r1.u32>guest.stack.address&&guest.context.r1.u32<guest.stack.address+guest.stack.size);
  check(guest.get_tls()==7&&other.get_tls()==41);guest.set_tls(9);check(guest.get_tls()==9&&other.get_tls()==41);guest.set_tls(7);
  check.refusal(Fault::ownership,[&]{slots.release(slot);});
  Slots foreign_slots;auto foreign_slot=foreign_slots.allocate();check.refusal(Fault::slot,[&]{slots.validate(foreign_slot);});foreign_slots.release(foreign_slot);
  PPCContext foreign_context{};check.refusal(Fault::no_thread,[&]{Scope::load<uint32_t>(foreign_context,guest.pcr.address);});
  {Scope scope(guest,foundation_functions,foundation_module_id);check.refusal(Fault::no_thread,[&]{Scope::load<uint32_t>(other.context,guest.pcr.address);});
   check.refusal(Fault::unsupported,[&]{scope.invoke(0xdeadbeec);});check.refusal(Fault::ownership,[&]{Scope::load<uint32_t>(guest.context,other.pcr.address);});
   check.refusal(Fault::ownership,[&]{guest.cleanup();});check.refusal(Fault::argument,[&]{Scope nested(guest,foundation_functions,foundation_module_id);});}
  check.group(3);
  const auto sp=guest.context.r1.u64;
  run_generated(guest,result);result.observed_ppc_result=guest.context.r3.u32;check(guest.context.r3.u64==12&&guest.get_tls()==8);
  check(guest.context.r1.u64==sp&&guest.context.r2.u64==0x82020000&&guest.context.r13.u64==guest.pcr.address&&guest.context.lr==0x8201dead);
  check(guest.context.r14.u64==0x1122334455667788&&guest.context.f14.u64==0x12345678&&guest.context.v14.u64[0]==0x87654321);
  check(result.checked_reads==5&&result.checked_writes==5&&result.safe_points==6);
  guest.set_tls(7);run_generated(guest,result,0);check(guest.context.r3.u64==8);
  guest.set_tls(UINT32_MAX);run_generated(guest,result,0);check(guest.context.r3.u64==UINT64_MAX);guest.set_tls(7);
  guest.context.r13.u64=other.pcr.address;check.refusal(Fault::argument,[&]{Scope scope(guest,foundation_functions,foundation_module_id);});guest.context.r13.u64=guest.pcr.address;
  guest.context.r1.u64=guest.stack.address;check.refusal(Fault::argument,[&]{Scope scope(guest,foundation_functions,foundation_module_id);});guest.context.r1.u64=sp;
  space.protect(guest.stack,brutal::horizon::Permission::read);check.refusal(Fault::protection,[&]{run_generated(guest,result);});space.protect(guest.stack,brutal::horizon::Permission::read_write);
  guest.context.r1.u64=sp;guest.set_tls(7);
  FoundationResult left,right;GuestTask task_a(guest),task_b(other);check(task_a.start([&]{run_generated(guest,left,3);}));check(task_b.start([&]{run_generated(other,right,5);}));
  check(task_a.join());check(task_b.join());check(!task_a.callback_error()&&!task_b.callback_error());check(guest.context.r3.u64==11&&other.context.r3.u64==47);
  check(guest.get_tls()==8&&other.get_tls()==42);check(left.checked_reads==5&&right.checked_reads==5);
  other.cleanup();other.cleanup();guest.cleanup();check(space.live()==0);
  check.group(4);
  ModuleLife stop_module(foundation_module_id);GuestThread stopping(space,stop_module,slots,slot);
  Stop stop;brutal::horizon::Event reached,continue_;unsigned loop_visits=0;std::atomic<bool> stopped{};FoundationResult stopping_counts;
  GuestTask task(stopping);check(task.start([&]{
   Scope scope(stopping,foundation_functions,foundation_module_id,&stop,[&](uint32_t address){if(address==foundation_step&&++loop_visits==2){reached.signal();if(!continue_.wait())throw Error(Fault::unsupported,"stop rendezvous timeout");}});
   try{scope.invoke(foundation_forever);}catch(const Scope::Stopped& e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}
   stopping_counts.safe_points=scope.polls;
  }));
  check(reached.wait());check.refusal(Fault::ownership,[&]{stopping.cleanup();});
  check(!task.join(0));check(stop.request());check(!stop.request());check(!stop_module.close());check.refusal(Fault::stale,[&]{GuestThread denied(space,stop_module,slots,slot);});continue_.signal();check(task.join());
  check(stopped&&!task.callback_error());check(stop.state()==Stop::State::exited);stop.joined();stopping.cleanup();stop.reclaimed();check(stop_module.close());stop.unloaded();
  check(stop.state()==Stop::State::unloaded&&space.live()==0&&stopping_counts.safe_points==3);
  check.group(5);
  Slots temp;auto first=temp.allocate();temp.release(first);check.refusal(Fault::slot,[&]{temp.validate(first);});auto second=temp.allocate();check(second.index==first.index&&second.generation!=first.generation);temp.release(second);
  {ModuleLife failed_module(foundation_module_id);GuestThread failed(space,failed_module,slots,slot);GuestTask failed_task(failed);check(!failed_task.start([]{},NativeThread::Injection::creation_failure));failed.cleanup();check(space.live()==0);check(failed_module.close());}
  {ModuleLife cleanup_module(foundation_module_id);GuestThread cleanup_guest(space,cleanup_module,slots,slot);
   {Space::Lease pinned(space,cleanup_guest.pcr);check.refusal(Fault::ownership,[&]{cleanup_guest.cleanup();});check(space.live()==3);check(space.load<uint32_t>(cleanup_guest.stack.address)==0);}
   cleanup_guest.cleanup();check(space.live()==0);check(cleanup_module.close());}
  ModuleLife small_module(foundation_module_id);Space small(16*1024);check.refusal(Fault::exhausted,[&]{GuestThread failed(small,small_module,slots,slot);});check(small.live()==0);check(small_module.close());
  check(module.close());check.refusal(Fault::stale,[&]{GuestThread denied(space,module,slots,slot);});
  slots.release(slot);check.refusal(Fault::slot,[&]{slots.validate(slot);});check(space.live()==0);result.remaining_allocations=uint32_t(space.live());
 }catch(...){check(false);}
 return result;
}
