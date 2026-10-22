// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Child-only macOS dyld interposer for the pinned SDK member ABI. No success mock.
#include <atomic>
#include <cstdint>
#include <new>
namespace rex::memory {class Memory;}
extern "C" uint32_t sdk_original_alloc(rex::memory::Memory*,uint32_t,uint32_t,uint32_t,const void*)
 __asm__("__ZN3rex6memory6Memory15SystemHeapAllocEjjjPKv");
static std::atomic<unsigned> mode{0},failures{0};
extern "C" void fixture_arm_header_failure(unsigned value){mode=value;}
extern "C" unsigned fixture_header_failure_count(){return failures.load();}
extern "C" uint32_t fixture_header_alloc(rex::memory::Memory* memory,uint32_t size,uint32_t alignment,uint32_t flags,const void* owner){
 auto armed=mode.load();if(armed&&owner&&size==4096){++failures;if(armed==2)throw std::bad_alloc();return 0;}
 return sdk_original_alloc(memory,size,alignment,flags,owner);
}
__attribute__((used,section("__DATA,__interpose")))static const struct{const void* replacement;const void* original;} interpose[]{
 {reinterpret_cast<const void*>(fixture_header_alloc),reinterpret_cast<const void*>(sdk_original_alloc)}};
