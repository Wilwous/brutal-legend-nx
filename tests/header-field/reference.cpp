// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Optional separately supplied pinned host parser; no downloaded source in Git.
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include "xex.h"
int main(int argc,char**argv){try{
 for(int i=1;i<argc;++i){
  std::ifstream file(argv[i],std::ios::binary);std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(file),{}};
  if(bytes.size()<24||bytes.size()>256)throw std::runtime_error("fixture length");
  auto word=[&](unsigned n){return uint32_t(bytes[n])<<24|uint32_t(bytes[n+1])<<16|uint32_t(bytes[n+2])<<8|bytes[n+3];};
  if(word(0)!=0x58455832||word(8)!=bytes.size()||word(20)>(bytes.size()-24)/8)throw std::runtime_error("fixture bounds");
  auto* p=static_cast<const uint8_t*>(getOptHeaderPtr(bytes.data(),0x20401));
  if(p&&(uintptr_t(p)<uintptr_t(bytes.data())||uintptr_t(p)+4>uintptr_t(bytes.data())+bytes.size()))throw std::runtime_error("parser pointer outside source");
  std::printf("{\"case\":\"%s\",\"offset\":%td}\n",std::filesystem::path(argv[i]).stem().string().c_str(),p?p-bytes.data():ptrdiff_t(-1));
 }return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}}
