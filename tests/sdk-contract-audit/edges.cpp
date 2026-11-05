// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/format_translation.h"
#include <iostream>
using namespace brutal::horizon;namespace c=compatibility;
int main(){try{unsigned positive=0,refusals=0,bytes=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("format edge assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing format edge refusal");};Space s;auto input=s.allocate(8192);
 // Bit-pattern/expected integer pairs are an independent fixed oracle, including
 // the asymmetric truncation immediately below negative full scale.
 constexpr std::array<uint32_t,12> bits{0,0x80000000,1,0x007fffff,0x3f800000,0xbf800000,0x3f000000,0xbf000000,0x3f7fffff,0xbf7fffff,0x38000000,0xb8000000};
 constexpr std::array<int16_t,12> expected{0,0,0,0,32767,-32768,16384,-16384,32767,-32767,1,-1};
 for(unsigned i=0;i<256;++i)for(unsigned ch=0;ch<2;++ch)s.store<uint32_t>(input.address+(ch*256+i)*4,bits[(i+ch)%bits.size()]);auto pcm=c::front_pcm_s16(s,{input,input.address,6144});
 for(unsigned i=0;i<512;++i){uint16_t value=uint16_t(expected[(i/2+i%2)%expected.size()]);if(pcm[i*2]!=std::byte(value&255)||pcm[i*2+1]!=std::byte(value>>8))throw std::runtime_error("PCM edge bytes");bytes+=2;}yes(pcm.size()==1024);
 for(uint32_t invalid:{0x7f800000u,0xff800000u,0x7fc00000u,0x7f800001u,0x3f800001u,0xbf800001u}){s.store<uint32_t>(input.address,invalid);no([&]{c::front_pcm_s16(s,{input,input.address,6144});});}
 s.store<uint32_t>(input.address,0);s.store<uint32_t>(input.address+2*256*4,0x80000000);yes(c::front_pcm_s16(s,{input,input.address,6144}).size()==1024);s.store<uint32_t>(input.address+5*256*4,1);no([&]{c::front_pcm_s16(s,{input,input.address,6144});});
 // Exact float bits, including signed zero and finite denormals, survive vertex
 // conversion; no unsupported quantization or implicit normalization is claimed.
 for(unsigned i=0;i<15;++i){auto v=bits[i%bits.size()];std::array<std::byte,4> wire{};for(unsigned j=0;j<4;++j)wire[j]=std::byte(v>>(8*j));s.write(input.address+4*i,wire);}auto vertices=c::vertices_be(s,{input,input.address,60},3,c::Endian::none);
 for(unsigned i=0;i<15;++i)for(unsigned j=0;j<4;++j){if(vertices[4*i+j]!=std::byte(bits[i%bits.size()]>>(24-8*j)))throw std::runtime_error("vertex bit oracle");++bytes;}yes(vertices.size()==60);
 auto snapshot=s.view(input.address,6144,Access::read);s.decommit(input);no([&]{c::front_pcm_s16(s,{input,input.address,6144});});s.commit(input);no([&]{s.read(snapshot);});s.release(input);no([&]{c::vertices_be(s,{input,input.address,60},3,c::Endian::none);});yes(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<bytes<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
