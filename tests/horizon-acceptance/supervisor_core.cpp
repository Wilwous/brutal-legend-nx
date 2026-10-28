// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include <initializer_list>
#include "supervisor.h"
#include <cstdio>
#include <stdexcept>
struct UI {unsigned frame{},plus=999,shutdown=999,draws{},inputs{};bool running(){return frame<shutdown;}void update_input(){++inputs;}bool plus_pressed(){return frame==plus;}unsigned elapsed_ms(){return frame*15000;}void progress(unsigned,bool){++draws;}void present(){}void yield_frame(){if(++frame>10)throw std::runtime_error("UI failed to retain/join task");}};
struct Task {UI&ui;unsigned done;bool joined{};bool join(unsigned timeout){if(timeout||joined)throw std::runtime_error("join contract");if(ui.frame<done)return false;joined=true;return true;}};
struct Control {bool was_cancelled{};unsigned requests{};bool cancelled()const{return was_cancelled;}void request_cancel(){was_cancelled=true;++requests;}unsigned stage(){return 3;}};
int main(){unsigned passed=0,failed=0;auto check=[&](bool b){b?++passed:++failed;};for(unsigned bits=0;bits<16;++bits){auto c=brutal::acceptance::platform_checks(bits&1,bits&2,bits&4,bits&8);unsigned count=0;for(unsigned i=0;i<4;++i)count+=(bits>>i)&1;check(c.passed==count&&c.failed==4-count);}
 {UI ui;Task task{ui,2};Control c;brutal::acceptance::supervise(ui,task,c);check(task.joined&&!c.was_cancelled&&ui.draws==2);}
 {UI ui;ui.plus=0;Task task{ui,2};Control c;brutal::acceptance::supervise(ui,task,c);check(task.joined&&c.was_cancelled&&ui.frame==2);}
 {UI ui;Task task{ui,5};Control c;brutal::acceptance::supervise(ui,task,c);check(task.joined&&c.was_cancelled&&ui.frame==5&&ui.draws==5);}
 {UI ui;ui.shutdown=1;Task task{ui,3};Control c;brutal::acceptance::supervise(ui,task,c);check(task.joined&&c.was_cancelled&&ui.inputs==1&&ui.draws==1);}
 auto expected=brutal::acceptance::platform_checks(true,true,true,true);std::printf("{\"passed\":%u,\"failed\":%u,\"expected_platform_passed\":%u}\n",passed,failed,expected.passed);return failed?1:0;}
