// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <array>
#include <cstdint>
#include <span>
namespace original_bink {
// Original metadata-only example. Opaque payload bytes are not valid codec data.
inline void put(std::span<uint8_t> bytes,size_t at,uint32_t v){for(unsigned i=0;i<4;++i)bytes[at+i]=uint8_t(v>>(8*i));}
inline constexpr size_t prefix_size=84,file_size=160;
inline std::array<uint8_t,file_size> make(){
 std::array<uint8_t,file_size> b{};const std::array<uint32_t,21> words{0x694b4942,152,3,40,3,640,360,30000,1001,0,2,0x20000,0x10000,0x3000bb80,0x1000ac44,17,23,85,104,143,160};
 for (size_t i = 0; i < words.size(); ++i) {
  put(b, 4 * i, words[i]);
 }
 return b;
}
}
