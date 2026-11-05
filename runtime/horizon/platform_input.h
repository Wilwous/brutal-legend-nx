// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace brutal::horizon::platform {
// One portable, single-owner sampler. Platform adapters supply stable logical
// slot and identity (not a guest address or host pointer). Missing slots mean
// disconnected; reconnect increments epoch and starts fresh button edges.
class Input {
 public:
  static constexpr uint32_t slots=8,wire_size=96,magic=0x494e5031;
  enum Button : uint64_t { a=1ull<<0,b=1ull<<1,x=1ull<<2,y=1ull<<3,
    up=1ull<<4,down=1ull<<5,left=1ull<<6,right=1ull<<7,
    l=1ull<<8,r=1ull<<9,zl=1ull<<10,zr=1ull<<11,
    plus=1ull<<12,minus=1ull<<13,stick_l=1ull<<14,stick_r=1ull<<15 };
  static constexpr uint64_t valid_buttons=0xffff;
  struct RawController {uint32_t slot{};uint64_t identity{};bool connected{};uint64_t buttons{};int32_t left_x{},left_y{},right_x{},right_y{};};
  struct Snapshot {
    uint32_t slot{};uint64_t frame{},identity{},epoch{};bool connected{};
    uint64_t held{},pressed{},released{};
    int32_t left_x{},left_y{},right_x{},right_y{}; // Q15 [-32767,32767].
    uint32_t left_trigger{},right_trigger{}; // Digital 0 or 65535.
  };
 private:
  std::array<Snapshot,slots> snapshots_{};uint64_t frame_{};int32_t deadzone_;
  static void require(bool ok,Fault fault,const char* message){if(!ok)throw Error(fault,message);}
  int32_t axis(int32_t raw)const{
    const int32_t magnitude=raw<0?-raw:raw;
    if(magnitude<=deadzone_)return 0;
    // Both signed native extremes map exactly to +/-32767, with a continuous
    // axial deadzone rescaled to the full range. Integer arithmetic is stable.
    const int32_t bounded=std::min(magnitude,32767);
    const int32_t normalized=int32_t(int64_t(bounded-deadzone_)*32767/(32767-deadzone_));
    return raw<0?-normalized:normalized;
  }
 public:
  explicit Input(int32_t deadzone=4096):deadzone_(deadzone){require(deadzone>=0&&deadzone<32767,Fault::argument,"input deadzone");for(uint32_t i=0;i<slots;++i)snapshots_[i].slot=i;}
  static float normalized(int32_t q15){require(q15>=-32767&&q15<=32767,Fault::bounds,"input normalized Q15 range");return float(q15)/32767.0f;}
  uint64_t last_frame()const{return frame_;}
  Snapshot snapshot(uint32_t slot=0)const{require(slot<slots,Fault::bounds,"input slot");return snapshots_[slot];}
  void update(uint64_t frame,std::span<const RawController> raw){
    require(frame>frame_,Fault::stale,"input nonmonotonic frame");require(raw.size()<=slots,Fault::bounds,"input controller count");
    std::array<const RawController*,slots> admitted{};
    for(const auto& r:raw){require(r.slot<slots&&!admitted[r.slot],Fault::argument,"input duplicate/invalid slot");require(!(r.buttons&~valid_buttons),Fault::argument,"input unknown button");require(!r.connected||r.identity,Fault::argument,"input connected identity");
      for(auto value:{r.left_x,r.left_y,r.right_x,r.right_y}){
        require(value>=-32768&&value<=32767,Fault::bounds,"input native stick range");
      }
      admitted[r.slot]=&r;
    }
    auto next=snapshots_;
    for(uint32_t i=0;i<slots;++i){const auto& old=snapshots_[i];auto& s=next[i];const auto* r=admitted[i];const bool connected=r&&r->connected;
      s.frame=frame;s.connected=connected;s.pressed=0;s.released=0;s.held=0;s.left_x=s.left_y=s.right_x=s.right_y=0;s.left_trigger=s.right_trigger=0;
      if(!connected){s.released=old.held;continue;}
      const bool new_connection=!old.connected||old.identity!=r->identity;
      if(new_connection){require(old.epoch!=UINT64_MAX,Fault::exhausted,"input connection epoch overflow");s.epoch=old.epoch+1;}
      s.identity=r->identity;s.held=r->buttons;
      s.pressed=new_connection?s.held:(s.held&~old.held);
      s.released=new_connection?old.held:(old.held&~s.held);
      s.left_x=axis(r->left_x);s.left_y=axis(r->left_y);s.right_x=axis(r->right_x);s.right_y=axis(r->right_y);
      s.left_trigger=(s.held&zl)?65535:0;s.right_trigger=(s.held&zr)?65535:0;
    }
    snapshots_=next;frame_=frame;
  }
  // 96-byte BE wire snapshot, aligned16: magic/version/size/slot u32; frame,
  // identity,epoch u64; connected/reserved u32; held/pressed/released u64;
  // lx,ly,rx,ry signed32 Q15; left/right digital trigger u32. No host layout ABI.
  void write_snapshot(Space& space,Space::Handle owner,uint32_t address,uint32_t slot=0)const{
    const auto s=snapshot(slot);require(!(address&15),Fault::argument,"input snapshot alignment");
    std::array<std::byte,wire_size> wire{};
    auto word=[&](size_t offset,uint32_t value){for(size_t i=0;i<4;++i)wire[offset+i]=std::byte(value>>(24-8*i));};
    auto wide=[&](size_t offset,uint64_t value){word(offset,uint32_t(value>>32));word(offset+4,uint32_t(value));};
    word(0,magic);word(4,1);word(8,wire_size);word(12,s.slot);wide(16,s.frame);wide(24,s.identity);wide(32,s.epoch);word(40,s.connected?1:0);
    wide(48,s.held);wide(56,s.pressed);wide(64,s.released);word(72,uint32_t(s.left_x));word(76,uint32_t(s.left_y));word(80,uint32_t(s.right_x));word(84,uint32_t(s.right_y));word(88,s.left_trigger);word(92,s.right_trigger);
    Space::Lease pin(space,owner);space.validate_owned_window(owner,address,wire_size,Access::write);space.write(space.view(address,wire_size,Access::write),wire);
  }
};
} // namespace brutal::horizon::platform
