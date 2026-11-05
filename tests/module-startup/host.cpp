// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
#include "program.h"
#include "runtime/horizon/variable_imports.h"
#include <fstream>
#include <iostream>
using namespace brutal::horizon;namespace ni=brutal::horizon::native_image;namespace nm=brutal::horizon::native_module;namespace hb=brutal::horizon::header_buffer;
unsigned pos{},neg{},queries{},runs{},stops{},zero_entries{};
void check(bool b){if(!b)throw std::runtime_error("startup assertion");++pos;}
template<class F>void no(F f,bool preentry=false){auto before=Scope::guest_entries.load();try{f();}catch(const Error&){++neg;if(preentry){check(Scope::guest_entries==before);++zero_entries;}return;}throw std::runtime_error("missing startup refusal");}
struct Frame {Scope&scope;PPCContext&ctx;ModuleLife&module;Space&space;nm::Binding::Reader&binding;bool exceptional{},instruction{};inline static thread_local Frame* current{};Frame(Scope&s,PPCContext&c,ModuleLife&m,Space&sp,nm::Binding::Reader&b):scope(s),ctx(c),module(m),space(sp),binding(b){if(current)throw Error(Fault::no_thread,"nested query frame");current=this;}~Frame(){current=nullptr;}};
void module_poll(uint32_t pc){auto*f=Frame::current;if(!f)throw Error(Fault::no_thread,"no native query frame");f->scope.point(pc);if(pc==program_query){
 ni::require(f->scope.belongs_to(f->module,f->space),"query context module/space");if(f->exceptional)Scope::deny_exception(f->ctx);if(f->instruction)Scope::deny_instruction(f->ctx);
 auto field=f->binding.field(f->ctx.r3.u32,f->ctx.r4.u32);f->ctx.r3.u64=field.address();++queries;
}}
int main(int argc,char**argv){if(argc!=2)return 2;try{
 std::ifstream file(argv[1],std::ios::binary);std::vector<uint8_t> source{std::istreambuf_iterator<char>(file),{}};
 Space space(4*1024*1024);auto plan=program_plan();ni::Image image(space,plan,source,program_identity.plan);ModuleLife module(plan.module);Slots slots;auto token=slots.allocate();auto initial=space.read(plan.tls_address,plan.tls_file_size);
 auto functions=std::span(module_functions,module_function_count);ni::Binding code(image,space,module,program_identity,program_identity,functions,program_bounds,program_control,functions,ni::ImportPolicy::no_function_imports);
 no([&]{ni::Binding wrong(image,space,module,program_identity,program_identity,functions,program_bounds,program_control,functions);},true);
 no([&]{ni::Binding wrong(image,space,module,program_identity,program_identity,functions,program_bounds,program_control,functions,ni::ImportPolicy(99));},true);
 auto bad=program_identity;bad.generated[0]^=1;no([&]{ni::Binding wrong(image,space,module,bad,program_identity,functions,program_bounds,program_control,functions,ni::ImportPolicy::no_function_imports);},true);
 hb::Source header_plan{module.identity,module.instance,0,4096,0x40000000,ni::digest(source),ni::digest(std::span(source).first(4096))};hb::Buffer header(space,module,header_plan,source,hb::identity(header_plan));
 nm::Metadata metadata(space,module,header,header.reference(),0x41000000);nm::VariableKey key{"xboxkrnl.exe",0x193,nm::ImportKind::variable};nm::Provider provider(space,module,metadata,metadata.reference(),key,0x42000000);nm::Registry registry;auto provider_ref=provider.reference();registry.publish(provider);nm::Binding variable(registry,space,module,key,provider_ref,0x43000000);auto variable_ref=variable.reference();
 auto out=space.allocate(4,0x44000000);GuestThread guest(space,module,slots,token,3,initial);auto initial_sp=guest.context.r1.u64;
 auto seed=[&]{guest.context.r1.u64=initial_sp;guest.context.r2.u64=0x82020000;guest.context.r13.u64=guest.pcr.address;guest.context.lr=program_control.exit;guest.context.r3.u64=0x43000000;guest.context.r4.u64=out.address;guest.context.r14.u64=0x1021324354657687ull;space.store<uint32_t>(out.address,0);};
 auto execute=[&](bool exceptional=false,bool instruction=false){seed();code.startup(guest);auto retained=variable.acquire(module,variable_ref);Scope scope(guest,functions,module.identity,nullptr,{},&program_control);code.lend(scope);for(auto h:retained.registrations())scope.lend(h);scope.lend(out);Frame frame(scope,guest.context,module,space,retained);frame.exceptional=exceptional;frame.instruction=instruction;code.invoke(scope,plan.entry);check(guest.context.r1.u64==initial_sp&&guest.context.lr==program_control.exit);check(guest.context.r14.u64==0x1021324354657687ull);check(guest.context.r3.u32==31&&space.load<uint32_t>(out.address)==31);++runs;};
 execute();check(queries==1);no([&]{execute(true);});no([&]{execute(false,true);});
 seed();auto r13=guest.context.r13.u64;guest.context.r13.u64=1;no([&]{Scope denied(guest,functions,module.identity,nullptr,{},&program_control);},true);guest.context.r13.u64=r13;
 space.protect(guest.stack,brutal::foundation::Permission::read);no([&]{Scope denied(guest,functions,module.identity,nullptr,{},&program_control);},true);space.protect(guest.stack,brutal::foundation::Permission::read_write);
 {seed();auto retained=variable.acquire(module,variable_ref);Scope scope(guest,functions,module.identity,nullptr,{},&program_control);code.lend(scope);for(auto h:retained.registrations())scope.lend(h);scope.lend(out);Frame frame(scope,guest.context,module,space,retained);
  no([&]{scope.invoke(plan.entry+4);},true);no([&]{scope.invoke(0xdeadbeec);},true);guest.context.lr=1;no([&]{scope.invoke(plan.entry);},true);guest.context.lr=program_control.exit;
  guest.context.r3.u64=0xdeadbeec;no([&]{scope.invoke(program_indirect);});guest.context.lr=program_control.exit;
 }
 {seed();auto retained=variable.acquire(module,variable_ref);brutal::synthetic::CooperativeStop stop;unsigned polls=0;Scope scope(guest,functions,module.identity,&stop,[&](uint32_t){if(++polls==4)stop.request();},&program_control);code.lend(scope);Frame frame(scope,guest.context,module,space,retained);bool stopped=false;try{scope.invoke(program_forever);}catch(const Scope::Stopped&e){check(e.owner==&scope);stopped=true;}check(stopped);stop.exited();stop.joined();stop.reclaimed();stop.unloaded();++stops;}
 {GuestTask task(guest);check(task.start([&]{execute();}));check(task.join());check(!task.callback_error());}
 guest.cleanup();slots.release(token);space.release(out);check(variable.close());registry.withdraw(key,provider_ref);check(registry.active()==0);no([&]{variable.acquire(module,variable_ref);},true);check(metadata.close());check(header.close());check(module.close());image.unload();check(space.live()==0&&space.leases()==0&&nm::Object::registrations()==0&&hb::Buffer::registrations()==0);
 std::cout<<"{\"positive\":"<<pos<<",\"refusals\":"<<neg<<",\"preentry_zero_checks\":"<<zero_entries<<",\"completed_runs\":"<<runs<<",\"native_queries\":"<<queries<<",\"result\":31,\"stops\":"<<stops<<",\"allocations\":0,\"leases\":0,\"proprietary_PPC\":false}\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
