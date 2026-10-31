// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "horizon/native_image_plan.h"
#include <limits>

// An inert host-data audit, deliberately NOT a NativeImagePlan or guest mapping.
// Hashes check supplied evidence identities; they do not authenticate a caller's
// provenance claim. No Space, executable storage or guest capability is created.
// Single audit-thread use; caller keeps the source immutable during construction.
namespace brutal::offline {
using Digest=horizon::native_image::Digest;
using horizon::native_image::digest;
using foundation::Error; using foundation::Fault;
inline void require(bool ok,const char* why){if(!ok)throw Error(Fault::argument,why);}
enum class Provenance:uint32_t {unknown,verified_derived};
enum class Declared:uint32_t {unknown,read=1,read_write=3,read_execute=5};
struct Interval {
 uint32_t address{},offset{},size{};uint64_t section{};
 Declared declared{Declared::unknown};Provenance provenance{Provenance::unknown};Digest bytes{};
};
struct Plan {
 uint32_t schema{1},base{};uint64_t module{},generation{};
 Digest original{},source{};std::vector<Interval> intervals;
};
inline Digest identity(const Plan&p){
 std::vector<uint8_t>b;auto n=[&](uint64_t v){for(int j=7;j>=0;--j)b.push_back(uint8_t(v>>(j*8)));};
 n(0x444154413031ull);n(p.schema);n(p.base);n(p.module);n(p.generation);
 for(auto d:{p.original,p.source})b.insert(b.end(),d.begin(),d.end());n(p.intervals.size());
 for(auto&r:p.intervals){n(r.address);n(r.offset);n(r.size);n(r.section);n(uint32_t(r.declared));n(uint32_t(r.provenance));b.insert(b.end(),r.bytes.begin(),r.bytes.end());}
 return digest(b);
}
inline void validate(const Plan&p,std::span<const uint8_t>source,const Digest&trusted){
 require(p.intervals.size()>0&&p.intervals.size()<=64&&source.size()<=64*1024*1024,"data input budget");
 require(p.schema==1&&p.base&&p.module&&p.generation,"data schema/base/owner identity");
 require(p.original!=Digest{}&&p.source!=Digest{}&&identity(p)==trusted,"stale or untrusted data plan");
 require(digest(source)==p.source,"data source fingerprint");
 uint64_t end=p.base,total=0;
 for(auto&r:p.intervals){
  require(r.size&&r.address>=end&&uint64_t(r.address)+r.size<=0x100000000ull,"data overlap/order/address overflow");
  require(r.address>=p.base&&uint64_t(r.address)-p.base==r.offset,"data guest/source coordinate mismatch");
  require(uint64_t(r.offset)+r.size<=source.size(),"data source extent");
  total+=r.size;require(total<=1024*1024,"inert data copy budget");
  require(r.provenance==Provenance::verified_derived,"missing verified data provenance");
  require(r.declared==Declared::unknown||r.declared==Declared::read||r.declared==Declared::read_write||r.declared==Declared::read_execute,"unknown declaration encoding");
  require((r.declared==Declared::unknown)==(r.section==0),"section/declaration evidence mismatch");
  require(digest(source.subspan(r.offset,r.size))==r.bytes,"data interval fingerprint");
  end=uint64_t(r.address)+r.size;
 }
}
// Owner must outlive its data object. No registration in any runtime resolver.
class Owner {
 std::mutex mutex_;bool busy_{},closed_{};
 public:
 const uint64_t module,generation;
 Owner(uint64_t m,uint64_t g):module(m),generation(g){require(m&&g,"unknown data owner");}
 Owner(const Owner&)=delete;
 ~Owner(){if(busy_)std::terminate();}
 void acquire(uint64_t m,uint64_t g){std::lock_guard lock(mutex_);require(m==module&&g==generation,"wrong data module/generation");if(closed_)throw Error(Fault::stale,"data owner closed");if(busy_)throw Error(Fault::ownership,"duplicate live materialization");busy_=true;}
 void release()noexcept{std::lock_guard lock(mutex_);if(!busy_)std::terminate();busy_=false;}
 void close(){std::lock_guard lock(mutex_);if(busy_)throw Error(Fault::ownership,"live data object");closed_=true;}
};
class Data {
 Owner&owner_;Plan plan_;Digest trusted_;std::vector<std::vector<uint8_t>>copies_;
 bool held_{},active_{};uint64_t bytes_{};
 inline static std::atomic<uint64_t>next_{1},live_{},live_bytes_{};
 const uint64_t instance_{next_.fetch_add(1)};
 void discard()noexcept{copies_.clear();if(bytes_){live_bytes_-=bytes_;bytes_=0;}if(active_){--live_;active_=false;}if(held_){owner_.release();held_=false;}}
 public:
 struct View {uint64_t instance;size_t index;uint32_t offset,size;};
 Data(Owner&owner,Plan p,std::span<const uint8_t>source,Digest trusted,int fail=-1):owner_(owner),plan_(std::move(p)),trusted_(trusted){
  validate(plan_,source,trusted_);owner_.acquire(plan_.module,plan_.generation);held_=true;
  try{
   int stage=0;auto point=[&]{if(stage++==fail)throw std::bad_alloc();};
   point();copies_.reserve(plan_.intervals.size());
   for(auto&r:plan_.intervals){point();std::vector<uint8_t>copy(source.begin()+r.offset,source.begin()+r.offset+r.size);copies_.push_back(std::move(copy));bytes_+=r.size;live_bytes_+=r.size;point();}
   point();active_=true;++live_;
  }catch(const std::bad_alloc&){discard();throw Error(Fault::exhausted,"inert data allocation failure");}catch(...){discard();throw;}
 }
 Data(const Data&)=delete;Data&operator=(const Data&)=delete;
 ~Data(){discard();}
 View view(size_t index,uint32_t offset,uint32_t size)const{if(!active_)throw Error(Fault::stale,"closed data object");require(index<copies_.size()&&size&&offset<=copies_[index].size()&&size<=copies_[index].size()-offset,"inert audit view bounds");return {instance_,index,offset,size};}
 std::vector<uint8_t> copy_bytes(View v)const{
  if(!active_||v.instance!=instance_)throw Error(Fault::stale,"stale/foreign inert data view");
  auto checked=view(v.index,v.offset,v.size);(void)checked;
  return {copies_[v.index].begin()+v.offset,copies_[v.index].begin()+v.offset+v.size};
 }
 // Even section-backed bytes receive no guest access through this abstraction.
 void require_guest_access(size_t index)const{if(!active_)throw Error(Fault::stale,"closed data object");require(index<plan_.intervals.size(),"data interval index");if(plan_.intervals[index].declared==Declared::unknown)throw Error(Fault::protection,"unknown guest permission");throw Error(Fault::unsupported,"inert host data is not guest memory");}
 void close()noexcept{discard();}
 static std::pair<uint64_t,uint64_t>totals(){return {live_.load(),live_bytes_.load()};}
};
}
