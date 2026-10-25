// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include "runtime/horizon/leaf.h"
#include <thread>
#include <cstdio>
REX_EXTERN(leaf_82010000);
int main(){
  auto a=run_leaf_probe();LeafResult b;
  // Native thread-local binding cannot be borrowed by another host thread.
  brutal::foundation::Memory memory(4096);PPCContext ctx{};
  brutal::horizon::LeafScope bound(memory,ctx);bool refused=false;
  std::thread worker([&]{
    try{leaf_82010000(ctx,nullptr);}catch(const brutal::foundation::Error& e){refused=e.fault==brutal::foundation::Fault::no_thread;}
    b=run_leaf_probe();
  });worker.join();
  const bool isolated=refused&&bound.entries==0;
  unsigned pass=a.passed+b.passed+(isolated?1:0),fail=a.failed+b.failed+(isolated?0:1);
  std::printf("{\"passed\":%u,\"failed\":%u,\"per_probe\":%u,\"entries_per_probe\":%u,\"loads_per_probe\":%u,\"stores_per_probe\":%u}\n",pass,fail,a.passed,a.invocations,a.reads,a.writes);
  return fail?1:0;
}
