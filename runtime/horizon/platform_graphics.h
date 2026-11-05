// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "space.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

namespace brutal::horizon::platform {
// Original CPU protocol: eight BE u32 words per 32-byte aligned record.
// RGBA8 is four literal R,G,B,A bytes; packed command colors are 0xRRGGBBAA.
// BEGIN,CLEAR,{COLOR,RECT,UPLOAD,BLIT}*,PRESENT is one complete submission.
// Unsigned rectangles must fit; blits use nearest-neighbor without blending.
class Graphics {
 public:
  enum class Op : uint32_t { begin=1,clear=2,color=3,rect=4,upload=5,blit=6,present=7 };
  static constexpr uint32_t rgba8=1,record_size=32,max_commands=2048,max_textures=8;
  static constexpr uint32_t max_texture_bytes=16*1024*1024;
  struct Upload {Space::Handle owner;uint32_t address{},size{};};
 private:
  struct Texture {uint32_t width{},height{},pitch{};std::vector<std::byte> bytes;};
  using TextureRef=std::shared_ptr<const Texture>;
  using Command=std::array<uint32_t,8>;
  uint32_t width_,height_;uint64_t frame_{};
  std::vector<std::byte> pixels_;std::array<TextureRef,max_textures> textures_{};
  static void require(bool ok,Fault fault,const char* message){if(!ok)throw Error(fault,message);}
  static void zeros(const Command& c,size_t first){for(size_t i=first;i<c.size();++i)require(!c[i],Fault::argument,"graphics reserved command word");}
  static std::vector<std::byte> copy(Space& s,Space::Handle h,uint32_t address,uint32_t size){
    Space::Lease pin(s,h);s.validate_owned_window(h,address,size,Access::read);
    return s.read(s.view(address,size,Access::read));
  }
  void rectangle(uint32_t x,uint32_t y,uint32_t w,uint32_t h)const{
    require(w&&h&&x<width_&&y<height_&&uint64_t(x)+w<=width_&&uint64_t(y)+h<=height_,Fault::bounds,"graphics rectangle outside framebuffer");
  }
  static void pixel(std::vector<std::byte>& pixels,size_t offset,uint32_t color){for(unsigned i=0;i<4;++i)pixels[offset+i]=std::byte(color>>(24-8*i));}
 public:
  explicit Graphics(uint32_t width=320,uint32_t height=180):width_(width),height_(height){
    require(width&&height&&width<=1280&&height<=720,Fault::bounds,"graphics framebuffer dimensions");
    try{pixels_.resize(size_t(width)*height*4);}catch(const std::bad_alloc&){throw Error(Fault::exhausted,"graphics framebuffer allocation");}
  }
  Graphics(const Graphics&)=delete;Graphics& operator=(const Graphics&)=delete;
  uint32_t width()const{return width_;}uint32_t height()const{return height_;}
  uint32_t pitch()const{return width_*4;}uint64_t last_frame()const{return frame_;}
  std::span<const std::byte> pixels()const{return pixels_;}
  // Word layouts (remaining words zero):
  // BEGIN frame_hi,frame_lo,width,height,format; CLEAR/COLOR RRGGBBAA;
  // RECT x,y,width,height; UPLOAD slot,width,height,pitch,source_index;
  // BLIT slot,x,y,width,height; PRESENT. Textures survive successful frames.
  // Exact caller capabilities accompany all guest spans. Host copies only.
  // Existing textures <=16 MiB; new captured uploads <=16 MiB; two framebuffers
  // <=7.1 MiB; commands <=64 KiB. Refusals publish no pixels/textures/frame.
  void submit(Space& space,Space::Handle commands,uint32_t address,uint32_t size,std::span<const Upload> sources={}){
    require(!(address&(record_size-1))&&size>=3*record_size&&!(size%record_size)&&size<=record_size*max_commands,Fault::argument,"graphics command alignment/count");
    require(sources.size()<=max_textures,Fault::bounds,"graphics upload descriptor count");
    try{
      auto raw=copy(space,commands,address,size);std::vector<Command> parsed(size/record_size);
      std::vector<TextureRef> captured(parsed.size());
      for(size_t i=0;i<parsed.size();++i)for(size_t w=0;w<8;++w){uint32_t v=0;for(size_t b=0;b<4;++b)v=(v<<8)|std::to_integer<uint32_t>(raw[i*record_size+w*4+b]);parsed[i][w]=v;}
      const auto& begin=parsed.front();require(begin[0]==uint32_t(Op::begin)&&parsed[1][0]==uint32_t(Op::clear)&&parsed.back()[0]==uint32_t(Op::present),Fault::argument,"graphics begin/clear/present sequence");
      uint64_t next=(uint64_t(begin[1])<<32)|begin[2];require(next>frame_,Fault::stale,"graphics nonmonotonic frame");
      require(begin[3]==width_&&begin[4]==height_&&begin[5]==rgba8,Fault::argument,"graphics framebuffer format mismatch");zeros(begin,6);
      auto textures=textures_;size_t copied=0;
      for(size_t i=1;i<parsed.size();++i){const auto& c=parsed[i];auto op=Op(c[0]);
        if(op==Op::clear){require(i==1,Fault::argument,"graphics clear sequence");zeros(c,2);}
        else if(op==Op::color){zeros(c,2);}
        else if(op==Op::rect){rectangle(c[1],c[2],c[3],c[4]);zeros(c,5);}
        else if(op==Op::upload){
          require(c[1]<max_textures&&c[5]<sources.size(),Fault::bounds,"graphics upload slot/source");
          require(c[2]&&c[3]&&c[2]<=1024&&c[3]<=1024&&c[4]>=uint64_t(c[2])*4&&!(c[4]&3),Fault::argument,"graphics RGBA8 texture dimensions/pitch");
          uint64_t bytes=uint64_t(c[4])*c[3];require(bytes<=max_texture_bytes&&copied+bytes<=max_texture_bytes,Fault::bounds,"graphics upload staging budget");
          const auto& src=sources[c[5]];require(src.size==bytes,Fault::bounds,"graphics upload descriptor size");zeros(c,6);
          auto data=copy(space,src.owner,src.address,src.size);copied+=size_t(bytes);
          captured[i]=std::make_shared<Texture>(Texture{c[2],c[3],c[4],std::move(data)});textures[c[1]]=captured[i];
          size_t resident=0;for(const auto& t:textures)if(t)resident+=t->bytes.size();require(resident<=max_texture_bytes,Fault::bounds,"graphics texture residency budget");
        }
        else if(op==Op::blit){require(c[1]<max_textures&&bool(textures[c[1]]),Fault::argument,"graphics absent texture");rectangle(c[2],c[3],c[4],c[5]);zeros(c,6);}
        else if(op==Op::present){require(i+1==parsed.size(),Fault::argument,"graphics early present");zeros(c,1);}
        else throw Error(Fault::unsupported,"graphics unknown command or repeated begin");
      }
      std::vector<std::byte> output(pixels_.size());uint32_t color=0xffffffff;auto execution=textures_;
      for(size_t i=1;i<parsed.size();++i){const auto& c=parsed[i];auto op=Op(c[0]);
        if(op==Op::clear){for(size_t p=0;p<output.size();p+=4)pixel(output,p,c[1]);}
        else if(op==Op::color)color=c[1];
        else if(op==Op::rect){for(uint32_t y=c[2];y<c[2]+c[4];++y)for(uint32_t x=c[1];x<c[1]+c[3];++x)pixel(output,(size_t(y)*width_+x)*4,color);}
        else if(op==Op::upload)execution[c[1]]=captured[i];
        else if(op==Op::blit){const auto& t=*execution[c[1]];for(uint32_t y=0;y<c[5];++y)for(uint32_t x=0;x<c[4];++x){size_t src=size_t(uint64_t(y)*t.height/c[5])*t.pitch+size_t(uint64_t(x)*t.width/c[4])*4;size_t dst=(size_t(c[3]+y)*width_+c[2]+x)*4;std::copy_n(t.bytes.begin()+src,4,output.begin()+dst);}}
      }
      pixels_.swap(output);textures_.swap(textures);frame_=next;
    }catch(const std::bad_alloc&){throw Error(Fault::exhausted,"graphics bounded staging allocation");}
  }
};
} // namespace brutal::horizon::platform
