// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/checked_memory.h"
#include <array>
#include <span>
#include <vector>

// Standalone BIKi metadata only. No codec, media packet, guest mapping or title
// admission. File size is supplied by the actual source, never the header alone.
namespace brutal::horizon::resources::bink {
using foundation::Error;using foundation::Fault;
inline constexpr size_t header_size=44;
inline constexpr uint32_t max_frames=16384,max_tracks=32,max_dimension=4096;
inline constexpr size_t max_prefix=header_size+12*max_tracks+4*(max_frames+1);
struct Frame {uint32_t offset,length;bool key_flag;bool operator==(const Frame&)const=default;};
struct Track {uint32_t opaque,identifier;uint16_t sample_rate,flags;bool operator==(const Track&)const=default;};
struct Layout {
 uint32_t extent,frames,largest_frame,opaque_count,width,height,rate_num,rate_den,flags,tracks;
 size_t index_offset,prefix_size;
};
struct Metadata {Layout layout;std::vector<Frame> frames;std::vector<Track> tracks;};
namespace detail {
inline void require(bool ok,Fault fault,const char* why){if(!ok)throw Error(fault,why);}
// Internal calls are limited to checked, fixed header/table extents.
inline uint32_t le(std::span<const uint8_t> bytes,size_t at,unsigned width){
 require(width>0&&width<=4&&at<=bytes.size()&&width<=bytes.size()-at,Fault::bounds,"BIKi scalar extent");
 uint32_t v=0;for(unsigned i=0;i<width;++i)v|=uint32_t(bytes[at+i])<<(8*i);return v;
}
}
inline Layout header(std::span<const uint8_t> bytes,uint64_t source_extent){
 using detail::le;using detail::require;
 require(bytes.size()>=header_size,Fault::bounds,"BIKi truncated fixed header");
 require(le(bytes,0,4)==0x694b4942u,Fault::unsupported,"only standalone BIKi metadata supported");
 const uint64_t declared=uint64_t(le(bytes,4,4))+8;
 require(declared<=UINT32_MAX&&declared==source_extent&&(declared&1)==0,Fault::bounds,"BIKi declared/source extent mismatch");
 Layout l{uint32_t(declared),le(bytes,8,4),le(bytes,12,4),le(bytes,16,4),le(bytes,20,4),le(bytes,24,4),le(bytes,28,4),le(bytes,32,4),le(bytes,36,4),le(bytes,40,4),0,0};
 require(l.frames&&l.frames<=max_frames&&l.tracks<=max_tracks,Fault::bounds,"BIKi frame/track budget");
 require(l.width&&l.width<=max_dimension&&l.height&&l.height<=max_dimension&&l.rate_num&&l.rate_den,Fault::argument,"BIKi dimensions/rate");
 require(l.largest_frame&&l.largest_frame<=l.extent,Fault::bounds,"BIKi declared largest frame");
 l.index_offset=header_size+12*size_t(l.tracks);l.prefix_size=l.index_offset+4*(size_t(l.frames)+1);
 require(l.prefix_size<=max_prefix&&l.prefix_size<l.extent,Fault::bounds,"BIKi metadata/payload geometry");return l;
}
inline Metadata parse(std::span<const uint8_t> prefix,uint64_t source_extent){
 using detail::le;using detail::require;auto l=header(prefix,source_extent);
 require(prefix.size()==l.prefix_size,Fault::bounds,"BIKi requires exact metadata prefix");
 require(le(prefix,l.index_offset+4*size_t(l.frames),4)==l.extent,Fault::bounds,"BIKi terminal index extent");
 Metadata result{l,{},{}};result.frames.reserve(l.frames);result.tracks.reserve(l.tracks);
 for(size_t i=0;i<l.tracks;++i){
  const auto opaque=le(prefix,header_size+4*i,4),rate=le(prefix,header_size+4*(l.tracks+i),2),flags=le(prefix,header_size+4*(l.tracks+i)+2,2),id=le(prefix,header_size+4*(2*l.tracks+i),4);
  require(rate!=0,Fault::argument,"BIKi invalid audio sample rate");result.tracks.push_back({opaque,id,uint16_t(rate),uint16_t(flags)});
 }
 require((le(prefix,l.index_offset,4)&~1u)==l.prefix_size,Fault::bounds,"BIKi first frame must follow complete index");
 for(size_t i=0;i<l.frames;++i){const size_t at=l.index_offset+4*i;auto wire=le(prefix,at,4),start=wire&~1u,end=le(prefix,at+4,4)&~1u;
  require(start>=l.prefix_size&&end<=l.extent&&end>start&&end-start<=l.largest_frame,Fault::bounds,"BIKi invalid indexed frame range");result.frames.push_back({start,end-start,bool(wire&1u)});
 }
 return result;
}
// Exactly two bounded reads, both in the metadata prefix. The caller owns a
// stable source/extent; this helper does not vouch for mutable file identities.
// Short reads refuse rather than guessing missing bytes or retrying forever.
template<class ReadAt> Metadata inspect(uint64_t source_extent,ReadAt&& read_at){
 std::array<uint8_t,header_size> fixed{};
 detail::require(source_extent>=header_size&&read_at(0,std::span(fixed))==fixed.size(),Fault::bounds,"BIKi short fixed-header read");
 auto l=header(fixed,source_extent);std::vector<uint8_t> prefix(l.prefix_size);std::copy(fixed.begin(),fixed.end(),prefix.begin());auto tail=std::span(prefix).subspan(header_size);
 detail::require(read_at(header_size,tail)==tail.size(),Fault::bounds,"BIKi short metadata read");return parse(prefix,source_extent);
}
}
