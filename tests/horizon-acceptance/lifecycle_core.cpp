// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/lifecycle.h"
#include "runtime/horizon/probe_control.h"
#include <cstdio>
#include <memory>
using namespace brutal::horizon;
int main(){unsigned pass=0,fail=0;auto check=[&](bool b){b?++pass:++fail;};auto refuse=[&](auto fn){try{fn();check(false);}catch(const brutal::foundation::Error&){check(true);}};using P=Lifecycle::Phase;auto trace=std::make_unique<brutal::diagnostic::Trace>();brutal::diagnostic::Binding root(trace.get(),91,17);Lifecycle life;
 refuse([&]{life.advance(P::running);});check(life.state()==P::uninitialized);
 for(unsigned n=1;n<=7;++n){life.advance(P(n));check(life.state()==P(n));refuse([&]{life.advance(P(n));});}
 refuse([&]{life.advance(P::cleanup,4,1);});check(life.state()==P::joined);life.advance(P::cleanup,4,0);refuse([&]{life.advance(P::unloaded,4,0);});life.advance(P::unloaded);life.advance(P::terminated);refuse([&]{life.advance(P::terminated);});
 auto events=trace->snapshot();check(events.back().phase==uint32_t(P::terminated));check(events.back().kind==brutal::diagnostic::Kind::refusal);check(brutal::diagnostic::context.phase==0);
 {brutal::diagnostic::PhaseBinding phase(6);brutal::diagnostic::AllocationBinding allocation(55,9);try{throw brutal::foundation::Error(brutal::foundation::Fault::protection,"original refusal");}catch(const brutal::foundation::Error&){}}auto e=trace->snapshot().back();check(e.phase==6&&e.allocation_owner==55&&e.allocation_id==9);check(brutal::diagnostic::context.allocation_id==0);
 auto copy=events;for(auto&event:copy){if(event.module)event.module+=500;if(event.thread)event.thread+=1000;}check(brutal::diagnostic::canonical(events)==brutal::diagnostic::canonical(copy));copy.back().pc=4;check(brutal::diagnostic::canonical(events)!=brutal::diagnostic::canonical(copy));check(trace->dropped()==0&&!trace->broken());ProbeControl control;{ProbeControl::Activation active(control);check(ProbeControl::current()==&control);refuse([&]{ProbeControl::Activation denied(control);});ProbeControl::mark(3);check(control.stage()==3);for(unsigned i=0;i<18;++i)control.failure("original failure");check(control.failures().used==16&&control.failures().dropped==2);check(control.request_cancel()&&!control.request_cancel());refuse([&]{ProbeControl::poll();});}check(ProbeControl::current()==nullptr);std::printf("{\"passed\":%u,\"failed\":%u}\n",pass,fail);return fail?1:0;}
