// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <bit>
#include <cmath>
#include <limits>
namespace brutal::horizon::compatibility {
// Four independently specified Xenos buffer-endian modes from the pinned SDK.
// No texture tiling, packed vertex fetch, shaders or GPU synchronization implied.
enum class Endian:uint32_t {none=0,swap8in16=1,swap8in32=2,swap16in32=3};
struct Span {Space::Handle owner;uint32_t address,size;};
inline uint32_t swap_word(uint32_t v,Endian mode){
 switch(mode){
  case Endian::none:return v;
  case Endian::swap8in16:return ((v&0x00ff00ff)<<8)|((v&0xff00ff00)>>8);
  case Endian::swap8in32:return ((v&255)<<24)|((v&0xff00)<<8)|((v>>8)&0xff00)|(v>>24);
  case Endian::swap16in32:return (v<<16)|(v>>16);
 }
 throw Error(Fault::unsupported,"unknown Xenos endian mode");
}
inline std::vector<std::byte> capture(Space&s,Span span,uint32_t alignment){
 if(!alignment||(alignment&(alignment-1))||!span.size||span.size>1024*1024||(span.address&(alignment-1)))throw Error(Fault::bounds,"bounded format source alignment/extent");
 Space::Lease pin(s,span.owner);s.validate_owned_window(span.owner,span.address,span.size,Access::read);return s.read(s.view(span.address,span.size,Access::read));
}
inline uint32_t little(std::span<const std::byte>b,size_t offset,uint32_t width){if((width!=2&&width!=4)||offset>b.size()||width>b.size()-offset)throw Error(Fault::bounds,"little-endian scalar extent");uint32_t value=0;for(uint32_t i=0;i<width;++i)value|=std::to_integer<uint32_t>(b[offset+i])<<(8*i);return value;}
inline void be(std::span<std::byte>b,size_t offset,uint32_t value,uint32_t width){if((width!=2&&width!=4)||offset>b.size()||width>b.size()-offset)throw Error(Fault::bounds,"big-endian scalar extent");for(uint32_t i=0;i<width;++i)b[offset+i]=std::byte(value>>(8*(width-1-i)));}
// Converts explicit uncompressed float5/stride20 geometry into the existing
// Scene3D BEfloat format. Format identity is supplied, never guessed from bytes.
inline std::vector<std::byte> vertices_be(Space&s,Span source,uint32_t vertices,Endian mode){
 if(!vertices||vertices>4096||source.size!=uint64_t(vertices)*20)throw Error(Fault::bounds,"float5 vertex extent");
 (void)swap_word(0,mode);auto data=capture(s,source,4);std::vector<std::byte> output(data.size());
 for(size_t i=0;i<data.size();i+=4){uint32_t bits=swap_word(little(data,i,4),mode);if(!std::isfinite(std::bit_cast<float>(bits)))throw Error(Fault::argument,"nonfinite vertex scalar");be(output,i,bits,4);}return output;
}
// The renderer currently consumes u16 triangles. Wider source indices are
// narrowed only when proven in range; no truncation or packed-format fallback.
inline std::vector<std::byte> indices_be16(Space&s,Span source,uint32_t count,uint32_t width,uint32_t vertex_count,Endian mode){
 if(!count||count>12288||count%3||(width!=2&&width!=4)||!vertex_count||vertex_count>4096||source.size!=uint64_t(count)*width)throw Error(Fault::bounds,"index conversion extent");
 (void)swap_word(0,mode);if(width==2&&mode!=Endian::none&&mode!=Endian::swap8in16)throw Error(Fault::unsupported,"u16 endian mode unsupported");
 auto data=capture(s,source,width);std::vector<std::byte> output(size_t(count)*2);
 for(uint32_t i=0;i<count;++i){uint32_t value=little(data,size_t(i)*width,width);if(width==4)value=swap_word(value,mode);else if(mode==Endian::swap8in16)value=((value&255)<<8)|(value>>8);
  if(value>=vertex_count||value>65535)throw Error(Fault::bounds,"index cannot address admitted vertex set");
  be(output,size_t(i)*2,value,2);
 }return output;
}
// Pinned SDL backend input representation: six BEfloat32 planes,256 samples,
// 48kHz. This native s16 adapter admits front-only frames; nonzero other channels
// refuse rather than inventing surround fold/gain/callback/XMA semantics.
inline std::vector<std::byte> front_pcm_s16(Space&s,Span source,uint32_t rate=48000){
 static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);
 if(rate!=48000||source.size!=6*256*4)throw Error(Fault::unsupported,"fixed Xbox driver PCM frame subset only");
 auto data=capture(s,source,4);std::array<float,512> fronts{};
 for(uint32_t channel=0;channel<6;++channel)for(uint32_t sample=0;sample<256;++sample){auto at=size_t(channel*256+sample)*4;uint32_t bits=0;for(unsigned n=0;n<4;++n)bits=(bits<<8)|std::to_integer<uint32_t>(data[at+n]);float value=std::bit_cast<float>(bits);
  if(!std::isfinite(value)||value< -1||value>1)throw Error(Fault::argument,"PCM frame finite unit range");
  if(channel>=2){if(value!=0)throw Error(Fault::unsupported,"surround contribution requires explicit downmix contract");}
  else fronts[sample*2+channel]=value;
 }
 std::vector<std::byte> output(1024);
 for(size_t i=0;i<fronts.size();++i){auto value=int32_t(double(fronts[i])*32768);value=std::clamp(value,-32768,32767);uint16_t wire=uint16_t(value);output[i*2]=std::byte(wire&255);output[i*2+1]=std::byte(wire>>8);}return output;
}
}
