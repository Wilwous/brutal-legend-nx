// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "sealed_object.h"
#include "runtime/offline_image_data.h"
namespace brutal::horizon::native_module {
// Explicit forensic RO projection, not executable image admission. Unknown
// intervals stay unreserved; original-address write requirements are NOT met.
class DataProjection {
 std::unique_ptr<Object> object_;std::vector<offline::Interval> mapped_,excluded_;
 public:
 DataProjection(Space&s,ModuleLife&m,const offline::Plan&p,std::span<const uint8_t>source,const Digest&trusted,int fail=-1){
  offline::validate(p,source,trusted);require(p.module==m.identity&&p.generation==m.instance,Fault::ownership,"projection module");
  std::vector<Bytes> blocks;for(auto&r:p.intervals){if(r.declared==offline::Declared::unknown){excluded_.push_back(r);continue;}
   // Declaration selects data for audit only; current native R comes from the
   // sealed allocation. No PE flag supplies current/historical RW or execute.
   Bytes b{r.address,{}};auto bytes=source.subspan(r.offset,r.size);b.data.assign(reinterpret_cast<const std::byte*>(bytes.data()),reinterpret_cast<const std::byte*>(bytes.data()+bytes.size()));blocks.push_back(std::move(b));mapped_.push_back(r);
  }
  require(!blocks.empty(),Fault::unsupported,"no source-supported audit intervals");object_=std::make_unique<Object>(s,m,blocks,std::vector<std::shared_ptr<void>>{},0x44415441,fail);
 }
 const auto&mapped()const{return mapped_;}const auto&excluded()const{return excluded_;}
 Object::Ref reference()const{return object_->reference();}
 Object::Reader acquire(ModuleLife&m,Object::Ref ref){return object_->acquire(m,ref);}
 bool close(){return object_->close();}
};
}
