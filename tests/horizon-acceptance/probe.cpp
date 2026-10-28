// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/query.h"
#include "xex-data.h"
#include "ii-metadata.h"
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
}
RuntimeIIResult acceptance_probe(unsigned cycles,bool tracing){auto r=runtime_ii_probe(cycles,tracing);auto completed=ImportFrame::sdk_completed.load(),entered=ImportFrame::sdk_entered.load(),entries=Scope::guest_entries.load();std::unique_ptr<brutal::diagnostic::Trace> trace;if(tracing)trace=std::make_unique<brutal::diagnostic::Trace>(ii_metadata);brutal::diagnostic::Binding binding(trace.get(),0,0);
 try{semantics({r});}catch(const std::exception&e){std::fprintf(stderr,"acceptance error: %s\n",e.what());++r.failed;}
 r.sdk_calls+=unsigned(ImportFrame::sdk_completed.load()-completed);r.sdk_entered+=unsigned(ImportFrame::sdk_entered.load()-entered);r.guest_entries+=unsigned(Scope::guest_entries.load()-entries);auto total=Space::totals();r.remaining=unsigned(total.first);r.leases=unsigned(total.second);if(r.remaining||r.leases)++r.failed;if(trace){r.diagnostics+=unsigned(trace->snapshot().size());r.dropped+=unsigned(trace->dropped());if(trace->broken()||r.dropped)++r.failed;}return r;}
