// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "runtime/horizon/platform_lifecycle.h"
#include <iostream>
using namespace brutal::horizon;using platform::Lifecycle;
int main(){try{unsigned positive=0,refusals=0;auto check=[&](bool v){if(!v)throw std::runtime_error("lifecycle assertion");++positive;};auto no=[&](auto f){try{f();}catch(const Error&){++refusals;return;}throw std::runtime_error("missing lifecycle refusal");};
 std::vector<int> order;Lifecycle life;check(life.activate("memory",true,[]{return true;},[&]{order.push_back(1);}));check(life.activate("framebuffer",true,[]{return true;},[&]{order.push_back(2);}));check(life.active("memory")&&life.live()==2);
 no([&]{life.activate("memory",true,[]{return true;},[]{});});check(!life.activate("audio",false,[]{return false;},[&]{order.push_back(3);}));check(!life.active("audio")&&order==std::vector<int>{3});
 no([&]{life.activate("module",true,[]{return false;},[&]{order.push_back(4);});});check(order==std::vector<int>({3,4,2,1}));check(life.live()==0&&life.closed()==2&&life.cleanup_failures()==0);life.shutdown();check(order.size()==4);no([&]{life.activate("late",true,[]{return true;},[]{});});
 Lifecycle failure;check(failure.activate("owned",true,[]{return true;},[]{throw std::runtime_error("injected cleanup");}));failure.shutdown();check(failure.cleanup_failures()==1&&failure.live()==0);failure.shutdown();check(failure.cleanup_failures()==1);
 // Temporary heap-backed names and mutation of caller storage cannot alter the
 // owned identity or leave duplicate checks referring to freed storage.
 unsigned temporary_closed=0;Lifecycle named;const std::string long_name(100,'x');
 check(named.activate(std::string(long_name),true,[]{return true;},[&]{++temporary_closed;}));
 check(named.active(long_name));no([&]{named.activate(std::string(long_name),true,[]{return true;},[]{});});
 std::string mutable_name="caller-owned-name";check(named.activate(mutable_name,true,[]{return true;},[&]{++temporary_closed;}));mutable_name.assign(100,'y');
 check(named.active("caller-owned-name")&&!named.active(mutable_name));named.shutdown();check(temporary_closed==2&&named.live()==0);
 // Initialization may shut down existing services, but its own successfully
 // initialized service must be rolled back exactly once before returning.
 for(bool required:{false,true}){
  Lifecycle closing;std::vector<int> cleanup;
  check(closing.activate("prior",true,[]{return true;},[&]{cleanup.push_back(1);closing.shutdown();}));
  auto activate=[&]{return closing.activate("pending",required,[&]{closing.shutdown();return true;},[&]{cleanup.push_back(2);closing.shutdown();});};
  if(required)no([&]{activate();});else check(!activate());
  check(cleanup==std::vector<int>({1,2})&&closing.live()==0&&!closing.active("pending"));closing.shutdown();check(cleanup.size()==2);
 }
 // Nested activation refuses before it can initialize or claim a cleanup. If
 // the initializer catches that refusal, its outer activation can still finish.
 Lifecycle nested;unsigned inner_initialized=0,inner_closed=0,outer_closed=0;
 check(nested.activate("outer",true,[&]{no([&]{nested.activate("inner",true,[&]{++inner_initialized;return true;},[&]{++inner_closed;});});return true;},[&]{++outer_closed;nested.shutdown();}));
 check(nested.live()==1&&nested.active("outer")&&!nested.active("inner")&&inner_initialized==0&&inner_closed==0);nested.shutdown();check(outer_closed==1&&nested.live()==0);
 // An uncaught nested-activation refusal rolls back partial outer init and
 // required predecessors. The admission guard also resets after optional init.
 Lifecycle rejected;unsigned rolled_back=0,prior_closed=0;
 check(rejected.activate("prior",true,[]{return true;},[&]{++prior_closed;}));
 no([&]{rejected.activate("outer",true,[&]{return rejected.activate("inner",false,[]{return true;},[]{});},[&]{++rolled_back;rejected.shutdown();});});
 check(rolled_back==1&&prior_closed==1&&rejected.live()==0);
 Lifecycle retry;check(!retry.activate("failed",false,[]{throw std::runtime_error("init failed");return true;},[]{}));check(retry.activate("retry",true,[]{return true;},[]{}));retry.shutdown();check(retry.live()==0);
 std::cout<<"{\"positive\":"<<positive<<",\"refusals\":"<<refusals<<",\"live\":0,\"cleanup_errors_observed\":1}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
