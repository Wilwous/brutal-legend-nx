// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Analysis-only SDK data carrier. No Runtime, XexModule, dispatcher or PPC call.
#include <rex/codegen/codegen_context.h>
#include <rex/system/module.h>
#include <rex/logging.h>
#include <nlohmann/json.hpp>
#include <rex/codegen/template_registry.h>
#include <fmt/format.h>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdio>
using Json=nlohmann::json;
struct Slice {uint32_t pc;std::vector<uint8_t> bytes;};
class Slices final:public rex::runtime::Module {
 std::string name_="offline-analysis-only";uint32_t base_,extent_,entry_;
 public:
 Slices(const std::vector<Slice>&s,uint32_t entry):Module(nullptr),base_(s.front().pc),entry_(entry){
  uint64_t end=base_;for(auto&v:s){end=std::max(end,uint64_t(v.pc)+v.bytes.size());binary_sections_.push_back({"offline",v.pc,uint32_t(v.bytes.size()),v.bytes.data(),true,false});}
  if(end>UINT32_MAX||end<=base_)throw std::runtime_error("envelope bounds");extent_=end-base_;
 }
 const std::string&name()const override{return name_;}bool is_executable()const override{return false;}
 uint32_t base_address()const override{return base_;}uint32_t image_size()const override{return extent_;}uint32_t entry_point()const override{return entry_;}
 bool ContainsAddress(uint32_t pc)override{for(auto&s:binary_sections_)if(pc>=s.virtual_address&&uint64_t(pc)<uint64_t(s.virtual_address)+s.virtual_size)return true;return false;}
};
int main(int argc,char**argv){if(argc!=3)return 2;try{
 rex::InitLoggingEarly();std::filesystem::path input=argv[1],output=argv[2];if(std::filesystem::exists(output))throw std::runtime_error("fresh output required");
 if(std::filesystem::file_size(input)>1048576)throw std::runtime_error("input budget");Json j;{std::ifstream f(input);f>>j;}
 if(j.at("offline_only")!=true||j.at("runtime_owner_verified")!=false)throw std::runtime_error("not an offline contract");
 std::vector<Slice>slices;uint64_t previous=0;size_t total=0;
 for(auto&v:j.at("slices")){Slice s{v.at("pc").get<uint32_t>(),v.at("bytes").get<std::vector<uint8_t>>()};if(s.bytes.empty()||s.bytes.size()%4||s.pc%4||s.pc<previous||uint64_t(s.pc)+s.bytes.size()>UINT32_MAX)throw std::runtime_error("slice bounds");previous=uint64_t(s.pc)+s.bytes.size();total+=s.bytes.size();slices.push_back(std::move(s));}
 if(slices.empty()||slices.size()>32||total>4096)throw std::runtime_error("slice budget");
 for(const auto&s:slices)for(size_t i=0;i<s.bytes.size();i+=4){uint32_t w=(uint32_t(s.bytes[i])<<24)|(uint32_t(s.bytes[i+1])<<16)|(uint32_t(s.bytes[i+2])<<8)|s.bytes[i+3];auto op=w>>26;if(op!=14&&op!=15&&op!=18&&op!=31&&op!=36&&op!=37&&op!=62&&w!=0x4e800020&&w!=0x60000000)throw std::runtime_error("unsupported offline opcode");if(op==62&&(w&3))throw std::runtime_error("unsupported DS form");if(op==31){auto xo=(w>>1)&1023,spr=((w>>16)&31)|((w>>6)&0x3e0);if(!((xo==339&&spr==8)||(xo==444&&((w>>21)&31)==((w>>11)&31)&&!(w&1))))throw std::runtime_error("unsupported X form");}}
 Slices module(slices,j.at("entry"));if(module.memory()||module.function_dispatcher()||module.is_executable())throw std::runtime_error("runtime forbidden");
 auto view=rex::codegen::BinaryView::fromModule(module);if(view.sections().size()!=slices.size())throw std::runtime_error("SDK dropped a slice");
 rex::codegen::RecompilerConfig config;
 for(auto&h:j.at("hooks")){rex::codegen::MidAsmHook hook;hook.name=h.at("name");hook.ret=h.at("exit");auto pc=h.at("pc").get<uint32_t>();if(!config.midAsmHooks.emplace(pc,std::move(hook)).second)throw std::runtime_error("duplicate hook");}
 auto ctx=rex::codegen::CodegenContext::Create(std::move(view),std::move(config));
 using namespace rex::codegen;
 for(auto&f:j.at("functions")){uint32_t start=f.at("start"),end=f.at("end");if(end<=start||!ctx.binary().translate(start)||!ctx.binary().translate(end-1))throw std::runtime_error("function extent");if(std::none_of(slices.begin(),slices.end(),[&](const Slice&s){return s.pc==start&&uint64_t(s.pc)+s.bytes.size()==end;}))throw std::runtime_error("function not an exact slice");auto*n=ctx.graph.addFunction(start,end-start,FunctionAuthority::CONFIG,true);std::vector<Block>blocks;for(auto&b:j.at("blocks"))if(b.at("owner")==start)blocks.push_back({b.at("start"),b.at("end").get<uint32_t>()-b.at("start").get<uint32_t>()});uint32_t cursor=start;for(const auto&block:blocks){if(block.base!=cursor||block.size==0||block.end()>end||block.size%4)throw std::runtime_error("block coverage");cursor=block.end();}if(cursor!=end)throw std::runtime_error("missing block");n->discover(std::move(blocks),{},{});ctx.graph.setFunctionName(start,"offline_"+std::to_string(start));}
 for(auto&c:j.at("calls")){auto*target=ctx.graph.getFunction(c.at("target"));if(!target)throw std::runtime_error("unknown admitted call");ctx.graph.addCallToFunction(c.at("owner"),c.at("pc"),CallTarget::function(target));}
 for(auto&[pc,n]:ctx.graph.functions())n->seal();
 std::filesystem::create_directories(output);std::ofstream out(output/"region.inc");std::vector<uint32_t>order;for(auto&[pc,n]:ctx.graph.functions())order.push_back(pc);std::sort(order.begin(),order.end());
 EmitContext emit{ctx.binary(),ctx.Config(),ctx.graph,j.at("entry"),nullptr};
 for(auto pc:order){auto body=ctx.graph.getFunction(pc)->emitCpp(emit);if(body.empty())throw std::runtime_error("empty production emission");out<<body<<'\n';}out.close();if(!out)throw std::runtime_error("output failure");
 TemplateRegistry templates;Json args{{"image_base",fmt::format("{:X}",ctx.binary().baseAddress())},{"image_size",fmt::format("{:X}",ctx.binary().imageSize())},{"code_base",fmt::format("{:X}",ctx.binary().baseAddress())},{"code_size",fmt::format("{:X}",ctx.binary().imageSize())}};
 std::ofstream configOut(output/"ppc_config.h");configOut<<templates.render("test/ppc_config_h",args.dump());if(!configOut)throw std::runtime_error("template output");
 std::printf("{\"result\":\"PASS\",\"slices\":%zu,\"instruction_bytes\":%zu,\"runtime_created\":false,\"guest_allocations\":0,\"guest_invocations\":0}\n",slices.size(),total);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"OFFLINE GENERATION REFUSED: %s\n",e.what());return 1;}}
