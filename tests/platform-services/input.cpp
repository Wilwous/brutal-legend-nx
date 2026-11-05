// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/platform_input.h"
#include <iostream>
using namespace brutal::horizon;using brutal::horizon::platform::Input;
namespace {unsigned positive{},refusals{};void check(bool ok){if(!ok)throw std::runtime_error("input semantic assertion");++positive;}template<class F>void no(F f,Fault fault){try{f();}catch(const Error& e){if(e.fault!=fault)throw std::runtime_error("input refusal category");++refusals;return;}throw std::runtime_error("missing input refusal");}}
int main(){try{
 Input input;std::array<Input::RawController,1> raw{{{0,42,true,Input::a|Input::zl,32767,-32768,4096,-4096}}};
 input.update(1,raw);auto first=input.snapshot();check(first.connected&&first.identity==42&&first.epoch==1&&first.frame==1&&first.held==(Input::a|Input::zl)&&first.pressed==first.held&&!first.released);
 check(first.left_x==32767&&first.left_y==-32767&&first.right_x==0&&first.right_y==0&&first.left_trigger==65535&&!first.right_trigger&&Input::normalized(first.left_y)==-1.0f);
 input.update(2,raw);check(input.snapshot().pressed==0&&input.snapshot().released==0&&input.snapshot().epoch==1);
 raw[0].buttons=Input::b|Input::zr;raw[0].right_x=18431;input.update(3,raw);auto third=input.snapshot();check(third.pressed==(Input::b|Input::zr)&&third.released==(Input::a|Input::zl)&&third.right_trigger==65535&&!third.left_trigger&&third.right_x==16382);
 input.update(4,{});auto disconnected=input.snapshot();check(!disconnected.connected&&disconnected.identity==42&&disconnected.epoch==1&&disconnected.released==(Input::b|Input::zr)&&!disconnected.held&&!disconnected.left_x&&!disconnected.right_trigger&&disconnected.frame==4);
 input.update(5,{});check(!input.snapshot().released);
 input.update(6,raw);check(input.snapshot().epoch==2&&input.snapshot().identity==42&&input.snapshot().pressed==(Input::b|Input::zr)&&!input.snapshot().released);
 raw[0].identity=43;raw[0].buttons=Input::a;input.update(7,raw);check(input.snapshot().epoch==3&&input.snapshot().identity==43&&input.snapshot().pressed==Input::a&&input.snapshot().released==(Input::b|Input::zr));
 std::array<Input::RawController,2> two{{raw[0],{7,99,true,Input::x,0,0,0,0}}};input.update(8,two);check(input.snapshot(7).identity==99&&input.snapshot(7).pressed==Input::x&&input.snapshot(7).epoch==1&&input.snapshot(0).frame==8);
 no([&]{input.update(8,raw);},Fault::stale);no([&]{input.snapshot(8);},Fault::bounds);no([&]{Input bad(32767);},Fault::argument);no([&]{Input::normalized(-32768);},Fault::bounds);
 auto bad=raw;bad[0].slot=8;no([&]{input.update(9,bad);},Fault::argument);bad=raw;bad[0].identity=0;no([&]{input.update(9,bad);},Fault::argument);bad=raw;bad[0].left_x=32768;no([&]{input.update(9,bad);},Fault::bounds);bad=raw;bad[0].buttons=1ull<<63;no([&]{input.update(9,bad);},Fault::argument);
 two[1]=two[0];no([&]{input.update(9,two);},Fault::argument);check(input.last_frame()==8&&input.snapshot().identity==43&&input.snapshot(7).connected);
 Space s(65536),other(65536);auto output=s.allocate(112),other_output=other.allocate(96);input.write_snapshot(s,output,output.address);
 check(s.load<uint32_t>(output.address)==Input::magic&&s.load<uint32_t>(output.address+4)==1&&s.load<uint32_t>(output.address+8)==96&&s.load<uint64_t>(output.address+16)==8&&s.load<uint64_t>(output.address+24)==43&&s.load<uint64_t>(output.address+32)==3&&s.load<uint32_t>(output.address+40)==1&&s.load<uint64_t>(output.address+48)==Input::a&&s.load<uint32_t>(output.address+76)==uint32_t(-32767));
 no([&]{input.write_snapshot(s,other_output,output.address);},Fault::ownership);auto forged=output;--forged.size;no([&]{input.write_snapshot(s,forged,output.address);},Fault::ownership);
 no([&]{input.write_snapshot(s,output,output.address+32);},Fault::bounds);no([&]{input.write_snapshot(s,output,output.address+1);},Fault::argument);
 auto untouched=s.read(output.address,Input::wire_size);s.protect(output,Permission::read);no([&]{input.write_snapshot(s,output,output.address);},Fault::protection);check(s.read(output.address,Input::wire_size)==untouched);
 s.protect(output,Permission::write);input.write_snapshot(s,output,output.address);check(s.leases()==0);s.protect(output,Permission::read_write);s.decommit(output);no([&]{input.write_snapshot(s,output,output.address);},Fault::uncommitted);s.release(output);no([&]{input.write_snapshot(s,output,output.address);},Fault::stale);other.release(other_output);
 check(s.live()==0&&s.leases()==0&&Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
