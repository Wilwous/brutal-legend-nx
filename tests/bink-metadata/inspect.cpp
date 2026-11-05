// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Host-only metadata inspection tool. Never writes, decodes, or reads frames.
#include "runtime/horizon/bink_metadata.h"
#include "runtime/horizon/native_image_plan.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iomanip>
#include <iostream>
namespace b=brutal::horizon::resources::bink;
int main(int argc,char**argv){if(argc!=2)return 2;try{
 struct File {int fd;~File(){if(fd>=0)::close(fd);}} file{::open(argv[1],O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK)};
 struct stat before{},after{};if(file.fd<0||::fstat(file.fd,&before)||!S_ISREG(before.st_mode)||before.st_size<0)return 2;
 size_t bytes=0;auto m=b::inspect(uint64_t(before.st_size),[&](size_t offset,std::span<uint8_t> dest){auto got=::pread(file.fd,dest.data(),dest.size(),off_t(offset));if(got<0)return size_t(0);bytes+=size_t(got);return size_t(got);});
 if(::fstat(file.fd,&after)||before.st_size!=after.st_size||before.st_mtime!=after.st_mtime||before.st_ctime!=after.st_ctime)return 2;
 std::vector<uint8_t> canonical;auto word=[&](uint32_t v){for(unsigned i=0;i<4;++i)canonical.push_back(uint8_t(v>>(8*i)));};
 for(const auto& f:m.frames){word(f.offset);word(f.length);canonical.push_back(f.key_flag?1:0);}for(const auto&t:m.tracks){word(t.opaque);word(t.identifier);word(t.sample_rate);word(t.flags);}
 auto hash=brutal::horizon::native_image::digest(canonical);const auto& l=m.layout;
 std::cout<<"{\"extent\":"<<l.extent<<",\"frames\":"<<l.frames<<",\"tracks\":"<<l.tracks<<",\"width\":"<<l.width<<",\"height\":"<<l.height<<",\"rate_num\":"<<l.rate_num<<",\"rate_den\":"<<l.rate_den<<",\"metadata_bytes\":"<<bytes<<",\"payload_bytes_read\":0,\"index_sha256\":\"";for(auto v:hash)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(v);std::cout<<"\"}\n";
 }catch(const b::Error&e){std::cerr<<e.what()<<'\n';return 3;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
