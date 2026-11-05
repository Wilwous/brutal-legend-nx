// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/space.h"
#include <iostream>
using namespace brutal::horizon;
int main(){try{unsigned positive=0,refusals=0;auto yes=[&](bool v){if(!v)throw std::runtime_error("word transaction assertion");++positive;};auto no=[&](auto fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing word refusal");};
 Space s(65536),foreign;auto a=s.allocate(256),b=s.allocate(16),alien=foreign.allocate(16);s.store<uint32_t>(a.address,0x11111111);s.store<uint32_t>(b.address,0x22222222);
 std::array<Space::WordWrite,2> writes{{{a,a.address,0x01020304},{b,b.address,0xaabbccdd}}};s.protect(b,Permission::read);no([&]{s.write_words(writes);});yes(s.load<uint32_t>(a.address)==0x11111111&&s.load<uint32_t>(b.address)==0x22222222);s.protect(b,Permission::read_write);s.write_words(writes);yes(s.load<uint32_t>(a.address)==0x01020304&&s.load<uint32_t>(b.address)==0xaabbccdd);
 auto saved=s.read(a.address,a.size);writes[1]={alien,a.address+4,0};no([&]{s.write_words(writes);});yes(s.read(a.address,a.size)==saved);writes[1]={a,a.address,0};no([&]{s.write_words(writes);});writes[1]={a,a.address+1,0};no([&]{s.write_words(writes);});writes[1]={a,a.address+256,0};no([&]{s.write_words(writes);});writes[1]={a,0xfffffffc,0};no([&]{s.write_words(writes);});auto forged=a;forged.size++;writes[1]={forged,a.address+4,0};no([&]{s.write_words(writes);});yes(s.read(a.address,a.size)==saved);
 std::array<Space::WordWrite,65> maximum{};for(unsigned i=0;i<64;++i)maximum[i]={a,a.address+i*4,0x10203040+i};s.write_words(std::span(maximum).first(64));for(unsigned i=0;i<64;++i)yes(s.load<uint32_t>(a.address+i*4)==0x10203040+i);no([&]{s.write_words(maximum);});no([&]{s.write_words({});});
 s.release(b);writes[1]={b,b.address,0};no([&]{s.write_words(writes);});s.release(a);foreign.release(alien);yes(Space::totals()==std::pair<size_t,size_t>{0,0});std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
