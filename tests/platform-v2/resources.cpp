// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/resource_stream.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <source_location>
#include <cstdlib>
using namespace brutal::horizon;
namespace r=resources;namespace fs=std::filesystem;
namespace {
unsigned checks{},refusals{},compared{},steps{};
void check(bool value,std::source_location at=std::source_location::current()){if(!value)throw std::runtime_error("stream assertion line "+std::to_string(at.line()));++checks;}
template<class F>void no(F f,Fault expected){try{f();}catch(const Error&e){check(e.fault==expected);++refusals;return;}throw std::runtime_error("missing streaming refusal");}
struct Temp {fs::path path;Temp(){auto name=(fs::canonical(fs::temp_directory_path())/"brutal-stream-XXXXXX").string();auto*p=::mkdtemp(name.data());if(!p)throw std::runtime_error("stream temporary root");path=p;}~Temp(){std::error_code ec;fs::remove_all(path,ec);}};
r::Digest unhex(std::string_view text){r::Digest result{};if(text.size()!=64)throw std::runtime_error("fixture digest length");auto digit=[](char c){return c<='9'?c-'0':c-'a'+10;};for(size_t i=0;i<32;++i)result[i]=uint8_t((digit(text[2*i])<<4)|digit(text[2*i+1]));return result;}
void put(const fs::path&p,std::span<const uint8_t>b){std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));if(!f)throw std::runtime_error("stream fixture write");}
void zero(r::Stream&s,Space&space,r::Service&source){check(s.live()==0&&s.bytes()==0&&s.reserved_bytes()==0&&space.live()==0&&space.leases()==0&&source.live()==0);}
}
int main(){try{
 Temp temp;std::vector<uint8_t> data(9001);for(size_t i=0;i<data.size();++i)data[i]=uint8_t(i*13+(i>>8));std::vector<uint8_t> small{4,9,16,25,36};
 put(temp.path/"geometry.bin",data);put(temp.path/"scene.bin",small);
 // Independent Python hashlib values; neither streaming nor native_image's
 // implementation serves as the sole content-identity oracle.
 auto hash=unhex("d2d8d7aa5b75f21809995d1efdefcd3580feb9fd663c086a93f9bbd45d65cb1b"),small_hash=unhex("54c9eaefb5d87ac60f705d9304ad7e599f5ab2911c479a370bef9b2b8b252d5e");
 std::vector<r::Manifest> manifest{{"geometry.bin",data.size(),hash},{"scene.bin",small.size(),small_hash}};
 std::vector<std::vector<uint8_t>> boundaries;
 const std::array<std::pair<unsigned,std::string_view>,7> lengths{{
  {55,"a84613fd22efecac05ebf81968d16aaf263eb4f2e4b36023d5fbd5acea984bd9"},
  {56,"41e30705dcb5d9088a720913e58d4a77cac0edb227beb862ea5785d0fa80e9b3"},
  {63,"88ba89e7e5fc33a44f73337963bf961a337b45728dafcdc979234098f692be3c"},
  {64,"07f49d6dbb014ae057b0222a8a8259d381619c27823dd0443edba0df501f1f5d"},
  {65,"ce5b8e57d94dadc5a963a58b7b7eaee35531e6e92fbf4b769e74be3ba2aeb935"},
  {4096,"6c1eb2a3c4696aba55b734662bf880a5702b288a82ca0f8b9f72eeb8f2cffd07"},
  {65536,"45af25ec7eeeec0163eeceae355e863fe05298409dd7af977bab20158a651be9"}}};
 for(auto [length,digest]:lengths){boundaries.emplace_back(length);auto&b=boundaries.back();for(size_t i=0;i<b.size();++i)b[i]=uint8_t(i*3+length);auto name="hash-"+std::to_string(length)+".bin";put(temp.path/name,b);manifest.push_back({name,b.size(),unhex(digest)});}
 // Standard SHA256 short-message and two-padding-block test vectors.
 for(auto [text,digest]:std::array<std::pair<std::string_view,std::string_view>,2>{{
  {"abc","ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
  {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq","248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"}}}){
  boundaries.emplace_back(text.begin(),text.end());auto name="standard-"+std::to_string(text.size())+".bin";put(temp.path/name,boundaries.back());manifest.push_back({name,text.size(),unhex(digest)});
 }
 r::Service source(temp.path.string(),manifest);
 const r::Stream::Expected expected{"demo:/geometry.bin",uint32_t(data.size()),hash},tiny{"demo:/scene.bin",uint32_t(small.size()),small_hash};
 {Space space(1024*1024);r::Stream stream(source,space);zero(stream,space,source);
  no([&]{r::Stream bad(source,space,4095);},Fault::argument);no([&]{stream.begin({expected.path,0,hash});},Fault::bounds);
  no([&]{stream.begin({expected.path,r::Service::max_file+1,hash});},Fault::bounds);no([&]{stream.begin({expected.path,expected.size,{}});},Fault::argument);
  no([&]{stream.begin({"demo:/../geometry.bin",expected.size,hash});},Fault::argument);no([&]{stream.begin({"demo:/absent.bin",expected.size,hash});},Fault::unsupported);
  no([&]{stream.begin({expected.path,expected.size-1,hash});},Fault::argument);zero(stream,space,source);
  auto job=stream.begin(expected);check(stream.bytes()==9001&&stream.reserved_bytes()==12288&&stream.live()==1&&space.live()==1&&space.leases()==1);
  auto token=job;token.owner++;no([&]{stream.progress(token);},Fault::ownership);token=job;token.slot=r::Stream::max_jobs;no([&]{stream.take(token);},Fault::ownership);
  no([&]{stream.take(job);},Fault::uncommitted);no([&]{stream.step(job,0);},Fault::bounds);no([&]{stream.step(job,4097);},Fault::bounds);check(stream.progress(job).transferred==0);
  fs::remove(temp.path/"geometry.bin");uint32_t transferred=0,verified=0;
  while(!stream.progress(job).ready){auto p=stream.step(job,128);++steps;check(p.transferred>=transferred&&p.verified>=verified&&(p.transferred-transferred)+(p.verified-verified)>0&&(p.transferred-transferred)+(p.verified-verified)<=128&&p.total==data.size());transferred=p.transferred;verified=p.verified;
   if(p.transferred==data.size()&&!p.verified)no([&]{space.write(0x10000000,std::array<std::byte,1>{});},Fault::protection);}
  check(transferred==data.size()&&verified==data.size()&&steps==142&&source.live()==0);check(stream.step(job,1).ready);no([&]{stream.take(job,Permission::none);},Fault::argument);
  auto output=stream.take(job);check(space.info(output).permission==Permission::read&&space.leases()==0);auto copy=space.read(output.address,output.size);
  check(std::equal(copy.begin(),copy.end(),std::as_bytes(std::span(data)).begin()));compared+=uint32_t(copy.size());no([&]{space.write(output.address,std::array<std::byte,1>{});},Fault::protection);
  no([&]{stream.progress(job);},Fault::stale);no([&]{stream.take(job);},Fault::stale);space.release(output);zero(stream,space,source);
  auto reused=stream.begin(tiny);check(reused.slot==job.slot&&reused.generation!=job.generation);no([&]{stream.cancel(job);},Fault::stale);
  auto p=stream.step(reused,2);check(p.transferred==2&&!p.ready);stream.cancel(reused);no([&]{stream.progress(reused);},Fault::stale);zero(stream,space,source);
  auto writable=stream.begin(tiny);check(!stream.step(writable).ready);check(stream.step(writable).ready);auto rw=stream.take(writable,Permission::read_write);space.write(rw.address,std::array<std::byte,1>{std::byte{8}});check(space.load<uint8_t>(rw.address)==8);space.release(rw);
  auto bad_hash=small_hash;bad_hash[3]^=1;auto corrupt=stream.begin({tiny.path,tiny.size,bad_hash});check(!stream.step(corrupt).ready);no([&]{stream.step(corrupt);},Fault::argument);no([&]{stream.progress(corrupt);},Fault::stale);zero(stream,space,source);
  for(size_t i=2;i<manifest.size();++i){const auto&m=manifest[i];auto boundary=stream.begin({"demo:/"+m.path,uint32_t(m.expected_size),*m.sha256});
   while(!stream.progress(boundary).ready)stream.step(boundary,m.expected_size==3?1:127);
   auto h=stream.take(boundary);auto b=space.read(h.address,h.size);check(std::equal(b.begin(),b.end(),std::as_bytes(std::span(boundaries[i-2])).begin()));compared+=uint32_t(b.size());space.release(h);
  }zero(stream,space,source);
  // A fully delivered but unverified object still owns its pin/allocation.
  auto verifying=stream.begin(tiny);check(!stream.step(verifying).ready&&stream.progress(verifying).verified==0);no([&]{stream.take(verifying);},Fault::uncommitted);stream.cancel(verifying);zero(stream,space,source);
  r::Stream foreign(source,space);auto another=stream.begin(tiny);no([&]{foreign.take(another);},Fault::ownership);no([&]{foreign.cancel(another);},Fault::ownership);stream.cancel(another);foreign.shutdown();
  stream.shutdown();no([&]{stream.begin(tiny);},Fault::stale);zero(stream,space,source);
 }
 {Space space(4096);r::Stream stream(source,space,4096);auto block=space.allocate(4096);no([&]{stream.begin(tiny);},Fault::exhausted);check(source.live()==0&&stream.live()==0&&space.leases()==0);space.release(block);}
 {Space space(65536);r::Stream stream(source,space,4096);auto a=stream.begin(tiny);no([&]{stream.begin(tiny);},Fault::exhausted);check(source.live()==1&&stream.bytes()==5&&stream.reserved_bytes()==4096);stream.cancel(a);zero(stream,space,source);}
 {Space space(65536);r::Stream stream(source,space,32768);std::array<r::Stream::Token,8> jobs{};for(auto&j:jobs)j=stream.begin(tiny);
  no([&]{stream.begin(tiny);},Fault::exhausted);check(space.live()==8&&space.leases()==8&&source.live()==8);stream.shutdown();zero(stream,space,source);for(auto j:jobs)no([&]{stream.progress(j);},Fault::stale);
 }
 {Space space(65536);r::Stream stream(source,space);auto a=stream.begin(tiny);auto b=stream.begin(tiny);{
   auto pin=space.pin(0x10000000,1);no([&]{stream.cancel(a);},Fault::ownership);no([&]{stream.shutdown();},Fault::ownership);check(stream.live()==2&&space.live()==2&&source.live()==2&&space.leases()==3);check(stream.progress(b).transferred==0);
  }stream.cancel(a);stream.cancel(b);zero(stream,space,source);
 }
 {Space space(65536);r::Stream stream(source,space);auto job=stream.begin(tiny);stream.step(job,2);
  space.write(0x10000000,std::array<std::byte,1>{std::byte{99}});stream.step(job);no([&]{stream.step(job);},Fault::argument);zero(stream,space,source);
 }
 {Space space(65536);{r::Stream stream(source,space);auto job=stream.begin(tiny);stream.step(job,1);}check(space.live()==0&&space.leases()==0&&source.live()==0);}
 {Space space(65536);r::Stream stream(source,space);auto job=stream.begin(tiny);stream.step(job,1);source.shutdown();no([&]{stream.step(job);},Fault::stale);zero(stream,space,source);}
 check(r::Service::totals()==std::pair<size_t,size_t>{0,0});check(Space::totals()==std::pair<size_t,size_t>{0,0});
 std::cout<<"{\"suite\":\"resource-stream\",\"checks\":"<<checks<<",\"refusals\":"<<refusals<<",\"byte_comparisons\":"<<compared<<",\"budgeted_steps\":"<<steps<<",\"live_jobs\":0,\"allocations\":0,\"leases\":0}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
