// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Child-only macOS dyld interposer for the pinned SDK member ABI. No success mock.
#include <atomic>
#include <cstdint>
#include <new>
namespace rex::memory {class Memory;}
extern "C" uint32_t sdk_original_alloc(rex::memory::Memory*,uint32_t,uint32_t,uint32_t,const void*)
 __asm__("__ZN3rex6memory6Memory15SystemHeapAllocEjjjPKv");
static std::atomic<unsigned> mode{0},failures{0},selected_size{0},tls_address{0};
extern "C" void fixture_arm_tls_failure(unsigned value,unsigned size){selected_size=size;tls_address=0;mode=value;}
extern "C" unsigned fixture_tls_failure_count(){return failures.load();}
extern "C" unsigned fixture_tls_address(){return tls_address.load();}
extern "C" uint32_t fixture_tls_alloc(rex::memory::Memory* memory,uint32_t size,uint32_t alignment,uint32_t flags,const void* owner){
 auto armed=mode.load();if(armed&&size==selected_size){++failures;if(armed==2)throw std::bad_alloc();return 0;}
 auto address=sdk_original_alloc(memory,size,alignment,flags,owner);
 if(size==288)tls_address=address;return address;
}
__attribute__((used,section("__DATA,__interpose")))static const struct{const void* replacement;const void* original;} interpose[]{
 {reinterpret_cast<const void*>(fixture_tls_alloc),reinterpret_cast<const void*>(sdk_original_alloc)}};
