// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Narrow host admission for reviewed value-only and capability-bound pointer SDK contracts. This adapter is
// opt-in; legacy generated imports do not automatically pass through it.
#include <rex/ppc.h>
#include "pointer_imports.h"
#include <rex/system/user_module.h>
#include <rex/system/xthread.h>
#include <rex/system/kernel_state.h>
#include <rex/system/export_resolver.h>
#include <array>
#include <string_view>
#include <stdexcept>
#include <utility>
REX_EXTERN(__imp__RtlCompareMemoryUlong);
REX_EXTERN(__imp__RtlInitializeCriticalSection);
REX_EXTERN(__imp__NtQueryVirtualMemory);
REX_EXTERN(__imp__KeTlsGetValue);
REX_EXTERN(__imp__KeTlsSetValue);
REX_EXTERN(__imp__RtlNtStatusToDosError);
REX_EXTERN(__imp__KeQueryPerformanceFrequency);
REX_EXTERN(__imp__XGetAVPack);
REX_EXTERN(__imp__XGetLanguage);
namespace brutal::imports {
enum class Signature { u32_void, u32_u32, u32_u32_u32, unverified,
                       typed_vector_argument, typed_vector_return, mixed_vector, variadic,
                       u32_in_words_u32_u32, void_inout28, u32_address_out28_u32 };
class Admission {
  struct Contract { std::string_view library,name;uint16_t ordinal;Signature signature;PPCFunc* function;bool tls;bool pointer; };
  static inline const std::array<Contract,9> contracts_{{
    {"xboxkrnl.exe","KeTlsGetValue",0x154,Signature::u32_u32,__imp__KeTlsGetValue,true,false},
    {"xboxkrnl.exe","KeTlsSetValue",0x155,Signature::u32_u32_u32,__imp__KeTlsSetValue,true,false},
    {"xboxkrnl.exe","RtlNtStatusToDosError",0x135,Signature::u32_u32,__imp__RtlNtStatusToDosError,false,false},
    {"xboxkrnl.exe","KeQueryPerformanceFrequency",0x83,Signature::u32_void,__imp__KeQueryPerformanceFrequency,false,false},
    {"xam.xex","XGetAVPack",0x3cb,Signature::u32_void,__imp__XGetAVPack,false,false},
    {"xam.xex","XGetLanguage",0x3cd,Signature::u32_void,__imp__XGetLanguage,false,false},
    {"xboxkrnl.exe","RtlCompareMemoryUlong",0x11b,Signature::u32_in_words_u32_u32,__imp__RtlCompareMemoryUlong,true,true},
    {"xboxkrnl.exe","RtlInitializeCriticalSection",0x12e,Signature::void_inout28,__imp__RtlInitializeCriticalSection,true,true},
    {"xboxkrnl.exe","NtQueryVirtualMemory",0xee,Signature::u32_address_out28_u32,__imp__NtQueryVirtualMemory,true,true}
  }};
  rex::system::object_ref<rex::system::UserModule> module_;
  rex::runtime::ExportResolver& resolver_;
  rex::memory::Memory& memory_;
  void invoke(std::string_view library,uint32_t ordinal,uint32_t target,bool indirect,Signature signature,PPCContext& context,PointerBinding* binding=nullptr){
    // Every call obtains a fresh lease: a close request prevents new admission.
    auto lease=module_->AcquireImageLease();
    if(!lease)throw std::logic_error("import module closing or unloaded");
    const Contract* selected=nullptr;
    for(const auto& lib:*module_->xex_module()->import_libraries()){
      for(const auto& import:lib.imports){
        if(indirect ? (!target||import.thunk_address!=target) : ((library!="xboxkrnl.exe"&&library!="xam.xex")||lib.name!=library.substr(0,library.find('.'))||import.ordinal!=ordinal))continue;
        for(const auto& c:contracts_)if(lib.name==c.library.substr(0,c.library.find('.'))&&import.ordinal==c.ordinal){
          if(selected)throw std::invalid_argument("ambiguous import");selected=&c;
        }
      }
    }
    if(!selected||selected->signature!=signature||selected->pointer!=bool(binding))throw std::invalid_argument("import signature not admitted");
    const auto& c=*selected;
    auto* actual=resolver_.GetExportByOrdinal(c.library,c.ordinal);
    if(!actual||actual->ordinal!=c.ordinal||actual->type!=rex::runtime::Export::Type::kFunction||std::string_view(actual->name)!=c.name)
      throw std::invalid_argument("import export identity mismatch");
    if(c.tls){
      if(!rex::system::XThread::IsInThread())throw std::logic_error("TLS requires bound XThread");
      auto* thread=rex::system::XThread::GetCurrentThread();
      if(thread->memory()!=&memory_||rex::runtime::current_ppc_context()!=&context||
         context.r13.u64!=thread->pcr_ptr())throw std::logic_error("TLS context mismatch");
    }
    if(c.pointer)binding->invoke(c.ordinal,c.function,context,memory_);
    else c.function(context,memory_.virtual_membase());
  }
 public:
  Admission(rex::system::object_ref<rex::system::UserModule> module,rex::runtime::ExportResolver& resolver,rex::memory::Memory& memory)
    :module_(std::move(module)),resolver_(resolver),memory_(memory){if(!module_||module_->kernel_state()->memory()!=&memory_)throw std::invalid_argument("import owner");}
  void direct(std::string_view library,uint32_t ordinal,Signature signature,PPCContext& context){invoke(library,ordinal,0,false,signature,context);}
  void indirect(uint32_t thunk,Signature signature,PPCContext& context){invoke({},0,thunk,true,signature,context);}
  void direct(std::string_view library,uint32_t ordinal,Signature signature,PPCContext& context,PointerBinding& binding){invoke(library,ordinal,0,false,signature,context,&binding);}
  void indirect(uint32_t thunk,Signature signature,PPCContext& context,PointerBinding& binding){invoke({},0,thunk,true,signature,context,&binding);}
};
}
