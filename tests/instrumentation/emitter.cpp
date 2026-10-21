// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Independently encoded synthetic PPC words -> actual SDK decoder and builders.
#include <rex/codegen/template_registry.h>
#include <rex/codegen/binary_view.h>
#include <rex/codegen/function_graph.h>
#include <rex/codegen/config.h>
#include <rex/logging.h>
#include "builder_context.h"
#include "../ppc/disasm.h"
#include <fstream>
#include <array>
#include <filesystem>
namespace rex::codegen {
#define DECL(n) bool build_##n(BuilderContext&);
DECL(lbz) DECL(lhz) DECL(lha) DECL(lwz) DECL(lwa) DECL(ld) DECL(stb) DECL(sth) DECL(stw) DECL(std)
DECL(lwbrx) DECL(lhbrx) DECL(stwbrx) DECL(sthbrx) DECL(lvx) DECL(stvx)
DECL(lwarx) DECL(ldarx) DECL(stwcx) DECL(stdcx) DECL(lvlx) DECL(lvrx) DECL(stvlx) DECL(stvrx)
DECL(bl) DECL(bctrl) DECL(lfs) DECL(lfd) DECL(stfs) DECL(stfd)
}
using namespace rex::codegen;
struct Case{const char* name;uint32_t word;bool(*builder)(BuilderContext&);};
constexpr uint32_t d(unsigned op){return (op<<26)|(3<<21)|(4<<16);}
constexpr uint32_t x(unsigned xo,bool rc=false){return (31u<<26)|(3<<21)|(4<<16)|(5<<11)|(xo<<1)|rc;}
int main(int argc,char** argv){
 if(argc!=2)return 2;rex::InitLoggingEarly();std::filesystem::path work=argv[1];
 TemplateRegistry registry;
 std::string data=R"({"image_base":4096,"image_size":4096,"code_base":4096,"code_size":4096,"thunk_reserve_size":4096,"rexcrt_heap":false,"config_flags":{"skip_lr":false,"ctr_as_local":false,"xer_as_local":false,"reserved_as_local":false,"skip_msr":false,"cr_as_local":false,"non_argument_as_local":false,"non_volatile_as_local":false}})";
 std::ofstream(work/"synthetic_pch.h")<<registry.render("codegen/pch_h",data);
 std::ofstream(work/"synthetic_test_pch.h")<<registry.render("test/ppc_config_h",R"({"image_base":"1000","image_size":"1000","code_base":"1000","code_size":"1000"})");
 const Case cases[]{
  Case{"lbz",d(34),build_lbz},{"lhz",d(40),build_lhz},{"lha",d(42),build_lha},{"lwz",d(32),build_lwz},{"lwa",d(58)|2,build_lwa},{"ld",d(58),build_ld},
  {"stb",d(38),build_stb},{"sth",d(44),build_sth},{"stw",d(36),build_stw},{"std",d(62),build_std},
  {"lwbrx",x(534),build_lwbrx},{"lhbrx",x(790),build_lhbrx},{"stwbrx",x(662),build_stwbrx},{"sthbrx",x(918),build_sthbrx},
  {"lvx",x(103),build_lvx},{"stvx",x(231),build_stvx},{"lwarx",x(20),build_lwarx},{"ldarx",x(84),build_ldarx},{"stwcx",x(150,true),build_stwcx},{"stdcx",x(214,true),build_stdcx},
  {"lvlx",x(519),build_lvlx},{"lvrx",x(551),build_lvrx},{"stvlx",x(647),build_stvlx},{"stvrx",x(679),build_stvrx},
  {"mm_lwz",d(32),build_lwz},{"mm_stw",d(36),build_stw},
  {"lfs",d(48),build_lfs},{"lfd",d(50),build_lfd},{"stfs",d(52),build_stfs},{"stfd",d(54),build_stfd},
  {"direct",0x48001001,build_bl},{"indirect",0x4e800421,build_bctrl}};
 BinaryView binary;RecompilerConfig config;FunctionGraph graph;
 auto* callee=graph.addFunction(0x2000,4,FunctionAuthority::CONFIG,std::string_view("synthetic_callee"));
 std::ofstream output(work/"synthetic.inc");output<<"#include \"synthetic_pch.h\"\nvoid synthetic_callee(PPCContext&,uint8_t*);\n";
 for(const auto& test:cases){
  std::array<uint8_t,4> bytes{uint8_t(test.word>>24),uint8_t(test.word>>16),uint8_t(test.word>>8),uint8_t(test.word)};ppc_insn instruction{};
  if(rex::codegen::ppc::Disassemble(bytes.data(),4,0x1000,instruction)!=4)return 3;
  std::string expected=test.name;
  if(expected.starts_with("mm_"))expected.erase(0,3);
  if(expected=="direct")expected="bl";
  if(expected=="indirect")expected="bctrl";
  if(expected=="stwcx"||expected=="stdcx")expected+=".";
  if(!instruction.opcode||expected!=instruction.opcode->name)return 6;
  auto* source=graph.getFunction(0x1000);if(!source)source=graph.addFunction(0x1000,4,FunctionAuthority::CONFIG,std::string_view("synthetic_source"));
  if(std::string_view(test.name)=="direct")graph.addCallToFunction(0x1000,0x1000,CallTarget::function(callee));
  const auto& function=*source;
  EmitContext emit{binary,config,graph};std::string body;RecompilerLocalVariables locals;CSRState csr=CSRState::Unknown;
  if(std::string_view(test.name).starts_with("mm_"))locals.set_mmio_base(4);
  BuilderContext ctx{body,emit,function,instruction,0x1000,nullptr,locals,csr};
  if(!test.builder(ctx))return 4;
  output<<"void syn_"<<test.name<<"(PPCContext& ctx,uint8_t* base){uint32_t ea{};PPCRegister temp{},reserved{};PPCVRegister vTemp{};\n"<<body<<"}\n";
 }
 std::ofstream(work/"emission.json")<<"{\"decoded_instructions\":32,\"result\":\"PASS\"}\n";
 return output?0:5;
}
