// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "generated_imports.h"
#include <rex/image_info.h>
namespace brutal::synthetic {
// Original fixture policy, not an arbitrary title loader or ABI inference.
struct ImageDescription {
 uint32_t base,entry,size,code_base;
 std::span<const uint8_t> code;
};
class XexBinding {
 using Module=rex::system::UserModule;
 using Memory=foundation::SdkCheckedMemory;
 rex::system::object_ref<Module> module_;
 std::optional<Module::ImageLease> lease_;
 std::vector<PPCFuncMapping> functions_;
 struct Import {uint32_t address,ordinal;PPCFunc* function;imports::Signature signature;bool indirect;};
 std::vector<Import> imports_;
 ImageDescription description_;
 public:
 XexBinding(rex::system::object_ref<Module> module,const rex::PPCImageInfo& generated,
     ImageDescription description,size_t mapping_count,rex::runtime::ExportResolver& resolver)
     :module_(std::move(module)),lease_(module_?module_->AcquireImageLease():std::nullopt),description_(description){
  auto deny=[](){throw std::invalid_argument("synthetic XEX/code binding mismatch");};
  if(!lease_||mapping_count!=7||!generated.func_mappings||generated.rexcrt_heap||generated.register_modules)deny();
  const auto& f=generated.codegen_flags;
  if(f.skip_lr||f.ctr_as_local||f.xer_as_local||f.reserved_as_local||f.skip_msr||f.cr_as_local||f.non_argument_as_local||f.non_volatile_as_local)deny();
  auto*x=module_->xex_module();
  if(generated.image_base!=description.base||generated.image_size!=description.size||
     generated.code_base!=description.code_base||generated.code_size!=65536||
     x->base_address()!=description.base||x->entry_point()!=description.entry||
     x->image_size()!=description.size||description.code.empty()||description.code.size()>4096)deny();
  if(x->pe_sections().size()!=4)deny();
  const char* names[]{".header",".text",".rdata",".data"};
  const uint32_t flags[]{0x40000040,0x60000020,0x40000040,0xc0000040};
  for(unsigned i=0;i<4;++i){auto&s=x->pe_sections()[i];if(std::string_view(s.name)!=names[i]||s.address!=description.base+i*65536||s.size!=65536||s.flags!=flags[i])deny();}
  std::vector<uint8_t> bytes(description.code.size());
  if(lease_->CopyMemory(description.code_base,bytes.data(),bytes.size(),Memory::Access::read)!=Memory::Result::ok||!std::equal(bytes.begin(),bytes.end(),description.code.begin()))deny();
  rex::xex2_opt_tls_info tls{};
  if(!x->ReadTlsInfo(&tls)||uint32_t(tls.slot_count)!=64||uint32_t(tls.raw_data_address)!=description.base+0x20000||uint32_t(tls.data_size)!=32||uint32_t(tls.raw_data_size)!=16)deny();
  struct Contract{uint32_t ordinal;const char*name;PPCFunc*function;imports::Signature signature;bool indirect;};
  const Contract contracts[]{
   {0x11b,"RtlCompareMemoryUlong",__imp__RtlCompareMemoryUlong,imports::Signature::u32_in_words_u32_u32,false},
   {0xee,"NtQueryVirtualMemory",__imp__NtQueryVirtualMemory,imports::Signature::u32_address_out28_u32,true},
   {0x12e,"RtlInitializeCriticalSection",__imp__RtlInitializeCriticalSection,imports::Signature::void_inout28,false}};
  const auto&libs=*x->import_libraries();if(libs.size()!=1||libs[0].name!="xboxkrnl"||libs[0].imports.size()!=3)deny();
  for(auto&entry:libs[0].imports){const Contract*selected=nullptr;
   for(auto&c:contracts)if(c.ordinal==entry.ordinal)selected=&c;
   if(!selected)deny();const auto&c=*selected;auto*symbol=resolver.GetExportByOrdinal("xboxkrnl.exe",c.ordinal);
   if(!symbol||symbol->type!=rex::runtime::Export::Type::kFunction||symbol->ordinal!=c.ordinal||std::string_view(symbol->name)!=c.name)deny();
   for(auto&previous:imports_)if(previous.ordinal==c.ordinal||previous.address==entry.thunk_address)deny();
   bool found=false;for(size_t i=0;i<mapping_count-1;++i)if(generated.func_mappings[i].guest==entry.thunk_address){if(found||generated.func_mappings[i].host!=c.function)deny();found=true;}
   if(!found)deny();imports_.push_back({entry.thunk_address,c.ordinal,c.function,c.signature,c.indirect});
  }
  if(generated.func_mappings[mapping_count-1].guest||generated.func_mappings[mapping_count-1].host)deny();
  for(size_t i=0;i<mapping_count-1;++i){auto fn=generated.func_mappings[i];bool imported=false;
   for(auto imp:imports_)if(imp.address==fn.guest)imported=true;
   if(imported)continue;
   if(!fn.host||(fn.guest!=description.entry&&fn.guest!=description.code_base&&fn.guest!=description.code_base+184))deny();
   for(auto previous:functions_)if(previous.guest==fn.guest||previous.host==fn.host)deny();functions_.push_back(fn);
  }
  if(functions_.size()!=3)deny();
 }
 XexBinding(const XexBinding&)=delete;XexBinding&operator=(const XexBinding&)=delete;
 uint32_t query_target()const{for(auto i:imports_)if(i.ordinal==0xee)return i.address;throw std::logic_error("query not bound");}
 void admit(instrumentation::Scope& scope,imports::GeneratedImports& dispatch,imports::PointerBinding& binding)const{
  // A new call scope may not be created after a close request.
  auto fresh=module_->AcquireImageLease();if(!fresh)throw std::logic_error("image closing");
  scope.lend_image(*lease_);
  for(auto fn:functions_)scope.allow_synthetic(uint32_t(fn.guest),fn.host);
  for(auto i:imports_)if(i.indirect)dispatch.bind_indirect(i.address,i.ordinal,i.signature,binding);
   else dispatch.bind(i.function,i.address,i.ordinal,i.signature,binding);
 }
 void enter(instrumentation::Scope& scope,PPCContext& context,uint8_t*base)const{auto fresh=module_->AcquireImageLease();if(!fresh)throw std::logic_error("entry image closing");scope.indirect(description_.entry,context,base);}
 const Module::ImageLease& lease()const{return *lease_;}
};
}
