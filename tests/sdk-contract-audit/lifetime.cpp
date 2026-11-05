// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/sdk_services.h"
#include <iostream>
using namespace brutal::horizon;using compatibility::SdkServices;
void context_only(PPCContext&,uint8_t*){}
int main(){try{unsigned positive=0,refusals=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("TLS lifetime assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing TLS lifetime refusal");};
 Space s;ModuleLife module(44),alias(44);Slots slots;auto startup=slots.allocate();GuestThread first(s,module,slots,startup),second(s,module,slots,startup),alien(s,alias,slots,startup);SdkServices sdk(s,module,slots);auto a=sdk.register_thread(first),b=sdk.register_thread(second);const Function functions[]{{0x82001000,context_only,"context_only"}};
 no([&]{sdk.register_thread(alien);});auto invoke=[&](GuestThread&g,SdkServices::Thread t,uint32_t op,uint32_t index=0,uint32_t value=0){Scope scope(g,functions,g.module_identity());g.context.r11.u64=op;g.context.r3.u64=index;g.context.r4.u64=value;sdk.invoke(t,g.context);return g.context.r3.u64;};
 no([&]{invoke(second,a,SdkServices::tls_alloc);});auto index=invoke(first,a,SdkServices::tls_alloc);yes(index==1);yes(invoke(first,a,SdkServices::tls_set,1,0xffffffff)==1);yes(invoke(second,b,SdkServices::tls_set,1,23)==1);yes(invoke(first,a,SdkServices::tls_free,1)==1);yes(invoke(first,a,SdkServices::tls_get,1)==0&&invoke(second,b,SdkServices::tls_get,1)==0);
 yes(invoke(first,a,SdkServices::tls_alloc)==index);yes(invoke(second,b,SdkServices::tls_set,uint32_t(index),97)==1);yes(invoke(second,b,SdkServices::tls_get,uint32_t(index))==97); // Raw Xbox u32 indices have no generation; do not invent stale-index rejection.
 Slots proof;auto old=proof.allocate();proof.release(old);auto fresh=proof.allocate();yes(old.index==fresh.index&&old.generation!=fresh.generation);no([&]{proof.validate(old);});no([&]{proof.release(old);});proof.release(fresh);
 sdk.unregister_thread(b);no([&]{invoke(second,b,SdkServices::tls_get,1);});no([&]{sdk.unregister_thread(b);});second.cleanup();no([&]{sdk.register_thread(second);});yes(sdk.registered()==1);
 NativeThread closer;yes(closer.start([&]{module.close();}));yes(closer.join()&&!closer.callback_error());auto entered=sdk.entered;no([&]{invoke(first,a,SdkServices::tls_get,1);});yes(sdk.entered==entered);no([&]{first.cleanup();});sdk.close();no([&]{sdk.register_thread(first);});first.cleanup();alien.cleanup();slots.release(startup);yes(module.close()&&alias.close());yes(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":0,\"guest_entries\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
