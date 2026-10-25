// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "probe.h"
#include <cstdio>
int main(){
  const auto r=portable_probe();
  std::printf("{\"passed\":%u,\"failed\":%u}\n",r.passed,r.failed);
  return r.failed?1:0;
}
