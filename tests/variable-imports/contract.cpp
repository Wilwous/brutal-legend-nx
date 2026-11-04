// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/variable_imports.h"
#include <iostream>
#include <latch>
#include <thread>
using namespace brutal::horizon;namespace nm=brutal::horizon::native_module;namespace hb=brutal::horizon::header_buffer;
unsigned pos{},neg{},bytes{};void check(bool v){if(!v)throw std::runtime_error("variable assertion");++pos;}template<class F>void no(F f){try{f();}catch(const Error&){++neg;return;}throw std::runtime_error("missing variable refusal");}
std::vector<uint8_t> data(bool present=true,bool duplicate=false){std::vector<uint8_t>b{0x58,0x45,0x58,0x32,0,0,0,1,0,0,0,32,0,0,0,0,0,0,0,0,0,0,0,1,0,2,4,uint8_t(present?1:0),0,0,0,21};if(duplicate){b[11]=40;b[23]=2;b.insert(b.end(),{0,2,4,1,0,0,0,22});}return b;}
hb::Source plan(ModuleLife&m,std::span<const uint8_t>b){return {m.identity,m.instance,0,b.size(),0x40000000,hb::digest(b),hb::digest(b)};}
int main(){try{nm::VariableKey key{"xboxkrnl.exe",0x193,nm::ImportKind::variable};
 for(unsigned variant=0;variant<4;++variant){Space s(1024*1024);ModuleLife m(7),other(7);auto b=data(variant!=1,variant==3);if(variant==2)b[31]=0;auto p=plan(m,b);hb::Buffer h(s,m,p,b,hb::identity(p));nm::Metadata meta(s,m,h,h.reference(),0x41000000);nm::Registry registry;
  auto graph=meta.reference();for(int stage=0;stage<4;++stage){no([&]{nm::Provider bad(s,m,meta,graph,key,0x42000000,stage);});check(s.live()==2);}
  nm::Provider provider(s,m,meta,graph,key,0x42000000);auto pref=provider.reference();registry.publish(provider);check(registry.active()==1);no([&]{registry.publish(provider);});
  for(int stage=0;stage<4;++stage){no([&]{nm::Binding bad(registry,s,m,key,pref,0x43000000,stage);});check(s.live()==3);}
  no([&]{nm::Binding wrong(registry,s,other,key,pref,0x43000000);});auto wrong=pref;++wrong.object;no([&]{nm::Binding stale(registry,s,m,key,wrong,0x43000000);});auto fk=key;fk.kind=nm::ImportKind::function;no([&]{nm::Binding function(registry,s,m,fk,pref,0x43000000);});auto unknown=key;++unknown.ordinal;no([&]{nm::Provider unsupported(s,m,meta,graph,unknown,0x42001000);});
  nm::Binding binding(registry,s,m,key,pref,0x43000000);auto ref=binding.reference();auto handles=std::vector<Space::Handle>{};
  {auto reader=binding.acquire(m,ref);handles=reader.registrations();check(handles.size()==4);check(s.load<uint32_t>(0x43000000)==0x42000000);check(s.load<uint32_t>(reader.cell_address())==0x41000000);check(s.load<uint32_t>(reader.module_base()+0x58)==0x40000000);
   auto raw=s.read(0x43000000,4);for(unsigned i=0;i<4;++i){check(raw[i]==nm::word(0x42000000)[i]);++bytes;}
   no([&]{reader.field(0,0x20401);});no([&]{reader.field(reader.header_address(),0x20400);});
   if(variant==3)no([&]{reader.field(reader.header_address(),0x20401);});else{auto field=reader.field(reader.header_address(),0x20401);if(variant==1){check(field.address()==0);no([&]{field.value();});}else{check(field.address()==0x4000001c);check(field.value()==(variant==2?0u:21u));}}
   auto slot=handles[0];s.protect(slot,Permission::none);no([&]{reader.cell_address();});s.protect(slot,Permission::read);no([&]{s.store<uint32_t>(slot.address,0);});no([&]{s.release(slot);});
   registry.withdraw(key,pref);check(registry.active()==0);no([&]{binding.acquire(m,ref);});check(reader.cell_address()==0x42000000);check(!binding.close());
  }
  check(binding.closed());check(s.live()==2);check(meta.close());check(h.close());check(s.live()==0&&s.leases()==0);check(m.close()&&other.close());
 }
 // Malformed backing is a refusal, never the missing-selector zero result.
 for(unsigned variant=0;variant<3;++variant){Space s(65536);ModuleLife m(17+variant);auto b=data();if(variant==0)b[0]=0;else if(variant==1)b[11]=31;else b[23]=2;auto p=plan(m,b);
  no([&]{hb::Buffer h(s,m,p,b,hb::identity(p));nm::Metadata meta(s,m,h,h.reference(),0x41000000);auto r=std::make_shared<nm::Metadata::Reader>(meta.acquire(m,meta.reference()));nm::lookup(r,0x40000000,0x20401);});
  check(s.live()==0&&s.leases()==0&&m.close());
 }
 // Registry replacement requires a fresh provider/capability. Old Ref cannot unbind it.
 {Space s(65536);ModuleLife m(8);auto b=data();auto p=plan(m,b);hb::Buffer h(s,m,p,b,hb::identity(p));nm::Metadata meta(s,m,h,h.reference(),0x41000000);nm::Registry registry;nm::Provider a(s,m,meta,meta.reference(),key,0x42000000);auto old=a.reference();registry.publish(a);nm::Binding binding(registry,s,m,key,old,0x43000000);auto oldbinding=binding.reference();registry.withdraw(key,old);check(binding.close());nm::Provider fresh(s,m,meta,meta.reference(),key,0x42000000);auto now=fresh.reference();registry.publish(fresh);no([&]{registry.withdraw(key,old);});no([&]{binding.acquire(m,oldbinding);});nm::Binding newer(registry,s,m,key,now,0x43000000);no([&]{newer.acquire(m,oldbinding);});check(newer.close());registry.withdraw(key,now);check(meta.close());check(h.close());check(s.live()==0);check(m.close());}
 check(Space::totals()==std::pair<size_t,size_t>{0,0});check(hb::Buffer::registrations()==0&&nm::Object::registrations()==0);
 std::cout<<"{\"positive\":"<<pos<<",\"refusals\":"<<neg<<",\"byte_comparisons\":"<<bytes<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
