// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include <atomic>
#include <stdexcept>
namespace brutal::synthetic {
// Host orchestration for bounded original fixtures. This is not asynchronous
// guest cancellation: poll only after a whole generated entry/import returns.
class CooperativeStop {
 public:
  enum class State { running,requested,boundary,exited,joined,reclaimed,unloaded };
 private:
  std::atomic<State> state_{State::running};
  void advance(State from,State to){if(!state_.compare_exchange_strong(from,to))throw std::logic_error("stop lifecycle order");}
 public:
  State state()const{return state_.load();}
  bool request(){auto from=State::running;return state_.compare_exchange_strong(from,State::requested);}
  bool safe_boundary(){auto from=State::requested;return state_.compare_exchange_strong(from,State::boundary);}
  void exited(){advance(State::boundary,State::exited);}
  void joined(){advance(State::exited,State::joined);}
  void reclaimed(){advance(State::joined,State::reclaimed);}
  void unloaded(){advance(State::reclaimed,State::unloaded);}
};
}
