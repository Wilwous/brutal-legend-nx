// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "foundation-data.h"
#include <iostream>
#include <source_location>
using namespace brutal::horizon;
unsigned positive{},negative{},zero_entry_checks{},valid_entries{},joins{};
void check(bool b){if(!b)throw std::runtime_error("entry assertion");++positive;}
template<class F>void no_entry(F fn,std::source_location where=std::source_location::current()){
 auto before=Scope::guest_entries.load();try{fn();}catch(const Error&){if(Scope::guest_entries!=before)throw std::runtime_error("rejected entry executed");++negative;++zero_entry_checks;return;}throw std::runtime_error("missing refusal line "+std::to_string(where.line()));
}
int main(){try{
 Space s(256*1024);ModuleLife m(foundation_module_id);Slots slots;auto token=slots.allocate();GuestThread g(s,m,slots,token),other(s,m,slots,token);
 auto step=*std::find_if(std::begin(foundation_functions),std::end(foundation_functions),[](auto f){return f.address==foundation_step;});
 auto invoke=[&]{Scope scope(g,foundation_functions,m.identity);scope.invoke(foundation_step);};
 auto saved_sp=g.context.r1.u64;g.context.r3.u64=10;invoke();check(g.context.r3.u64==11);++valid_entries;
 {GuestThread::Execution guard(g);check(guard.required_window()==std::pair<uint32_t,uint32_t>{g.stack.address,g.stack.size});}
 auto narrow=g.relative_stack_contract(-32,32);{GuestThread::Execution guard(g,&narrow);check(guard.required_window()==std::pair<uint32_t,uint32_t>{uint32_t(saved_sp-32),64});}
 {Scope scope(g,foundation_functions,m.identity,nullptr,{},nullptr,&narrow);scope.invoke(foundation_step);++valid_entries;}
 s.protect(g.stack,Permission::read);no_entry(invoke);check(s.leases()==0);s.protect(g.stack,Permission::none);no_entry(invoke);s.protect(g.stack,Permission::write);no_entry(invoke);s.protect(g.stack,Permission::read_write);
 s.decommit(g.stack);no_entry(invoke);check(s.leases()==0);s.commit(g.stack);
 auto stack=g.stack;g.stack.owner=0;no_entry(invoke);g.stack=other.stack;no_entry(invoke);g.stack=stack;
 auto contract=g.whole_stack_contract();contract.thread=other.identity;no_entry([&]{Scope scope(g,foundation_functions,m.identity,nullptr,{},nullptr,&contract);scope.invoke(foundation_step);});
 contract=g.whole_stack_contract();++contract.module_instance;no_entry([&]{GuestThread::Execution guard(g,&contract);});
 g.context.r1.u64|=1;no_entry(invoke);g.context.r1.u64=UINT64_MAX;no_entry(invoke);g.context.r1.u64=saved_sp;
 for(auto bounds:{std::pair<int64_t,int64_t>{INT64_MIN,16},{-16,INT64_MAX},{32,32}}){auto c=g.relative_stack_contract(bounds.first,bounds.second);no_entry([&]{Scope scope(g,foundation_functions,m.identity,nullptr,{},nullptr,&c);scope.invoke(foundation_step);});}
 s.protect(g.pcr,Permission::read);auto crossing=g.relative_stack_contract(-32,int64_t(uint64_t(stack.address)+stack.size-saved_sp+1));no_entry([&]{Scope scope(g,foundation_functions,m.identity,nullptr,{},nullptr,&crossing);scope.invoke(foundation_step);});s.protect(g.pcr,Permission::read_write);
 // All production-generated entry forms, after a previously valid Scope.
 for(unsigned path=0;path<4;++path){Scope scope(g,foundation_functions,m.identity);
  if(path==0)scope.invoke(foundation_step);else if(path==1)Scope::direct(step.implementation,g.context,nullptr);else if(path==2)Scope::indirect(g.context,nullptr,foundation_step);else step.implementation(g.context,nullptr);++valid_entries;
  s.protect(stack,Permission::read);auto old=g.context.r3.u64;
  no_entry([&]{if(path==0)scope.invoke(foundation_step);else if(path==1)Scope::direct(step.implementation,g.context,nullptr);else if(path==2)Scope::indirect(g.context,nullptr,foundation_step);else step.implementation(g.context,nullptr);});
  check(g.context.r3.u64==old);s.protect(stack,Permission::read_write);
 }
 // Revocation in an observer before the generated first instruction.
 {Scope scope(g,foundation_functions,m.identity,nullptr,[&](uint32_t){s.protect(stack,Permission::read);});no_entry([&]{scope.invoke(foundation_step);});s.protect(stack,Permission::read_write);}
 no_entry([&]{step.implementation(g.context,nullptr);});
 {Scope scope(g,foundation_functions,m.identity);no_entry([&]{step.implementation(other.context,nullptr);});}
 {Scope scope(g,foundation_functions,m.identity);scope.invoke(foundation_step);++valid_entries;s.protect(stack,Permission::read);no_entry([&]{Scope::store<uint32_t>(g.context,uint32_t(saved_sp),1);});s.protect(stack,Permission::read_write);no_entry([&]{s.decommit(stack);});no_entry([&]{s.release(stack);});}
 check(s.leases()==0);g.cleanup();other.cleanup();slots.release(token);check(m.close());check(s.live()==0);
 // A retired original stack is never admitted; remaining PCR/TLS are reclaimable.
 {Space stale(65536);ModuleLife mod(91);Slots ts;auto t=ts.allocate();GuestThread retired(stale,mod,ts,t);stale.release(retired.stack);no_entry([&]{Scope scope(retired,foundation_functions,mod.identity);scope.invoke(foundation_step);});retired.cleanup();check(stale.live()==0&&stale.leases()==0);ts.release(t);check(mod.close());}
 for(uint32_t cap:{4096u,16384u,20480u}){Space failed(cap);ModuleLife mod(92);Slots ts;auto t=ts.allocate();no_entry([&]{GuestThread partial(failed,mod,ts,t);});check(failed.live()==0&&failed.leases()==0);ts.release(t);check(mod.close());}
 // Repeated deterministic worker start/stop/join with no generated entry.
 for(unsigned i=0;i<3;++i){Space mem(65536);ModuleLife mod(foundation_module_id);Slots ts;auto t=ts.allocate();GuestThread worker(mem,mod,ts,t);GuestTask task(worker);brutal::synthetic::CooperativeStop stop;check(stop.request());std::atomic<bool>stopped{};auto before=Scope::guest_entries.load();
  check(task.start([&]{try{Scope scope(worker,foundation_functions,mod.identity,&stop);scope.invoke(foundation_step);}catch(const Scope::Stopped&){stopped=true;stop.exited();}}));check(task.join()&&!task.callback_error());++joins;check(stopped&&Scope::guest_entries==before);stop.joined();worker.cleanup();stop.reclaimed();stop.unloaded();ts.release(t);check(mod.close());check(mem.live()==0&&mem.leases()==0);
 }
 check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"zero_entry_checks\":"<<zero_entry_checks<<",\"valid_leaf_entries\":"<<valid_entries<<",\"stop_join_cycles\":"<<joins<<",\"byte_comparisons\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
