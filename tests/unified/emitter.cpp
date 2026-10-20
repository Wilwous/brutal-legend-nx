// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Calls the actual newly built SDK vector builder on original synthetic operands.
#include <rex/codegen/binary_view.h>
#include <rex/codegen/function_graph.h>
#include <rex/codegen/config.h>
#include <rex/logging.h>
#include "builder_context.h"
#include <dis-asm.h>
#include <fstream>
#include <string>
namespace rex::codegen { bool build_vpkd3d128(BuilderContext&); }
int main(int argc,char** argv){
 if(argc!=2)return 2;rex::InitLoggingEarly();std::ofstream file(argv[1]);
 file<<"#include <rex/ppc.h>\n#include <cstring>\n#include <cstdio>\n";
 rex::codegen::BinaryView binary;rex::codegen::RecompilerConfig config;rex::codegen::FunctionGraph graph;
 rex::codegen::EmitContext emit{binary,config,graph};rex::codegen::FunctionNode fn(0x1000,4,rex::codegen::FunctionAuthority::CONFIG);
 int index=0;
 for(unsigned mask:{2u,3u})for(unsigned shift:{0u,1u,2u})for(unsigned alias:{0u,1u}){
  std::string out;ppc_insn insn{};insn.operands[0]=0;insn.operands[1]=alias?0:1;insn.operands[2]=5;insn.operands[3]=mask;insn.operands[4]=shift;
  rex::codegen::RecompilerLocalVariables locals;rex::codegen::CSRState csr=rex::codegen::CSRState::Unknown;
  rex::codegen::BuilderContext ctx{out,emit,fn,insn,0x1000,nullptr,locals,csr};
  if(!rex::codegen::build_vpkd3d128(ctx)||out.find("const PPCVRegister vpkSource")==std::string::npos)return 3;
  file<<"void f"<<index++<<"(PPCContext& ctx){PPCRegister temp{};PPCVRegister vTemp{};\n"<<out<<"}\n";
 }
 file<<"int main(){void(*functions[])(PPCContext&)={";
 for(int n=0;n<12;++n)file<<(n?",":"")<<"f"<<n;
 file<<"};unsigned invocations=0;for(int n=0;n<257;++n){for(int mode=0;mode<12;mode+=2){PPCContext a{},b{};a.fpscr.InitHost();b.fpscr.InitHost();for(int lane=0;lane<4;++lane){a.v0.f32[lane]=float(n*4+lane-512)/8.0f;}a.v1=a.v0;b.v0=a.v0;b.v1=a.v0;functions[mode](a);functions[mode+1](b);invocations+=2;if(std::memcmp(&a.v0,&b.v0,16))return 1;}}std::printf(\"{\\\"group\\\":\\\"unified_codegen_alias_regression\\\",\\\"result\\\":\\\"PASS\\\",\\\"invocations\\\":%u}\\n\",invocations);}\n";
 return file?0:4;
}
