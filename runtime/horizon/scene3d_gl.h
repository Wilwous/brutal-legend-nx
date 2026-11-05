// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "scene3d.h"
#if defined(__SWITCH__)
#include <switch.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/glad.h>
#include <cstddef>
#include <string>

namespace brutal::horizon::platform {
// Real Mesa/nouveau OpenGL 4.3 path, following the public Switch examples' EGL
// API selection and library layout. No CPU graphics fallback. One owner thread
// and one default native window: never run libnx console/framebuffer concurrently.
class Scene3DGl {
 struct GpuMesh {GLuint vao{},vertices{},indices{};uint32_t generation{};size_t bytes{};};
 struct GpuTexture {GLuint object{};uint32_t generation{};size_t bytes{};};
 inline static std::atomic<bool> claimed_{};
 EGLDisplay display_{EGL_NO_DISPLAY};EGLContext context_{EGL_NO_CONTEXT};EGLSurface surface_{EGL_NO_SURFACE};
 GLuint program_{};GLint matrix_location_{-1},texture_location_{-1};
 GLuint overlay_vao_{},overlay_vertices_{},overlay_texture_{};uint32_t overlay_width_{},overlay_height_{};uint64_t overlay_updates_{},overlay_draws_{};
 std::array<GpuMesh,Scene3D::max_resources> meshes_{};std::array<GpuTexture,Scene3D::max_resources> textures_{};
 uint32_t width_{},height_{};uint64_t owner_{},renders_{},draws_{},uploads_{};bool claimed_here_{},egl_initialized_{},gl_ready_{},ready_{};unsigned cleanup_errors_{};
 std::string driver_,last_error_;size_t mesh_bytes_{},texture_bytes_{};
 [[noreturn]] void failure(const char* message){last_error_=message;throw Error(Fault::unsupported,message);}
 void require(bool ok,const char* message){if(!ok)failure(message);}
 void checked(const char* message){require(glGetError()==GL_NO_ERROR,message);}
 void active(){require(ready_,"3D GL renderer not initialized");require(eglGetCurrentContext()==context_,"3D GL context ownership");}
 static void destroy(GpuMesh& m)noexcept{if(m.indices)glDeleteBuffers(1,&m.indices);if(m.vertices)glDeleteBuffers(1,&m.vertices);if(m.vao)glDeleteVertexArrays(1,&m.vao);m={};}
 static void destroy(GpuTexture& t)noexcept{if(t.object)glDeleteTextures(1,&t.object);t={};}
 GLuint shader(GLenum type,const char* text){
  GLuint object=glCreateShader(type);require(object!=0,"3D GL shader allocation");glShaderSource(object,1,&text,nullptr);glCompileShader(object);GLint compiled=0;glGetShaderiv(object,GL_COMPILE_STATUS,&compiled);
  if(!compiled){char info[512]{};glGetShaderInfoLog(object,sizeof(info),nullptr,info);glDeleteShader(object);last_error_=std::string("3D shader compile: ")+info;throw Error(Fault::unsupported,"3D GL shader compile failed");}return object;
 }
 void create_program(){
  // Original shaders; column-major MVP and linear RGBA8 sampling are the complete
  // supported material model. Scene geometry supplies its own UV coordinates.
  static constexpr char vertex[]=R"glsl(#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec2 uv;
uniform mat4 mvp;
out vec2 texcoord;
void main(){gl_Position=mvp*vec4(position,1.0);texcoord=uv;}
)glsl";
  static constexpr char fragment[]=R"glsl(#version 330 core
in vec2 texcoord;
uniform sampler2D material;
out vec4 output_color;
void main(){output_color=texture(material,texcoord);}
)glsl";
  GLuint vs=0,fs=0;
  try{vs=shader(GL_VERTEX_SHADER,vertex);fs=shader(GL_FRAGMENT_SHADER,fragment);program_=glCreateProgram();require(program_!=0,"3D GL program allocation");glAttachShader(program_,vs);glAttachShader(program_,fs);glLinkProgram(program_);GLint linked=0;glGetProgramiv(program_,GL_LINK_STATUS,&linked);
   if(!linked){char info[512]{};glGetProgramInfoLog(program_,sizeof(info),nullptr,info);last_error_=std::string("3D shader link: ")+info;throw Error(Fault::unsupported,"3D GL program link failed");}
   glDetachShader(program_,vs);glDetachShader(program_,fs);glDeleteShader(vs);glDeleteShader(fs);vs=fs=0;
   matrix_location_=glGetUniformLocation(program_,"mvp");texture_location_=glGetUniformLocation(program_,"material");require(matrix_location_>=0&&texture_location_>=0,"3D GL required uniforms");checked("3D GL shader setup error");
  }catch(...){if(vs)glDeleteShader(vs);if(fs)glDeleteShader(fs);throw;}
 }
 GpuMesh& mesh(const Scene3D::Mesh& source){
  require(source.handle.owner==owner_&&source.handle.slot<meshes_.size(),"3D GL mesh ownership");auto& m=meshes_[source.handle.slot];if(m.vao&&m.generation==source.handle.generation)return m;
  const size_t bytes=source.vertices.size()*sizeof(Scene3D::Vertex)+source.indices.size()*sizeof(uint16_t);require(bytes&&mesh_bytes_-m.bytes+bytes<=Scene3D::mesh_budget,"3D GL mesh memory budget");mesh_bytes_-=m.bytes;destroy(m);
  glGenVertexArrays(1,&m.vao);glGenBuffers(1,&m.vertices);glGenBuffers(1,&m.indices);require(m.vao&&m.vertices&&m.indices,"3D GL mesh objects");glBindVertexArray(m.vao);
  glBindBuffer(GL_ARRAY_BUFFER,m.vertices);glBufferData(GL_ARRAY_BUFFER,GLsizeiptr(source.vertices.size()*sizeof(Scene3D::Vertex)),source.vertices.data(),GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.indices);glBufferData(GL_ELEMENT_ARRAY_BUFFER,GLsizeiptr(source.indices.size()*sizeof(uint16_t)),source.indices.data(),GL_STATIC_DRAW);
  static_assert(sizeof(Scene3D::Vertex)==20);glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Scene3D::Vertex),reinterpret_cast<const void*>(offsetof(Scene3D::Vertex,x)));glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Scene3D::Vertex),reinterpret_cast<const void*>(offsetof(Scene3D::Vertex,u)));
  checked("3D GL vertex/index upload error");m.generation=source.handle.generation;m.bytes=bytes;mesh_bytes_+=bytes;++uploads_;return m;
 }
 GpuTexture& texture(const Scene3D::Texture& source){
  require(source.handle.owner==owner_&&source.handle.slot<textures_.size(),"3D GL texture ownership");auto& t=textures_[source.handle.slot];if(t.object&&t.generation==source.handle.generation)return t;
  const size_t bytes=source.rgba.size();require(bytes&&texture_bytes_-t.bytes+bytes<=Scene3D::texture_budget,"3D GL texture memory budget");texture_bytes_-=t.bytes;destroy(t);
  glGenTextures(1,&t.object);require(t.object!=0,"3D GL texture allocation");glBindTexture(GL_TEXTURE_2D,t.object);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,GLsizei(source.width),GLsizei(source.height),0,GL_RGBA,GL_UNSIGNED_BYTE,source.rgba.data());checked("3D GL RGBA8 upload error");t.generation=source.handle.generation;t.bytes=bytes;texture_bytes_+=bytes;++uploads_;return t;
 }
 static std::array<float,4> color(uint32_t rgba){return {float((rgba>>24)&255)/255,float((rgba>>16)&255)/255,float((rgba>>8)&255)/255,float(rgba&255)/255};}
 public:
 Scene3DGl()=default;Scene3DGl(const Scene3DGl&)=delete;Scene3DGl&operator=(const Scene3DGl&)=delete;
 ~Scene3DGl(){shutdown();}
 void initialize(uint32_t width=1280,uint32_t height=720){
  require(!ready_&&!claimed_here_&&width&&height&&width<=1920&&height<=1080,"3D GL initialization dimensions/state");bool expected=false;require(claimed_.compare_exchange_strong(expected,true),"3D GL native window already owned");claimed_here_=true;width_=width;height_=height;
  try{
   require(R_SUCCEEDED(nwindowSetDimensions(nwindowGetDefault(),width,height)),"3D GL native window dimensions");display_=eglGetDisplay(EGL_DEFAULT_DISPLAY);require(display_!=EGL_NO_DISPLAY,"3D EGL display");require(eglInitialize(display_,nullptr,nullptr)==EGL_TRUE,"3D EGL initialize");egl_initialized_=true;require(eglBindAPI(EGL_OPENGL_API)==EGL_TRUE,"3D EGL OpenGL API");
   constexpr EGLint attributes[]{EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_BIT,EGL_RED_SIZE,8,EGL_GREEN_SIZE,8,EGL_BLUE_SIZE,8,EGL_ALPHA_SIZE,8,EGL_DEPTH_SIZE,24,EGL_STENCIL_SIZE,8,EGL_NONE};EGLConfig config{};EGLint count=0;require(eglChooseConfig(display_,attributes,&config,1,&count)==EGL_TRUE&&count==1,"3D EGL depth framebuffer configuration");
   surface_=eglCreateWindowSurface(display_,config,nwindowGetDefault(),nullptr);require(surface_!=EGL_NO_SURFACE,"3D EGL window surface");constexpr EGLint context_attributes[]{EGL_CONTEXT_OPENGL_PROFILE_MASK_KHR,EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT_KHR,EGL_CONTEXT_MAJOR_VERSION_KHR,4,EGL_CONTEXT_MINOR_VERSION_KHR,3,EGL_NONE};context_=eglCreateContext(display_,config,EGL_NO_CONTEXT,context_attributes);require(context_!=EGL_NO_CONTEXT,"3D EGL core43 context");require(eglMakeCurrent(display_,surface_,surface_,context_)==EGL_TRUE,"3D EGL make current");
   require(gladLoadGL()!=0,"3D GL function loader");gl_ready_=true;require(GLAD_GL_VERSION_4_3,"3D OpenGL43 capability");const auto* name=glGetString(GL_RENDERER);driver_=name?reinterpret_cast<const char*>(name):"unknown";
   GLint depth=0;glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER,GL_DEPTH,GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE,&depth);require(depth>=24,"3D GL depth buffer unavailable");create_program();require(eglSwapInterval(display_,1)==EGL_TRUE,"3D EGL swap interval");checked("3D GL initialization error");ready_=true;
  }catch(...){shutdown();throw;}
 }
 bool ready()const{return ready_;}uint32_t width()const{return width_;}uint32_t height()const{return height_;}
 uint64_t renders()const{return renders_;}uint64_t draws()const{return draws_;}uint64_t uploads()const{return uploads_;}
 size_t mesh_bytes()const{return mesh_bytes_;}size_t texture_bytes()const{return texture_bytes_;}
 unsigned cleanup_errors()const{return cleanup_errors_;}
 size_t overlay_bytes()const{return size_t(overlay_width_)*overlay_height_*4;}
 uint64_t overlay_updates()const{return overlay_updates_;}uint64_t overlay_draws()const{return overlay_draws_;}
 const std::string& driver()const{return driver_;}const std::string& last_error()const{return last_error_;}
 // Rendering can repeat an immutable accepted frame between simulation ticks.
 // No swap here: the application may add its original diagnostic overlay first.
 void clear_frame(uint32_t rgba){active();auto c=color(rgba);glViewport(0,0,GLsizei(width_),GLsizei(height_));glDisable(GL_SCISSOR_TEST);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glDepthMask(GL_TRUE);glClearColor(c[0],c[1],c[2],c[3]);glClearDepth(1.0);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);checked("3D GL frame clear error");}
 void render(const Scene3D::RenderFrame& frame){
  active();require(frame.scene_owner&&(!owner_||owner_==frame.scene_owner)&&!frame.draws.empty()&&frame.draws.size()<=Scene3D::max_draws,"3D GL frame ownership/count");owner_=frame.scene_owner;clear_frame(frame.clear_rgba);glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);glUseProgram(program_);glActiveTexture(GL_TEXTURE0);glUniform1i(texture_location_,0);
  for(const auto& draw:frame.draws){require(draw.mesh&&draw.texture&&draw.index_count&&uint64_t(draw.first_index)+draw.index_count<=draw.mesh->indices.size(),"3D GL accepted draw contract");if(!draw.visible)continue;scene_math::validate(draw.mvp);auto& geometry=mesh(*draw.mesh);auto& material=texture(*draw.texture);glBindVertexArray(geometry.vao);glBindTexture(GL_TEXTURE_2D,material.object);glUniformMatrix4fv(matrix_location_,1,GL_FALSE,draw.mvp.data());glDrawElements(GL_TRIANGLES,GLsizei(draw.index_count),GL_UNSIGNED_SHORT,reinterpret_cast<const void*>(size_t(draw.first_index)*sizeof(uint16_t)));++draws_;}
  checked("3D GL indexed draw error");++renders_;
 }
 // Small diagnostic rectangles in top-left framebuffer coordinates. Bounds
 // checked; clears color only, preserving scene depth. Intended for bit glyphs.
 void clear_rect(uint32_t x,uint32_t y,uint32_t width,uint32_t height,uint32_t rgba){
  active();require(width&&height&&uint64_t(x)+width<=width_&&uint64_t(y)+height<=height_,"3D GL overlay bounds");auto c=color(rgba);glEnable(GL_SCISSOR_TEST);glScissor(GLint(x),GLint(height_-y-height),GLsizei(width),GLsizei(height));glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);glClearColor(c[0],c[1],c[2],c[3]);glClear(GL_COLOR_BUFFER_BIT);glDisable(GL_SCISSOR_TEST);
 }
 // Host-authored diagnostics only, literal top-down RGBA8, max307200 bytes.
 // Upload only when the bitmap changes. A dedicated GPU texture and six-vertex
 // quad render the bitmap at 2x scale in the top-left corner, in one draw call.
 // No host bitmap pointer is retained; no guest capability enters this API.
 void set_overlay(std::span<const std::byte> rgba,uint32_t width,uint32_t height){
  active();require(width&&height&&width<=640&&height<=120&&uint64_t(width)*2<=width_&&uint64_t(height)*2<=height_&&rgba.size()==size_t(width)*height*4,"3D GL overlay RGBA extent");
  if(!overlay_vao_)glGenVertexArrays(1,&overlay_vao_);
  if(!overlay_vertices_)glGenBuffers(1,&overlay_vertices_);
  if(!overlay_texture_)glGenTextures(1,&overlay_texture_);
  require(overlay_vao_&&overlay_vertices_&&overlay_texture_,"3D GL overlay objects");
  const float right=-1+float(width)*4/float(width_),bottom=1-float(height)*4/float(height_);
  const std::array<Scene3D::Vertex,6> vertices{{{-1,1,0,0,0},{-1,bottom,0,0,1},{right,bottom,0,1,1},{-1,1,0,0,0},{right,bottom,0,1,1},{right,1,0,1,0}}};
  glBindVertexArray(overlay_vao_);glBindBuffer(GL_ARRAY_BUFFER,overlay_vertices_);glBufferData(GL_ARRAY_BUFFER,sizeof(vertices),vertices.data(),GL_DYNAMIC_DRAW);
  glEnableVertexAttribArray(0);glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Scene3D::Vertex),reinterpret_cast<const void*>(offsetof(Scene3D::Vertex,x)));glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(Scene3D::Vertex),reinterpret_cast<const void*>(offsetof(Scene3D::Vertex,u)));
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,overlay_texture_);glPixelStorei(GL_UNPACK_ALIGNMENT,1);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
  if(width!=overlay_width_||height!=overlay_height_)glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,GLsizei(width),GLsizei(height),0,GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
  else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,GLsizei(width),GLsizei(height),GL_RGBA,GL_UNSIGNED_BYTE,rgba.data());
  checked("3D GL overlay upload error");overlay_width_=width;overlay_height_=height;++overlay_updates_;
 }
 void draw_overlay(){
  active();require(overlay_width_&&overlay_height_,"3D GL overlay absent");glViewport(0,0,GLsizei(width_),GLsizei(height_));glDisable(GL_SCISSOR_TEST);glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glUseProgram(program_);glActiveTexture(GL_TEXTURE0);glUniform1i(texture_location_,0);auto mvp=scene_math::identity();glUniformMatrix4fv(matrix_location_,1,GL_FALSE,mvp.data());glBindTexture(GL_TEXTURE_2D,overlay_texture_);glBindVertexArray(overlay_vao_);glDrawArrays(GL_TRIANGLES,0,6);checked("3D GL overlay draw error");++overlay_draws_;
 }
 void present(){active();checked("3D GL overlay error");require(eglSwapBuffers(display_,surface_)==EGL_TRUE,"3D EGL present failed");}
 void shutdown()noexcept{
  if(display_!=EGL_NO_DISPLAY&&egl_initialized_){
   bool current=context_!=EGL_NO_CONTEXT&&eglMakeCurrent(display_,surface_,surface_,context_)==EGL_TRUE;
   if(current&&gl_ready_){glFinish();for(auto& m:meshes_)destroy(m);for(auto& t:textures_)destroy(t);if(overlay_texture_)glDeleteTextures(1,&overlay_texture_);if(overlay_vertices_)glDeleteBuffers(1,&overlay_vertices_);if(overlay_vao_)glDeleteVertexArrays(1,&overlay_vao_);if(program_)glDeleteProgram(program_);if(glGetError()!=GL_NO_ERROR)++cleanup_errors_;}
   else if(gl_ready_)++cleanup_errors_;
   if(eglMakeCurrent(display_,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT)!=EGL_TRUE)++cleanup_errors_;
   if(context_!=EGL_NO_CONTEXT&&eglDestroyContext(display_,context_)!=EGL_TRUE)++cleanup_errors_;
   if(surface_!=EGL_NO_SURFACE&&eglDestroySurface(display_,surface_)!=EGL_TRUE)++cleanup_errors_;
   if(eglTerminate(display_)!=EGL_TRUE)++cleanup_errors_;
  }
  display_=EGL_NO_DISPLAY;context_=EGL_NO_CONTEXT;surface_=EGL_NO_SURFACE;program_=0;meshes_={};textures_={};mesh_bytes_=texture_bytes_=0;owner_=0;ready_=gl_ready_=egl_initialized_=false;
  overlay_texture_=overlay_vertices_=overlay_vao_=0;overlay_width_=overlay_height_=0;
  if(claimed_here_){claimed_=false;claimed_here_=false;}
 }
};
} // namespace brutal::horizon::platform
#endif // __SWITCH__
