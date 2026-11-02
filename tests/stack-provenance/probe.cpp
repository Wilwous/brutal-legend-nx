// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "contract.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
using namespace provenance_test;
std::atomic<uint64_t> positive{},negative{},zero_entry{},oracle_bytes{};
uint64_t completed{},equivalent{},counterexamples{},raw_differences{},differential_bytes{};
std::ofstream*snapshots{};
void word(uint32_t v){char b[4];for(unsigned i=0;i<4;++i)b[i]=char(v>>(24-8*i));snapshots->write(b,4);}
void check(bool value,std::source_location loc=std::source_location::current()){
 if(!value)throw std::runtime_error("provenance assertion line "+std::to_string(loc.line()));++positive;
}
template<class F>void refused(F f,std::source_location loc=std::source_location::current()){
 auto before=Scope::guest_entries.load();try{f();}catch(const Error&){if(Scope::guest_entries!=before)throw std::runtime_error("refusal added PPC entry");++negative;++zero_entry;return;}
 throw std::runtime_error("missing refusal line "+std::to_string(loc.line()));
}
struct Rig {
 const Program&p;Space space;ModuleLife module;Slots slots;Slots::Token token{slots.allocate()};
 Space::Handle cell{},prefix{};std::unique_ptr<GuestThread>holder;
 Rig(const Program&program,unsigned pages,uint32_t capacity=128*1024):p(program),space(capacity),module(program.module){
  try{cell=space.allocate(4,cell_address);if(pages)prefix=space.allocate(pages*4096);holder=std::make_unique<GuestThread>(space,module,slots,token);}
  catch(...){if(prefix.id)space.release(prefix);if(cell.id)space.release(cell);slots.release(token);throw;}
 }
 GuestThread&g(){return *holder;}
 ~Rig(){holder->cleanup();holder.reset();if(prefix.id)space.release(prefix);space.release(cell);slots.release(token);if(!module.close())std::terminate();}
};
uint32_t marker(unsigned seed){return 0x10203040^seed;}
uint32_t tls_value(unsigned seed){return seed?0x55667788:0x11223344;}
Inputs seed(Rig&r,unsigned set){
 auto&g=r.g();g.context.r22.u64=marker(set);g.context.r23.u64=0x1122334410004e40ull;
 g.context.lr=exit_cookie;g.context.r4.u64=cell_address;g.context.f14.u64=0x778899aabbccddeeull;g.set_tls(tls_value(set));
 r.space.write(g.stack.address,std::vector<std::byte>(g.stack.size,std::byte{0xa5}));r.space.store<uint32_t>(cell_address,0xdeadbeef);
 return {g.identity,g.module_instance(),r.p.source,g.context.r1.u64,g.context.r23.u64,marker(set),tls_value(set),g.stack,g.pcr,g.tls,r.cell,true};
}
struct Observation {
 uint32_t sp,result,cell;std::vector<std::byte>raw,normalized;
 bool operator==(const Observation&b)const{return normalized==b.normalized&&result==b.result&&cell==b.cell;}
};
Observation execute(Rig&r,unsigned set,bool concurrent=false){
 auto&g=r.g();auto in=seed(r,set);in.require_equivalence=r.p.relocatable;const auto sp=uint32_t(in.sp);
 std::vector<std::byte>expected(g.stack.size,std::byte{0xa5});
 // Literal byte oracle authored independently from the encoder/decoder.
 auto put=[&](int offset,uint32_t value){auto at=sp-g.stack.address+offset;for(unsigned i=0;i<4;++i)expected.at(at+i)=std::byte(value>>(24-8*i));};
 put(-64,sp);put(-160,sp-64);put(-192,sp-160);put(-188,sp-160);put(-152,marker(set));
 {Contract c(g,r.module,r.p,in);Bound b(g,r.module,r.p,c);auto before=Scope::guest_entries.load();b.invoke();
  check(b.calls()==3&&c.entries()==3);if(!concurrent)check(Scope::guest_entries-before==3);
  check(b.writes()==(r.p.mode==Mode::persistent?6u:5u));check(b.reads()==(r.p.mode==Mode::tls?11u:9u));}
 auto raw=r.space.read(g.stack.address,g.stack.size);check(raw.size()==expected.size());
 for(size_t i=0;i<raw.size();++i){++oracle_bytes;if(raw[i]!=expected[i])throw std::runtime_error("literal oracle byte mismatch");}
 check(g.context.r1.u64==sp&&g.context.lr==exit_cookie&&g.context.r15.u64==exit_cookie&&g.context.r16.u64==0x82010014);
 check(g.context.r5.u64==sp-160&&g.context.r6.u64==sp-64&&g.context.r7.u64==sp);
 check(g.context.r8.u64==g.context.r5.u64&&g.context.r10.u64==g.context.r5.u64&&g.context.r11.u64==marker(set));
 check(g.context.r18.u64==sp&&g.context.r20.u64==sp-64&&g.context.r21.u64==sp-160);
 check(g.context.r22.u64==marker(set)&&g.context.r23.u64==in.payload&&g.context.f14.u64==0x778899aabbccddeeull);
 check(g.context.r13.u64==g.pcr.address&&g.get_tls()==tls_value(set));
 const auto result=uint32_t(g.context.r3.u64),cell=r.space.load<uint32_t>(cell_address);
 const uint32_t wanted=r.p.mode==Mode::tls?tls_value(set):r.p.mode==Mode::integer?sp-185:r.p.mode==Mode::returned?sp-192:r.p.mode==Mode::absolute?uint32_t(sp-192==0x10004e40):marker(set);
 check(result==wanted);check(cell==(r.p.mode==Mode::persistent?sp-192:0xdeadbeef));
 // Allocation still lives after frames popped. Runtime capabilities are NOT
 // per-frame lifetimes. Recording a returned address does not admit its use.
 check(r.space.load<uint32_t>(sp-152)==marker(set));
 auto normalized=raw;
 for(auto [field,target]:std::array<std::pair<int,int>,4>{{{-64,0},{-160,-64},{-192,-160},{-188,-160}}}){
  check(r.space.load<uint32_t>(sp+field)==sp+target);auto at=sp-g.stack.address+field;uint32_t value=sp-g.stack.address+target;
  for(unsigned i=0;i<4;++i)normalized[at+i]=std::byte(value>>(24-8*i));
 }
 check(r.space.leases()==0);return {sp,result,cell,raw,normalized};
}
void differential(){
 for(unsigned mode=0;mode<6;++mode)for(unsigned set=0;set<2;++set){std::vector<Observation>out;
  for(unsigned pages:{0u,1u,3u}){Rig r(programs[mode],pages);auto o=execute(r,set);auto&c=r.g().context;
   check(o.sp==0x10004f00+pages*4096);
   for(uint32_t v:std::array<uint32_t,20>{mode,set,pages,r.g().stack.address,o.sp,r.g().pcr.address,r.g().tls.address,o.result,o.cell,uint32_t(o.raw.size()),c.r5.u32,c.r6.u32,c.r7.u32,c.r8.u32,c.r10.u32,c.r11.u32,c.r18.u32,c.r20.u32,c.r21.u32,c.r1.u32})word(v);
   snapshots->write(reinterpret_cast<const char*>(o.raw.data()),std::streamsize(o.raw.size()));out.push_back(std::move(o));++completed;}
  for(unsigned a=0;a<3;++a)for(unsigned b=a+1;b<3;++b){
   bool same=true;for(size_t i=0;i<out[a].normalized.size();++i){++differential_bytes;if(out[a].normalized[i]!=out[b].normalized[i])same=false;}
   check(same);check(out[a].raw!=out[b].raw);++raw_differences;
   if(mode<2){check(out[a]==out[b]);++equivalent;}
   else{bool refutes=mode!=5||a==0;check((!(out[a]==out[b]))==refutes);if(refutes)++counterexamples;}
  }
 }
 check(completed==36&&equivalent==12&&counterexamples==22&&raw_differences==36);
}
void invalid(){
 Rig r(programs[0],0);auto&g=r.g();auto input=seed(r,0);
 auto invoke=[&](Inputs in){Contract c(g,r.module,r.p,in);Bound b(g,r.module,r.p,c);b.invoke();};
 for(unsigned field=0;field<7;++field){auto in=input;if(field==0)++in.thread;if(field==1)++in.module;if(field==2)++in.source;if(field==3)in.stack.owner=0;if(field==4)in.payload^=1;if(field==5)++in.marker;if(field==6)in.cell=r.g().tls;refused([&]{invoke(in);});}
 for(uint64_t sp:{input.sp+1,uint64_t(g.stack.address)+64,UINT64_MAX}){g.context.r1.u64=sp;auto in=input;in.sp=sp;refused([&]{invoke(in);});}g.context.r1.u64=input.sp;
 g.context.lr=exit_cookie+4;refused([&]{invoke(input);});g.context.lr=exit_cookie;
 g.context.r13.u64+=4096;refused([&]{invoke(input);});g.context.r13.u64=g.pcr.address;
 g.set_tls(input.tls_value+1);refused([&]{invoke(input);});g.set_tls(input.tls_value);
 {GuestThread other(r.space,r.module,r.slots,r.token);r.space.store<uint32_t>(g.pcr.address,other.tls.address);refused([&]{invoke(input);});r.space.store<uint32_t>(g.pcr.address,g.tls.address);other.cleanup();}
 r.space.protect(g.stack,Permission::read);refused([&]{invoke(input);});r.space.protect(g.stack,Permission::read_write);
 {Contract c(g,r.module,r.p,input);Bound b(g,r.module,r.p,c);r.space.protect(g.stack,Permission::read);refused([&]{b.invoke();});r.space.protect(g.stack,Permission::read_write);refused([&]{r.space.release(g.stack);});refused([&]{r.space.decommit(g.stack);});refused([&]{g.cleanup();});}
 {Contract c(g,r.module,r.p,input);refused([&]{Scope b(g,r.p.functions,r.module.identity);b.invoke(r.p.entry);});}
 refused([&]{r.p.functions[0].implementation(g.context,nullptr);});
 for(unsigned mode:{2u,3u,4u,5u,6u}){Rig denied(programs[mode],0);auto in=seed(denied,0);refused([&]{Contract c(denied.g(),denied.module,denied.p,in);Bound b(denied.g(),denied.module,denied.p,c);b.invoke();});}
 {auto old=r.space.view(g.stack.address,4,Access::read);r.space.decommit(g.stack);r.space.commit(g.stack);refused([&]{r.space.read(old);});}
 for(unsigned field=0;field<3;++field){auto in=seed(r,0);{Contract c(g,r.module,r.p,in);Bound b(g,r.module,r.p,c,nullptr,[&](uint32_t){if(field==0)g.context.r1.u64=g.stack.address+64;if(field==1)g.context.lr^=4;if(field==2)g.context.r22.u64^=1;});refused([&]{b.invoke();});}g.context.r1.u64=input.sp;seed(r,0);}
 check(r.space.leases()==0);
}
void lifetime(){
 for(uint32_t capacity:{4096u,20480u,24576u}){auto before=Space::totals();refused([&]{Rig bad(programs[0],0,capacity);});check(Space::totals()==before);}
 {Rig r(programs[4],0);auto o=execute(r,0);auto old=r.g().stack;auto view=r.space.view(o.result,4,Access::read);r.g().cleanup();
  refused([&]{r.space.read(view);});refused([&]{r.space.load<uint32_t>(o.result);});refused([&]{r.space.validate_owned_window(old,o.result,4,Access::read,true);});
  auto fresh=r.space.allocate(old.size,old.address);check(fresh.id!=old.id);refused([&]{r.space.read(view);});
  // Raw numeric addresses can name a new allocation after reuse: the old
  // capability must still fail. No universal stale-raw-pointer claim is made.
  check(r.space.load<uint32_t>(o.result)==0);refused([&]{r.space.release(old);});r.space.release(fresh);}
 {Rig a(programs[1],0),b(programs[1],1);GuestTask left(a.g()),right(b.g());std::unique_ptr<Observation>x,y;std::string le,re;
  check(left.start([&]{try{x=std::make_unique<Observation>(execute(a,0,true));}catch(const std::exception&e){le=e.what();throw;}}));
  check(right.start([&]{try{y=std::make_unique<Observation>(execute(b,0,true));}catch(const std::exception&e){re=e.what();throw;}}));
  bool l=left.join(),rr=right.join();check(l&&rr);if(left.callback_error()||right.callback_error())throw std::runtime_error(le+" / "+re);check(x&&y&&*x==*y);check(a.g().pcr.address!=b.g().pcr.address&&a.g().tls.address!=b.g().tls.address);}
 {Rig r(programs[0],0);auto&g=r.g();auto in=seed(r,0);Event reached,proceed;GuestTask task(g);brutal::synthetic::CooperativeStop stop;std::atomic<bool>stopped{};auto before=Scope::guest_entries.load();
  check(task.start([&]{Contract c(g,r.module,r.p,in);Bound b(g,r.module,r.p,c,&stop,[&](uint32_t pc){if(pc==r.p.leaf){reached.signal();if(!proceed.wait())throw Error(Fault::unsupported,"stop rendezvous");}});try{b.invoke();}catch(const Scope::Stopped&e){if(!b.owns(e.owner))throw;stopped=true;stop.exited();}}));
  check(reached.wait());refused([&]{g.cleanup();});refused([&]{r.space.release(g.stack);});check(stop.request());proceed.signal();check(task.join()&&!task.callback_error());
  check(stopped&&Scope::guest_entries-before==2);check(r.space.load<uint32_t>(uint32_t(in.sp)-152)==0xa5a5a5a5);check(r.space.load<uint32_t>(uint32_t(in.sp)-64)==in.sp&&r.space.load<uint32_t>(uint32_t(in.sp)-160)==in.sp-64);
  stop.joined();g.cleanup();stop.reclaimed();check(r.module.close());stop.unloaded();}
 check(Space::totals()==std::pair<size_t,size_t>{0,0});
}
int main(int argc,char**argv){try{
 if(argc!=2||std::filesystem::exists(argv[1]))throw std::runtime_error("fresh snapshot path required");
 std::ofstream out(argv[1],std::ios::binary);snapshots=&out;out.write("SPV1",4);word(36);
 differential();invalid();lifetime();check(Scope::guest_entries==119);out.flush();if(!out)throw std::runtime_error("snapshot output failed");
 std::cout<<"{\"positive\":"<<positive.load()<<",\"refusals\":"<<negative.load()<<",\"zero_entry_checks\":"<<zero_entry.load()<<",\"oracle_bytes\":"<<oracle_bytes.load()<<",\"differential_bytes\":"<<differential_bytes<<",\"completed_runs\":"<<completed<<",\"equivalent_pairs\":"<<equivalent<<",\"counterexample_pairs\":"<<counterexamples<<",\"raw_stack_differences\":"<<raw_differences<<",\"guest_entries\":"<<Scope::guest_entries.load()<<",\"allocations\":0,\"leases\":0}\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
