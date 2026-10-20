// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Original fixture; calls real SDK shims, never loads an XEX.
#include <rex/runtime.h>
#include <rex/kernel/init.h>
#include <rex/logging.h>
#include <rex/ppc.h>
#include <rex/system/xthread.h>
#include <rex/thread.h>
#include <array>
#include <atomic>
#include <barrier>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <dlfcn.h>
#define DECLARE(n) REX_EXTERN(__imp__##n)
DECLARE(RtlUpperChar); DECLARE(RtlLowerChar); DECLARE(RtlFillMemoryUlong);
DECLARE(RtlCompareMemory); DECLARE(NtAllocateVirtualMemory); DECLARE(NtFreeVirtualMemory);
DECLARE(NtProtectVirtualMemory); DECLARE(KeTlsAlloc); DECLARE(KeTlsFree);
DECLARE(KeTlsGetValue); DECLARE(KeTlsSetValue); DECLARE(StfsCreateDevice); DECLARE(StfsControlDevice);
static_assert(offsetof(PPCContext,f31)-offsetof(PPCContext,f0)==31*sizeof(PPCRegister));
static_assert(offsetof(PPCContext,v127)-offsetof(PPCContext,v0)==127*sizeof(PPCVRegister));
static int passed=0;
static uint32_t args10(uint32_t a,uint32_t b,uint32_t c,uint32_t d,uint32_t e,uint32_t f,uint32_t g,uint32_t h,uint32_t i,uint32_t j){
  return a+2*b+3*c+4*d+5*e+6*f+7*g+8*h+9*i+10*j;
}
static double floating(double x,double y){return x+2*y;}
REX_HOOK(fixture_args10,args10)
REX_HOOK(fixture_floating,floating)

