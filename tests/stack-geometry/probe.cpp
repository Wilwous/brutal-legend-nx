// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "admission.h"
#include <iostream>
#include <source_location>
using namespace geometry_test;
unsigned positive{},negative{},zero_entries{},valid_calls{},cycles{};uint64_t byte_checks{};
void check(bool v){if(!v)throw std::runtime_error("geometry assertion");++positive;}
template<class F>void refused(F f,std::source_location loc=std::source_location::current()){
 auto before=Scope::guest_entries.load();try{f();}catch(const Error&){if(Scope::guest_entries!=before)throw std::runtime_error("invalid entry executed");++negative;++zero_entries;return;}
 throw std::runtime_error("missing geometry refusal line "+std::to_string(loc.line()));
}
struct Rig {
 Space space{128*1024};ModuleLife module{geometry_module_id};Slots slots;Slots::Token token{slots.allocate()};GuestThread guest{space,module,slots,token};
 ~Rig(){guest.cleanup();slots.release(token);if(!module.close())std::terminate();}
};
constexpr uint64_t payloads[]{0x1021324354657687,0x89abcdef76543210,0xfedcba9876543210,0x01030507090b0d0f};
constexpr uint64_t root_lr=0x123456789abcdef0;
void seed(Rig& r){auto&c=r.guest.context;c.r20.u64=payloads[0];c.r21.u64=payloads[1];c.r22.u64=payloads[2];c.r23.u64=payloads[3];c.lr=root_lr;c.f14.u64=0x1122334455667788;c.v14.u64[0]=0x9988776655443322;c.v14.u64[1]=0x1234567890abcdef;
 std::vector<std::byte> fill(r.guest.stack.size,std::byte{0xa5});r.space.write(r.guest.stack.address,fill);}
