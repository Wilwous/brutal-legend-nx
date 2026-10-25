#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Real production CLI generation, bounded host tests and optional Horizon link.

No device access or game inputs. Generated C++ is never modified after rendering.
"""
import argparse,hashlib,json,subprocess,sys,uuid
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import write,BINARY_SHA256

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def inputs():
    names=['runtime/horizon/leaf.h','runtime/checked_memory.h','patches/rexglue-sdk/horizon-leaf.patch',
           'tools/switch/sanitize_elf.py','tests/switch-horizon/result_ui.h','tests/switch-horizon/toolchain.json']
    names+=['tests/horizon-runtime/'+n for n in ['lock.json','probe.h','probe.cpp','main.cpp','host.cpp','Makefile','run.py','prepare.py','fixture.py','inspect_nro.py','templates/test/ppc_config_h.inja','templates/test/ppc_test_functions_cpp.inja']]
    return {n:sha(ROOT/n) for n in names}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['toolchain','workdir']:ap.add_argument('--'+n,type=Path,required=True)
    ap.add_argument('--switch-build',action='store_true');ap.add_argument('--icon',type=Path)
    a=ap.parse_args();work=a.workdir.resolve();tc=a.toolchain.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    work.mkdir(parents=True,exist_ok=False);records=[]
    def execute(cmd,label,success=True):
        r=run(list(map(str,cmd)),work,label,30);r['expected_exit_code']=0 if success else 1;records.append(r)
        if r['timeout'] or r['exit_code']!=r['expected_exit_code']:raise RuntimeError(label+' unexpected result; inspect private logs')
        return (work/(label+'.stdout')).read_text()
    receipt=json.loads((tc/'prepared.json').read_text())
    if sha(tc/'rexglue')!=receipt['cli_sha256'] or sha(ROOT/'patches/rexglue-sdk/horizon-leaf.patch')!=receipt['patch_sha256']:raise ValueError('toolchain receipt mismatch')
    pinned=json.loads((ROOT/'tests/horizon-runtime/lock.json').read_text())
    if receipt['patch_sha256']!=pinned['patch_sha256']:raise ValueError('leaf patch is not pinned')
    source=work/'input';write(source)
    if sha(source/'leaf.bin')!=BINARY_SHA256:raise ValueError('fixture identity changed')
    generated=work/'generated';baseline=work/'baseline';generated.mkdir();baseline.mkdir()
    cli=[tc/'rexglue','recompile-tests','--bin-dir',source,'--asm-dir',source]
    execute([*cli,'--output',baseline],'baseline-generation')
    execute([*cli,'--output',generated,'--templates',ROOT/'tests/horizon-runtime/templates'],'horizon-generation')
    original=(baseline/'ppc_test_functions.cpp').read_text();body=(generated/'ppc_test_functions.cpp').read_text()
    if sha(baseline/'ppc_test_functions.cpp')!='8608e2c49883d9e2f42992ed0c3aa1184648559db580b2ab4bcdb32ef6280b3c':raise ValueError('pinned default output changed')
    function=body[body.index('DEFINE_REX_FUNC('):]
    if function!=original[original.index('DEFINE_REX_FUNC('):]:raise ValueError('generated instruction body changed')
    expected=['// lwz r3,0(r4)','ctx.r3.u64 = REX_LOAD_U32(ctx.r4.u32 + 0);','// addi r3,r3,1','ctx.r3.s64 = ctx.r3.s64 + 1;',
              '// stw r3,0(r4)','REX_STORE_U32(ctx.r4.u32 + 0, ctx.r3.u32);','// blr','return;']
    lines=[l.strip() for l in function.splitlines()[1:-1] if l.strip() and l.strip() not in {'REX_FUNC_PROLOGUE();','}'}]
    if lines!=expected:raise ValueError('independent exact instruction correspondence failed')
    hashes={p.name:sha(p) for p in generated.iterdir()};host=[]
    if hashes!=pinned['generated_sha256']:raise ValueError('generated output lock mismatch')
    for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
        binary=work/('host-'+profile)
        execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',generated,ROOT/'tests/horizon-runtime/host.cpp',ROOT/'tests/horizon-runtime/probe.cpp',generated/'ppc_test_functions.cpp','-o',binary],'compile-'+profile)
        result=json.loads(execute([binary],'run-'+profile))
        if result!={'passed':59,'failed':0,'per_probe':29,'entries_per_probe':10,'loads_per_probe':4,'stores_per_probe':3}:raise ValueError('host checks failed or count drift')
        host.append({'profile':profile,**result})
    # Compile refusal probes: unavailable helpers cannot become unsafe calls.
    denied=['REX_RAW_ADDR(0)','REX_CALL_INDIRECT_FUNC(0)','REX_CALL_FUNC(leaf_82010000)',
            'REX_LOAD_VECTOR(ctx.v0,0)','REX_QUERY_TIMEBASE()','ppc_trap(ctx,base,0)','REX_LOAD_U64(0)','leaf_82010000(ctx.v0,base)']
    for i,expression in enumerate(denied):
        negative=work/('negative-'+str(i)+'.cpp')
        negative.write_text('#include "ppc_config.h"\nREX_EXTERN(leaf_82010000);\nvoid deny(PPCContext& ctx,uint8_t* base){(void)ctx;(void)base;'+expression+';}\n')
        execute(['clang++','-std=c++23','-isystem',tc/'include','-iquote',ROOT,'-I',generated,'-c',negative,'-o',work/'denied.o'],'negative-'+str(i),False)
        if expression.split('(')[0] not in (work/('negative-'+str(i)+'.stderr')).read_text():raise ValueError('unclassified compile refusal')
    result={'host':host,'negative_compile_refusals':len(denied),'generated_sha256':hashes,'input_sha256':BINARY_SHA256,
            'generator_cli_sha256':receipt['cli_sha256'],'body_identical_to_pinned_default':True,
            'instruction_count':4,'functions':1,'checked_load_sites':1,'checked_store_sites':1,
            'build_sources_sha256':inputs(),'hardware_execution':'UNKNOWN','game_executed':False}
    if a.switch_build:
        lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text());outputs=[]
        for i in range(2):
            out=work/('switch-'+str(i));out.mkdir()
            def container(command,label):
                name='brutal-leaf-'+uuid.uuid4().hex[:12]
                cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,'--network','none',
                     '--cap-drop','ALL','--security-opt','no-new-privileges','--pids-limit','128','--memory','1g','--cpus','2',
                     '--read-only','--tmpfs','/tmp:rw,size=64m','--mount',f'type=bind,src={ROOT},dst=/src,readonly',
                     '--mount',f'type=bind,src={tc},dst=/toolchain,readonly','--mount',f'type=bind,src={generated},dst=/generated,readonly',
                     '--mount',f'type=bind,src={out},dst=/build']
                if a.icon:cmd+=['--mount',f'type=bind,src={a.icon.resolve()},dst=/icon.jpg,readonly']
                cmd+=['-w','/build',lock['image'],*command]
                try:execute(cmd,label+'-'+str(i))
                finally:
                    r=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
                    if r.returncode and 'No such container' not in r.stderr:raise RuntimeError('container cleanup failed')
            container(['make','-f','/src/tests/horizon-runtime/Makefile','REPO=/src',*(['ICON=/icon.jpg'] if a.icon else [])],'horizon-link')
            container(['python3','/src/tests/horizon-runtime/inspect_nro.py'],'inspect')
            outputs.append({n:sha(out/n) for n in ['probe.elf','probe.nro','leaf.o']})
        if outputs[0]!=outputs[1]:raise ValueError('clean build identity mismatch')
        result['switch']={'sha256':outputs[0],'deterministic_builds':2,'inspection':json.loads((work/'switch-0/inspection.json').read_text()),'classification':'BUILD VERIFIED / HARDWARE EXECUTION UNKNOWN'}
    if hashes!={p.name:sha(p) for p in generated.iterdir()}:raise ValueError('generated output mutated')
    result['records']=records;(work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k!='records'},indent=2))
if __name__=='__main__':main()
