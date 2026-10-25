#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Actual pinned ReXGlue CLI -> unchanged generated TU -> real SDK fixtures."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import create

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ['sdk','rexglue','workdir']:ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--sanitize',action='store_true');a=ap.parse_args()
    sdk=a.sdk.resolve();cli=a.rexglue.resolve();work=a.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    metadata=json.loads((sdk/'fixture-sdk.json').read_text())
    if metadata['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2':raise RuntimeError('wrong revision')
    if hashlib.sha256((sdk/'lib'/metadata['runtime_filename']).read_bytes()).hexdigest()!=metadata['runtime_sha256']:raise RuntimeError('runtime hash mismatch')
    if (metadata['config']=='Sanitized')!=a.sanitize:raise RuntimeError('wrong sanitizer profile')
    for name in list(os.environ):
        if name.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[name]
    os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
    work.mkdir(parents=True,exist_ok=False);layout=create(work/'input');output=work/'generated';output.mkdir()
    records=[]
    def execute(cmd,label):
        rec=run(cmd,work,label,30,True);records.append(rec)
        if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; inspect private diagnostics')
        return rec
    execute([str(cli),'recompile-tests','--bin-dir',str(work/'input'),'--asm-dir',str(work/'input'),'--output',str(output)],'generation')
    generated=output/'ppc_test_functions.cpp';body=generated.read_text()
    declarations=re.findall(r'^DECLARE_REX_FUNC\(module_([0-9A-F]+)\);',body,re.M)
    expected=[f'{layout["analysis_base"]+offset:X}' for offset in layout['functions'].values()]
    if declarations!=expected or len(re.findall(r'^DEFINE_REX_FUNC\(',body,re.M))!=9:raise RuntimeError('missing generated function')
    instructions=re.findall(r'^\t// (.+)$',body,re.M)
    if len(instructions)!=layout['instruction_count']:raise RuntimeError('instruction count drift')
    for banned in ['REX_FATAL(', 'REX_UNIMPLEMENTED(', 'base +', 'ppc_setjmp(', 'ppc_longjmp(', 'ppc_trap(']:
        if banned in body:raise RuntimeError('unsafe generated helper: '+banned)
    counts={token:body.count(token) for token in ['REX_LOAD_U32(', 'REX_STORE_U32(', 'REX_LOAD_U64(', 'REX_STORE_U64(', 'REX_LOAD_VECTOR(', 'REX_STORE_VECTOR(', 'REX_RAW_ADDR(', 'REX_PARTIAL_VECTOR_GUARD(', 'REX_CALL_FUNC(', 'REX_CALL_INDIRECT_FUNC(']}
    expected_counts=dict(zip(counts,[5,7,1,1,1,1,2,1,1,1]))
    if counts!=expected_counts:raise RuntimeError('generated access census drift: '+str(counts))
    before=hashlib.sha256(generated.read_bytes()).hexdigest()
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if metadata['config']=='Debug' else '-O2','-g','-I'+str(sdk/'include'),'-I'+str(output),'-I'+str(work/'input'),'-iquote',str(ROOT),'-DREX_CHECKED_ACCESS_HEADER="runtime/generated_access.h"']
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    binary=work/'synthetic';libraries=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
    execute(['clang++',*flags,str(ROOT/'tests/synthetic/contracts.cpp'),str(generated),*libraries,'-o',str(binary)],'build-synthetic')
    checks=[];failures=[]
    for mode in ['loading','memory','calls','threads','unsupported','loans']:
        rec=run([str(binary),str(work/('runtime-'+mode)),str(work/'input'),mode],work,mode,30,True);records.append(rec)
        rows=[json.loads(line) for line in (work/(mode+'.stdout')).read_text().splitlines() if line.startswith('{')]
        rec['checks']=rows
        passed=[row for row in rows if row.get('result')=='PASS']
        if rec['timeout'] or rec['exit_code'] or len(passed)!=1:failures.append(mode)
        else:checks+=passed
    lifetime=work/'lifetime';injector=work/'create-failure.dylib'
    execute(['clang++',*flags,str(ROOT/'tests/synthetic/lifetime.cpp'),*libraries,'-o',str(lifetime)],'build-lifetime')
    execute(['clang++',*flags,'-dynamiclib',str(ROOT/'tests/synthetic/create-failure.cpp'),'-o',str(injector)],'build-create-failure')
    lifetime_modes=['gate-synchronization','normal','self-terminate','return','export-exit','callback-error','callback-exit',
      'creation-failure','host-creation-failure','native-creation-failure','owners','release-before-exit',
      'foreign-termination','repeat','simultaneous']
    for mode in lifetime_modes:
        label='lifetime-'+mode
        command=[str(lifetime),str(work/('runtime-'+label)),mode]
        if mode in ['host-creation-failure','native-creation-failure']:
            command=['/usr/bin/env','DYLD_INSERT_LIBRARIES='+str(injector),*command]
        rec=run(command,work,label,30,True);records.append(rec)
        rows=[json.loads(line) for line in (work/(label+'.stdout')).read_text().splitlines() if line.startswith('{')]
        rec['checks']=rows;passed=[row for row in rows if row.get('result')=='PASS']
        if rec['timeout'] or rec['exit_code'] or len(passed)!=1:failures.append(label)
        else:checks+=passed
    if hashlib.sha256(generated.read_bytes()).hexdigest()!=before:raise RuntimeError('generated output mutated')
    # Verify the actual generated PCH selects every intended checked memory/call seam.
    execute(['clang++',*flags,'-dM','-E','-x','c++',str(output/'ppc_config.h'),'-o',str(work/'macros.txt')],'preprocess')
    definitions=(work/'macros.txt').read_text()
    for name in ['REX_LOAD_U32','REX_STORE_U32','REX_LOAD_U64','REX_STORE_U64','REX_LOAD_VECTOR','REX_STORE_VECTOR','REX_RAW_ADDR','REX_CALL_FUNC','REX_CALL_INDIRECT_FUNC','REX_PARTIAL_VECTOR_GUARD']:
        line=next((line for line in definitions.splitlines() if line.startswith('#define '+name+'(')), '')
        if not line or 'base +' in line or not any(word in line for word in ['brutal::','brutal_reject']):raise RuntimeError('macro not mediated: '+name)
    inputs=['tests/synthetic/'+name for name in ['run.py','fixture.py','module.h','contracts.cpp','lifetime.cpp','create-failure.cpp']]+['runtime/generated_access.h','runtime/sdk_checked_memory.h']
    source_hashes={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in inputs}
    result={'fixture_sources_sha256':source_hashes,'sdk':metadata,'rexglue_sha256':hashlib.sha256(cli.read_bytes()).hexdigest(),'generated_sha256':before,
      'instructions':len(instructions),'functions':len(declarations),'access_census':counts,'passed_groups':len(checks),
      'records':records,'checks':checks,'failed_groups':failures,'unexpected_failures':len(failures),'game_loaded':False}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['sdk','records']},indent=2))
    return 1 if failures else 0
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,RuntimeError,ValueError) as error:print(str(error),file=sys.stderr);sys.exit(1)
