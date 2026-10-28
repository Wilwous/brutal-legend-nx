// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <initializer_list>
namespace brutal::acceptance {
struct Counts {unsigned passed{},failed{};};
inline Counts platform_checks(bool console,bool started,bool joined,bool callback){Counts c;for(bool ok:{console,started,joined,callback})ok?++c.passed:++c.failed;return c;}
// Keep UI alive while retaining native ownership. A timeout requests cooperative
// cancellation; it never closes a live task or claims a native wait is bounded.
template<class UI,class Task,class Control>void supervise(UI&ui,Task&task,Control&control){
 for(;;){if(task.join(0))return;bool running=ui.running();if(running)ui.update_input();if(!running||(running&&ui.plus_pressed())||ui.elapsed_ms()>=45000)control.request_cancel();if(running){ui.progress(control.stage(),control.cancelled());ui.present();}ui.yield_frame();}
}
}
