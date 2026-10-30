// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "native_image_plan.h"
#include "generated.h"
namespace brutal::horizon::native_image {
struct CodeIdentity {Digest source,ppc,generated,sdk,runtime,contracts;uint32_t entry,tls;bool operator==(const CodeIdentity&)const=default;};
struct Boundary {uint32_t pc,size;};
inline Digest contract_identity(std::span<const Boundary>bounds,const ControlPlan&control){
 std::vector<uint8_t>bytes;auto n=[&](uint64_t v){for(int i=7;i>=0;--i)bytes.push_back(uint8_t(v>>(8*i)));};
 n(bounds.size());for(auto b:bounds){n(b.pc);n(b.size);}n(control.sites.size());for(auto s:control.sites){n(s.pc);n(s.owner);n(uint32_t(s.kind));n(s.width);n(s.target);}n(control.roots.size());for(auto root:control.roots)n(root);n(control.exit);return digest(bytes);
}
class Binding {
 Image& image_;Space& space_;ModuleLife& module_;std::span<const Function> functions_;const ControlPlan&control_;
 public:
 Binding(Image&i,Space&s,ModuleLife&m,CodeIdentity actual,const CodeIdentity&trusted,std::span<const Function>functions,std::span<const Boundary>bounds,const ControlPlan&control):image_(i),space_(s),module_(m),functions_(functions),control_(control){
  auto&p=i.plan();require(i.admitted()&&m.identity==p.module&&actual==trusted,"native binding identity");
  require(actual.source==p.source&&actual.generated==p.code&&actual.sdk==p.sdk&&actual.runtime==p.runtime&&actual.entry==p.entry&&actual.tls==p.tls_address,"plan/code/startup mismatch");
  require(contract_identity(bounds,control)==actual.contracts,"control/boundary identity");
  require(actual.ppc!=Digest{}&&actual.contracts!=Digest{}&&functions.size()==bounds.size()&&functions.size()<=32,"function/contract extent");
  bool entry=false;for(size_t n=0;n<functions.size();++n){auto f=functions[n];auto b=bounds[n];require(f.address==b.pc&&b.size&&f.implementation&&f.strict,"function identity/boundary");
   bool owned=false;for(auto&r:p.regions)if(b.pc>=r.address&&uint64_t(b.pc)+b.size<=uint64_t(r.address)+r.file_size&&(r.rights==Rights::read_execute||b.pc==0x82030004))owned=true;require(owned,"unbacked function");
   if(f.address==p.entry)entry=true;
   for(size_t j=0;j<n;++j)require(f.address!=functions[j].address&&f.name!=functions[j].name&&f.implementation!=functions[j].implementation,"duplicate binding");
  }require(entry,"missing bound entry");
  for(auto site:control.sites){bool owned=false;for(auto b:bounds)if(site.owner==b.pc&&site.pc>=b.pc&&uint64_t(site.pc)+4<=uint64_t(b.pc)+b.size)owned=true;require(owned,"site/function mismatch");}
  require(p.imports==std::vector<Import>{{0x11b,1}},"selected native import manifest");
 }
 void lend(Scope&scope){require(image_.admitted()&&scope.belongs_to(module_,space_),"closed/foreign native scope");for(auto h:image_.handles())scope.lend(h);}
 void startup(GuestThread&guest){
  require(image_.admitted()&&guest.belongs_to(module_)&&&guest.space()==&space_,"startup module/space");
  require(space_.read(guest.tls.address,image_.plan().tls_file_size)==space_.read(image_.plan().tls_address,image_.plan().tls_file_size),"startup TLS contents");
 }
 void invoke(Scope&scope,uint32_t entry){require(image_.admitted()&&scope.belongs_to(module_,space_)&&entry==image_.plan().entry,"native entry admission");scope.invoke(entry);}
 const ControlPlan&control()const{return control_;}
};
}
