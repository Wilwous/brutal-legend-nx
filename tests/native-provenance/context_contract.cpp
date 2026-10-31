// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original synthetic data only. No input files, codegen or dispatched PPC.
#include "runtime/horizon/generated.h"
#include <iostream>
#include <source_location>
using namespace brutal::horizon;
unsigned positive{},negative{},bulk{},callbacks{};
void check(bool b){if(!b)throw std::runtime_error("synthetic context assertion");++positive;}
template<class F>void refused(F fn,std::source_location where=std::source_location::current()){try{fn();}catch(const Error&){++negative;return;}throw std::runtime_error("missing context refusal at line "+std::to_string(where.line()));}
void never_called(PPCContext&,uint8_t*){++callbacks;throw std::runtime_error("fixture must not dispatch");}
int main(){try{
 for(uint32_t capacity:{4096u,16384u,20480u}){Space failed(capacity);ModuleLife m(71);Slots slots;auto token=slots.allocate();refused([&]{GuestThread g(failed,m,slots,token);});check(failed.live()==0&&failed.leases()==0);slots.release(token);check(m.close());}
 {Space failed;ModuleLife m(72);Slots slots;auto token=slots.allocate();std::array<std::byte,33>bad{};refused([&]{GuestThread g(failed,m,slots,token,0,bad);});check(failed.live()==0);slots.release(token);check(m.close());}
 Space memory(256*1024);ModuleLife module(73),wrong(74);Slots slots;auto token=slots.allocate();
 std::array<std::byte,32>initial;for(unsigned i=0;i<32;++i)initial[i]=std::byte(i+11);
 GuestThread a(memory,module,slots,token,17,initial),b(memory,module,slots,token,29,initial);
 check(memory.live()==6);check(a.identity!=b.identity);check(a.stack.address!=b.stack.address&&a.pcr.address!=b.pcr.address&&a.tls.address!=b.tls.address);
 check(a.belongs_to(module)&&!a.belongs_to(wrong));check(a.context.r1.u64%16==0&&a.context.r1.u64>=a.stack.address+64&&a.context.r1.u64<a.stack.address+a.stack.size);
 check(a.context.r13.u64==a.pcr.address&&memory.load<uint64_t>(a.pcr.address+0x30)==a.pcr.address&&memory.load<uint32_t>(a.pcr.address)==a.tls.address);
 auto tls=memory.read(a.tls.address,32);for(unsigned j=0;j<32;++j){if(tls[j]!=initial[j])throw std::runtime_error("synthetic TLS byte oracle");++bulk;}
 check(a.get_tls()==17&&b.get_tls()==29);a.set_tls(99);check(a.get_tls()==99&&b.get_tls()==29);
 refused([&]{slots.release(token);});
 refused([&]{memory.read(a.stack.address-1,1);});
 // The next byte is a valid separately owned PCR, not an unmapped address.
 check(a.pcr.address==a.stack.address+a.stack.size);
 refused([&]{memory.read(a.stack.address+a.stack.size-1,2);});
 refused([&]{memory.read(a.pcr.address+0x2d8,1);});
 refused([&]{memory.read(a.tls.address+32+64*4,1);});
 auto r1=a.context.r1.u64;a.context.r1.u64|=1;refused([&]{GuestThread::Execution guard(a);});a.context.r1.u64=a.stack.address;refused([&]{GuestThread::Execution guard(a);});a.context.r1.u64=r1;
 auto r13=a.context.r13.u64;a.context.r13.u64=b.pcr.address;refused([&]{GuestThread::Execution guard(a);});a.context.r13.u64=r13;
 auto r2=a.context.r2.u64;a.context.r2.u64=0;refused([&]{GuestThread::Execution guard(a);});a.context.r2.u64=r2;
 memory.store<uint32_t>(a.pcr.address,b.tls.address);refused([&]{GuestThread::Execution guard(a);});memory.store<uint32_t>(a.pcr.address,a.tls.address);
 memory.store<uint64_t>(a.pcr.address+0x30,b.pcr.address);refused([&]{GuestThread::Execution guard(a);});memory.store<uint64_t>(a.pcr.address+0x30,a.pcr.address);
 memory.protect(a.stack,Permission::read);refused([&]{memory.view(a.stack.address,4,Access::write);});
 // Existing Execution validates metadata/pins, not stack write permission.
 {GuestThread::Execution metadata_only(a);check(memory.leases()==3);}memory.protect(a.stack,Permission::read_write);
 Function function{0x1010,never_called,"independent_never_called"};
 refused([&]{Scope scope(a,std::span(&function,1),wrong.identity);});
 {Scope scope(a,std::span(&function,1),module.identity);refused([&]{a.cleanup();});check(memory.leases()==6);}
 {auto lease=memory.pin(a.stack.address,1);refused([&]{a.cleanup();});check(memory.live()==6);}
 auto stale=memory.view(a.tls.address,4,Access::read);check(!module.close());a.cleanup();a.cleanup();check(memory.live()==3);refused([&]{GuestThread::Execution guard(a);});refused([&]{memory.read(stale);});b.cleanup();check(memory.live()==0&&memory.leases()==0);
 slots.release(token);refused([&]{slots.validate(token);});check(module.close());check(callbacks==0&&Scope::guest_entries==0);check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<negative<<",\"byte_comparisons\":"<<bulk<<",\"live_allocations\":0,\"leases\":0,\"guest_entries\":0,\"stack_write_guard\":\"checked-memory only; metadata guard insufficient\"}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
