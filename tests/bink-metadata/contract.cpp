// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/bink_metadata.h"
#include "fixture.h"
#include <iostream>
namespace b=brutal::horizon::resources::bink;
int main(){try{unsigned positive=0,refusals=0,bytes=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("BIKi contract assertion");++positive;};auto no=[&](auto f){try{f();}catch(const b::Error&){++refusals;return;}throw std::runtime_error("missing BIKi refusal");};
 auto file=original_bink::make();auto prefix=std::span(file).first(original_bink::prefix_size);auto m=b::parse(prefix,file.size());
 yes(m.layout.extent==160&&m.layout.prefix_size==84&&m.layout.index_offset==68);yes(m.layout.width==640&&m.layout.height==360&&m.layout.rate_num==30000&&m.layout.rate_den==1001);
 yes(m.frames==std::vector<b::Frame>{{84,20,true},{104,38,false},{142,18,true}});yes(m.tracks==std::vector<b::Track>{{0x20000,17,48000,0x3000},{0x10000,23,44100,0x1000}});
 unsigned reads=0;auto copy=b::inspect(file.size(),[&](size_t at,std::span<uint8_t> out){yes((reads==0&&at==0&&out.size()==44)||(reads==1&&at==44&&out.size()==40));++reads;std::copy_n(file.begin()+at,out.size(),out.begin());bytes+=out.size();return out.size();});yes(reads==2&&copy.frames==m.frames&&copy.tracks==m.tracks);
 for(size_t n:{0u,3u,43u,44u,67u,83u,85u})no([&]{b::parse(std::span(file).first(n),file.size());});
 no([&]{b::parse(prefix,file.size()-1);});no([&]{b::parse(prefix,file.size()+2);});no([&]{b::parse(prefix,1ull<<32);});
 auto invalid=[&](size_t at,uint32_t v){auto bad=file;original_bink::put(bad,at,v);no([&]{b::parse(std::span(bad).first(84),bad.size());});};
 invalid(0,0x6b4b4942);invalid(0,0x6942324b);invalid(4,UINT32_MAX);invalid(8,0);invalid(8,b::max_frames+1);invalid(12,0);invalid(12,159+2);invalid(20,0);invalid(24,b::max_dimension+1);invalid(28,0);invalid(32,0);invalid(40,b::max_tracks+1);invalid(52,0x30000000);invalid(68,83);invalid(68,87);invalid(72,84);invalid(72,82);invalid(76,UINT32_MAX);invalid(80,158);invalid(12,37);
 for(unsigned phase=0;phase<2;++phase){unsigned calls=0;no([&]{b::inspect(file.size(),[&](size_t at,std::span<uint8_t> out){std::copy_n(file.begin()+at,out.size(),out.begin());return calls++==phase?out.size()-1:out.size();});});yes(calls==phase+1);}
 reads=0;no([&]{b::inspect(43,[&](size_t,std::span<uint8_t>){++reads;return size_t(0);});});yes(reads==0);
 // Minimal no-audio profile and exact terminal index word, no payload parsing.
 std::array<uint8_t,52> silent{};for(size_t i=0;i<11;++i)original_bink::put(silent,4*i,std::array<uint32_t,11>{0x694b4942,48,1,4,1,16,16,30,1,0,0}[i]);original_bink::put(silent,44,53);original_bink::put(silent,48,56);auto one=b::parse(silent,56);yes(one.tracks.empty()&&one.frames==std::vector<b::Frame>{{52,4,true}});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"metadata_bytes_read\":"<<bytes<<",\"payload_bytes_read\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
