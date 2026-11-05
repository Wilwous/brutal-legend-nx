// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <span>
#include <vector>

namespace brutal::horizon::platform {
namespace scene_math {
using Mat4=std::array<float,16>;
using Vec3=std::array<float,3>;
inline void require(bool value,const char* message){if(!value)throw Error(Fault::argument,message);}
inline void finite(float value){require(std::isfinite(value)&&std::abs(value)<=1000000.0f,"3D finite bounded scalar");}
inline Mat4 identity(){return {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};}
inline void validate(const Mat4& m){for(float value:m)finite(value);}
inline Mat4 multiply(const Mat4& a,const Mat4& b){
 validate(a);validate(b);Mat4 result{};
 for(size_t column=0;column<4;++column)for(size_t row=0;row<4;++row){double sum=0;for(size_t k=0;k<4;++k)sum+=double(a[k*4+row])*b[column*4+k];require(std::isfinite(sum)&&std::abs(sum)<=1000000,"3D transform product bounds");result[column*4+row]=float(sum);}
 return result;
}
inline Mat4 translation(float x,float y,float z){finite(x);finite(y);finite(z);auto m=identity();m[12]=x;m[13]=y;m[14]=z;return m;}
inline Mat4 scale(float x,float y,float z){finite(x);finite(y);finite(z);auto m=identity();m[0]=x;m[5]=y;m[10]=z;return m;}
inline Mat4 rotation_y(float radians){finite(radians);auto m=identity();float c=std::cos(radians),s=std::sin(radians);m[0]=c;m[2]=-s;m[8]=s;m[10]=c;return m;}
inline Mat4 rotation_x(float radians){finite(radians);auto m=identity();float c=std::cos(radians),s=std::sin(radians);m[5]=c;m[6]=s;m[9]=-s;m[10]=c;return m;}
inline Mat4 perspective(float fovy,float aspect,float near_plane,float far_plane){
 for(auto n:{fovy,aspect,near_plane,far_plane})finite(n);
 require(fovy>0.01f&&fovy<3.13f&&aspect>0.001f&&near_plane>0&&far_plane>near_plane,"3D perspective parameters");
 float f=1/std::tan(fovy/2);Mat4 m{};m[0]=f/aspect;m[5]=f;m[10]=(far_plane+near_plane)/(near_plane-far_plane);m[11]=-1;m[14]=(2*far_plane*near_plane)/(near_plane-far_plane);validate(m);return m;
}
inline Vec3 normalized(Vec3 v){double squared=0;for(auto n:v){finite(n);squared+=double(n)*n;}require(squared>1e-16&&std::isfinite(squared),"3D degenerate direction");float inverse=float(1/std::sqrt(squared));for(auto& n:v)n*=inverse;return v;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
inline float dot(Vec3 a,Vec3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline Mat4 look_at(Vec3 eye,Vec3 target,Vec3 up){
 for(auto n:eye)finite(n);
 for(auto n:target)finite(n);
 auto f=normalized(Vec3{target[0]-eye[0],target[1]-eye[1],target[2]-eye[2]});auto s=normalized(cross(f,normalized(up)));auto u=cross(s,f);
 Mat4 m{s[0],u[0],-f[0],0,s[1],u[1],-f[1],0,s[2],u[2],-f[2],0,-dot(s,eye),-dot(u,eye),dot(f,eye),1};validate(m);return m;
}
inline std::array<float,4> clip(const Mat4& m,Vec3 position){
 validate(m);for(float n:position)finite(n);std::array<float,4> result{};
 for(size_t row=0;row<4;++row){double sum=m[12+row];for(size_t k=0;k<3;++k)sum+=double(m[k*4+row])*position[k];require(std::isfinite(sum)&&std::abs(sum)<=1e12,"3D clip-coordinate bounds");result[row]=float(sum);}return result;
}
} // namespace scene_math

// Original synthetic interface, no Xbox GPU command or SDK compatibility claim.
// Calls are serialized by the application. All incoming guest bytes are copied
// under exact Space capabilities; no guest address or lease survives admission.
class Scene3D {
 public:
 using Mat4=scene_math::Mat4;
 static constexpr uint32_t record_size=32,max_commands=512,max_draws=64,max_resources=8;
 static constexpr uint32_t max_vertices=4096,max_indices=12288,max_texture_bytes=4*1024*1024;
 static constexpr size_t mesh_budget=2*1024*1024,texture_budget=8*1024*1024;
 enum class Op:uint32_t {begin=1,bind_mesh=2,camera=3,model=4,bind_texture=5,draw=6,end=7};
 struct GuestSpan {Space::Handle owner;uint32_t address{},size{};};
 struct MeshHandle {uint64_t owner{};uint32_t slot{},generation{};};
 struct TextureHandle {uint64_t owner{};uint32_t slot{},generation{};};
 struct Vertex {float x,y,z,u,v;};
 struct Mesh {MeshHandle handle;std::vector<Vertex> vertices;std::vector<uint16_t> indices;scene_math::Vec3 minimum{},maximum{};};
 struct Texture {TextureHandle handle;uint32_t width{},height{};std::vector<std::byte> rgba;};
 struct Draw {std::shared_ptr<const Mesh> mesh;std::shared_ptr<const Texture> texture;Mat4 model{},mvp{};uint32_t first_index{},index_count{};bool visible{};};
 struct RenderFrame {uint64_t scene_owner{},number{};uint32_t clear_rgba{};Mat4 view{},projection{};std::vector<Draw> draws;uint32_t visible_draws{};};
 private:
 template<class T>struct Slot {uint32_t generation{1};std::shared_ptr<const T> data;};
 inline static std::atomic<uint64_t> serial_{1};const uint64_t owner_{serial_.fetch_add(1)};
 std::array<Slot<Mesh>,max_resources> meshes_{};std::array<Slot<Texture>,max_resources> textures_{};
 std::shared_ptr<const RenderFrame> frame_;size_t mesh_bytes_{},texture_bytes_{};bool closed_{};
 static void require(bool ok,Fault fault,const char* message){if(!ok)throw Error(fault,message);}
 void live()const{require(!closed_,Fault::stale,"3D scene closed");}
 static uint32_t word(std::span<const std::byte> bytes,size_t at){uint32_t value=0;for(size_t i=0;i<4;++i)value=(value<<8)|std::to_integer<uint32_t>(bytes[at+i]);return value;}
 static float scalar(std::span<const std::byte> bytes,size_t at){static_assert(sizeof(float)==4&&std::numeric_limits<float>::is_iec559);float value=std::bit_cast<float>(word(bytes,at));scene_math::finite(value);return value;}
 static std::vector<std::byte> copy(Space& space,GuestSpan source,uint32_t alignment){
  require(source.size&&!(source.address&(alignment-1)),Fault::argument,"3D guest span alignment/size");Space::Lease pin(space,source.owner);space.validate_owned_window(source.owner,source.address,source.size,Access::read);return space.read(space.view(source.address,source.size,Access::read));
 }
 template<class T,class H>const std::shared_ptr<const T>& owned(const std::array<Slot<T>,max_resources>& slots,H handle)const{
  live();require(handle.owner==owner_,Fault::ownership,"3D resource owner");require(handle.slot<max_resources,Fault::bounds,"3D resource slot");const auto& s=slots[handle.slot];require(s.data&&s.generation==handle.generation,Fault::stale,"3D resource generation");return s.data;
 }
 template<class T>static uint32_t empty(const std::array<Slot<T>,max_resources>& slots){for(uint32_t i=0;i<max_resources;++i)if(!slots[i].data&&slots[i].generation!=UINT32_MAX)return i;throw Error(Fault::exhausted,"3D bounded resource slots exhausted");}
 static size_t cost(const Mesh& mesh){return mesh.vertices.size()*sizeof(Vertex)+mesh.indices.size()*sizeof(uint16_t);}
 static bool visible(const Mesh& mesh,const Mat4& mvp){
  uint32_t common=63;for(unsigned corner=0;corner<8;++corner){auto c=scene_math::clip(mvp,{(corner&1)?mesh.maximum[0]:mesh.minimum[0],(corner&2)?mesh.maximum[1]:mesh.minimum[1],(corner&4)?mesh.maximum[2]:mesh.minimum[2]});uint32_t code=0;for(unsigned i=0;i<3;++i){if(c[i]<-c[3])code|=1u<<(2*i);if(c[i]>c[3])code|=1u<<(2*i+1);}common&=code;}return !common;
 }
 public:
 Scene3D()=default;Scene3D(const Scene3D&)=delete;Scene3D&operator=(const Scene3D&)=delete;
 ~Scene3D(){shutdown();}
 uint64_t owner()const{return owner_;}size_t mesh_bytes()const{return mesh_bytes_;}size_t texture_bytes()const{return texture_bytes_;}
 size_t meshes()const{size_t count=0;for(const auto& s:meshes_)count+=bool(s.data);return count;}
 size_t textures()const{size_t count=0;for(const auto& s:textures_)count+=bool(s.data);return count;}
 uint64_t last_frame()const{return frame_?frame_->number:0;}
 std::shared_ptr<const RenderFrame> snapshot()const{live();require(bool(frame_),Fault::stale,"3D frame absent");return frame_;}
 // Explicit vertex format1: x,y,z,u,v BE IEEE754 float32, stride20. Index
 // format1: BEu16 triangle list. Texture format1: linear literal RGBA8 bytes.
 MeshHandle upload_mesh(Space& space,GuestSpan vertices,uint32_t vertex_count,GuestSpan indices,uint32_t index_count,uint32_t vertex_format=1,uint32_t index_format=1){
  live();require(vertex_format==1&&index_format==1,Fault::unsupported,"3D mesh format");require(vertex_count>=3&&vertex_count<=max_vertices&&index_count&&index_count<=max_indices&&!(index_count%3),Fault::bounds,"3D mesh count");require(vertices.size==uint64_t(vertex_count)*20&&indices.size==uint64_t(index_count)*2,Fault::bounds,"3D mesh byte extent");
  size_t bytes=size_t(vertex_count)*sizeof(Vertex)+size_t(index_count)*2;require(mesh_bytes_+bytes<=mesh_budget,Fault::exhausted,"3D mesh byte budget");uint32_t slot=empty(meshes_);
  try{auto v=copy(space,vertices,4);auto ix=copy(space,indices,2);auto mesh=std::make_shared<Mesh>();mesh->vertices.reserve(vertex_count);mesh->indices.reserve(index_count);
   for(uint32_t i=0;i<vertex_count;++i){size_t at=size_t(i)*20;Vertex vertex{scalar(v,at),scalar(v,at+4),scalar(v,at+8),scalar(v,at+12),scalar(v,at+16)};require(std::abs(vertex.x)<=10000&&std::abs(vertex.y)<=10000&&std::abs(vertex.z)<=10000&&std::abs(vertex.u)<=1024&&std::abs(vertex.v)<=1024,Fault::bounds,"3D vertex/UV bounds");mesh->vertices.push_back(vertex);
    std::array<float,3> p{vertex.x,vertex.y,vertex.z};for(size_t axis=0;axis<3;++axis){if(!i)mesh->minimum[axis]=mesh->maximum[axis]=p[axis];else{mesh->minimum[axis]=std::min(mesh->minimum[axis],p[axis]);mesh->maximum[axis]=std::max(mesh->maximum[axis],p[axis]);}}
   }
   for(uint32_t i=0;i<index_count;++i){uint16_t index=uint16_t(std::to_integer<uint16_t>(ix[i*2])<<8|std::to_integer<uint16_t>(ix[i*2+1]));require(index<vertex_count,Fault::bounds,"3D vertex index");mesh->indices.push_back(index);}
   MeshHandle handle{owner_,slot,meshes_[slot].generation};mesh->handle=handle;meshes_[slot].data=std::move(mesh);mesh_bytes_+=bytes;return handle;
  }catch(const std::bad_alloc&){throw Error(Fault::exhausted,"3D mesh allocation");}
 }
 TextureHandle upload_texture(Space& space,GuestSpan pixels,uint32_t width,uint32_t height,uint32_t pitch,uint32_t format=1){
  live();require(format==1,Fault::unsupported,"3D texture format");require(width&&height&&width<=1024&&height<=1024&&pitch>=uint64_t(width)*4&&!(pitch&3),Fault::bounds,"3D RGBA8 dimensions/pitch");uint64_t source_size=uint64_t(pitch)*height;size_t packed=size_t(width)*height*4;require(source_size<=max_texture_bytes&&pixels.size==source_size,Fault::bounds,"3D texture source extent");require(texture_bytes_+packed<=texture_budget,Fault::exhausted,"3D texture byte budget");uint32_t slot=empty(textures_);
  try{auto source=copy(space,pixels,4);auto texture=std::make_shared<Texture>();texture->width=width;texture->height=height;texture->rgba.resize(packed);for(uint32_t row=0;row<height;++row)std::copy_n(source.begin()+size_t(row)*pitch,size_t(width)*4,texture->rgba.begin()+size_t(row)*width*4);TextureHandle handle{owner_,slot,textures_[slot].generation};texture->handle=handle;textures_[slot].data=std::move(texture);texture_bytes_+=packed;return handle;}catch(const std::bad_alloc&){throw Error(Fault::exhausted,"3D texture allocation");}
 }
 void release_mesh(MeshHandle handle){auto& data=owned(meshes_,handle);require(data.use_count()==1,Fault::ownership,"3D mesh retained by a frame");mesh_bytes_-=cost(*data);auto& slot=meshes_[handle.slot];slot.data.reset();++slot.generation;}
 void release_texture(TextureHandle handle){auto& data=owned(textures_,handle);require(data.use_count()==1,Fault::ownership,"3D texture retained by a frame");texture_bytes_-=data->rgba.size();auto& slot=textures_[handle.slot];slot.data.reset();++slot.generation;}
 // Eight BEu32 words, command address aligned32. Unused words MUST be zero:
 // BEGIN frame_hi,frame_lo,clearRGBA; CAMERA view_span,projection_span;
 // BIND_MESH/BIND_TEXTURE owner_hi,owner_lo,slot,generation;
 // MODEL matrix_span; DRAW first_index,index_count; END.
 // CAMERA occurs exactly once directly after BEGIN. Each DRAW requires fresh
 // MODEL and explicit geometry/texture binding. Matrices are 16BEfloat32 in
 // column-major OpenGL convention. Whole-frame refusal preserves prior state.
 void submit(Space& space,GuestSpan commands,std::span<const GuestSpan> matrix_sources){
  live();require(commands.size>=7*record_size&&!(commands.size%record_size)&&commands.size<=max_commands*record_size,Fault::argument,"3D command count");require(matrix_sources.size()<=max_draws+2,Fault::bounds,"3D matrix descriptor count");
  try{auto bytes=copy(space,commands,record_size);const size_t count=bytes.size()/record_size;auto frame=std::make_shared<RenderFrame>();frame->draws.reserve(std::min<size_t>(max_draws,count));frame->scene_owner=owner_;bool camera=false,model_ready=false;std::shared_ptr<const Mesh> mesh;std::shared_ptr<const Texture> texture;Mat4 model{};
   auto matrix=[&](uint32_t index){require(index<matrix_sources.size(),Fault::bounds,"3D matrix descriptor");auto span=matrix_sources[index];require(span.size==64,Fault::bounds,"3D matrix extent");auto data=copy(space,span,16);Mat4 m{};for(size_t j=0;j<16;++j)m[j]=scalar(data,j*4);return m;};
   for(size_t i=0;i<count;++i){std::array<uint32_t,8> c{};for(size_t j=0;j<8;++j)c[j]=word(bytes,i*record_size+j*4);auto zero=[&](size_t from){for(size_t j=from;j<8;++j)require(!c[j],Fault::argument,"3D reserved command word");};auto op=Op(c[0]);
    if(i==0){require(op==Op::begin,Fault::argument,"3D missing begin");frame->number=(uint64_t(c[1])<<32)|c[2];require(frame->number>last_frame(),Fault::stale,"3D frame sequence");frame->clear_rgba=c[3];zero(4);continue;}
    require(i+1<count||op==Op::end,Fault::argument,"3D missing end");
    if(op==Op::camera){require(i==1&&!camera,Fault::argument,"3D camera sequence");frame->view=matrix(c[1]);frame->projection=matrix(c[2]);zero(3);camera=true;}
    else if(op==Op::bind_mesh){require(camera,Fault::argument,"3D geometry before camera");mesh=owned(meshes_,MeshHandle{(uint64_t(c[1])<<32)|c[2],c[3],c[4]});zero(5);}
    else if(op==Op::bind_texture){require(camera,Fault::argument,"3D texture before camera");texture=owned(textures_,TextureHandle{(uint64_t(c[1])<<32)|c[2],c[3],c[4]});zero(5);}
    else if(op==Op::model){require(camera,Fault::argument,"3D transform before camera");model=matrix(c[1]);model_ready=true;zero(2);}
    else if(op==Op::draw){require(camera&&mesh&&texture&&model_ready,Fault::argument,"3D incomplete draw bindings");require(frame->draws.size()<max_draws,Fault::bounds,"3D draw limit");require(c[2]&&!(c[1]%3)&&!(c[2]%3)&&uint64_t(c[1])+c[2]<=mesh->indices.size(),Fault::bounds,"3D draw index range");zero(3);auto mvp=scene_math::multiply(frame->projection,scene_math::multiply(frame->view,model));bool shown=visible(*mesh,mvp);frame->draws.push_back(Draw{mesh,texture,model,mvp,c[1],c[2],shown});frame->visible_draws+=shown;model_ready=false;}
    else if(op==Op::end){require(i+1==count&&camera&&!frame->draws.empty()&&!model_ready,Fault::argument,"3D end sequence");zero(1);}
    else throw Error(Fault::unsupported,"3D unsupported command/repeated begin");
   }
   frame_=std::move(frame);
  }catch(const std::bad_alloc&){throw Error(Fault::exhausted,"3D frame staging allocation");}
 }
 void shutdown()noexcept{if(closed_)return;frame_.reset();for(auto& s:meshes_)s.data.reset();for(auto& s:textures_)s.data.reset();mesh_bytes_=texture_bytes_=0;closed_=true;}
};
} // namespace brutal::horizon::platform
