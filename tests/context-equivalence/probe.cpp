// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "contract.h"
#include <iostream>
#include <memory>
#include <source_location>
#include <filesystem>
#include <fstream>
using namespace context_test;
std::atomic<uint64_t> positive{},negative{},zero_entry{},oracle_bytes{},differential_bytes{};
uint64_t completed{},equivalent{},counterexamples{},raw_differences{};
std::ofstream* snapshots{};
void word32(uint32_t v){char b[4];for(unsigned i=0;i<4;++i)b[i]=char(v>>(24-i*8));snapshots->write(b,4);}
void check(bool x,std::source_location loc=std::source_location::current()){if(!x)throw std::runtime_error("native equivalence assertion line "+std::to_string(loc.line()));++positive;}
template<class F>void refused(F f,std::source_location loc=std::source_location::current()){
 auto before=Scope::guest_entries.load();try{f();}catch(const Error&){if(Scope::guest_entries!=before)throw std::runtime_error("invalid native context entered PPC");++negative;++zero_entry;return;}throw std::runtime_error("missing native refusal line "+std::to_string(loc.line()));
}
struct Rig {
 const Program&p;Space space;ModuleLife module;Slots slots;Slots::Token token{slots.allocate()};Space::Handle cell{},prefix{};std::unique_ptr<GuestThread> holder;
 Rig(const Program&program,unsigned pages,uint32_t capacity=128*1024):p(program),space(capacity),module(program.module){try{cell=space.allocate(4,cell_address);if(pages)prefix=space.allocate(pages*4096);holder=std::make_unique<GuestThread>(space,module,slots,token);}catch(...){if(prefix.id)space.release(prefix);if(cell.id)space.release(cell);slots.release(token);throw;}}
 GuestThread&g(){return *holder;}
 ~Rig(){holder->cleanup();holder.reset();if(prefix.id)space.release(prefix);space.release(cell);slots.release(token);if(!module.close())std::terminate();}
};
std::array<uint64_t,4> values(unsigned set){return {0x1021324354657687ull^set,0x10004f10ull,0xfedcba9876543210ull^set,0x01030507090b0d0full};}
uint32_t tls_seed(unsigned set){return set?0x55667788:0x11223344;}
Inputs seed(Rig&r,unsigned set){auto&g=r.g();auto v=values(set);g.context.r20.u64=v[0];g.context.r21.u64=v[1];g.context.r22.u64=v[2];g.context.r23.u64=v[3];g.context.lr=exit_cookie;g.context.r3.u64=9;g.context.r4.u64=cell_address;g.context.f14.u64=0x1234567811223344;g.set_tls(tls_seed(set));std::vector<std::byte> fill(g.stack.size,std::byte{0xa5});r.space.write(g.stack.address,fill);r.space.store<uint32_t>(cell_address,0xdeadbeef);
 return {g.identity,g.module_instance(),r.p.source,g.context.r1.u64,r.p.entry,15,tls_seed(set),g.stack,g.pcr,g.tls,r.cell,v,true};}
void compare_oracle(const std::vector<std::byte>&a,const std::vector<std::byte>&b){check(a.size()==b.size());for(size_t i=0;i<a.size();++i){++oracle_bytes;if(a[i]!=b[i])throw std::runtime_error("independent byte oracle failed");}}
struct Observation {std::vector<std::byte> raw,normalized;uint64_t sp;uint32_t result,persistent;bool operator==(const Observation&b)const{return normalized==b.normalized&&result==b.result&&persistent==b.persistent;}};
Observation execute(Rig&r,unsigned set,bool allow_counterexample=false,bool concurrent=false){auto&g=r.g();auto in=seed(r,set);in.require_equivalence=!allow_counterexample;auto sp=in.sp;check(sp>=g.stack.address+496&&sp<=uint64_t(g.stack.address)+g.stack.size-128);std::vector<std::byte> oracle(g.stack.size,std::byte{0xa5});
 // Separate literal oracle, not fixture decoder/table or Python normalizer.
 auto put=[&](int offset,unsigned width,uint64_t value){size_t at=sp-g.stack.address+offset;for(unsigned i=0;i<width;++i)oracle.at(at+i)=std::byte((value>>(8*(width-1-i)))&255);};
 auto v=values(set);put(-40,8,v[0]);put(-32,8,v[1]);put(-24,8,v[2]);put(-16,8,v[3]);put(-8,4,r.p.mode==Mode::tls?tls_seed(set):exit_cookie);put(-496,4,sp);
 {Contract c(g,r.module,r.p,in);Bound bound(g,r.module,r.p,c);auto before=Scope::guest_entries.load();bound.invoke();check(bound.calls()==2&&c.entries()==2);if(!concurrent)check(Scope::guest_entries-before==2);check(bound.writes()==(r.p.mode==Mode::escape?7u:6u));check(bound.reads()==(r.p.mode==Mode::tls?2u:0u));}
 auto raw=r.space.read(g.stack.address,g.stack.size);compare_oracle(raw,oracle);
 check(g.context.r20.u64==v[0]&&g.context.r21.u64==v[1]&&g.context.r22.u64==v[2]&&g.context.r23.u64==v[3]);check(g.context.r1.u64==sp&&g.context.r10.u64==sp-496&&g.context.lr==exit_cookie);check(g.context.f14.u64==0x1234567811223344);check(g.context.r13.u64==g.pcr.address&&g.get_tls()==tls_seed(set));
 auto result=uint32_t(g.context.r3.u64);check(result==(r.p.mode==Mode::fixed?uint32_t(sp==0x10004f00):r.p.mode==Mode::tls?tls_seed(set):9));
 auto persistent=r.space.load<uint32_t>(cell_address);check(persistent==(r.p.mode==Mode::escape?uint32_t(sp):0xdeadbeef));
 if(r.p.mode==Mode::tls)check(g.context.r5.u64==g.tls.address&&g.context.r6.u64==tls_seed(set));
 auto normalized=raw;auto at=sp-g.stack.address-496;check(r.space.load<uint32_t>(uint32_t(sp)-496)==sp);for(unsigned i=0;i<4;++i)normalized[at+i]=std::byte{};
 // No persistent-cell or opaque-payload renaming, even if it looks like a pointer.
 check(r.space.leases()==0);return {raw,normalized,sp,result,persistent};}
