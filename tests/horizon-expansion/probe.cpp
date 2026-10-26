// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/xex.h"
#include "xex-data.h"
#include <cstdio>
#include <memory>
#ifndef STAGE_MAX
#define STAGE_MAX 4
#endif
using namespace brutal::horizon;
namespace {
struct Check {
 ExpansionResult&r;unsigned stage;
 void operator()(bool b,const char* label){if(b){++r.passed;++r.stage_pass[stage-4];}else{++r.failed;++r.stage_fail[stage-4];std::fprintf(stderr,"FAIL stage %u: %s\n",stage,label);}}
 template<class F>void refuse(Fault fault,F f,const char* label){try{f();(*this)(false,label);}catch(const Error&e){(*this)(e.fault==fault,label);}catch(...){(*this)(false,label);}}
};
void memory_checks(Check c){
 Space s(256*1024),foreign(8192);auto a=s.reserve(8192,0x90000000);
 c(s.live()==1,"reservation");c.refuse(Fault::uncommitted,[&]{s.load<uint32_t>(a.address);},"uncommitted");
 s.commit(a);c(s.load<uint64_t>(a.address)==0,"zero commit");
 s.store<uint64_t>(a.address+4092,0x123456789abcdef0ull);c(s.load<uint64_t>(a.address+4092)==0x123456789abcdef0ull,"cross page endian");
 c(s.read(a.address+4092,2)==std::vector<std::byte>{std::byte{0x12},std::byte{0x34}},"big endian bytes");
 auto read=s.view(a.address,8,Access::read),write=s.view(a.address,8,Access::write);std::array<std::byte,8> bytes{};bytes[0]=std::byte{0xaa};
 s.write(write,bytes);c(s.read(read)[0]==std::byte{0xaa},"temporary view");
 auto before=s.read(a.address,8);c.refuse(Fault::bounds,[&]{s.write(write,std::span(bytes).first(7));},"short copy refused");c(s.read(a.address,8)==before,"failure atomic short write");
 c.refuse(Fault::bounds,[&]{s.write(a.address+8190,bytes);},"cross allocation write");c(s.read(a.address,8)==before,"bounds failure atomic");
 s.protect(a,brutal::foundation::Permission::read);c.refuse(Fault::protection,[&]{s.write(write,bytes);},"recheck RO view");
 s.protect(a,brutal::foundation::Permission::write);s.write(write,bytes);c.refuse(Fault::protection,[&]{s.read(read);},"read from write only");
 c.refuse(Fault::protection,[&]{s.load<uint32_t>(a.address);},"translated write only");
 s.protect(a,brutal::foundation::Permission::none);c.refuse(Fault::protection,[&]{s.write(write,bytes);},"no access");s.protect(a,brutal::foundation::Permission::read_write);
 {Space::Lease lease(s,a);c(s.leases()==1,"lease counted");c.refuse(Fault::ownership,[&]{s.decommit(a);},"lease decommit");c.refuse(Fault::ownership,[&]{s.release(a);},"lease release");}
 c(s.leases()==0,"lease balanced");s.decommit(a);s.commit(a);c(s.load<uint64_t>(a.address)==0,"recommit zeroes");
 c.refuse(Fault::stale,[&]{s.read(read);},"epoch stale read");c.refuse(Fault::stale,[&]{s.write(write,bytes);},"epoch stale write");
 auto fresh=s.view(a.address,8,Access::read);c.refuse(Fault::ownership,[&]{foreign.read(fresh);},"foreign view");
 auto forged=fresh;forged.backing.allocation.id++;c.refuse(Fault::ownership,[&]{s.read(forged);},"forged backing");
 s.release(a);c.refuse(Fault::stale,[&]{s.read(fresh);},"released view");c.refuse(Fault::stale,[&]{s.release(a);},"double free");
 c.refuse(Fault::bounds,[&]{s.read(a.address,4);},"freed address");
 c.refuse(Fault::bounds,[&]{s.reserve(8192,0xfffff000);},"address wrap");c.refuse(Fault::argument,[&]{s.reserve(4,0x90000001);},"alignment");
 c.refuse(Fault::exhausted,[&]{s.allocate(UINT32_MAX);},"size rounding overflow");c.refuse(Fault::bounds,[&]{s.read(0xfffffffcu,8);},"span overflow");
 auto b=s.allocate(4096),d=s.allocate(4096);{Space::Lease pin(s,d);std::array group{b,d};c.refuse(Fault::ownership,[&]{s.release_all(group);},"transaction rollback");c(s.live()==2,"no partial release");}
 std::array group{b,d};s.release_all(group);
 for(unsigned i=0;i<8;++i){auto h=s.allocate(4096);s.store<uint32_t>(h.address,i);c(s.load<uint32_t>(h.address)==i,"cycle data");s.release(h);}
 auto counter=s.allocate(4096);std::mutex rmw;NativeThread one,two;
 auto increment=[&]{for(unsigned i=0;i<8;++i){std::lock_guard lock(rmw);s.store<uint32_t>(counter.address,s.load<uint32_t>(counter.address)+1);}};
 c(one.start(increment),"memory worker one");c(two.start(increment),"memory worker two");c(one.join(),"memory join one");c(two.join(),"memory join two");c(s.load<uint32_t>(counter.address)==16,"concurrent serialized RMW");s.release(counter);
 c(s.live()==0&&s.leases()==0,"memory final balance");
}
void pipeline(Check c){
 Space space;XexImage image(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();
 {GuestThread guest(space,image.life(),slots,slot,7,image.tls_template());guest.context.r3.u64=4;guest.context.lr=0x8201feed;
  {Scope scope(guest,image.functions(),image.life().identity);image.lend(scope);ImportFrame imports(image,guest);HookFrame hooks(scope,xex_hooks);
   image.invoke(scope,xex_entry);c(guest.context.r3.u64==17,"production generated entry result");c(imports.calls==1,"actual SDK compare executed");c.r.result=guest.context.r3.u32;c.r.sdk_calls+=imports.calls;}
  guest.cleanup();}
 slots.release(slot);c(image.unload(),"image unload");c(space.live()==0&&space.leases()==0,"pipeline balance");
}
}
ExpansionResult expansion_probe(){ExpansionResult r;try{memory_checks({r,4});pipeline({r,4});}catch(const std::exception&e){std::fprintf(stderr,"ERROR: %s\n",e.what());++r.failed;}return r;}
