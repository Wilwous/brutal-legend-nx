// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "xex.h"
namespace brutal::horizon {
// Narrow real SDK leaf: retained checked OUT only, never a raw guest mapping.
class FillFrame {
 XexImage& image_;GuestThread& guest_;ModuleLife::Lease module_;
 inline static thread_local FillFrame* current_{};
 public:
 unsigned entered{},returned{},committed{};std::function<void()> admitted;
 FillFrame(XexImage&image,GuestThread&guest):image_(image),guest_(guest),module_(image.life()){
  Scope::current(guest.context);image.admit_import(0x126,XexImage::Signature::fill_u32_out);
  if(current_||!Scope::current(guest.context).belongs_to(image.life(),guest.space()))throw Error(Fault::argument,"fill image/thread context");
  current_=this;
 }
 ~FillFrame(){if(current_!=this)std::terminate();current_=nullptr;}
 FillFrame(const FillFrame&)=delete;
 static FillFrame& current(PPCContext&ctx){if(!current_||&current_->guest_.context!=&ctx)throw Error(Fault::no_thread,"fill context missing");Scope::current(ctx).import_target(0x8203001c);return *current_;}
 void fill(uint32_t destination,uint32_t length,uint32_t pattern);
};
}