void differential(){
 for(unsigned mode=0;mode<4;++mode)for(unsigned set=0;set<2;++set){std::vector<Observation> out;
  for(unsigned pages:{0u,1u,3u}){Rig r(programs[mode],pages);check(r.g().context.r1.u64==0x10004f00+pages*4096);out.push_back(execute(r,set,mode>=2));++completed;
   const auto&o=out.back();for(uint32_t v:{mode,set,pages,r.g().stack.address,uint32_t(o.sp),r.g().pcr.address,r.g().tls.address,o.result,o.persistent,uint32_t(o.raw.size())})word32(v);snapshots->write(reinterpret_cast<const char*>(o.raw.data()),std::streamsize(o.raw.size()));}
  for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b){bool bytes_equal=true;for(size_t i=0;i<out[a].normalized.size();++i){++differential_bytes;if(out[a].normalized[i]!=out[b].normalized[i])bytes_equal=false;}
   check(bytes_equal);check(out[a].raw!=out[b].raw);++raw_differences;
   if(mode<2){check(out[a]==out[b]);++equivalent;}else{bool expected=mode==2?(a==0):true;check((!(out[a]==out[b]))==expected);if(expected)++counterexamples;}
  }
 }
 check(completed==24&&equivalent==12&&counterexamples==10&&raw_differences==24);
}
void invalid(){
 Rig r(programs[0],0);auto&g=r.g();auto input=seed(r,0);auto run=[&](Inputs in){Contract c(g,r.module,r.p,in);Bound b(g,r.module,r.p,c);b.invoke();};
 for(unsigned field=0;field<6;++field){auto p=input;if(field==0)++p.thread;if(field==1)++p.module;if(field==2)++p.source;if(field==3)p.mask=14;if(field==4)p.entry=r.p.helper;if(field==5)p.stack.owner=0;refused([&]{run(p);});}
 for(uint64_t sp:{input.sp+1,uint64_t(g.stack.address)+64,UINT64_MAX}){g.context.r1.u64=sp;auto p=input;p.sp=sp;refused([&]{run(p);});}g.context.r1.u64=input.sp;
 g.context.lr=exit_cookie+4;refused([&]{run(input);});g.context.lr=exit_cookie;
 g.context.r13.u64+=4096;refused([&]{run(input);});g.context.r13.u64=g.pcr.address;
 g.set_tls(input.tls_value+1);refused([&]{run(input);});g.set_tls(input.tls_value);
 {GuestThread other(r.space,r.module,r.slots,r.token);r.space.store<uint32_t>(g.pcr.address,other.tls.address);refused([&]{run(input);});r.space.store<uint32_t>(g.pcr.address,g.tls.address);other.cleanup();}
 r.space.protect(g.stack,Permission::read);refused([&]{run(input);});r.space.protect(g.stack,Permission::read_write);
 {Contract c(g,r.module,r.p,input);Bound b(g,r.module,r.p,c);r.space.protect(g.stack,Permission::read);refused([&]{b.invoke();});refused([&]{r.p.functions[0].implementation(g.context,nullptr);});r.space.protect(g.stack,Permission::read_write);refused([&]{r.space.release(g.stack);});refused([&]{r.space.decommit(g.stack);});refused([&]{g.cleanup();});}
 {Contract c(g,r.module,r.p,input);refused([&]{Scope scope(g,r.p.functions,r.module.identity);scope.invoke(r.p.entry);});}
 {Contract c(g,r.module,r.p,input);Bound b(g,r.module,r.p,c);refused([&]{programs[2].functions[0].implementation(g.context,nullptr);});}
 refused([&]{r.p.functions[0].implementation(g.context,nullptr);});
 for(unsigned mode:{2u,3u,4u}){Rig denied(programs[mode],0);auto p=seed(denied,0);refused([&]{Contract c(denied.g(),denied.module,denied.p,p);Bound b(denied.g(),denied.module,denied.p,c);b.invoke();});}
 {auto old=r.space.view(g.stack.address,4,Access::read);r.space.decommit(g.stack);r.space.commit(g.stack);refused([&]{r.space.read(old);});}
 for(unsigned field=0;field<3;++field){auto p=seed(r,0);{Contract c(g,r.module,r.p,p);Bound b(g,r.module,r.p,c,nullptr,[&](uint32_t){if(field==0)g.context.r1.u64=g.stack.address+64;if(field==1)g.context.lr^=4;if(field==2)g.context.r20.u64^=1;});refused([&]{b.invoke();});}g.context.r1.u64=input.sp;seed(r,0);}
 check(r.space.leases()==0);
}
void lifetime(){
 for(uint32_t cap:{4096u,20480u,24576u}){auto before=Space::totals();refused([&]{Rig bad(programs[0],0,cap);});check(Space::totals()==before);}
 {Rig a(programs[1],0),b(programs[1],1);GuestTask left(a.g()),right(b.g());std::unique_ptr<Observation>x,y;std::string left_error,right_error;
  check(left.start([&]{try{x=std::make_unique<Observation>(execute(a,0,false,true));}catch(const std::exception&e){left_error=e.what();throw;}}));check(right.start([&]{try{y=std::make_unique<Observation>(execute(b,0,false,true));}catch(const std::exception&e){right_error=e.what();throw;}}));
  bool joined_left=left.join(),joined_right=right.join();check(joined_left&&joined_right);if(left.callback_error()||right.callback_error())throw std::runtime_error("worker: "+left_error+" / "+right_error);check(x&&y&&*x==*y);check(a.g().pcr.address!=b.g().pcr.address&&a.g().tls.address!=b.g().tls.address);}
 {Rig r(programs[0],0);auto&g=r.g();auto p=seed(r,0);auto original=r.space.read(g.stack.address,g.stack.size);brutal::horizon::Event reached,proceed;GuestTask task(g);brutal::synthetic::CooperativeStop stop;std::atomic<bool>stopped{};auto entries=Scope::guest_entries.load();
  check(task.start([&]{Contract c(g,r.module,r.p,p);Bound b(g,r.module,r.p,c,&stop,[&](uint32_t pc){if(pc==r.p.helper){reached.signal();if(!proceed.wait())throw Error(Fault::unsupported,"bounded stop rendezvous");}});try{b.invoke();}catch(const Scope::Stopped&e){if(!b.owns(e.owner))throw;stopped=true;stop.exited();}}));
  check(reached.wait());refused([&]{g.cleanup();});refused([&]{r.space.decommit(g.stack);});check(stop.request());proceed.signal();check(task.join()&&!task.callback_error());check(stopped&&Scope::guest_entries-entries==1);check(r.space.read(g.stack.address,g.stack.size)==original);stop.joined();g.cleanup();stop.reclaimed();check(r.module.close());stop.unloaded();check(r.space.live()==1);}
 check(Space::totals()==std::pair<size_t,size_t>{0,0});
}
int main(int argc,char**argv){try{if(argc!=2||std::filesystem::exists(argv[1]))throw std::runtime_error("fresh snapshot path required");std::ofstream output(argv[1],std::ios::binary);snapshots=&output;output.write("CEQ1",4);word32(24);differential();invalid();lifetime();check(Scope::guest_entries==53);output.flush();if(!output)throw std::runtime_error("snapshot write failure");std::cout<<"{\"positive\":"<<positive.load()<<",\"refusals\":"<<negative.load()<<",\"zero_entry_checks\":"<<zero_entry.load()<<",\"oracle_bytes\":"<<oracle_bytes.load()<<",\"differential_bytes\":"<<differential_bytes.load()<<",\"completed_differential_runs\":"<<completed<<",\"equivalent_pairs\":"<<equivalent<<",\"counterexample_pairs\":"<<counterexamples<<",\"raw_stack_differences\":"<<raw_differences<<",\"guest_entries\":"<<Scope::guest_entries.load()<<",\"allocations\":0,\"leases\":0}\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
