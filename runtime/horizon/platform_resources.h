// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "native_image_plan.h"
#include <cerrno>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#ifndef __SWITCH__
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
namespace brutal::horizon::resources {
using Digest=native_image::Digest;
struct Manifest {std::string path;uint64_t expected_size{};std::optional<Digest> sha256;};
// Original synthetic resource namespace only. Construction snapshots a small,
// explicit manifest from native read-only files and closes every native file.
// Thus later reads neither retain a source lifetime nor trust mutable host data.
class Service {
 public:
  struct Handle {uint64_t owner{};uint32_t slot{},generation{};};
  static constexpr uint32_t max_read=4096,max_file=64*1024,max_files=16,max_handles=32;
 private:
  struct Resource {std::string path;std::vector<uint8_t> bytes;};
  struct Slot {uint32_t generation{1};size_t resource{},cursor{};bool active{};};
  inline static std::atomic<uint64_t> next_owner_{1};
  inline static std::atomic<size_t> handles_total_{},resident_total_{};
  uint64_t owner_{next_owner_.fetch_add(1)};std::mutex mutex_;
  std::vector<Resource> resources_;std::array<Slot,max_handles> slots_{};
  size_t resident_{};bool alive_{};
  static void require(bool v,Fault f,const char*why){if(!v)throw Error(f,why);}
  static void path(std::string_view p){
    require(!p.empty()&&p.size()<=240,Fault::argument,"resource path extent");
    size_t start=0;
    for(size_t i=0;i<=p.size();++i){
      if(i==p.size()||p[i]=='/'){
        auto part=p.substr(start,i-start);
        require(!part.empty()&&part!="."&&part!="..",Fault::argument,"resource path component");start=i+1;
      }else{
        auto c=p[i];require((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_'||c=='-'||c=='.',Fault::argument,"resource path character");
      }
    }
  }
#ifndef __SWITCH__
  struct Descriptor {
    int value{-1};explicit Descriptor(int v=-1):value(v){}Descriptor(const Descriptor&)=delete;
    Descriptor(Descriptor&&d)noexcept:value(std::exchange(d.value,-1)){}
    Descriptor&operator=(Descriptor&&d)noexcept{if(value>=0)::close(value);value=std::exchange(d.value,-1);return *this;}
    ~Descriptor(){if(value>=0)::close(value);}
  };
  // Resolve every component by its parent descriptor; neither a symlinked root
  // ancestor nor a swapped descendant can redirect an admitted open.
  static Descriptor descend(Descriptor parent,std::string_view relative,bool file){
    size_t start=0;
    while(start<relative.size()){
      auto end=relative.find('/',start);if(end==std::string_view::npos)end=relative.size();
      std::string component(relative.substr(start,end-start));bool last=end==relative.size();
      int flags=O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK;
      if(!file||!last)flags|=O_DIRECTORY;
      Descriptor next(::openat(parent.value,component.c_str(),flags));
      require(next.value>=0,Fault::unsupported,"resource path open refused");parent=std::move(next);start=end+1;
    }
    return parent;
  }
  static Descriptor root(std::string_view p){
    require(!p.empty(),Fault::argument,"resource root extent");bool absolute=p.front()=='/';if(absolute)p.remove_prefix(1);
    require(p.find('\0')==std::string_view::npos,Fault::argument,"native resource root NUL");
    size_t start=0;
    while(start<p.size()){
      auto end=p.find('/',start);if(end==std::string_view::npos)end=p.size();auto part=p.substr(start,end-start);
      require(!part.empty()&&part!="."&&part!="..",Fault::argument,"native resource root component");start=end+1;
    }
    require(p.empty()||p.back()!='/',Fault::argument,"native resource root trailing slash");
    Descriptor base(::open(absolute?"/":".",O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC));
    require(base.value>=0,Fault::unsupported,"resource root open refused");return descend(std::move(base),p,false);
  }
  static std::vector<uint8_t> load(int rootfd,const Manifest&m){
    // Duplicate a trusted descriptor, not a path; descend owns its argument.
    Descriptor file=descend(Descriptor(::dup(rootfd)),m.path,true);
    struct stat st{};require(::fstat(file.value,&st)==0&&S_ISREG(st.st_mode)&&st.st_size>=0&&uint64_t(st.st_size)==m.expected_size,Fault::argument,"resource expected regular file size");
    std::vector<uint8_t> bytes(size_t(m.expected_size));size_t done=0;
    while(done<bytes.size()){
      auto n=::read(file.value,bytes.data()+done,bytes.size()-done);
      if(n<0&&errno==EINTR)continue;
      require(n>0,Fault::unsupported,"resource source truncated or unreadable");done+=size_t(n);
    }
    uint8_t extra{};ssize_t n;do{n=::read(file.value,&extra,1);}while(n<0&&errno==EINTR);
    require(n==0,Fault::argument,"resource source exceeded expected size");return bytes;
  }
#else
  static std::vector<uint8_t> load(const std::string&root,const Manifest&m){
    // libnx ROMFS is an immutable packaged filesystem with no symbolic links.
    // Only this literal native mount/root is admitted; no guest path reaches it.
    std::string name=root+"/"+m.path;std::FILE* file=std::fopen(name.c_str(),"rb");
    require(file!=nullptr,Fault::unsupported,"ROMFS resource open refused");
    struct Close {std::FILE*f;~Close(){std::fclose(f);}} close{file};
    std::vector<uint8_t> bytes(size_t(m.expected_size));size_t done=0;
    while(done<bytes.size()){
      auto n=std::fread(bytes.data()+done,1,bytes.size()-done,file);
      require(n>0,Fault::unsupported,"ROMFS resource truncated or unreadable");done+=n;
    }
    require(std::fgetc(file)==EOF&&!std::ferror(file),Fault::argument,"ROMFS resource expected size");return bytes;
  }
#endif
  Slot& owned(Handle h){
    require(alive_,Fault::stale,"resource service closed");require(h.owner==owner_,Fault::ownership,"foreign resource handle");
    require(h.slot<slots_.size(),Fault::ownership,"invalid resource handle slot");auto&s=slots_[h.slot];
    require(s.active&&h.generation==s.generation,Fault::stale,"stale resource handle generation");return s;
  }
 public:
  Service(std::string native_root,std::span<const Manifest> manifest){
    require(owner_!=0&&!manifest.empty()&&manifest.size()<=max_files,Fault::argument,"resource manifest budget");
    // Validate the entire manifest before performing any I/O.
    for(size_t i=0;i<manifest.size();++i){auto&m=manifest[i];path(m.path);
      require(m.expected_size<=max_file,Fault::bounds,"resource file budget");
      for(size_t j=0;j<i;++j)require(m.path!=manifest[j].path,Fault::argument,"duplicate resource manifest path");
    }
#ifndef __SWITCH__
    auto directory=root(native_root);
#else
    require(native_root=="romfs:/resources",Fault::argument,"only packaged ROMFS resource root admitted");
#endif
    resources_.reserve(manifest.size());
    for(auto&m:manifest){
#ifndef __SWITCH__
      auto bytes=load(directory.value,m);
#else
      auto bytes=load(native_root,m);
#endif
      require(!m.sha256||native_image::digest(bytes)==*m.sha256,Fault::argument,"resource SHA256 identity mismatch");
      resident_+=bytes.size();resources_.push_back({m.path,std::move(bytes)});
    }
    resident_total_+=resident_;alive_=true;
  }
  Service(const Service&)=delete;Service&operator=(const Service&)=delete;
  ~Service(){shutdown();}
  Handle open(std::string_view virtual_path){
    std::lock_guard lock(mutex_);require(alive_,Fault::stale,"resource service closed");
    constexpr std::string_view prefix="demo:/";require(virtual_path.starts_with(prefix),Fault::argument,"resource virtual root");
    virtual_path.remove_prefix(prefix.size());path(virtual_path);
    size_t index=0;while(index<resources_.size()&&resources_[index].path!=virtual_path)++index;
    require(index<resources_.size(),Fault::unsupported,"resource outside explicit manifest");
    for(uint32_t i=0;i<slots_.size();++i){auto&s=slots_[i];if(!s.active&&s.generation!=UINT32_MAX){s.resource=index;s.cursor=0;s.active=true;++handles_total_;return {owner_,i,s.generation};}}
    throw Error(Fault::exhausted,"resource handle budget");
  }
  uint64_t size(Handle h){std::lock_guard lock(mutex_);return resources_[owned(h).resource].bytes.size();}
  uint64_t position(Handle h){std::lock_guard lock(mutex_);return owned(h).cursor;}
  uint32_t read(Handle h,Space&space,Space::Handle destination,uint32_t address,uint32_t count){
    std::lock_guard lock(mutex_);auto&s=owned(h);require(count<=max_read,Fault::bounds,"resource read budget");
    if(!count)return 0;
    Space::Lease pin(space,destination);space.validate_owned_window(destination,address,count,Access::write);
    const auto&bytes=resources_[s.resource].bytes;auto n=uint32_t(std::min<size_t>(count,bytes.size()-s.cursor));
    if(n){auto out=space.view(address,n,Access::write);space.write(out,std::as_bytes(std::span(bytes).subspan(s.cursor,n)));}
    // Validation, pinning, and the actual checked write all precede publication.
    s.cursor+=n;return n;
  }
  void close(Handle h){std::lock_guard lock(mutex_);auto&s=owned(h);s.active=false;++s.generation;--handles_total_;}
  void shutdown(){
    std::lock_guard lock(mutex_);if(!alive_)return;
    for(auto&s:slots_)if(s.active){s.active=false;++s.generation;--handles_total_;}
    resident_total_-=resident_;resident_=0;resources_.clear();alive_=false;
  }
  size_t live(){std::lock_guard lock(mutex_);size_t n=0;for(auto&s:slots_)n+=s.active;return n;}
  static std::pair<size_t,size_t> totals(){return {handles_total_.load(),resident_total_.load()};}
};
}