static void require(bool value,const char* what){if(!value)throw std::runtime_error(what);}
static void pass(const char* name){++passed;std::printf("{\"group\":\"%s\",\"result\":\"PASS\"}\n",name);}
static auto registers(PPCContext& c) {
  return std::array<PPCRegister*,32>{&c.r0,&c.r1,&c.r2,&c.r3,&c.r4,&c.r5,&c.r6,&c.r7,&c.r8,&c.r9,&c.r10,&c.r11,&c.r12,&c.r13,&c.r14,&c.r15,&c.r16,&c.r17,&c.r18,&c.r19,&c.r20,&c.r21,&c.r22,&c.r23,&c.r24,&c.r25,&c.r26,&c.r27,&c.r28,&c.r29,&c.r30,&c.r31};
}
// Only r3 may change for these integer/void SDK shims. Arguments are set before snapshot.
static uint32_t call(PPCFunc* f,PPCContext& c,uint8_t* base,std::initializer_list<uint32_t> args) {
  size_t i=0;for(auto arg:args)rex::ppc::ArgTranslator::SetIntegerArgumentValue(c,base,i++,arg);
  PPCContext before=c;
  f(c,base);
  auto old=registers(before), now=registers(c);
  for(size_t r=0;r<32;++r)if(r!=3)require(old[r]->u64==now[r]->u64,"GPR clobber");
  require(std::memcmp(&before.f0,&c.f0,32*sizeof(PPCRegister))==0,"FPR clobber");
  require(std::memcmp(&before.v0,&c.v0,128*sizeof(PPCVRegister))==0,"vector clobber");
  require(before.lr==c.lr&&before.ctr.u64==c.ctr.u64&&before.msr==c.msr,"control register clobber");
  require(std::memcmp(&before.cr0,&c.cr0,8*sizeof(PPCCRRegister))==0,"CR clobber");
  return c.r3.u32;
}
static void seed(PPCContext& c) {
  auto r=registers(c);for(size_t i=0;i<32;++i)r[i]->u64=0x1234567800000000ull+i;
  PPCRegister* floats[]{&c.f0,&c.f1,&c.f2,&c.f3,&c.f4,&c.f5,&c.f6,&c.f7,&c.f8,&c.f9,&c.f10,&c.f11,&c.f12,&c.f13,&c.f14,&c.f15,&c.f16,&c.f17,&c.f18,&c.f19,&c.f20,&c.f21,&c.f22,&c.f23,&c.f24,&c.f25,&c.f26,&c.f27,&c.f28,&c.f29,&c.f30,&c.f31};
  for(size_t i=0;i<32;++i)floats[i]->f64=double(i)+.25;
  auto* raw=reinterpret_cast<unsigned char*>(&c);
  for(size_t i=0;i<128*sizeof(PPCVRegister);++i)raw[offsetof(PPCContext,v0)+i]=static_cast<unsigned char>(i*37+i/16); c.lr=0x87654321;c.ctr.u64=0x12345678;
}
// Real XThread storage and real Memory, with a small synthetic TLS layout.
// No host thread is started by this fixture; lifecycle is tested separately below.
class BoundaryThread final:public rex::system::XThread {
 public:
  explicit BoundaryThread(rex::system::KernelState* k):XThread(k){
    tls_total_size_=28+64*4;
    tls_static_address_=memory()->SystemHeapAlloc(tls_total_size_);
    require(tls_static_address_!=0,"boundary TLS allocation");
    tls_dynamic_address_=tls_static_address_+28;
    memory()->Zero(tls_static_address_,tls_total_size_);
    memory()->Fill(tls_static_address_,28,0x6d);
  }
  bool rejects_invalid_metadata(){
    auto original_static=tls_static_address_,original_dynamic=tls_dynamic_address_,original_total=tls_total_size_;
    uint32_t value=0x1234;bool ok=true;
    tls_static_address_=0;ok&=!GetTLSValue(0,&value)&&!SetTLSValue(0,7);
    tls_static_address_=original_static;tls_dynamic_address_=original_static-4;
    ok&=!GetTLSValue(0,&value)&&!SetTLSValue(0,7);
    tls_dynamic_address_=original_dynamic;tls_total_size_=16;
    ok&=!GetTLSValue(0,&value)&&!SetTLSValue(0,7)&&value==0x1234;
    tls_static_address_=original_static;tls_dynamic_address_=original_dynamic;tls_total_size_=original_total;
    return ok;
  }
};
class ExecutionThread final:public rex::system::XThread {
  std::function<int()> body_;
 public:
  ExecutionThread(rex::system::KernelState* kernel,std::function<int()> body)
      :XThread(kernel,0x10000,0,0,0,1,true),body_(std::move(body)){}
  void Execute() override { kernel_state()->OnThreadExecute(this); int result=body_(); Exit(result); }
};
int main(int argc,char** argv){
  if(argc!=3 && argc!=4)return 2;
  std::setbuf(stdout,nullptr);
  try{
    bool unified=std::string(argv[2])=="unified";
    bool patched=unified||std::string(argv[2])=="patched";
    rex::InitLoggingEarly();std::filesystem::path work=argv[1];
    std::filesystem::create_directories(work/"empty-game");
    rex::Runtime runtime(work/"empty-game",work/"user",work/"updates",work/"cache",work/"metadata");
    require(runtime.Setup(rex::RuntimeConfig{.kernel_init=rex::kernel::InitializeKernel,.tool_mode=true})==0,"runtime setup");
    auto* memory=runtime.memory();auto* kernel=runtime.kernel_state();auto* base=memory->virtual_membase();
    struct ExportCheck { const char* name; PPCFunc* function; bool replaced; };
    ExportCheck exports[]{
      {"RtlUpperChar",__imp__RtlUpperChar,false},
      {"RtlLowerChar",__imp__RtlLowerChar,false},
      {"RtlFillMemoryUlong",__imp__RtlFillMemoryUlong,false},
      {"RtlCompareMemory",__imp__RtlCompareMemory,false},
      {"NtAllocateVirtualMemory",__imp__NtAllocateVirtualMemory,true},
      {"NtFreeVirtualMemory",__imp__NtFreeVirtualMemory,true},
      {"NtProtectVirtualMemory",__imp__NtProtectVirtualMemory,true},
      {"KeTlsAlloc",__imp__KeTlsAlloc,true},
      {"KeTlsFree",__imp__KeTlsFree,true},
      {"KeTlsGetValue",__imp__KeTlsGetValue,true},
      {"KeTlsSetValue",__imp__KeTlsSetValue,true},
      {"StfsCreateDevice",__imp__StfsCreateDevice,true},
      {"StfsControlDevice",__imp__StfsControlDevice,true}};
    for(auto item:exports){
      const rex::runtime::Export* found=nullptr;
      for(auto* entry:runtime.export_resolver()->all_exports_by_name())if(std::strcmp(entry->name,item.name)==0)found=entry;
      require(found&&found->type==rex::runtime::Export::Type::kFunction,"export identity missing");
      Dl_info image{};require(dladdr(reinterpret_cast<void*>(item.function),&image)!=0,"export owner unknown");
      bool in_library=std::string(image.dli_fname).find("librexruntime")!=std::string::npos;
      require(in_library!=bool(!unified&&patched&&item.replaced),"unexpected shim image binding");
      if(unified){
        require(argc==4,"unified library root argument missing");
        require(std::filesystem::weakly_canonical(image.dli_fname).parent_path()==std::filesystem::weakly_canonical(argv[3])/"lib","shim outside unified SDK");
      }
      std::printf("EXPORT %s ordinal=%u image=%s\n",item.name,found->ordinal,in_library?(unified?"unified-runtime":"prebuilt-runtime"):"isolated-executable");
    }
    uint32_t stack=memory->SystemHeapAlloc(0x4000,16), data=memory->SystemHeapAlloc(0x1000,16);
    require(stack&&data,"fixture allocations");
    PPCContext ctx{};seed(ctx);ctx.r1.u64=stack+0x3e00;ctx.r2.u64=0x13572468;ctx.r13.u64=0x24681350;
    require((ctx.r1.u32&15)==0,"stack alignment");
    require(memory->HostToGuestVirtual(memory->TranslateVirtual(data))==data,"pointer roundtrip");
    require(call(__imp__RtlUpperChar,ctx,base,{'q'})=='Q',"upper");
    require(call(__imp__RtlLowerChar,ctx,base,{'Q'})=='q',"lower");pass("integer_arguments_and_register_preservation");
    require(call(fixture_args10,ctx,base,{1,2,3,4,5,6,7,8,9,10})==385,"10 argument fixture");
    require(rex::memory::load_and_swap<uint32_t>(base+ctx.r1.u32+0x54)==9&&rex::memory::load_and_swap<uint32_t>(base+ctx.r1.u32+0x5c)==10,"BE stack arguments");
    ctx.f1.f64=1.25;ctx.f2.f64=2.5;fixture_floating(ctx,base);
    require(ctx.f1.f64==6.25&&ctx.f2.f64==2.5,"FPR argument/return fixture");
    pass("original_marshaller_stack_and_FPR_fixtures");
    memory->Fill(data,64,0xcc);
    call(__imp__RtlFillMemoryUlong,ctx,base,{data+4,16,0x01234567});
    auto* bytes=memory->TranslateVirtual(data);
    const uint8_t pattern[4]{1,0x23,0x45,0x67};
    for(size_t i=4;i<20;++i)require(bytes[i]==pattern[i%4],"big endian fill");
    require(bytes[3]==0xcc&&bytes[20]==0xcc,"fill bounds");
    std::memcpy(bytes+32,bytes+4,16);
    require(call(__imp__RtlCompareMemory,ctx,base,{data+4,data+32,16})==16,"compare");
    bytes[35]^=1;
    require(call(__imp__RtlCompareMemory,ctx,base,{data+4,data+32,16})==15,"compare mismatch count");
    pass("mapped_pointer_endian_and_guard_bytes");
    auto put=[&](uint32_t offset,uint32_t value){rex::memory::store_and_swap<uint32_t>(memory->TranslateVirtual(data+offset),value);};
    auto get=[&](uint32_t offset){return rex::memory::load_and_swap<uint32_t>(memory->TranslateVirtual(data+offset));};
    put(0,0);put(4,0x1234);
    require(call(__imp__NtAllocateVirtualMemory,ctx,base,{data,data+4,0x3000,4,0})==0,"NT allocation");
    uint32_t allocation=get(0),size=get(4);require(allocation&&!(allocation&0xfff)&&size==0x10000,"NT alignment/size");
    require(memory->TranslateVirtual(allocation)[0]==0,"zero fill");
    memory->TranslateVirtual(allocation)[size-1]=0x7e;
    require(call(__imp__NtProtectVirtualMemory,ctx,base,{data,data+4,2,data+8,0})==0,"read protection");
    require(get(8)==(patched?4u:3u)&&memory->TranslateVirtual(allocation)[size-1]==0x7e,"old protection/read");
    require(call(__imp__NtProtectVirtualMemory,ctx,base,{data,data+4,4,data+8,0})==0,"restore protection");
    put(4,0);require(call(__imp__NtFreeVirtualMemory,ctx,base,{data,data+4,0x8000,0})==0,"release");
    rex::memory::HeapAllocationInfo info{};require(memory->LookupHeap(allocation)->QueryRegionInfo(allocation,&info)&&info.state==0,"released allocation");
    put(0,0);put(4,0);require(call(__imp__NtAllocateVirtualMemory,ctx,base,{data,data+4,0x3000,4,0})==0xc000000d,"zero size failure");
    put(4,4096);require(call(__imp__NtAllocateVirtualMemory,ctx,base,{data,data+4,0,4,0})==0xc000000d,"bad allocation type");
    require(get(0)==0&&get(4)==4096,"failure output preservation");pass("memory_lifecycle_protection_and_failure");
    auto stfs_create=call(__imp__StfsCreateDevice,ctx,base,{0,0,data+16});
    auto stfs_control=call(__imp__StfsControlDevice,ctx,base,{0,0,0,0,data+16,4});
    if(patched){require(get(16)==0x01234567,"STFS output modified");require(stfs_create==0xc0000002&&stfs_control==0xc0000002,"STFS false success");require(call(__imp__StfsCreateDevice,ctx,base,{0xffffffff,0,0xffffffff})==0xc0000002,"STFS invalid pointer path");
      require(call(__imp__StfsControlDevice,ctx,base,{0xffffffff,7,0xffffffff,16,0xffffffff,16})==0xc0000002,"STFS invalid control pointer path");pass("patched_STFS_rejects");}
    else {require(stfs_create==0&&stfs_control==0,"baseline STFS changed");pass("baseline_STFS_false_success_reproduced");}
    if(patched){
      rex::system::object_ref<BoundaryThread> thread(new BoundaryThread(kernel));
      require(thread->ReleaseHandle(),"boundary handle close");uint32_t value=0xabcdef;
      require(thread->SetTLSValue(63,0xa1b2c3d4)&&thread->GetTLSValue(63,&value)&&value==0xa1b2c3d4,"last TLS slot");
      require(!thread->GetTLSValue(64,&value)&&!thread->SetTLSValue(64,7),"one-past TLS");
      require(!thread->GetTLSValue(0x40000000,&value)&&!thread->SetTLSValue(0x40000000,7),"TLS overflow");
      require(!thread->GetTLSValue(0,nullptr),"null TLS output");require(value==0xa1b2c3d4,"failed get output changed");
      for(int i=0;i<28;++i)require(memory->TranslateVirtual(thread->tls_ptr())[i]==0x6d,"static TLS prefix modified");
      require(thread->rejects_invalid_metadata(),"malformed TLS metadata accepted");pass("patched_real_XThread_TLS_boundaries");
    }
    auto* process=memory->TranslateVirtual<rex::system::X_KPROCESS*>(kernel->GetTitleProcess());
    process->tls_slot_size=256;for(int i=0;i<8;++i)process->tls_slot_bitmap[i]=i<2?0xffffffffu:0;
    std::barrier sync(2);std::atomic<int> errors{0};std::atomic<uint32_t> slot{0xffffffff};
    std::array<uint32_t,2> pcr{},tls{};
    std::array<rex::system::object_ref<ExecutionThread>,2> threads;
    for(int id=0;id<2;++id){
      threads[id]=rex::system::object_ref<ExecutionThread>(new ExecutionThread(kernel,[&,id]{
        auto* self=rex::system::XThread::GetCurrentThread();auto* c=rex::runtime::current_ppc_context();
        pcr[id]=self->pcr_ptr();tls[id]=self->tls_ptr();
        if(c->r13.u32!=pcr[id]||(c->r1.u32&15)||rex::memory::load_and_swap<uint32_t>(memory->TranslateVirtual(pcr[id]))!=tls[id])++errors;
        try{
          if(id==0)slot=call(__imp__KeTlsAlloc,*c,base,{});
          sync.arrive_and_wait();uint32_t s=slot.load();
          if(s==0xffffffff||call(__imp__KeTlsGetValue,*c,base,{s})!=0)++errors;
          if(call(__imp__KeTlsSetValue,*c,base,{s,0x11110000u+uint32_t(id)})!=1)++errors;
          sync.arrive_and_wait();
          if(call(__imp__KeTlsGetValue,*c,base,{s})!=0x11110000u+uint32_t(id))++errors;
          sync.arrive_and_wait();
          if(id==0&&call(__imp__KeTlsFree,*c,base,{s})!=1)++errors;
          sync.arrive_and_wait();
          if(call(__imp__KeTlsGetValue,*c,base,{s})!=0)++errors;
          sync.arrive_and_wait();
          if(id==0){auto again=call(__imp__KeTlsAlloc,*c,base,{});if(again!=s)++errors;call(__imp__KeTlsFree,*c,base,{again});
            std::array<uint32_t,64> slots{};
            for(uint32_t n=0;n<64;++n){slots[n]=call(__imp__KeTlsAlloc,*c,base,{});if(slots[n]!=n)++errors;}
            if(call(__imp__KeTlsAlloc,*c,base,{})!=0xffffffff)++errors;
            for(auto allocated:slots)if(call(__imp__KeTlsFree,*c,base,{allocated})!=1)++errors;
            if(patched){
              if(call(__imp__KeTlsSetValue,*c,base,{1024,7})!=0)++errors;
              if(call(__imp__KeTlsSetValue,*c,base,{0x40000000,7})!=0)++errors;
              if(unified){
                if(call(__imp__KeTlsFree,*c,base,{256})!=0)++errors;
                if(call(__imp__KeTlsFree,*c,base,{0xffffffff})!=0)++errors;
                if(call(__imp__KeTlsFree,*c,base,{s})!=0)++errors;
              }
            }}
        }catch(...){++errors;std::abort();}
        return 0;
      }));
      require(threads[id]->Create()==0,"XHostThread create");
    }
    for(auto& thread:threads)require(thread->Resume()==0,"resume");
    for(auto& thread:threads)require(rex::thread::Wait(thread->thread(),false,std::chrono::seconds(5))==rex::thread::WaitResult::kSuccess,"thread completion");
    require(errors==0&&pcr[0]!=pcr[1]&&tls[0]!=tls[1],"TLS/PCR isolation");
    for(auto& thread:threads){require(thread->ReleaseHandle(),"thread handle close");thread.reset();}
    pass("real_XThread_PCR_r13_TLS_lifecycle_isolation");
    memory->SystemHeapFree(data);memory->SystemHeapFree(stack);
    std::printf("{\"passed\":%d,\"failed\":0,\"game_loaded\":false,\"patched\":%s}\n",passed,patched?"true":"false");
    return 0;
  }catch(const std::exception& e){std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}
}
