// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
namespace brutal::probe {
enum class ExitReason { plus, applet_shutdown };
// No elapsed-time exit: retain the diagnostic screen until PLUS or shutdown.
template<class Platform> ExitReason wait_for_exit(Platform& platform) {
  for(;;) {
    if(!platform.running())return ExitReason::applet_shutdown;
    platform.update_input();
    if(platform.plus_pressed())return ExitReason::plus;
    platform.present();
    platform.yield_frame();
  }
}
}
