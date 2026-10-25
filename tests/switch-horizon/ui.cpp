// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "result_ui.h"
#include <cstdio>
#include <stdexcept>
struct Fake {
  unsigned frame=0,inputs=0,presents=0,yields=0;
  unsigned plus_at,shutdown_at;
  bool running(){if(frame>1000)throw std::runtime_error("UI failed to exit");return frame<shutdown_at;}
  void update_input(){++inputs;}
  bool plus_pressed(){return frame==plus_at;}
  void present(){++presents;}
  void yield_frame(){++yields;++frame;}
};
int main(){
  unsigned passed=0,failed=0;
  auto check=[&](bool value){++(value?passed:failed);};
  using namespace brutal::probe;
  Fake delayed{0,0,0,0,300,999};
  check(wait_for_exit(delayed)==ExitReason::plus);
  check(delayed.frame==300 && delayed.presents==300 && delayed.inputs==301);
  Fake immediate{0,0,0,0,0,999};
  check(wait_for_exit(immediate)==ExitReason::plus && immediate.presents==0);
  Fake shutdown{0,0,0,0,999,25};
  check(wait_for_exit(shutdown)==ExitReason::applet_shutdown && shutdown.yields==25);
  Fake already_closed{0,0,0,0,0,0};
  check(wait_for_exit(already_closed)==ExitReason::applet_shutdown && already_closed.inputs==0);
  Fake no_timer_exit{0,0,0,0,999,900};
  check(wait_for_exit(no_timer_exit)==ExitReason::applet_shutdown && no_timer_exit.presents==900);
  std::printf("{\"passed\":%u,\"failed\":%u}\n",passed,failed);
  return failed?1:0;
}
