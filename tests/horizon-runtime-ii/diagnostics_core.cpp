// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/native.h"
#include <cstdio>
#include <memory>
using namespace brutal::horizon;using brutal::foundation::Error;using brutal::foundation::Fault;
int main(){unsigned passed=0,failed=0;auto check=[&](bool b){b?++passed:++failed;};using namespace brutal::diagnostic;
 auto trace=std::make_unique<Trace>();check(trace->snapshot().empty()&&trace->dropped()==0);{Binding a(trace.get(),17,3);site(0x82000000);emit(Kind::function_enter);{Binding b(trace.get(),18,4);site(0x82000004);emit(Kind::refusal,7);}emit(Kind::cleanup);}
 auto events=trace->snapshot();check(events.size()==3);check(events[0].module==17&&events[0].thread==3&&events[0].pc==0x82000000);check(events[1].module==18&&events[1].thread==4&&events[1].detail==7);check(events[2].module==17&&events[2].thread==3&&events[2].pc==0x82000000);check(current_trace()==nullptr);
 trace->clear();for(size_t i=0;i<Trace::capacity+8;++i)trace->write({Kind::instruction,0,0,0,1,1});check(trace->snapshot().size()==Trace::capacity);check(trace->dropped()==8&&!trace->broken());trace->clear();
 std::array<NativeThread,4> workers;for(unsigned i=0;i<4;++i){check(workers[i].start([&,i]{Binding binding(trace.get(),50,i+1);for(unsigned n=0;n<32;++n){site(0x82000100+n*4);emit(Kind::function_enter);}}));}for(auto&w:workers){check(w.join()&&!w.callback_error());}
 events=trace->snapshot();std::array<unsigned,4> counts{};for(auto e:events){if(e.module==50&&e.thread>=1&&e.thread<=4&&e.pc>=0x82000100)++counts[e.thread-1];}check(events.size()==128);check(counts==std::array<unsigned,4>{32,32,32,32});check(trace->dropped()==0&&!trace->broken());
 {Binding binding(trace.get(),3,9);try{throw Error(Fault::protection,"original private-data-free test");}catch(const Error&e){check(e.fault==Fault::protection);}}check(trace->snapshot().back().kind==Kind::refusal);check(trace->snapshot().back().detail==uint32_t(Fault::protection));
 std::printf("{\"passed\":%u,\"failed\":%u}\n",passed,failed);return failed?1:0;}
