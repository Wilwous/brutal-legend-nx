// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <bit>
#include <cstring>
#include <set>
namespace brutal::horizon::native_image {
using Digest=std::array<uint8_t,32>;
// SHA-256 over explicit bytes; canonical encoding below never hashes padding.
inline Digest digest(std::span<const uint8_t> input){
 constexpr uint32_t k[]{0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 std::vector<uint8_t> b(input.begin(),input.end());uint64_t bits=uint64_t(b.size())*8;b.push_back(128);while(b.size()%64!=56)b.push_back(0);for(int i=7;i>=0;--i)b.push_back(uint8_t(bits>>(8*i)));
 uint32_t h[]{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
 for(size_t o=0;o<b.size();o+=64){uint32_t w[64]{};for(unsigned i=0;i<16;++i)for(unsigned j=0;j<4;++j)w[i]=(w[i]<<8)|b[o+4*i+j];for(unsigned i=16;i<64;++i){auto a=w[i-15],z=w[i-2];w[i]=w[i-16]+(std::rotr(a,7)^std::rotr(a,18)^(a>>3))+w[i-7]+(std::rotr(z,17)^std::rotr(z,19)^(z>>10));}
 auto a=h[0],c=h[2],bb=h[1],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];for(unsigned i=0;i<64;++i){auto t=z+(std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25))+((e&f)^(~e&g))+k[i]+w[i];auto u=(std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22))+((a&bb)^(a&c)^(bb&c));z=g;g=f;f=e;e=d+t;d=c;c=bb;bb=a;a=t+u;}h[0]+=a;h[1]+=bb;h[2]+=c;h[3]+=d;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=z;}
 Digest out{};for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)out[4*i+j]=uint8_t(h[i]>>(24-8*j));return out;
}
enum class Provenance:uint32_t {unknown,original_synthetic};
enum class Init:uint32_t {unknown,file_then_authored_zero};
enum class Rights:uint32_t {read=1,read_write=3,read_execute=5};
struct Region {uint64_t owner{};uint32_t address{},size{},source_offset{},file_size{},zero_size{},alignment{4096};Rights rights{Rights::read};Init init{Init::unknown};bool reserve{true},commit{true};};
struct Import {uint32_t ordinal{},signature{};bool operator==(const Import&)const=default;};
struct Plan {
 uint32_t schema{1};Provenance provenance{Provenance::unknown};uint64_t module{},lifetime{};
 uint32_t base{},limit{},entry{},tls_address{},tls_file_size{},tls_total_size{},tls_slots{},relocations{};
 Digest source{},code{},sdk{},runtime{};std::vector<Region> regions;std::vector<Import> imports;
};
inline Digest identity(const Plan&p){std::vector<uint8_t>b;auto n=[&](uint64_t v){for(int i=7;i>=0;--i)b.push_back(uint8_t(v>>(8*i)));};n(p.schema);n(uint32_t(p.provenance));n(p.module);n(p.lifetime);for(auto v:{p.base,p.limit,p.entry,p.tls_address,p.tls_file_size,p.tls_total_size,p.tls_slots,p.relocations})n(v);for(auto d:{p.source,p.code,p.sdk,p.runtime})b.insert(b.end(),d.begin(),d.end());n(p.regions.size());for(auto&r:p.regions){n(r.owner);for(auto v:{r.address,r.size,r.source_offset,r.file_size,r.zero_size,r.alignment,uint32_t(r.rights),uint32_t(r.init),uint32_t(r.reserve),uint32_t(r.commit)})n(v);}n(p.imports.size());for(auto i:p.imports){n(i.ordinal);n(i.signature);}return digest(b);}
inline void require(bool v,const char* why){if(!v)throw Error(Fault::argument,why);}
inline void validate(const Plan&p,std::span<const uint8_t>source,const Digest&expected){
 require(source.size()<=64*1024*1024&&p.regions.size()<=64&&p.imports.size()<=4,"identity input budget");
 require(p.schema==1&&p.provenance==Provenance::original_synthetic&&p.module&&p.lifetime,"schema/origin/module/lifetime");
 require(identity(p)==expected&&digest(source)==p.source,"plan/source identity");
 for(auto d:{p.code,p.sdk,p.runtime})require(d!=Digest{},"missing contract identity");
 require(p.base&&p.limit>p.base&&p.regions.size()>0&&p.regions.size()<=64&&p.relocations==0,"address space/relocation policy");
 std::set<uint64_t> owners;uint64_t previous=p.base;bool entry=false,tls=false;
 for(auto&r:p.regions){
  require(r.owner&&owners.insert(r.owner).second,"duplicate region owner");
  require(r.size&&r.alignment==4096&&(r.address%r.alignment)==0&&(r.size%r.alignment)==0,"range alignment");
  uint64_t end=uint64_t(r.address)+r.size;require(r.address>=previous&&end<=p.limit,"overlap/order/overflow/limit");previous=end;
  require(r.reserve&&r.commit,"reservation/commit policy");
  require(r.init==Init::file_then_authored_zero&&uint64_t(r.file_size)+r.zero_size==r.size,"initialization coverage");
  require(uint64_t(r.source_offset)+r.file_size<=source.size()&&(r.file_size||r.source_offset==0),"source extent");
  require(r.rights==Rights::read||r.rights==Rights::read_write||r.rights==Rights::read_execute,"permission policy");
  if(p.entry>=r.address&&uint64_t(p.entry)+4<=end&&r.rights==Rights::read_execute&&(p.entry%4)==0&&uint64_t(p.entry-r.address)+4<=r.file_size)entry=true;
  if(p.tls_address>=r.address&&uint64_t(p.tls_address)+p.tls_file_size<=uint64_t(r.address)+r.file_size&&r.rights!=Rights::read_execute)tls=true;
 }
 require(entry&&tls&&p.tls_file_size<=32&&p.tls_total_size==32&&p.tls_slots==64,"entry/TLS contract");
 std::set<uint32_t>ordinals;for(auto i:p.imports){require(ordinals.insert(i.ordinal).second,"duplicate import");require((i.ordinal==0x11b&&i.signature==1)||(i.ordinal==0x12e&&i.signature==2)||(i.ordinal==0xee&&i.signature==3)||(i.ordinal==0x126&&i.signature==4),"unsupported import signature");}
}
struct State {bool declared{},reserved{},backed{},initialized{},explicit_zero{},committed{},accessible{},executable{},released{};};
class Image {
 Space& space_;const Plan plan_;const Digest identity_;std::vector<Space::Handle> handles_;std::vector<State> states_;bool registered_{},admitted_{};
 inline static std::mutex registry_mutex_;inline static std::set<std::pair<Space*,uint64_t>> registry_;
 void unregister()noexcept{if(registered_){std::lock_guard lock(registry_mutex_);registry_.erase({&space_,plan_.module});registered_=false;}}
 public:
 Image(Space&s,Plan p,std::span<const uint8_t>bytes,Digest expected,int fail=-1):space_(s),plan_(std::move(p)),identity_(expected){
  validate(plan_,bytes,expected);handles_.reserve(plan_.regions.size());states_.resize(plan_.regions.size());
  {std::lock_guard lock(registry_mutex_);require(registry_.insert({&space_,plan_.module}).second,"conflicting module owner");registered_=true;}
  try{int step=0;auto point=[&]{if(step++==fail)throw Error(Fault::exhausted,"injected materialization failure");};
   for(size_t i=0;i<plan_.regions.size();++i){auto&r=plan_.regions[i];auto&st=states_[i];st.declared=true;point();auto h=space_.reserve(r.size,r.address);handles_.push_back(h);st.reserved=true;st.backed=true;point();space_.commit(h);st.committed=true;
    std::vector<std::byte> initialized(r.size);if(r.file_size)std::memcpy(initialized.data(),bytes.data()+r.source_offset,r.file_size);space_.write(h.address,initialized);st.initialized=true;st.explicit_zero=r.zero_size!=0;point();
    space_.protect(h,r.rights==Rights::read_write?Permission::read_write:Permission::read);st.executable=r.rights==Rights::read_execute;st.accessible=true;
   }admitted_=true;
  }catch(...){space_.release_all(handles_);handles_.clear();unregister();throw;}
 }
 Image(const Image&)=delete;
 ~Image(){if(registered_)try{unload();}catch(...){std::terminate();}}
 const Plan&plan()const{return plan_;}const Digest&fingerprint()const{return identity_;}bool admitted()const{return admitted_;}
 const std::vector<State>&states()const{return states_;}
 std::span<const Space::Handle>handles()const{return handles_;}
 void unload(){space_.release_all(handles_);handles_.clear();admitted_=false;for(auto&st:states_){st.accessible=false;st.committed=false;st.backed=false;st.released=true;}unregister();}
};
}
