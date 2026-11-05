// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/platform_graphics.h"
#include <iostream>
#include <initializer_list>
using namespace brutal::horizon;using brutal::horizon::platform::Graphics;
namespace {
unsigned positive{},refusals{};
void check(bool v){if(!v)throw std::runtime_error("graphics semantic assertion");++positive;}
template<class F>void no(F f,Fault fault){try{f();}catch(const Error& e){if(e.fault!=fault)throw std::runtime_error("graphics refusal category");++refusals;return;}throw std::runtime_error("missing graphics refusal");}
using C=std::array<uint32_t,8>;
std::vector<std::byte> encode(std::initializer_list<C> commands){std::vector<std::byte> out;for(auto c:commands)for(auto word:c)for(unsigned i=0;i<4;++i)out.push_back(std::byte(word>>(24-8*i)));return out;}
uint32_t pixel(const Graphics& g,unsigned x,unsigned y){uint32_t out=0;for(auto b:g.pixels().subspan((y*g.width()+x)*4,4))out=(out<<8)|std::to_integer<unsigned>(b);return out;}
}
int main(){try{
 Space s(1024*1024),foreign(65536);auto cmd=s.allocate(4096),source=s.allocate(32),foreign_cmd=foreign.allocate(4096);Graphics g(4,3);
 auto submit=[&](std::vector<std::byte> bytes,std::span<const Graphics::Upload> uploads={}){s.write(cmd.address,bytes);g.submit(s,cmd,cmd.address,uint32_t(bytes.size()),uploads);};
 auto simple=[&](uint32_t frame){return encode({C{1,0,frame,4,3,1},C{2,0x11223344},C{7}});};
 submit(encode({C{1,0,1,4,3,1},C{2,0x11223344},C{3,0xff000080},C{4,1,1,2,1},C{7}}));
 check(g.width()==4&&g.height()==3&&g.pitch()==16&&g.last_frame()==1);
 check(pixel(g,0,0)==0x11223344&&pixel(g,1,1)==0xff000080&&pixel(g,2,1)==0xff000080&&pixel(g,3,1)==0x11223344);
 // Padded two-row texture: exact RGBA bytes, nearest-neighbor enlargement.
 std::array<std::byte,24> tex{std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{9},std::byte{9},std::byte{9},std::byte{9},std::byte{0},std::byte{0},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255}};
 s.write(source.address,tex);std::array<Graphics::Upload,1> uploads{{{source,source.address,24}}};
 submit(encode({C{1,0,2,4,3,1},C{2,0},C{5,0,2,2,12,0},C{6,0,0,0,4,2},C{7}}),uploads);
 check(pixel(g,0,0)==0xff0000ff&&pixel(g,1,0)==0xff0000ff&&pixel(g,2,0)==0x00ff00ff&&pixel(g,0,1)==0x0000ffff&&pixel(g,3,1)==0xffffffff&&pixel(g,0,2)==0);
 s.release(source);
 submit(encode({C{1,0,3,4,3,1},C{2,0},C{6,0,0,0,2,2},C{7}}));
 check(pixel(g,0,0)==0xff0000ff&&pixel(g,1,1)==0xffffffff); // Copied lifetime.
 const auto before=std::vector<std::byte>(g.pixels().begin(),g.pixels().end());
 no([&]{submit(simple(3));},Fault::stale);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{4,3,0,2,1},C{7}}));},Fault::bounds);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{4,UINT32_MAX,0,2,1},C{7}}));},Fault::bounds);
 check(g.last_frame()==3&&std::equal(before.begin(),before.end(),g.pixels().begin()));
 no([&]{submit(encode({C{1,0,4,4,3,2},C{2,0},C{7}}));},Fault::argument);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{3,0},C{7}}));},Fault::argument);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{7},C{7}}));},Fault::argument);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{99},C{7}}));},Fault::unsupported);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{6,7,0,0,1,1},C{7}}));},Fault::argument);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{4,0,0,0,1},C{7}}));},Fault::bounds);
 no([&]{submit(encode({C{1,0,4,4,3,1},C{2,0},C{3,0,1},C{7}}));},Fault::argument);
 auto bytes=simple(4);s.write(cmd.address,bytes);
 no([&]{g.submit(s,cmd,cmd.address+1,uint32_t(bytes.size()));},Fault::argument);
 no([&]{g.submit(s,cmd,cmd.address,95);},Fault::argument);
 no([&]{g.submit(s,cmd,cmd.address,Graphics::record_size*(Graphics::max_commands+1));},Fault::argument);
 no([&]{g.submit(s,foreign_cmd,cmd.address,uint32_t(bytes.size()));},Fault::ownership);
 auto forged=cmd;--forged.size;no([&]{g.submit(s,forged,cmd.address,uint32_t(bytes.size()));},Fault::ownership);
 no([&]{g.submit(s,cmd,cmd.address+4064,uint32_t(bytes.size()));},Fault::bounds);
 s.protect(cmd,Permission::write);no([&]{g.submit(s,cmd,cmd.address,uint32_t(bytes.size()));},Fault::protection);s.protect(cmd,Permission::read);g.submit(s,cmd,cmd.address,uint32_t(bytes.size()));check(g.last_frame()==4);s.protect(cmd,Permission::read_write);
 auto live=s.allocate(16);std::array<std::byte,16> red{};red[0]=std::byte{255};s.write(live.address,red);std::array<Graphics::Upload,1> up{{{live,live.address,16}}};
 auto uploadframe=encode({C{1,0,5,4,3,1},C{2,0},C{5,1,1,1,16,0},C{6,1,0,0,1,1},C{7}});
 s.protect(live,Permission::none);no([&]{submit(uploadframe,up);},Fault::protection);s.protect(live,Permission::read);
 up[0].owner=foreign_cmd;no([&]{submit(uploadframe,up);},Fault::ownership);up[0].owner=live;
 up[0].size=15;no([&]{submit(uploadframe,up);},Fault::bounds);up[0].size=16;
 no([&]{submit(encode({C{1,0,5,4,3,1},C{2,0},C{5,0,2,1,4,0},C{7}}),up);},Fault::argument);
 no([&]{submit(encode({C{1,0,5,4,3,1},C{2,0},C{5,0,1,1,Graphics::max_texture_bytes+4,0},C{7}}),up);},Fault::bounds);
 submit(uploadframe,up);check(pixel(g,0,0)==0xff000000&&g.last_frame()==5);s.release(live);
 no([&]{submit(encode({C{1,0,6,4,3,1},C{2,0},C{5,1,1,1,16,0},C{7}}),up);},Fault::stale);
 // Sequential upload/blit/replace/blit must use each captured version.
 auto a=s.allocate(4),b=s.allocate(4);s.store<uint32_t>(a.address,0xaabbccdd);s.store<uint32_t>(b.address,0x55667788);std::array<Graphics::Upload,2> both{{{a,a.address,4},{b,b.address,4}}};
 submit(encode({C{1,0,6,4,3,1},C{2,0},C{5,1,1,1,4,0},C{6,1,0,0,1,1},C{5,1,1,1,4,1},C{6,1,1,0,1,1},C{7}}),both);
 check(pixel(g,0,0)==0xaabbccdd&&pixel(g,1,0)==0x55667788);
 no([&]{submit(encode({C{1,0,7,4,3,1},C{2,0},C{5,1,1,1,4,0},C{4,4,0,1,1},C{7}}),both);},Fault::bounds);
 submit(encode({C{1,0,7,4,3,1},C{2,0},C{6,1,0,0,1,1},C{7}}));
 check(pixel(g,0,0)==0x55667788&&g.last_frame()==7); // Failed upload did not replace the resident texture.
 s.release(a);s.release(b);s.release(cmd);no([&]{g.submit(s,cmd,cmd.address,96);},Fault::stale);foreign.release(foreign_cmd);
 no([&]{Graphics invalid(1281,720);},Fault::bounds);check(s.live()==0&&s.leases()==0&&foreign.live()==0&&Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
