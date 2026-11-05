// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/sdk_services.h"
#include <iostream>
using namespace brutal::horizon;using compatibility::SdkServices;
void native_context_only(PPCContext&,uint8_t*){}
int main(){try{unsigned positive=0,refusals=0,bytes=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("SDK contract assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing SDK refusal");};
 Space space(1024*1024),foreign;ModuleLife module(0x53444b),other(0x53444b);Slots slots,wrong_slots;auto startup=slots.allocate();GuestThread a(space,module,slots,startup,7),b(space,module,slots,startup,9);SdkServices sdk(space,module,slots);auto ta=sdk.register_thread(a),tb=sdk.register_thread(b);auto data=space.allocate(512);auto alien=foreign.allocate(16);
 const Function functions[]{{0x82001000,native_context_only,"native contract context only",false,false}};
 auto call=[&](GuestThread&g,SdkServices::Thread t,uint32_t ordinal,uint32_t r3=0,uint32_t r4=0,uint32_t r5=0,Space::Handle cap=Space::Handle{}){Scope scope(g,functions,module.identity);if(cap.id&&cap.owner==data.owner)scope.lend(cap);g.context.r3.u64=r3;g.context.r4.u64=r4;g.context.r5.u64=r5;g.context.r11.u64=ordinal;sdk.invoke(t,g.context,cap);return g.context.r3.u32;};
 yes(sdk.registered()==2);no([&]{sdk.register_thread(a);});{SdkServices invalid(space,module,wrong_slots);no([&]{invalid.register_thread(a);});}
 no([&]{a.cleanup();});yes(space.live()==7); // two thread contexts plus data
 a.context.r31.u64=0x123456789abcdef0ull;a.context.r20.u64=0xffeeddccbbaa9988ull;
 auto value=call(a,ta,SdkServices::fill_ulong,data.address,16,0x01020304,data);yes(value==data.address);auto actual=space.read(data.address,16);for(unsigned i=0;i<16;++i){if(actual[i]!=std::byte((i&3)+1))throw std::runtime_error("fill byte mismatch");++bytes;}
 yes(a.context.r31.u64==0x123456789abcdef0ull&&a.context.r20.u64==0xffeeddccbbaa9988ull);
 auto before=space.read(data.address,16);no([&]{call(a,ta,SdkServices::fill_ulong,data.address+1,4,0,data);});no([&]{call(a,ta,SdkServices::fill_ulong,data.address,3,0,data);});no([&]{call(a,ta,SdkServices::fill_ulong,data.address,0,0,data);});no([&]{call(a,ta,SdkServices::fill_ulong,data.address+508,8,0,data);});no([&]{call(a,ta,SdkServices::fill_ulong,data.address,4,0,alien);});space.protect(data,Permission::read);no([&]{call(a,ta,SdkServices::fill_ulong,data.address,4,0,data);});yes(space.read(data.address,16)==before);space.protect(data,Permission::read_write);
 // SDK source allows an in-capacity cell access before allocation. Allocation
 // resets only the caller; FreeTLS clears every registered thread atomically.
 yes(call(a,ta,SdkServices::tls_set,1,11)==1);yes(call(b,tb,SdkServices::tls_set,1,22)==1);yes(call(a,ta,SdkServices::tls_alloc)==1);yes(call(a,ta,SdkServices::tls_get,1)==0&&call(b,tb,SdkServices::tls_get,1)==22);
 yes(call(a,ta,SdkServices::tls_set,1,0xaabbccdd)==1);yes(call(b,tb,SdkServices::tls_set,1,0x11223344)==1);space.protect(b.tls,Permission::read);no([&]{call(a,ta,SdkServices::tls_free,1);});yes(call(a,ta,SdkServices::tls_get,1)==0xaabbccdd&&call(b,tb,SdkServices::tls_get,1)==0x11223344);space.protect(b.tls,Permission::read_write);
 yes(call(a,ta,SdkServices::tls_free,1)==1);yes(call(a,ta,SdkServices::tls_get,1)==0&&call(b,tb,SdkServices::tls_get,1)==0);yes(call(a,ta,SdkServices::tls_free,1)==0);yes(call(a,ta,SdkServices::tls_alloc)==1);
 for(uint32_t i=2;i<64;++i)yes(call(a,ta,SdkServices::tls_alloc)==i);yes(call(a,ta,SdkServices::tls_alloc)==UINT32_MAX);for(uint32_t i=1;i<64;++i)yes(call(a,ta,SdkServices::tls_free,i)==1);
 for(uint32_t invalid:{64u,256u,0x40000000u,UINT32_MAX}){yes(call(a,ta,SdkServices::tls_get,invalid)==0);yes(call(a,ta,SdkServices::tls_set,invalid,5)==0);yes(call(a,ta,SdkServices::tls_free,invalid)==0);}
 no([&]{call(a,ta,SdkServices::tls_get,0);});yes(a.get_tls()==7&&b.get_tls()==9);no([&]{call(a,ta,0xffff);});no([&]{call(a,tb,SdkServices::tls_get,1);});auto bad=ta;++bad.process;no([&]{call(a,bad,SdkServices::tls_get,1);});
 {Scope scope(a,functions,module.identity);a.context.r13.u64=1;a.context.r11.u64=SdkServices::tls_alloc;auto entered=sdk.entered;no([&]{sdk.invoke(ta,a.context);});yes(sdk.entered==entered);a.context.r13.u64=a.pcr.address;}
 sdk.unregister_thread(tb);no([&]{call(b,tb,SdkServices::tls_get,1);});b.cleanup();yes(sdk.registered()==1);sdk.close();yes(sdk.registered()==0);no([&]{call(a,ta,SdkServices::tls_alloc);});a.cleanup();slots.release(startup);space.release(data);foreign.release(alien);yes(module.close()&&other.close());yes(Space::totals()==std::pair<size_t,size_t>{0,0});
 {Space s;ModuleLife m(77);Slots ts;auto t=ts.allocate();GuestThread g(s,m,ts,t);SdkServices owner(s,m,ts);yes(!m.close());no([&]{owner.register_thread(g);});yes(owner.registered()==0);owner.close();g.cleanup();ts.release(t);yes(m.close());}yes(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<bytes<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