void compare(const std::vector<std::byte>& actual,const std::vector<std::byte>& expected){if(actual.size()!=expected.size())throw std::runtime_error("oracle size");for(size_t i=0;i<actual.size();++i){++byte_checks;if(actual[i]!=expected[i])throw std::runtime_error("independent byte oracle mismatch");}}
void valid(unsigned path=0,unsigned boundary=0){
 Rig r;auto&g=r.guest;if(boundary==1)g.context.r1.u64=g.stack.address+496;if(boundary==2)g.context.r1.u64=uint64_t(g.stack.address)+g.stack.size-128;seed(r);
 const auto sp=g.context.r1.u64;auto expected=r.space.read(g.stack.address,g.stack.size);
 // Independent oracle: literal byte offsets/payloads, not generated C++ or the
 // parser's store ordering/table. Also compare every untouched allocation byte.
 auto put=[&](int offset,unsigned width,uint64_t value){size_t at=sp-g.stack.address+offset;for(unsigned i=0;i<width;++i)expected.at(at+i)=std::byte((value>>(8*(width-i-1)))&255);};
 put(-40,8,payloads[0]);put(-32,8,payloads[1]);put(-24,8,payloads[2]);put(-16,8,payloads[3]);put(-8,4,root_lr);put(-496,4,sp);
 {Admission contract(g,r.module,declare(g));BoundScope scope(g,r.module,contract);
  auto before=Scope::guest_entries.load();
  if(path==0)scope.invoke(geometry_entry);else if(path==1)geometry_functions[0].implementation(g.context,nullptr);else if(path==2)Scope::direct(geometry_functions[0].implementation,g.context,nullptr);else Scope::indirect(g.context,nullptr,geometry_entry);
  check(Scope::guest_entries-before==2&&contract.entries()==2);check(scope.writes()==6&&scope.reads()==0);++valid_calls;
  refused([&]{r.space.release(g.stack);});refused([&]{r.space.decommit(g.stack);});refused([&]{g.cleanup();});
 }
 compare(r.space.read(g.stack.address,g.stack.size),expected);
 check(g.context.r1.u64==sp&&g.context.lr==root_lr);check(g.context.r20.u64==payloads[0]&&g.context.r21.u64==payloads[1]&&g.context.r22.u64==payloads[2]&&g.context.r23.u64==payloads[3]);
 check(g.context.r2.u64==0x82020000&&g.context.r13.u64==g.pcr.address&&g.context.msr==PPCContext{}.msr);
 check(g.context.f14.u64==0x1122334455667788&&g.context.v14.u64[0]==0x9988776655443322&&g.context.v14.u64[1]==0x1234567890abcdef);check(r.space.leases()==0);
}
void negatives(){
 Rig r;auto&g=r.guest;seed(r);auto run=[&](Declaration p){Admission a(g,r.module,p);BoundScope s(g,r.module,a);s.invoke(geometry_entry);};
 r.space.protect(g.stack,Permission::read);refused([&]{run(declare(g));});r.space.protect(g.stack,Permission::none);refused([&]{run(declare(g));});r.space.protect(g.stack,Permission::write);refused([&]{run(declare(g));});r.space.protect(g.stack,Permission::read_write);
 r.space.decommit(g.stack);refused([&]{run(declare(g));});r.space.commit(g.stack);
 for(unsigned k=0;k<7;++k){auto p=declare(g);if(k==0)++p.thread;if(k==1)++p.module;if(k==2)p.entry=geometry_helper;if(k==3)p.closed_calls=false;if(k==4)++p.source;if(k==5)p.stack.owner=0;if(k==6)p.stores[0].width=4;refused([&]{run(p);});}
 auto original=g.context.r1.u64;
 for(uint64_t sp:{uint64_t(g.stack.address)+64,original+1,UINT64_MAX,uint64_t(UINT32_MAX)-15}){g.context.r1.u64=sp;refused([&]{run(declare(g));});}g.context.r1.u64=original;
 {auto p=declare(g);p.begin=-32;p.end=32;refused([&]{run(p);});}
 {auto p=declare(g);p.stack.size=492;refused([&]{run(p);});}
 // Permission changes after successful construction/Scope; every applicable
 // generated entry path must still refuse before its first instruction.
 for(unsigned path=0;path<4;++path){Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a);r.space.protect(g.stack,Permission::read);
  refused([&]{if(path==0)s.invoke(geometry_entry);else if(path==1)geometry_functions[0].implementation(g.context,nullptr);else if(path==2)Scope::direct(geometry_functions[0].implementation,g.context,nullptr);else Scope::indirect(g.context,nullptr,geometry_entry);});r.space.protect(g.stack,Permission::read_write);}
 // Revoke after the declared guard's coherent check but before the production
 // prologue's final T8 recheck: no first guest instruction may execute.
 {Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a,nullptr,{},true);refused([&]{s.invoke(geometry_entry);});r.space.protect(g.stack,Permission::read_write);}
 {Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a);refused([&]{geometry_functions[1].implementation(g.context,nullptr);});}
 // Merely creating an ordinary Scope must not bypass this fixture's separately
 // declared footprint contract, through either dispatch or a raw generated call.
 refused([&]{Scope s(g,geometry_functions,r.module.identity);s.invoke(geometry_entry);});
 refused([&]{Scope s(g,geometry_functions,r.module.identity);geometry_functions[0].implementation(g.context,nullptr);});
 {Admission a(g,r.module,declare(g));refused([&]{Scope s(g,geometry_functions,r.module.identity);geometry_functions[0].implementation(g.context,nullptr);});}
 // Controller changes are checked before the first instruction, including the
 // pointer and payload inputs, not merely permissions.
 for(unsigned field=0;field<3;++field){auto sp=g.context.r1.u64,lr=g.context.lr,value=g.context.r20.u64;
  {Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a,nullptr,[&](uint32_t){if(field==0)g.context.r1.u64=g.stack.address+64;if(field==1)g.context.lr^=4;if(field==2)g.context.r20.u64^=1;});refused([&]{s.invoke(geometry_entry);});}
  g.context.r1.u64=sp;g.context.lr=lr;g.context.r20.u64=value;
 }
 refused([&]{geometry_functions[0].implementation(g.context,nullptr);});
 auto saved=r.space.view(g.stack.address,8,Access::read);r.space.decommit(g.stack);r.space.commit(g.stack);refused([&]{r.space.read(saved);});
 {GuestThread other(r.space,r.module,r.slots,r.token);auto old=g.stack;g.stack=other.stack;refused([&]{run(declare(g));});g.stack=old;other.cleanup();}
 {Space split(16384);auto a=split.allocate(4096,0x10000),uncommitted=split.reserve(4096,0x11000);
  refused([&]{Admission::layout(split,a,0x11010);run(declare(g));});split.release(a);split.release(uncommitted);}
 {Space guard(16384);auto none=guard.allocate(4096,0xf000),a=guard.allocate(4096,0x10000);guard.protect(none,Permission::none);
  refused([&]{Admission::layout(guard,a,0x10040);run(declare(g));});guard.release(none);guard.release(a);}
 refused([&]{Admission::address(UINT32_MAX,8,4);});refused([&]{Admission::address(16,INT64_MIN,8);});refused([&]{Admission::address(UINT32_MAX-3,0,8);});
 check(r.space.leases()==0);
 {Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a);check(!r.module.close());refused([&]{s.invoke(geometry_entry);});}
}
void lifecycle(){
 for(uint32_t cap:{4096u,16384u,20480u}){Space small(cap);ModuleLife m(geometry_module_id);Slots slots;auto t=slots.allocate();refused([&]{GuestThread failed(small,m,slots,t);});check(small.live()==0&&small.leases()==0);slots.release(t);check(m.close());}
 {Rig r;auto stale=r.guest.stack;r.space.release(stale);refused([&]{Admission a(r.guest,r.module,declare(r.guest));});r.guest.cleanup();check(r.space.live()==0);}
 for(unsigned i=0;i<3;++i){valid();++cycles;}
 {Rig r;auto&g=r.guest;seed(r);auto unchanged=r.space.read(g.stack.address,g.stack.size);brutal::horizon::Event reached,proceed;GuestTask task(g);brutal::synthetic::CooperativeStop stop;std::atomic<bool> stopped{};auto entries=Scope::guest_entries.load();
  check(task.start([&]{Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a,&stop,[&](uint32_t pc){if(pc==geometry_helper){reached.signal();if(!proceed.wait())throw Error(Fault::unsupported,"bounded rendezvous");}});try{s.invoke(geometry_entry);}catch(const Scope::Stopped&e){if(!s.owns(e.owner))throw;stopped=true;stop.exited();}}));
  check(reached.wait());refused([&]{g.cleanup();});refused([&]{r.space.decommit(g.stack);});refused([&]{r.space.release(g.stack);});check(stop.request());proceed.signal();check(task.join()&&!task.callback_error());
  check(stopped&&Scope::guest_entries-entries==1);compare(r.space.read(g.stack.address,g.stack.size),unchanged);stop.joined();g.cleanup();stop.reclaimed();check(r.module.close());stop.unloaded();check(r.space.live()==0&&r.space.leases()==0);
 }
 // Independent concurrent module shutdown: stale admission propagates through
 // the native callback error channel; no guest exception continuation is faked.
 {Rig r;auto&g=r.guest;seed(r);auto unchanged=r.space.read(g.stack.address,g.stack.size);brutal::horizon::Event reached,proceed;GuestTask task(g);auto entries=Scope::guest_entries.load();
  check(task.start([&]{Admission a(g,r.module,declare(g));BoundScope s(g,r.module,a,nullptr,[&](uint32_t pc){if(pc==geometry_helper){reached.signal();if(!proceed.wait())throw Error(Fault::unsupported,"shutdown rendezvous");}});s.invoke(geometry_entry);}));
  check(reached.wait());check(!r.module.close());proceed.signal();check(task.join()&&task.callback_error());check(Scope::guest_entries-entries==1);compare(r.space.read(g.stack.address,g.stack.size),unchanged);g.cleanup();check(r.module.close());check(r.space.live()==0&&r.space.leases()==0);
 }
 check(Space::totals()==std::pair<size_t,size_t>{0,0});
}
int main(){try{for(unsigned p=0;p<4;++p)valid(p);valid(0,1);valid(0,2);negatives();lifecycle();std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"zero_entry_checks\":"<<zero_entries<<",\"byte_comparisons\":"<<byte_checks<<",\"valid_calls\":"<<valid_calls<<",\"repeat_cycles\":"<<cycles<<",\"guest_entries\":"<<Scope::guest_entries.load()<<",\"expected_callback_errors\":1,\"allocations\":0,\"leases\":0}\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
