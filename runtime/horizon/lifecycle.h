// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "runtime/checked_memory.h"
namespace brutal::horizon {
// Controller policy for the original acceptance fixture, not an Xbox scheduler.
class Lifecycle {
 public:
 enum class Phase:uint32_t {uninitialized,initialized,image,contexts,entry,running,stop_requested,joined,cleanup,unloaded,terminated};
 private:Phase state_{Phase::uninitialized};
 public:
 Phase state()const{return state_;}
 void advance(Phase next,size_t allocations=0,size_t leases=0){
  diagnostic::PhaseBinding phase{uint32_t(state_)};
  if(uint32_t(next)!=uint32_t(state_)+1||state_==Phase::terminated)throw foundation::Error(foundation::Fault::argument,"acceptance lifecycle transition rejected");
  if((next==Phase::cleanup&&leases)||(next>=Phase::unloaded&&(allocations||leases)))throw foundation::Error(foundation::Fault::ownership,"acceptance lifecycle resources retained");
  state_=next;diagnostic::PhaseBinding changed{uint32_t(state_)};diagnostic::emit(diagnostic::Kind::lifecycle,uint32_t(allocations),uint32_t(leases));
 }
};
}
