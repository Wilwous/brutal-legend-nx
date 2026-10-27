// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "xex.h"
#include <optional>
namespace brutal::horizon {
// Selected SDK query binding, not a complete SDK BaseHeap implementation.
// Only page-aligned, uniform, whole-page, retained logical regions are admitted.
class QueryFrame {
 XexImage& image_;GuestThread& guest_;ModuleLife::Lease module_;
 std::optional<Space::Query> snapshot_;
 inline static thread_local QueryFrame* current_{};
 public:
 unsigned entered{},returned{},committed{};std::function<void()> admitted;
 QueryFrame(XexImage& image,GuestThread& guest):image_(image),guest_(guest),module_(image.life()){
  Scope::current(guest.context);image.admit_import(0xee,XexImage::Signature::query_basic28);
  if(current_||!Scope::current(guest.context).belongs_to(image.life(),guest.space()))throw Error(Fault::argument,"query image/thread binding");
  current_=this;
 }
 ~QueryFrame(){if(current_!=this)std::terminate();current_=nullptr;}
 QueryFrame(const QueryFrame&)=delete;
 static QueryFrame& current(PPCContext& ctx){if(!current_||&current_->guest_.context!=&ctx)throw Error(Fault::no_thread,"query has no matching guest context");Scope::current(ctx).import_target(0x82030014);return *current_;}
 static const Space::Query* snapshot(uint32_t address){if(!current_||!current_->snapshot_||current_->snapshot_->base!=address)return nullptr;return &*current_->snapshot_;}
 uint32_t query(uint32_t address,uint32_t output,uint32_t kind);
};
}
