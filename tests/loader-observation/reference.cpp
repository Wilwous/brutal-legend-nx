// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Observation adapter, not Xenia Memory/CPU/kernel. Extracted methods are unchanged.
#include <algorithm>
#include <cstddef>
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include "xenia/kernel/util/xex2_info.h"
#include "third_party/pe/pe_image.h"
extern "C" {
#include "third_party/crypto/rijndael-alg-fst.h"
}
#define XELOGE(...) ((void)0) // logging only; all method results retained
void aes_decrypt_buffer(const uint8_t*,const uint8_t*,size_t,uint8_t*,size_t){throw std::runtime_error("encrypted corpus input prohibited");}
namespace xe {
constexpr uint32_t kMemoryAllocationReserve=1,kMemoryAllocationCommit=2,kMemoryProtectRead=1,kMemoryProtectWrite=2;
struct ObservationMemory {
 std::vector<uint8_t> bytes;std::vector<std::string> events;std::array<uint32_t,5> permissions{};uint32_t requested{},rounded{};
 ObservationMemory* LookupHeap(uint32_t){return this;}
 uint32_t page_size()const{return 65536;}
 bool AllocFixed(uint32_t address,uint32_t size,uint32_t alignment,uint32_t flags,uint32_t protect){
  requested=size;rounded=(uint64_t(size)+65535)&~uint64_t(65535);
  if(address!=0x82000000||!size||size>4*65536||alignment!=4096||flags!=3)throw std::runtime_error("adapter input budget");
  bytes.assign(rounded,0xcd);permissions.fill(0);for(unsigned i=0;i<rounded/65536;++i)permissions[i]=protect;
  events.push_back("allocate-request:"+std::to_string(size));return true;
 }
 uint8_t* TranslateVirtual(uint32_t address){if(address<0x82000000||uint64_t(address)-0x82000000>=bytes.size())throw std::runtime_error("adapter translation refused");return bytes.data()+address-0x82000000;}
 bool Protect(uint32_t address,uint32_t size,uint32_t protect){
  bool ok=address>=0x82000000&&uint64_t(address)-0x82000000+size<=bytes.size();events.push_back("protect-request:"+std::to_string(address)+":"+std::to_string(size)+":"+(ok?"ok":"outside-adapter"));
  if(ok)for(unsigned i=(address-0x82000000)/65536;i<(uint64_t(address)-0x82000000+size+65535)/65536;++i)permissions[i]=protect;
  return ok;
 }
};
namespace cpu {
struct PESection {char name[9];uint32_t raw_address,raw_size,address,size,flags;};
class XexModule {
 public:
 ObservationMemory mem;std::vector<uint8_t> input;uint32_t base_address_=0x82000000;uint8_t session_key_[16]{};std::vector<PESection> pe_sections_;
 struct Security {uint32_t page_descriptor_count;const xex2_page_descriptor* page_descriptors;};Security security{};
 const xex2_header* xex_header()const{return reinterpret_cast<const xex2_header*>(input.data());}
 const xex2_opt_file_format_info* opt_file_format_info()const{return reinterpret_cast<const xex2_opt_file_format_info*>(input.data()+0x300);}
 const Security* xex_security_info()const{return &security;}
 ObservationMemory* memory()const{return const_cast<ObservationMemory*>(&mem);}
 int ReadImageUncompressed(const void*,size_t);int ReadImageBasicCompressed(const void*,size_t);int ReadPEHeaders();void ObserveProtection();
};
#include "reference-methods.inc"
}
}
int main(int argc,char**argv){
 if(argc!=2)return 2;
 try{xe::cpu::XexModule m;std::ifstream f(argv[1],std::ios::binary);m.input={std::istreambuf_iterator<char>(f),{}};
  if(!f||m.input.size()<0x21000||m.input.size()>0x41000)return 2;
  auto word=[&](size_t at){return uint32_t(m.input.at(at))<<24|uint32_t(m.input.at(at+1))<<16|uint32_t(m.input.at(at+2))<<8|m.input.at(at+3);};
  if(word(0)!=0x58455832||word(8)!=0x1000||word(16)!=0x100||word(36)!=0x82000000||word(0x280)!=3)return 2;
  if(uint16_t(m.opt_file_format_info()->encryption_type)!=0)return 2;
  m.security={3,reinterpret_cast<const xe::xex2_page_descriptor*>(m.input.data()+0x284)};
  unsigned compression=m.opt_file_format_info()->compression_type;if(compression>1)return 2;
  int image=compression?m.ReadImageBasicCompressed(m.input.data(),m.input.size()):m.ReadImageUncompressed(m.input.data(),m.input.size());
  int pe=image?-1:m.ReadPEHeaders();if(!image&&!pe)m.ObserveProtection();
  unsigned marker=0;bool marker_available=m.mem.bytes.size()>0x20044;if(marker_available)std::memcpy(&marker,m.mem.bytes.data()+0x20040,4);
  unsigned content=m.mem.bytes.size()>0x20000?m.mem.bytes[0x20000]:0,tail=m.mem.bytes.size()>0x30000?m.mem.bytes[0x30000]:0;
  std::printf("{\"image_return\":%d,\"pe_return\":%d,\"allocation_request\":%u,\"adapter_rounded\":%u,\"marker_available\":%s,\"marker\":%u,\"content\":%u,\"tail\":%u,\"permission_model\":[",image,pe,m.mem.requested,m.mem.rounded,marker_available?"true":"false",marker,content,tail);
  for(unsigned i=0;i<5;++i)std::printf("%s%u",i?",":"",m.mem.permissions[i]);
  std::printf("],\"trace\":[");for(size_t i=0;i<m.mem.events.size();++i)std::printf("%s\"%s\"",i?",":"",m.mem.events[i].c_str());
  size_t before=m.mem.bytes.size();m.mem.bytes.clear();std::printf("],\"adapter_cleanup_bytes\":%zu,\"full_xenia_load\":\"UNOBSERVED\",\"guest_invocations\":0}\n",before);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"observation error: %s\n",e.what());return 3;}
}
