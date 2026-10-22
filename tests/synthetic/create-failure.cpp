// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original macOS injection library, loaded only into its bounded synthetic child.
#include <atomic>
#include <cerrno>
#include <pthread.h>
#include <semaphore.h>
static std::atomic<bool> armed{false};
static std::atomic<unsigned> failures{0},sem_closes{0};
extern "C" void fixture_arm_create_failure(){armed=true;}
extern "C" unsigned fixture_create_failure_count(){return failures.load();}
extern "C" unsigned fixture_sem_close_count(){return sem_closes.load();}
extern "C" int fixture_sem_close(sem_t* semaphore){if(armed)++sem_closes;return sem_close(semaphore);}
extern "C" int fixture_pthread_create(pthread_t* thread,const pthread_attr_t* attr,void*(*start)(void*),void* arg){
 if(armed){++failures;return EAGAIN;}
 // dyld leaves references from this interposing image bound to the original.
 return pthread_create(thread,attr,start,arg);
}
__attribute__((used,section("__DATA,__interpose"))) static const struct{const void* replacement;const void* original;} interpose[]{
 {reinterpret_cast<const void*>(fixture_pthread_create),reinterpret_cast<const void*>(pthread_create)},
 {reinterpret_cast<const void*>(fixture_sem_close),reinterpret_cast<const void*>(sem_close)}};
