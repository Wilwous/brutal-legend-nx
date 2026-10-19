// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original adapter: compiles the pinned open-source emitter case, not game code.
#define FMT_HEADER_ONLY
#include <fmt/format.h>
#include <array>
#include <iostream>
#include <string>
struct Context {
  struct { std::array<unsigned, 8> operands{}; } insn;
  unsigned base = 0;
  std::string temp() const { return "tmp"; }
  std::string v_temp() const { return "scratch"; }
  std::string v(unsigned n) const { return n == 0 ? "d" : "s"; }
  template<class... A> void println(const char* f, A&&... a) {
    std::cout << fmt::format(fmt::runtime(f), std::forward<A>(a)...) << '\n';
  }
};
#define REXCODEGEN_WARN(...) ((void)0)
int main() {
  std::cout << "#include <cstdint>\n#include <iostream>\n#include <bit>\n"
    "union Vec { uint8_t u8[16]; uint32_t u32[4]; uint16_t u16[8]; float f32[4]; };\n"
    "union Reg { uint32_t u32; uint16_t u16; float f32; };\n";
  for (unsigned mask : {2u, 3u}) for (unsigned shift : {0u, 1u, 2u})
    for (unsigned alias : {0u, 1u}) {
      Context ctx;
      ctx.insn.operands = {0, alias ? 0u : 1u, 5, mask, shift};
      std::cout << "void p" << mask << shift << alias
        << "(Vec& d, [[maybe_unused]] const Vec& s) { Reg tmp{}; Vec scratch{};\n";
      switch (5) {
#include "emitter_case.inc"
      }
      std::cout << "}\n";
    }
  std::cout << R"(
int main() {
  static_assert(std::endian::native == std::endian::little);
  unsigned mode;
  while (std::cin >> std::dec >> mode) {
    Vec s{}, d{};
    for (int i=3;i>=0;--i) std::cin >> std::hex >> s.u32[i];
    for (int i=3;i>=0;--i) std::cin >> std::hex >> d.u32[i];
    switch (mode) {
)";
  unsigned mode = 0;
  for (unsigned mask : {2u,3u}) for (unsigned shift : {0u,1u,2u})
    for (unsigned alias : {0u,1u}) {
      std::cout << "case " << mode++ << ": ";
      if (alias) std::cout << "d=s; ";
      std::cout << "p" << mask << shift << alias << "(d,s); break;\n";
    }
  std::cout << "default: return 2; }\n"
    "for(int i=3;i>=0;--i) std::cout << std::hex << d.u32[i] << ' ';"
    "std::cout << '\\n' << std::dec; } }\n";
}
