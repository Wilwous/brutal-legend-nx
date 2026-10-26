// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "generated.h"
#include <algorithm>
namespace brutal::horizon {
REX_EXTERN(__imp__RtlCompareMemoryUlong);
REX_EXTERN(__imp__RtlInitializeCriticalSection);
struct GeneratedXex {uint32_t image_base,image_size,code_base,code_size;std::span<const Function> functions;PPCFunc* initializer{};};
// Intentionally exact unencrypted/uncompressed original-fixture family.
// No decryption, arbitrary PE virtual-size adoption, relocation or title loader.
class XexImage {
 public:
  struct Layout {uint32_t base{},entry{},tls{},slots{},static_size{},raw_size{},thunk{};bool initialize{};std::vector<uint32_t> functions;};
  struct Policy {std::span<const uint8_t> image;const GeneratedXex& generated;uint32_t entry;};
  enum class Signature { compare_u32_read_words,initialize_cs_inout,typed_vector,unverified };
 private:
  std::mutex unload_mutex_;
  Space& space_;ModuleLife life_{0x58455833};Layout layout_{};
  std::vector<Space::Handle> sections_;std::span<const Function> functions_;bool loaded_{};
  static uint32_t number(std::span<const uint8_t> data,size_t at,bool little=false,unsigned width=4){
    if(at>data.size()||width>data.size()-at)throw Error(Fault::bounds,"truncated synthetic XEX field");
    uint32_t value=0;for(unsigned i=0;i<width;++i)value|=uint32_t(data[at+i])<<(8*(little?i:width-1-i));return value;
  }
 public:
  static Layout parse(std::span<const uint8_t> data){
    if(data.size()!=0x41000)throw Error(Fault::bounds,"only complete original flat XEX supported");
    auto be=[&](size_t p){return number(data,p);};auto le=[&](size_t p,unsigned n=4){return number(data,0x1000+p,true,n);};
    auto require=[](bool ok){if(!ok)throw Error(Fault::argument,"synthetic XEX structural contract");};
    require(be(0)==0x58455832&&be(8)==0x1000&&be(16)==0x100&&be(20)==5);
    const uint32_t keys[]{0x3ff,0x10201,0x10100,0x20104,0x103ff};
    for(unsigned i=0;i<5;++i)require(be(24+i*8)==keys[i]);
    Layout l;l.base=be(36);l.entry=be(44);
    require(l.base==0x82000000&&be(28)==0x300&&be(52)==0x380&&be(60)==0x400);
    require(be(0x100)==0x1cc&&be(0x104)==0x40000&&be(0x210)==l.base&&be(0x280)==4);
    const uint32_t kinds[]{3,1,3,2};for(unsigned i=0;i<4;++i)require(be(0x284+i*24)==0x10+kinds[i]);
    require(be(0x300)==8&&be(0x304)==0); // no encryption/compression
    require(le(0)==0x905a4d&&le(0x3c)==0x80&&le(0x80)==0x4550&&le(0x84,2)==0x1f2&&le(0x86,2)==4&&le(0x94,2)==224);
    const size_t opt=0x98;
    require(le(opt,2)==0x10b&&le(opt+28)==l.base&&le(opt+16)+uint64_t(l.base)==l.entry&&le(opt+32)==65536&&le(opt+36)==512&&le(opt+56)==0x40000&&le(opt+60)==0x1000);
    const char* names[]{".header",".text",".rdata",".data"};const uint32_t flags[]{0x40000040,0x60000020,0x40000040,0xc0000040};
    for(unsigned i=0;i<4;++i){size_t s=opt+224+i*40;
      auto name=std::span(data).subspan(0x1000+s,8);std::array<uint8_t,8> expected{};std::copy_n(names[i],std::char_traits<char>::length(names[i]),expected.begin());
      require(std::equal(name.begin(),name.end(),expected.begin())&&le(s+8)==65536&&le(s+12)==i*65536&&le(s+16)==65536&&le(s+20)==i*65536&&le(s+36)==flags[i]);
    }
    require(le(opt+92)==16);
    for(unsigned i=0;i<16;++i)if(i!=3)require(le(opt+96+i*8)==0&&le(opt+100+i*8)==0);
    auto function_bytes=le(opt+100+24);require(le(opt+96+24)==0x20100&&function_bytes>=8&&function_bytes<=16*8&&function_bytes%8==0);
    uint32_t next=l.base+65536;
    for(unsigned i=0;i<function_bytes/8;++i){auto pc=be(0x21100+i*8),encoded=be(0x21104+i*8),count=(encoded&0x3fffff00)>>8;
      require(pc==next&&count>0&&(encoded&0xc00000ff)==0x40000000&&uint64_t(pc)+count*4<=uint64_t(l.base)+0x20000);
      l.functions.push_back(pc);next=pc+count*4;
    }
    require(std::find(l.functions.begin(),l.functions.end(),l.entry)!=l.functions.end());
    l.slots=be(0x380);l.tls=be(0x384);l.static_size=be(0x388);l.raw_size=be(0x38c);
    require(l.slots==64&&l.tls==l.base+0x20000&&l.static_size==32&&l.raw_size==16);
    l.initialize=be(0x400)==0x54;require((be(0x400)==0x4c||l.initialize)&&be(0x404)==16&&be(0x408)==1);
    constexpr std::string_view library="xboxkrnl.exe";require(std::equal(library.begin(),library.end(),data.begin()+0x40c)&&data[0x418]==0);
    require(be(0x41c)==(l.initialize?56:48)&&number(data,0x41c+38,false,2)==(l.initialize?4:2)&&be(0x444)==l.base+0x30000&&be(0x448)==l.base+0x30004);
    if(be(0x31000)!=0x11b||be(0x31004)!=0x100011b)throw Error(Fault::unsupported,"unadmitted XEX import ordinal/type");
    if(l.initialize){require(be(0x44c)==l.base+0x30008&&be(0x450)==l.base+0x3000c);
      if(be(0x31008)!=0x12e||be(0x3100c)!=0x100012e)throw Error(Fault::unsupported,"unadmitted initializer ordinal/type");}
    l.thunk=l.base+0x30004;return l;
  }
  XexImage(Space& space,std::span<const uint8_t> input,Policy policy,int fail_after=-1):space_(space){
    layout_=parse(input);
    if(policy.generated.image_base!=layout_.base||policy.generated.image_size!=0x40000||policy.generated.code_base!=layout_.base+0x10000||policy.generated.code_size!=65536)throw Error(Fault::argument,"generated image metadata mismatch");
    if(policy.image.size()!=input.size()||!std::equal(input.begin(),input.end(),policy.image.begin())||policy.entry!=layout_.entry||policy.generated.functions.size()!=layout_.functions.size()+(layout_.initialize?2:1))
      throw Error(Fault::argument,"image/generated-code identity mismatch");
    for(size_t i=0;i<policy.generated.functions.size();++i){auto f=policy.generated.functions[i];
      if(!f.implementation||f.address!=(i<layout_.functions.size()?layout_.functions[i]:layout_.thunk+8*(i-layout_.functions.size())))throw Error(Fault::argument,"generated mapping mismatch");
      if(i==layout_.functions.size()&&(f.implementation!=__imp__RtlCompareMemoryUlong||f.name!="__imp__RtlCompareMemoryUlong"))throw Error(Fault::unsupported,"import symbol/signature admission");
    }
    if(layout_.initialize){auto f=policy.generated.functions.back();if(!policy.generated.initializer||f.implementation!=policy.generated.initializer||f.name!="__imp__RtlInitializeCriticalSection")throw Error(Fault::unsupported,"initializer symbol binding");}
    functions_=policy.generated.functions;sections_.reserve(4);
    try{
      for(unsigned i=0;i<4;++i){if(int(i)==fail_after)throw Error(Fault::exhausted,"injected image initialization failure");
        auto h=space_.allocate(65536,layout_.base+i*65536);sections_.push_back(h);
        space_.write(h.address,std::as_bytes(input.subspan(0x1000+i*65536,65536)));
        if(i!=3)space_.protect(h,Permission::read);
      }loaded_=true;
    }catch(...){space_.release_all(sections_);throw;}
  }
  XexImage(const XexImage&)=delete;
  ~XexImage(){if(loaded_)try{if(!unload())std::terminate();}catch(...){std::terminate();}}
  ModuleLife& life(){return life_;}
  const Layout& layout()const{return layout_;}
  std::span<const Function> functions()const{return functions_;}
  std::vector<std::byte> tls_template(){ModuleLife::Lease admission(life_);if(!loaded_)throw Error(Fault::stale,"image unloaded");return space_.read(layout_.tls,layout_.raw_size);}
  void lend(Scope& scope){ModuleLife::Lease admission(life_);if(!scope.belongs_to(life_,space_))throw Error(Fault::argument,"foreign module/space call scope");if(!loaded_)throw Error(Fault::stale,"image unloaded");for(auto h:sections_)scope.lend(h);}
  void invoke(Scope& scope,uint32_t entry){ModuleLife::Lease admission(life_);if(!scope.belongs_to(life_,space_))throw Error(Fault::argument,"foreign module/space entry scope");if(!loaded_||entry!=layout_.entry)throw Error(Fault::argument,"XEX entry admission");scope.invoke(entry);}
  void admit_import(uint32_t ordinal,Signature signature){ModuleLife::Lease admission(life_);if(!((ordinal==0x11b&&signature==Signature::compare_u32_read_words)||(layout_.initialize&&ordinal==0x12e&&signature==Signature::initialize_cs_inout)))throw Error(Fault::unsupported,"unverified import or ABI signature");}
  // A specific retained read-only image capability, not a generic raw pointer.
 private:
  friend class ImportFrame;
  std::vector<std::byte> compare_input(uint32_t address,uint32_t size){
    if(!loaded_)throw Error(Fault::stale,"image unloaded");
    auto h=sections_.at(2);if(!size||size>64||(address&3)||(size&3)||address<h.address||uint64_t(address)+size>uint64_t(h.address)+h.size)
      throw Error(Fault::bounds,"compare IN capability bounds/alignment");
    return space_.read(address,size);
  }
  Space::Handle initialize_capability(uint32_t address){
    if(!loaded_||!layout_.initialize)throw Error(Fault::unsupported,"initializer not bound");
    auto h=sections_.at(3);if((address&3)||address<h.address||uint64_t(address)+28>uint64_t(h.address)+h.size||address<h.address+0x100)
      throw Error(Fault::bounds,"initializer INOUT bounds/alignment/ownership");
    return h;
  }
 public:
  struct SectionView {uint32_t address,size;};
  SectionView section(unsigned index){ModuleLife::Lease admission(life_);if(!loaded_||index>=sections_.size())throw Error(Fault::stale,"section capability unavailable");return {sections_[index].address,sections_[index].size};}
  bool unload(){std::lock_guard lock(unload_mutex_);if(!life_.close())return false;if(loaded_){space_.release_all(sections_);sections_.clear();loaded_=false;}return true;}
};
class HookFrame {
 Scope& scope_;std::span<const uint32_t> sites_;HookFrame* previous_;
 inline static thread_local HookFrame* current_{};
 std::function<void(uint32_t)> observed_;
 public:
 std::vector<unsigned> hits;
 HookFrame(Scope& scope,std::span<const uint32_t> sites,std::function<void(uint32_t)> observed={}):scope_(scope),sites_(sites),previous_(current_),observed_(std::move(observed)),hits(sites.size()){
  if(previous_||sites.empty())throw Error(Fault::argument,"hook frame admission");
  current_=this;
 }
 ~HookFrame(){if(current_!=this)std::terminate();current_=previous_;}
 static void poll(uint32_t pc){auto*self=current_;if(!self)throw Error(Fault::no_thread,"missing image hook frame");
  for(size_t i=0;i<self->sites_.size();++i)if(self->sites_[i]==pc){++self->hits[i];if(self->observed_)self->observed_(pc);self->scope_.point(pc);return;}
  throw Error(Fault::unsupported,"unadmitted stop hook");
 }
};
class ImportFrame {
 XexImage& image_;GuestThread& guest_;ModuleLife::Lease module_;Space::Lease input_pin_;
 inline static thread_local ImportFrame* current_{};
 public:
 inline static std::atomic<uint64_t> sdk_entered{},sdk_completed{};
 unsigned calls{},initialized{},trace_events{};std::function<void()> admitted;
 ImportFrame(XexImage& image,GuestThread& guest):image_(image),guest_(guest),module_(image.life()),input_pin_(guest.space(),image.sections_.at(2)){
  Scope::current(guest.context);image.admit_import(0x11b,XexImage::Signature::compare_u32_read_words);
  if(current_||!guest.belongs_to(image.life())||&guest.space()!=&image.space_)throw Error(Fault::argument,"import context/module binding");
  current_=this;
 }
 ~ImportFrame(){if(current_!=this)std::terminate();current_=nullptr;}
 static ImportFrame& current(PPCContext& context){if(!current_||&current_->guest_.context!=&context)throw Error(Fault::no_thread,"unbound import frame");Scope::current(context);return *current_;}
 static void trace(uint32_t address){if(!current_||address!=current_->guest_.context.r3.u32)throw Error(Fault::no_thread,"SDK trace context");++current_->trace_events;}
 uint32_t compare(uint32_t source,uint32_t size,uint32_t pattern);
 void initialize(uint32_t address);
};
}
