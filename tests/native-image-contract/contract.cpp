// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original finite native-image contract experiment, NOT a production XEX loader.
#include "runtime/horizon/space.h"
#include <fstream>
#include <iostream>
using namespace brutal::horizon;
unsigned passed=0;
void check(bool v){if(!v)throw std::runtime_error("assertion failed");++passed;}
template<class F> void refused(F f){bool caught=false;try{f();}catch(const Error&){caught=true;}check(caught);}
constexpr uint32_t base=0x82000000, page=65536;
struct Image {
 Space& s;std::vector<Space::Handle> handles;
 Image(Space& space,const std::vector<std::byte>& bytes,bool authored_zero,int fail=-1,bool overlap=false):s(space){
  if(bytes.size()!=0x41000||bytes[0]!=std::byte{0x58}||bytes[1]!=std::byte{0x45})throw Error(Fault::argument,"fixture identity");
  // Only this known producer's finite fixture is supported. PE V is not authority.
  if(!authored_zero)throw Error(Fault::ownership,"no zero provenance");
  std::array<uint32_t,5> plan{};
  for(unsigned i=0;i<5;++i)plan[i]=base+i*page;
  if(overlap)plan[4]=plan[3];
  for(unsigned i=0;i<5;++i)for(unsigned j=0;j<i;++j)
   if(uint64_t(plan[i])<uint64_t(plan[j])+page&&uint64_t(plan[j])<uint64_t(plan[i])+page)
    throw Error(Fault::ownership,"overlapping plan");
  try{for(unsigned i=0;i<5;++i){
   if(int(i)==fail)throw Error(Fault::exhausted,"injected transaction failure");
   auto h=s.allocate(page,plan[i]);handles.push_back(h);
   if(i<4)s.write(h.address,std::span(bytes).subspan(4096+i*page,page));
   else {std::vector<std::byte> zero(page);s.write(h.address,zero);}
   if(i<3)s.protect(h,Permission::read);
  }}catch(...){s.release_all(handles);handles.clear();throw;}
 }
 void unload(){s.release_all(handles);handles.clear();}
 ~Image(){if(!handles.empty())unload();}
};
int main(int argc,char**argv){try{
 check(argc==2);std::ifstream f(argv[1],std::ios::binary);std::vector<char> chars((std::istreambuf_iterator<char>(f)),{});std::vector<std::byte> bytes(chars.size());for(size_t i=0;i<chars.size();++i)bytes[i]=std::byte(uint8_t(chars[i]));
 check(bytes.size()==0x41000);
 auto le=[&](size_t o){uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::to_integer<uint32_t>(bytes.at(o+i))<<(8*i);return v;};
 check(le(0x1000+0x98+224+3*40+8)==0x20000);check(le(0x1000+0x98+56)==0x50000);
 Space s(2*1024*1024);refused([&]{Image no(s,bytes,false);});check(s.live()==0);
 refused([&]{Image bad(s,bytes,true,-1,true);});check(s.live()==0);
 for(int i=0;i<5;++i){refused([&]{Image fail(s,bytes,true,i);});check(s.live()==0);}
 {Space small(4*page);refused([&]{Image fail(small,bytes,true);});check(small.live()==0);}
 auto neighbor=s.allocate(page,base+4*page);s.store<uint32_t>(neighbor.address,0xabcdef01);
 refused([&]{Image collision(s,bytes,true);});check(s.live()==1);check(s.load<uint32_t>(neighbor.address)==0xabcdef01);s.release(neighbor);
 Image image(s,bytes,true);check(s.live()==5);
 // Exhaustive expected bytes for the entire accepted logical image; no raw host pointers.
 for(unsigned i=0;i<5;++i){auto q=s.query(base+i*page);check(q.committed&&q.allocation_base==base+i*page&&q.allocation_size==page);auto b=s.read(base+i*page,page);for(unsigned j=0;j<page;++j)check(b[j]==(i<4?bytes[4096+i*page+j]:std::byte{0}));}
 refused([&]{s.store<uint32_t>(base+page,1);});
 s.store<uint32_t>(base+4*page+page-4,0x12345678);check(s.load<uint32_t>(base+5*page-4)==0x12345678);
 refused([&]{s.read(base+5*page-1,2);});refused([&]{s.read(base+5*page,1);});refused([&]{s.read(0,1);});
 auto stack=s.allocate(16384,0x70000000);uint32_t r1=stack.address+stack.size-16;check((r1&15)==0);s.store<uint32_t>(r1,42);check(s.load<uint32_t>(r1)==42);refused([&]{s.read(stack.address-1,1);});s.release(stack);
 auto stale=s.view(base+4*page,4,Access::read);
 {auto lease=s.pin(base+4*page,1);refused([&]{image.unload();});check(s.live()==5);check(s.leases()==1);}
 image.unload();check(s.live()==0);refused([&]{s.read(stale);});
 {Image again(s,bytes,true);refused([&]{s.read(stale);});}check(s.live()==0);check(s.leases()==0);
 std::cout<<"{\"passed\":"<<passed<<",\"failed\":0,\"accepted_image_bytes\":327680,\"proprietary_execution\":false}"<<std::endl;
 return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
