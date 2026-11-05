// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/scene3d.h"
#include <iostream>
#include <initializer_list>
#include <limits>
using namespace brutal::horizon;using brutal::horizon::platform::Scene3D;namespace math=brutal::horizon::platform::scene_math;
namespace {
unsigned semantic{},refusals{},byte_comparisons{};
void check(bool ok){if(!ok)throw std::runtime_error("3D graphics semantic assertion");++semantic;}
void close(float actual,float expected){check(std::abs(actual-expected)<0.0001f);}
template<class F>void no(F fn,Fault fault){try{fn();}catch(const Error& error){if(error.fault!=fault)throw std::runtime_error("3D refusal category");++refusals;return;}throw std::runtime_error("missing 3D refusal");}
using C=std::array<uint32_t,8>;
std::vector<std::byte> encode(std::span<const C> commands){std::vector<std::byte> bytes;for(auto command:commands)for(auto word:command)for(unsigned b=0;b<4;++b)bytes.push_back(std::byte(word>>(24-8*b)));return bytes;}
void matrix(Space& s,uint32_t address,const math::Mat4& m){for(uint32_t i=0;i<16;++i)s.store<uint32_t>(address+4*i,std::bit_cast<uint32_t>(m[i]));}
C bind(uint32_t op,uint64_t owner,uint32_t slot,uint32_t generation){return {op,uint32_t(owner>>32),uint32_t(owner),slot,generation};}
}
int main(){try{
 Space s(1024*1024),foreign(65536);auto data=s.allocate(8192),commands=s.allocate(16384),foreign_commands=foreign.allocate(4096);
 auto span=[&](uint32_t offset,uint32_t size){return Scene3D::GuestSpan{data,data.address+offset,size};};
 const std::array<float,15> vertices{-1,-1,0,0,0,1,-1,0,1,0,0,1,0,0.5f,1};
 for(uint32_t i=0;i<vertices.size();++i)s.store<uint32_t>(data.address+4*i,std::bit_cast<uint32_t>(vertices[i]));
 std::array<std::byte,6> indices{std::byte{0},std::byte{0},std::byte{0},std::byte{1},std::byte{0},std::byte{2}};s.write(data.address+128,indices);
 // Padded two-row texture; padding must not reach the GPU staging snapshot.
 std::array<std::byte,24> rgba{std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{9},std::byte{9},std::byte{9},std::byte{9},std::byte{0},std::byte{0},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255}};s.write(data.address+256,rgba);
 Scene3D scene,other;auto mesh=scene.upload_mesh(s,span(0,60),3,span(128,6),3);auto texture=scene.upload_texture(s,span(256,24),2,2,12);
 check(scene.meshes()==1&&scene.textures()==1&&scene.mesh_bytes()==66&&scene.texture_bytes()==16);
 no([&]{scene.snapshot();},Fault::stale);
 auto view=math::look_at({0,0,5},{0,0,0},{0,1,0}),projection=math::perspective(1.57079632679f,1,1,10);
 matrix(s,data.address+512,view);matrix(s,data.address+576,projection);matrix(s,data.address+640,math::translation(-1,0,0));matrix(s,data.address+704,math::translation(0,0,1));matrix(s,data.address+768,math::translation(1,0,-1));matrix(s,data.address+832,math::translation(100,0,0));
 std::array<Scene3D::GuestSpan,6> matrices{span(512,64),span(576,64),span(640,64),span(704,64),span(768,64),span(832,64)};
 const auto mb=bind(2,mesh.owner,mesh.slot,mesh.generation),tb=bind(5,texture.owner,texture.slot,texture.generation);
 std::vector<C> list{{1,0,1,0x102030ff},{3,0,1},mb,tb,{4,2},{6,0,3},{4,3},{6,0,3},{4,4},{6,0,3},{4,5},{6,0,3},{7}};
 auto submit=[&](const std::vector<C>& records){auto bytes=encode(records);s.write(commands.address,bytes);scene.submit(s,{commands,commands.address,uint32_t(bytes.size())},matrices);};
 submit(list);auto frame=scene.snapshot();check(frame->number==1&&frame->scene_owner==scene.owner()&&frame->clear_rgba==0x102030ff&&frame->draws.size()==4&&frame->visible_draws==3&&!frame->draws[3].visible);
 check(frame->draws[0].mesh->vertices[2].u==0.5f&&frame->draws[0].mesh->indices==std::vector<uint16_t>({0,1,2}));
 const std::array<std::byte,16> packed{std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{0},std::byte{255},std::byte{0},std::byte{0},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255}};
 for(size_t i=0;i<packed.size();++i){if(frame->draws[0].texture->rgba[i]!=packed[i])throw std::runtime_error("3D texture byte comparison");++byte_comparisons;}
 auto a=math::clip(frame->draws[0].mvp,{0,0,0}),b=math::clip(frame->draws[1].mvp,{0,0,0}),c=math::clip(frame->draws[2].mvp,{0,0,0});close(a[0]/a[3],-0.2f);close(a[2]/a[3],7.0f/9);check(b[2]/b[3]<a[2]/a[3]&&a[2]/a[3]<c[2]/c[3]);
 auto near=math::clip(projection,{0,0,-1}),far=math::clip(projection,{0,0,-10});close(near[2]/near[3],-1);close(far[2]/far[3],1);
 auto child=math::multiply(math::translation(2,0,0),math::multiply(math::rotation_y(1.57079632679f),math::translation(1,0,0)));auto p=math::clip(child,{0,0,0});close(p[0],2);close(p[2],-1);
 no([&]{scene.release_mesh(mesh);},Fault::ownership);no([&]{scene.release_texture(texture);},Fault::ownership);
 no([&]{submit(list);},Fault::stale);auto bad=list;bad[0][2]=2;bad[5][2]=6;no([&]{submit(bad);},Fault::bounds);check(scene.snapshot()==frame&&scene.last_frame()==1);
 bad=list;bad[0][2]=2;bad[2]=bind(2,other.owner(),mesh.slot,mesh.generation);no([&]{submit(bad);},Fault::ownership);
 bad=list;bad[0][2]=2;bad[2][4]++;no([&]{submit(bad);},Fault::stale);
 bad=list;bad[0][2]=2;bad[3][3]=Scene3D::max_resources;no([&]{submit(bad);},Fault::bounds);
 bad=list;bad[0][2]=2;bad[1]={4,2};no([&]{submit(bad);},Fault::argument);
 bad=list;bad[0][2]=2;bad[6]={6,0,3};no([&]{submit(bad);},Fault::argument);
 bad=list;bad[0][2]=2;bad[6]={7};no([&]{submit(bad);},Fault::argument);
 bad=list;bad[0][2]=2;bad[5][7]=1;no([&]{submit(bad);},Fault::argument);
 bad=list;bad[0][2]=2;bad[4][1]=99;no([&]{submit(bad);},Fault::bounds);
 bad=list;bad[0][2]=2;bad[4]={99};no([&]{submit(bad);},Fault::unsupported);
 bad=list;bad[0][2]=2;bad.back()={4,2};no([&]{submit(bad);},Fault::argument);
 auto bytes=encode(list);s.write(commands.address,bytes);
 no([&]{scene.submit(s,{foreign_commands,commands.address,uint32_t(bytes.size())},matrices);},Fault::ownership);
 auto forged=commands;--forged.size;no([&]{scene.submit(s,{forged,commands.address,uint32_t(bytes.size())},matrices);},Fault::ownership);
 no([&]{scene.submit(s,{commands,commands.address+1,uint32_t(bytes.size())},matrices);},Fault::argument);
 no([&]{scene.submit(s,{commands,commands.address+16352,uint32_t(bytes.size())},matrices);},Fault::bounds);
 no([&]{scene.submit(s,{commands,commands.address,63},matrices);},Fault::argument);
 s.protect(commands,Permission::write);no([&]{scene.submit(s,{commands,commands.address,uint32_t(bytes.size())},matrices);},Fault::protection);s.protect(commands,Permission::read_write);
 bad=list;bad[0][2]=2;s.store<uint32_t>(data.address+640,0x7fc00000);no([&]{submit(bad);},Fault::argument);matrix(s,data.address+640,math::translation(-1,0,0));
 matrices[2].size=60;no([&]{submit(bad);},Fault::bounds);matrices[2].size=64;
 matrices[2].owner=foreign_commands;no([&]{submit(bad);},Fault::ownership);matrices[2].owner=data;
 no([&]{scene.upload_mesh(s,span(0,60),3,span(128,6),3,2);},Fault::unsupported);
 no([&]{scene.upload_mesh(s,span(0,60),3,span(128,6),2);},Fault::bounds);
 s.store<uint16_t>(data.address+132,3);no([&]{scene.upload_mesh(s,span(0,60),3,span(128,6),3);},Fault::bounds);s.store<uint16_t>(data.address+132,2);
 s.store<uint32_t>(data.address,0x7f800000);no([&]{scene.upload_mesh(s,span(0,60),3,span(128,6),3);},Fault::argument);s.store<uint32_t>(data.address,std::bit_cast<uint32_t>(-1.0f));
 no([&]{scene.upload_texture(s,span(256,24),2,2,4);},Fault::bounds);no([&]{scene.upload_texture(s,span(256,24),2,2,12,2);},Fault::unsupported);
 s.protect(data,Permission::write);no([&]{scene.upload_texture(s,span(256,24),2,2,12);},Fault::protection);s.protect(data,Permission::read);
 auto other_mesh=other.upload_mesh(s,span(0,60),3,span(128,6),3);auto other_texture=other.upload_texture(s,span(256,24),2,2,12);other.release_mesh(other_mesh);no([&]{other.release_mesh(other_mesh);},Fault::stale);other.release_texture(other_texture);auto replaced=other.upload_mesh(s,span(0,60),3,span(128,6),3);check(replaced.slot==other_mesh.slot&&replaced.generation==other_mesh.generation+1);other.release_mesh(replaced);
 // All mesh/texture/matrix reads accept RO input and preserve copied lifetime.
 auto success=list;success[0][2]=2;submit(success);check(scene.snapshot()->number==2&&frame->number==1);
 s.release(data);check(frame->draws[0].texture->rgba==std::vector<std::byte>(packed.begin(),packed.end())&&frame->draws[0].mesh->vertices[0].x==-1);
 success[0][2]=3;no([&]{submit(success);},Fault::stale);check(scene.last_frame()==2&&scene.meshes()==1&&scene.textures()==1);
 s.release(commands);foreign.release(foreign_commands);scene.shutdown();other.shutdown();check(!scene.meshes()&&!scene.textures()&&!scene.mesh_bytes()&&!scene.texture_bytes());no([&]{scene.snapshot();},Fault::stale);no([&]{scene.release_mesh(mesh);},Fault::stale);
 no([&]{math::perspective(0,1,1,10);},Fault::argument);no([&]{math::perspective(1,1,10,1);},Fault::argument);no([&]{math::look_at({0,0,0},{0,0,0},{0,1,0});},Fault::argument);no([&]{math::rotation_y(std::numeric_limits<float>::infinity());},Fault::argument);
 {
  Space limited(6*1024*1024);auto source=limited.allocate(4*1024*1024);Scene3D budget;
  Scene3D::GuestSpan large{source,source.address,source.size};auto one=budget.upload_texture(limited,large,1024,1024,4096),two=budget.upload_texture(limited,large,1024,1024,4096);
  no([&]{budget.upload_texture(limited,large,1024,1024,4096);},Fault::exhausted);check(budget.texture_bytes()==Scene3D::texture_budget&&budget.textures()==2);budget.release_texture(one);budget.release_texture(two);
  std::array<Scene3D::TextureHandle,Scene3D::max_resources> tokens{};
  for(auto& token:tokens)token=budget.upload_texture(limited,{source,source.address,4},1,1,4);
  no([&]{budget.upload_texture(limited,{source,source.address,4},1,1,4);},Fault::exhausted);check(budget.textures()==8&&budget.texture_bytes()==32);
  for(auto token:tokens)budget.release_texture(token);
  check(budget.textures()==0&&budget.texture_bytes()==0);limited.release(source);
 }
 check(Space::totals()==std::pair<size_t,size_t>{0,0});frame.reset();
 std::cout<<"{\"semantic\":"<<semantic<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<byte_comparisons<<",\"draws\":4,\"visible_draws\":3,\"allocations\":0,\"leases\":0,\"gpu_execution\":false}\n";
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
