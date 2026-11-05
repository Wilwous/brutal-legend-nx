// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
// Original T85 glyphs/button mapping reused with a CPU HUD for the GL adapter.
#include <switch.h>
#include "runtime/horizon/platform_input.h"
#include <span>
namespace brutal::platform_v2::ui {
namespace p=brutal::horizon::platform;
const uint8_t* glyph(char c){
 static constexpr uint8_t letters[][7]={{14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},{30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},{14,17,16,23,17,17,14},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},{7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},{17,27,21,21,17,17,17},{17,25,25,21,19,19,17},{14,17,17,17,17,17,14},{30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},{15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},{17,17,17,17,17,10,4},{17,17,17,21,21,27,17},{17,17,10,4,10,17,17},{17,17,10,4,4,4,4},{31,1,2,4,8,16,31}};
 static constexpr uint8_t digits[][7]={{14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},{30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},{14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},{14,17,17,15,1,1,14}};
 static constexpr uint8_t blank[7]{},dash[7]{0,0,0,31,0,0,0},colon[7]{0,4,4,0,4,4,0};
 if(c>='A'&&c<='Z')return letters[c-'A'];
 if(c>='0'&&c<='9')return digits[c-'0'];
 return c=='-'?dash:c==':'?colon:blank;
}
uint64_t buttons(uint64_t hid){uint64_t result=0;const std::pair<uint64_t,uint64_t> map[]={{HidNpadButton_A,p::Input::a},{HidNpadButton_B,p::Input::b},{HidNpadButton_X,p::Input::x},{HidNpadButton_Y,p::Input::y},{HidNpadButton_Up,p::Input::up},{HidNpadButton_Down,p::Input::down},{HidNpadButton_Left,p::Input::left},{HidNpadButton_Right,p::Input::right},{HidNpadButton_L,p::Input::l},{HidNpadButton_R,p::Input::r},{HidNpadButton_ZL,p::Input::zl},{HidNpadButton_ZR,p::Input::zr},{HidNpadButton_Plus,p::Input::plus},{HidNpadButton_Minus,p::Input::minus},{HidNpadButton_StickL,p::Input::stick_l},{HidNpadButton_StickR,p::Input::stick_r}};for(auto [native,portable]:map)if(hid&native)result|=portable;return result;}
inline void text(std::span<std::byte> image,uint32_t width,uint32_t height,uint32_t x,uint32_t y,const char*message){
 for(;*message;++message,x+=6){if(x+5>width||y+7>height)return;auto rows=glyph(*message);for(unsigned r=0;r<7;++r)for(unsigned c=0;c<5;++c)if(rows[r]&(1u<<(4-c))){auto i=(size_t(y+r)*width+x+c)*4;image[i]=std::byte{230};image[i+1]=std::byte{240};image[i+2]=std::byte{255};image[i+3]=std::byte{255};}}
}
}
