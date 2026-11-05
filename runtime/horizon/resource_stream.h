// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "platform_resources.h"
#include <memory>
namespace brutal::horizon::resources {
// Incremental checked guest delivery from the existing immutable Service
// snapshots, not snapshot-free filesystem I/O. One serialized application owner;
// Service and Space must outlive this stream. No output token is exposed before
// its complete size and SHA256 identity are verified. There is no duplicate cache.
class Stream {
 public:
  static constexpr uint32_t max_jobs=8,max_step=Service::max_read;
  struct Token {uint64_t owner{};uint32_t slot{},generation{};};
  struct Expected {std::string path;uint32_t size{};Digest sha256{};};
  struct Progress {uint32_t transferred{},total{};bool ready{};uint32_t verified{};};
 private:
  // Fixed-storage SHA256 state permits verification to consume the same byte
  // budget as delivery. No complete guest/source copy is made for hashing.
  class Hash {
    std::array<uint32_t,8> state_{0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::array<uint8_t,64> block_{};size_t used_{};uint64_t length_{};
    void compress(){
      constexpr uint32_t k[]{0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
      uint32_t words[64]{};
      for(unsigned i=0;i<16;++i)for(unsigned j=0;j<4;++j)words[i]=(words[i]<<8)|block_[4*i+j];
      for(unsigned i=16;i<64;++i){auto a=words[i-15],b=words[i-2];words[i]=words[i-16]+(std::rotr(a,7)^std::rotr(a,18)^(a>>3))+words[i-7]+(std::rotr(b,17)^std::rotr(b,19)^(b>>10));}
      auto a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
      for(unsigned i=0;i<64;++i){auto t=h+(std::rotr(e,6)^std::rotr(e,11)^std::rotr(e,25))+((e&f)^(~e&g))+k[i]+words[i];auto u=(std::rotr(a,2)^std::rotr(a,13)^std::rotr(a,22))+((a&b)^(a&c)^(b&c));h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;}
      state_[0]+=a;state_[1]+=b;state_[2]+=c;state_[3]+=d;state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;
    }
    void push(uint8_t value){block_[used_++]=value;if(used_==64){compress();used_=0;}}
   public:
    void update(std::span<const std::byte> bytes){for(auto b:bytes){push(std::to_integer<uint8_t>(b));++length_;}}
    Digest finish(){
      auto bits=length_*8;push(128);while(used_!=56)push(0);for(int i=7;i>=0;--i)push(uint8_t(bits>>(8*i)));
      Digest result{};for(unsigned i=0;i<8;++i)for(unsigned j=0;j<4;++j)result[4*i+j]=uint8_t(state_[i]>>(24-8*j));return result;
    }
  };
  struct Job {
    Expected expected;Service::Handle source{};Space::Handle output{};
    std::optional<Space::Lease> pin;Hash hash;uint32_t loaded{},verified{},charge{};bool source_open{},ready{};
    explicit Job(Expected e):expected(std::move(e)){}
  };
  struct Slot {uint32_t generation{1};std::unique_ptr<Job> job;};
  Service& service_;Space& space_;const uint32_t budget_;
  inline static std::atomic<uint64_t> serial_{1};const uint64_t owner_{serial_.fetch_add(1)};
  std::array<Slot,max_jobs> slots_{};uint32_t bytes_{},reserved_{};bool alive_{true};
  static void require(bool ok,Fault f,const char*why){if(!ok)throw Error(f,why);}
  Slot& owned(Token token){
    require(alive_,Fault::stale,"resource stream closed");require(token.owner==owner_,Fault::ownership,"foreign streaming job");
    require(token.slot<max_jobs,Fault::ownership,"invalid streaming slot");auto&s=slots_[token.slot];
    require(s.job&&s.generation==token.generation,Fault::stale,"stale streaming generation");return s;
  }
  void close_source(Job&j)noexcept{
    if(!j.source_open)return;
    try{service_.close(j.source);}catch(const Error&e){if(e.fault!=Fault::stale)std::terminate();}catch(...){std::terminate();}
    j.source_open=false;
  }
  void retire(Slot&s)noexcept{
    bytes_-=s.job->expected.size;reserved_-=s.job->charge;close_source(*s.job);s.job.reset();++s.generation;
  }
  void rollback(Slot&s){
    // Competing host pins refuse without retiring a token or changing ownership.
    require(space_.release_exclusive(*s.job->pin),Fault::ownership,"stream output has an external lease");retire(s);
  }
 public:
  explicit Stream(Service&service,Space&space,uint32_t guest_budget=256*1024):service_(service),space_(space),budget_(guest_budget){
    require(owner_&&guest_budget>=4096&&guest_budget<=max_jobs*Service::max_file,Fault::argument,"stream guest memory budget");
  }
  Stream(const Stream&)=delete;Stream&operator=(const Stream&)=delete;
  ~Stream(){try{shutdown();}catch(...){std::terminate();}}
  Token begin(Expected expected){
    require(alive_,Fault::stale,"resource stream closed");
    require(expected.size&&expected.size<=Service::max_file,Fault::bounds,"stream resource extent");
    require(expected.sha256!=Digest{},Fault::argument,"stream requires immutable content identity");
    uint32_t charge=(expected.size+4095)&~uint32_t(4095);
    require(charge<=budget_-reserved_,Fault::exhausted,"stream guest memory budget exhausted");
    uint32_t index=0;while(index<max_jobs&&(slots_[index].job||slots_[index].generation==UINT32_MAX))++index;
    require(index<max_jobs,Fault::exhausted,"stream job budget exhausted");
    auto job=std::make_unique<Job>(std::move(expected));job->charge=charge;
    job->source=service_.open(job->expected.path);job->source_open=true;
    try{
      require(service_.size(job->source)==job->expected.size,Fault::argument,"stream expected source size");
      job->output=space_.allocate(job->expected.size);
      try{job->pin.emplace(space_,job->output);}catch(...){space_.release(job->output);throw;}
    }catch(...){close_source(*job);throw;}
    auto&s=slots_[index];bytes_+=job->expected.size;reserved_+=charge;s.job=std::move(job);return {owner_,index,s.generation};
  }
  Progress progress(Token token){auto&j=*owned(token).job;return {j.loaded,j.expected.size,j.ready,j.verified};}
  Progress step(Token token,uint32_t budget=max_step){
    auto&s=owned(token);auto&j=*s.job;
    require(budget&&budget<=max_step,Fault::bounds,"stream step byte budget");
    if(j.ready)return {j.loaded,j.expected.size,true,j.verified};
    try{
      if(j.loaded<j.expected.size){
        uint32_t requested=std::min(budget,j.expected.size-j.loaded);
        auto n=service_.read(j.source,space_,j.output,j.output.address+j.loaded,requested);
        require(n&&n<=requested,Fault::unsupported,"stream premature EOF");j.loaded+=n;
        if(j.loaded==j.expected.size){space_.freeze_writes(j.output,*j.pin);close_source(j);}
      }else{
        auto count=std::min(budget,j.expected.size-j.verified);auto address=j.output.address+j.verified;
        space_.validate_owned_window(j.output,address,count,Access::read);
        auto bytes=space_.read(space_.view(address,count,Access::read));j.hash.update(bytes);j.verified+=count;
        if(j.verified==j.expected.size){
          require(j.hash.finish()==j.expected.sha256,Fault::argument,"stream complete SHA256 identity mismatch");j.ready=true;
        }
      }
      return {j.loaded,j.expected.size,j.ready,j.verified};
    }catch(...){rollback(s);throw;}
  }
  Space::Handle take(Token token,Permission permission=Permission::read){
    auto&s=owned(token);auto&j=*s.job;require(j.ready,Fault::uncommitted,"stream resource is incomplete");
    require(permission==Permission::read||permission==Permission::read_write,Fault::argument,"stream publication permission");
    space_.publish_frozen(*j.pin,permission);auto output=j.output;j.pin.reset();retire(s);return output;
  }
  void cancel(Token token){rollback(owned(token));}
  // Transactional with respect to active host leases: an external pin refuses
  // the whole shutdown. The owner must retire that pin before stream destruction.
  void shutdown(){
    if(!alive_)return;
    std::array<Space::Lease*,max_jobs> pins{};size_t count=0;
    for(auto&s:slots_)if(s.job)pins[count++]=&*s.job->pin;
    require(space_.release_exclusive_all(std::span(pins).first(count)),Fault::ownership,"stream shutdown has external leases");
    for(auto&s:slots_)if(s.job){retire(s);}
    alive_=false;
  }
  size_t live()const{size_t n=0;for(auto&s:slots_)n+=bool(s.job);return n;}
  // Budget/accounting cover jobs still owned here; take() transfers accounting
  // and allocation-release responsibility to the caller.
  uint32_t bytes()const{return bytes_;}uint32_t reserved_bytes()const{return reserved_;}
};
}
