// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/platform_resources.h"
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>
#include <source_location>
using namespace brutal::horizon;
namespace r=brutal::horizon::resources;
namespace fs=std::filesystem;
unsigned positive{},refusals{},copied{};
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("resource assertion line "+std::to_string(at.line()));++positive;}
template<class F>void no(F fn){try{fn();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing resource refusal");}
const std::string original="BRUTAL NX SYNTHETIC PLATFORM\nseed=7\n";
const r::Digest expected{0x36,0x4d,0xdc,0x10,0xa1,0xf9,0x71,0x52,0xa9,0xff,0xb5,0xa8,0x75,0xee,0x8f,0xfa,0xf0,0x13,0x27,0xa2,0xf5,0x63,0xff,0x23,0xb4,0xa9,0xc0,0x7d,0xdc,0xd6,0xa5,0x41};
std::vector<uint8_t> bytes(std::string_view s){return {s.begin(),s.end()};}
void put(const fs::path&p,std::span<const uint8_t>b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));if(!f)throw std::runtime_error("fixture write");}
struct Temp {
 fs::path path;
 Temp(){std::string name=(fs::canonical(fs::temp_directory_path())/"brutal-resource-XXXXXX").string();auto*p=::mkdtemp(name.data());if(!p)throw std::runtime_error("temporary resource root");path=p;}
 ~Temp(){std::error_code ec;fs::remove_all(path,ec);}
};
int main(int argc,char**argv){try{
 check(r::Service::totals()==std::pair<size_t,size_t>{0,0});check(native_image::digest(bytes(original))==expected);
 std::array<r::Manifest,1> manifest{{{"demo.txt",original.size(),expected}}};
 if(argc>1){r::Service shipped(fs::canonical(argv[1]).string(),manifest);auto h=shipped.open("demo:/demo.txt");check(shipped.size(h)==original.size());shipped.close(h);}
 Temp temp;auto root=temp.path;put(root/"demo.txt",bytes(original));
 std::vector<uint8_t> large(5000);for(size_t i=0;i<large.size();++i)large[i]=uint8_t(i*7+3);put(root/"large.bin",large);
 fs::create_directory(root/"sub");put(root/"sub"/"nested.txt",bytes("nested"));
 auto metadata=std::array<r::Manifest,3>{{manifest[0],{"large.bin",large.size(),native_image::digest(large)},{"sub/nested.txt",6,std::nullopt}}};
 Space space(1024*1024),foreign(64*1024);auto out=space.allocate(8192),other=foreign.allocate(4096),ro=space.allocate(4096),reserved=space.reserve(4096);
 space.protect(ro,Permission::read);
 {
  r::Service service(root.string(),metadata);check(service.live()==0);check(r::Service::totals().second==5042);
  for(auto bad:{"demo:/../demo.txt","demo:/sub/../../demo.txt","demo:/./demo.txt","demo://demo.txt","demo:/sub//nested.txt","demo:/demo.txt/","/demo.txt","game:/demo.txt","demo:/C:/demo.txt","demo:/sub\\nested.txt","demo:/%2e%2e/demo.txt","demo:/"})no([&]{service.open(bad);});
  no([&]{service.open(std::string("demo:/demo.txt\0extra",20));});no([&]{service.open("demo:/missing.txt");});
  auto h=service.open("demo:/demo.txt");check(service.size(h)==36&&service.position(h)==0);
  auto token=h;token.owner++;no([&]{service.size(token);});token=h;token.slot=100;no([&]{service.size(token);});
  no([&]{service.read(h,space,out,out.address,r::Service::max_read+1);});
  no([&]{service.read(h,space,other,other.address,8);});no([&]{service.read(h,space,ro,ro.address,8);});
  no([&]{service.read(h,space,reserved,reserved.address,8);});
  no([&]{service.read(h,space,out,0xdead0000,8);});no([&]{service.read(h,space,out,out.address+out.size-1,2);});
  auto forged=out;forged.size--;no([&]{service.read(h,space,forged,out.address,8);});
  auto stale=space.allocate(4096);space.release(stale);no([&]{service.read(h,space,stale,stale.address,8);});
  check(service.position(h)==0&&space.leases()==0);check(service.read(h,space,out,out.address,0)==0);
  check(service.read(h,space,out,out.address,7)==7&&service.position(h)==7);
  auto first=space.read(out.address,7);check(std::equal(first.begin(),first.end(),std::as_bytes(std::span(original.data(),size_t(7))).begin()));copied+=7;
  // Native sources and manifest storage are unnecessary after construction.
  fs::remove(root/"demo.txt");metadata[0].path="changed.txt";
  check(service.read(h,space,out,out.address+7,4096)==29&&service.position(h)==36);copied+=29;
  auto complete=space.read(out.address,36);check(std::equal(complete.begin(),complete.end(),std::as_bytes(std::span(original.data(),original.size())).begin()));
  no([&]{service.read(h,space,ro,ro.address,1);});check(service.read(h,space,out,out.address,8)==0&&service.position(h)==36);
  service.close(h);no([&]{service.close(h);});no([&]{service.read(h,space,out,out.address,1);});
  auto reused=service.open("demo:/demo.txt");check(reused.slot==h.slot&&reused.generation!=h.generation);no([&]{service.size(h);});
  r::Service second(root.string(),std::span<const r::Manifest>(metadata).subspan(1));no([&]{second.size(reused);});second.shutdown();
  auto big=service.open("demo:/large.bin");check(service.size(big)==5000);
  check(service.read(big,space,out,out.address,4096)==4096);check(service.read(big,space,out,out.address+4096,4096)==904);check(service.read(big,space,out,out.address,1)==0);
  auto got=space.read(out.address,5000);check(std::equal(got.begin(),got.end(),std::as_bytes(std::span(large)).begin()));copied+=5000;
  service.close(big);service.close(reused);
  std::vector<r::Service::Handle> handles;for(unsigned i=0;i<r::Service::max_handles;++i)handles.push_back(service.open("demo:/demo.txt"));
  no([&]{service.open("demo:/demo.txt");});check(service.live()==r::Service::max_handles);service.shutdown();check(service.live()==0);
  no([&]{service.size(handles[0]);});no([&]{service.open("demo:/demo.txt");});service.shutdown();
 }
 check(r::Service::totals()==std::pair<size_t,size_t>{0,0});put(root/"demo.txt",bytes(original));
 auto failing=[&](std::vector<r::Manifest> m){no([&]{r::Service service(root.string(),m);});check(r::Service::totals()==std::pair<size_t,size_t>{0,0});};
 auto wrong=expected;wrong[0]^=1;failing({{"demo.txt",36,wrong}});failing({{"demo.txt",33,expected}});failing({{"demo.txt",37,expected}});failing({{"demo.txt",r::Service::max_file+1,std::nullopt}});
 failing({});failing({manifest[0],manifest[0]});failing({manifest[0],{"missing.txt",1,std::nullopt}});failing({{"../demo.txt",36,expected}});failing({{"/demo.txt",36,expected}});failing({{"sub",0,std::nullopt}});
 fs::create_symlink(root/"demo.txt",root/"link.txt");failing({{"link.txt",36,expected}});
 fs::create_directory_symlink(root/"sub",root/"linked");failing({{"linked/nested.txt",6,std::nullopt}});
 fs::create_directory_symlink(root,root/"root-link");no([&]{r::Service service((root/"root-link").string(),manifest);});
 no([&]{r::Service service((root/"root-link"/"sub").string(),std::array<r::Manifest,1>{{{"nested.txt",6,std::nullopt}}});});
 ::mkfifo((root/"pipe").c_str(),0600);failing({{"pipe",0,std::nullopt}});
 no([&]{r::Service service((root/".."/root.filename()).string(),manifest);});
 {r::Service service(root.string(),manifest);service.open("demo:/demo.txt");}check(r::Service::totals()==std::pair<size_t,size_t>{0,0});
 space.release(out);space.release(ro);space.release(reserved);foreign.release(other);check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"copied_bytes\":"<<copied<<",\"demo_bytes\":"<<original.size()<<",\"demo_sha256\":\"";
 for(auto b:expected)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(b);
 std::cout<<std::dec<<"\",\"handles\":0,\"resident_bytes\":0,\"guest_allocations\":0,\"guest_leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
