// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#pragma once
#include "checked_memory.h"
#include <array>
#include <string>
namespace brutal::foundation {
struct DiagnosticContext {
  std::array<uint32_t,32> gpr{};
  uint32_t pc{}, lr{}, ctr{};
};
struct Outcome { bool ok{}; Fault fault{}; std::string message; DiagnosticContext snapshot; };
// A host-side diagnostic boundary. The snapshot is NOT an Xbox CONTEXT record,
// and catching an Error is NOT guest exception dispatch or stack unwinding.
template<class F> Outcome execute(const DiagnosticContext& context,F&& action) {
  try { action();return {true,{},"completed",context}; }
  catch(const Error& e){return {false,e.fault,e.what(),context};}
  catch(const std::exception& e){return {false,Fault::unsupported,e.what(),context};}
}
[[noreturn]] inline void unsupported_exception() {
  throw Error(Fault::unsupported,"guest context capture/unwind/dispatch is unavailable");
}
} // namespace brutal::foundation
