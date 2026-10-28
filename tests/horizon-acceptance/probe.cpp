// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/query.h"
#include "xex-data.h"
#include "ii-metadata.h"
#if STAGE_MAX >= 13
#include "runtime/horizon/fill.h"
#endif
#include <cstdio>
#include <set>
using namespace brutal::horizon;
namespace {
struct Check {RuntimeIIResult&r;void operator()(bool ok,const char*label){if(ok){++r.passed;++r.group_pass[3];}else{++r.failed;++r.group_fail[3];std::fprintf(stderr,"acceptance FAIL: %s\n",label);}}
 template<class F>void refuses(Fault f,F fn,const char*label){try{fn();(*this)(false,label);}catch(const Error&e){(*this)(e.fault==f,label);++r.refusals;}}
};
void semantics(Check c){
 Space s;XexImage image(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();GuestThread g(s,image.life(),slots,slot,7,image.tls_template());auto memory=s.allocate(8192);std::set<uint32_t> sites;
 const std::array<uint32_t,4> a{0xffffffff,0x10203040,0x80000000,0x01020304},b{1,0x11223344,0x80000000,0x04030201};
 for(unsigned i=0;i<4;++i){s.store<uint32_t>(memory.address+i*4,a[i]);s.store<uint32_t>(memory.address+16+i*4,b[i]);}
 auto call=[&](uint32_t pc){g.context.lr=0x8201feed;Scope scope(g,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(memory);ImportFrame imports(image,g);QueryFrame query(image,g);HookFrame hooks(scope,xex_hooks,[&](uint32_t at){sites.insert(at);});scope.invoke(pc);};
 for(uint64_t v:{uint64_t(0),uint64_t(1),uint64_t(0x80000000),uint64_t(UINT32_MAX),uint64_t(0x100000000),UINT64_MAX}){
  g.context.r3.u64=v;g.context.r4.u64=v;g.context.xer.ca=1;g.context.xer.ov=1;g.context.xer.so=1;g.context.cr0.eq=1;
  unsigned zeros=32;for(unsigned bit=0;bit<32;++bit)if(uint32_t(v)&(0x80000000u>>bit)){zeros=bit;break;}
  // Bit-by-bit rotate/mask oracle, independent of the SDK's rotate helper.
  uint32_t rotated=0;for(unsigned dst=8;dst<=23;++dst){unsigned src=(dst+8)%32;if((uint32_t(v)>>(31-src))&1)rotated|=1u<<(31-dst);}
  call(xex_word_logic);c(g.context.r6.u64==zeros,"cntlzw ignores high word and handles zero");c(g.context.r3.u64==rotated,"rlwinm independently selected bits");c(g.context.xer.ca==1&&g.context.xer.ov==1&&g.context.xer.so==1&&g.context.cr0.eq==1,"non-record scalar leaves flags intact");
 }
 const auto sp=g.context.r1.u64;for(unsigned i=0;i<16;++i){
  g.context.r3.u64=memory.address+i;g.context.r4.u64=memory.address+16+i;g.context.r5.u64=memory.address+32+i;for(auto&v:g.context.v14.u8)v=0xa5;call(xex_vector_blocks);
  bool sum=true,xor_bits=true;for(unsigned j=0;j<4;++j){sum&=s.load<uint32_t>(memory.address+32+4*j)==uint32_t(uint64_t(a[j])+b[j]);xor_bits&=s.load<uint32_t>(memory.address+48+4*j)==(a[j]^b[j]);}
  c(sum&&xor_bits,"VMX modulo word and bitwise oracle at all address low bits");bool preserved=true;for(auto v:g.context.v14.u8)preserved&=v==0xa5;c(preserved&&g.context.r1.u64==sp&&g.context.r13.u64==g.pcr.address,"nonvolatile vector stack PCR preservation");
 }
 auto before=s.read(memory.address+32,32);s.protect(memory,brutal::foundation::Permission::read);g.context.r3.u64=memory.address;g.context.r4.u64=memory.address+16;g.context.r5.u64=memory.address+32;c.refuses(Fault::protection,[&]{call(xex_vector_blocks);},"vector write to RO denied");c(s.read(memory.address+32,32)==before,"refused 16-byte publication unchanged");s.protect(memory,brutal::foundation::Permission::read_write);
 g.context.r3.u64=0xfffffff0;c.refuses(Fault::ownership,[&]{call(xex_vector_blocks);},"invalid rounded vector pointer denied");
 s.decommit(memory);g.context.r3.u64=memory.address;c.refuses(Fault::uncommitted,[&]{call(xex_vector_blocks);},"decommitted vector block denied");s.commit(memory);
 auto data=image.section(3);g.context.lr=0x8201feed;g.context.f1.f64=1.5;g.context.f2.f64=2;call(xex_acceptance_multicall);c(g.context.r3.u64==197650&&g.context.f1.f64==3,"nested original integer FP SDK vector multicall result");c.r.result=g.context.r3.u32;
 bool output=true;for(unsigned i=0;i<4;++i)output&=s.load<uint32_t>(data.address+0xc20+i*4)==uint32_t(uint64_t(a[i])+b[i]);c(output,"generated multicall vector memory result");
 unsigned new_memory=0;for(auto site:expansion_sites)if(site.owner>=xex_word_logic&&(site.kind==SiteKind::read||site.kind==SiteKind::write)&&sites.contains(site.pc))++new_memory;c(new_memory==9,"all nine appended memory sites executed");
 s.release(memory);g.cleanup();slots.release(slot);c(image.unload()&&s.live()==0&&s.leases()==0,"semantic ownership cleanup");
}
#if STAGE_MAX >= 13
void services(Check c){
 Space s;XexImage image(s,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots slots;auto slot=slots.allocate();GuestThread guest(s,image.life(),slots,slot,7,image.tls_template());auto out=s.allocate(8192);auto pointer=out.address+4092;
 auto invoke=[&](uint32_t destination,uint32_t length,uint32_t pattern,std::function<void()> admission={}){
  guest.context.r3.u64=destination;guest.context.r4.u64=length;guest.context.r5.u64=pattern;guest.context.lr=0x8201feed;
  Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(out);FillFrame fill(image,guest);fill.admitted=std::move(admission);HookFrame hooks(scope,xex_hooks);scope.invoke(xex_fill_only);return std::array<unsigned,3>{fill.entered,fill.returned,fill.committed};
 };
 for(auto pattern:{0u,UINT32_MAX,0x10203040u,0x80000001u}){auto result=invoke(pointer,64,pattern);bool exact=true;for(unsigned i=0;i<16;++i)exact&=s.load<uint32_t>(pointer+i*4)==pattern;c(result==std::array<unsigned,3>{1,1,1}&&exact,"real SDK bounded OUT cross-page fill/endian oracle");c(guest.context.r3.u64==pointer,"void SDK leaf does not fabricate guest status");}
 auto before=s.read(pointer,64);for(auto length:{0u,1u,3u,65u,UINT32_MAX}){auto count=ImportFrame::sdk_entered.load();c.refuses(Fault::bounds,[&]{invoke(pointer,length,7);},"fill invalid size refused before SDK");c(ImportFrame::sdk_entered.load()==count&&s.read(pointer,64)==before,"fill size failure leaves output/call count unchanged");}
 for(auto address:{0u,pointer+1,0xfffffff0u})c.refuses(Fault::bounds,[&]{invoke(address,64,7);},"fill invalid pointer/overflow refused");
 c.refuses(Fault::ownership,[&]{invoke(out.address+8192-4,64,7);},"fill full span requires one retained allocation");
 s.protect(out,brutal::foundation::Permission::read);c.refuses(Fault::protection,[&]{invoke(pointer,64,7);},"fill RO refuses before SDK");s.protect(out,brutal::foundation::Permission::write);auto result=invoke(pointer,64,0x55667788);c(result[2]==1,"fill OUT-only write-only storage");s.protect(out,brutal::foundation::Permission::read_write);
 before=s.read(pointer,64);auto count=ImportFrame::sdk_completed.load();auto entries=ImportFrame::sdk_entered.load();bool continued=false;
 c.refuses(Fault::protection,[&]{invoke(pointer,64,7,[&]{s.protect(out,brutal::foundation::Permission::read);});continued=true;},"fill publication rechecks changed protection");
 c(!continued&&ImportFrame::sdk_entered.load()==entries+1&&ImportFrame::sdk_completed.load()==count&&s.read(pointer,64)==before,"real SDK return cannot hide refused publication");s.protect(out,brutal::foundation::Permission::read_write);
 image.admit_import(0x126,XexImage::Signature::fill_u32_out);c(true,"fill exact module ordinal signature admitted");c.refuses(Fault::unsupported,[&]{image.admit_import(0x126,XexImage::Signature::query_basic28);},"fill wrong ABI remains denied");
 {auto wrong=xex_generated;wrong.fill=nullptr;Space other;c.refuses(Fault::unsupported,[&]{XexImage denied(other,xex_bytes,{xex_bytes,wrong,xex_entry});},"fill generated binding required");c(other.live()==0,"fill symbol refusal allocates no image");}
 // Query the exact stack, retained image data and an address-limit mapping.
 auto high=s.allocate(4096,0xfffff000);auto query=[&](uint32_t address,std::function<void()> admitted={}){
  guest.context.r3.u64=address;guest.context.r4.u64=pointer;guest.context.r5.u64=0;guest.context.lr=0x8201feed;
  Scope scope(guest,image.functions(),image.life().identity,nullptr,{},expansion_plan_ptr);image.lend(scope);scope.lend(out);scope.lend(high);QueryFrame frame(image,guest);frame.admitted=std::move(admitted);HookFrame hooks(scope,xex_hooks);scope.invoke(xex_query_only);
 };
 query(guest.stack.address);c(s.load<uint32_t>(pointer)==guest.stack.address&&s.load<uint32_t>(pointer+12)==guest.stack.size&&s.load<uint32_t>(pointer+16)==0x1000,"SDK stack query matches actual checked region");
 query(image.section(3).address);c(s.load<uint32_t>(pointer+20)==4,"image data query RW matches checks");query(high.address);c(s.load<uint32_t>(pointer)==0xfffff000&&s.load<uint32_t>(pointer+12)==4096,"query region ending at 2^32 does not wrap");
 query(high.address,[&]{s.protect(high,brutal::foundation::Permission::read);});c(s.load<uint32_t>(pointer+20)==4,"query reports the admitted metadata snapshot");c.refuses(Fault::protection,[&]{s.store<uint32_t>(high.address,1);},"later protection change enforced by checked memory");s.release(high);c.refuses(Fault::bounds,[&]{s.query(0xfffff000);},"freed mapping query refused");c.refuses(Fault::bounds,[&]{s.read(0xfffffff0,32);},"overflowing checked span refused");
 s.release(out);guest.cleanup();slots.release(slot);c(image.unload()&&s.live()==0&&s.leases()==0,"SDK extension ownership cleanup");
 for(unsigned mode=0;mode<2;++mode){Space space;XexImage module(space,xex_bytes,{xex_bytes,xex_generated,xex_entry});Slots registry;auto token=registry.allocate();GuestThread g(space,module.life(),registry,token);auto output=space.allocate(4096);auto saved=space.read(output.address,64);GuestTask task(g);brutal::horizon::Event ready,release;brutal::synthetic::CooperativeStop stop;bool stopped=false,continued=false;
  c(task.start([&]{g.context.r3.u64=output.address;g.context.r4.u64=64;g.context.r5.u64=0x12345678;g.context.lr=0x8201feed;Scope scope(g,module.functions(),module.life().identity,mode?&stop:nullptr,{},expansion_plan_ptr);module.lend(scope);scope.lend(output);FillFrame frame(module,g);frame.admitted=[&]{ready.signal();if(!release.wait())throw Error(Fault::unsupported,"fill lifetime gate");};HookFrame hooks(scope,xex_hooks);try{scope.invoke(xex_fill_only);continued=true;}catch(const Scope::Stopped&e){if(e.owner!=&scope)throw;stopped=true;stop.exited();}}),"fill lifetime worker starts");
  c(ready.wait(),"fill lifetime admission observed");c.refuses(Fault::ownership,[&]{space.release(output);},"active fill OUT allocation remains pinned");c(!task.join(0),"fill failed join retains ownership");
  if(mode){c(!module.unload()&&stop.request(),"active fill closes module and requests stop");c.refuses(Fault::stale,[&]{module.admit_import(0x126,XexImage::Signature::fill_u32_out);},"new fill admission after close denied");}
  else space.protect(output,brutal::foundation::Permission::read);
  release.signal();c(task.join(),"fill lifetime native join");if(mode){c(stopped&&!continued&&!task.callback_error()&&space.load<uint32_t>(output.address)==0x12345678,"accepted fill commits then stops at next generated hook");stop.joined();}else c(task.callback_error()&&!continued&&space.read(output.address,64)==saved,"fill failed publication propagates callback error atomically");
  space.release(output);g.cleanup();registry.release(token);if(mode)stop.reclaimed();c(module.unload()&&space.live()==0&&space.leases()==0,"fill lifetime final cleanup");if(mode)stop.unloaded();
 }

}
#endif

}
RuntimeIIResult acceptance_probe(unsigned cycles,bool tracing){auto r=runtime_ii_probe(cycles,tracing);auto completed=ImportFrame::sdk_completed.load(),entered=ImportFrame::sdk_entered.load(),entries=Scope::guest_entries.load();std::unique_ptr<brutal::diagnostic::Trace> trace;if(tracing)trace=std::make_unique<brutal::diagnostic::Trace>(ii_metadata);brutal::diagnostic::Binding binding(trace.get(),0,0);
 try{semantics({r});
#if STAGE_MAX >= 13
 services({r});
#endif
 }catch(const std::exception&e){std::fprintf(stderr,"acceptance error: %s\n",e.what());++r.failed;}
 r.sdk_calls+=unsigned(ImportFrame::sdk_completed.load()-completed);r.sdk_entered+=unsigned(ImportFrame::sdk_entered.load()-entered);r.guest_entries+=unsigned(Scope::guest_entries.load()-entries);auto total=Space::totals();r.remaining=unsigned(total.first);r.leases=unsigned(total.second);if(r.remaining||r.leases)++r.failed;if(trace){r.diagnostics+=unsigned(trace->snapshot().size());r.dropped+=unsigned(trace->dropped());if(trace->broken()||r.dropped)++r.failed;}return r;}
