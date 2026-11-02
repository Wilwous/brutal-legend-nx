#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Production-generated original differential tests, never title input."""
from pathlib import Path
import argparse,hashlib,json,re,sys
from fixture import create,MODES,cfg
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--workdir',type=Path,required=True);ap.add_argument('--toolchain',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);records=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def execute(cmd,label):
    r=run(list(map(str,cmd)),w,label,30);records.append(r)
    if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics preserved')
    return (w/(label+'.stdout')).read_text()
names=['runtime/horizon/'+n for n in ['space.h','guest.h','generated.h']]+['tests/context-equivalence/'+n for n in ['fixture.py','model.py','contract.h','probe.cpp','run.py','verify.py','templates/test/ppc_config_h.inja','templates/test/ppc_test_functions_cpp.inja']]
before={n:sha(ROOT/n) for n in names}
pin=json.loads((tc/'prepared.json').read_text());assert pin['sdk_revision']=='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' and sha(tc/'rexglue')==pin['cli_sha256']=='6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c'
meta=create(w/'input');generated=w/'generated';baseline=w/'baseline';generated.mkdir();baseline.mkdir()
cli=[tc/'rexglue','recompile-tests','--bin-dir',w/'input','--asm-dir',w/'input']
execute([*cli,'--output',generated,'--templates',ROOT/'tests/context-equivalence/templates'],'generation')
execute([*cli,'--output',baseline],'default-generation');hashes={p.name:sha(p) for p in generated.iterdir()}
execute([*cli,'--output',generated,'--templates',ROOT/'tests/context-equivalence/templates'],'determinism')
assert hashes=={p.name:sha(p) for p in generated.iterdir()}
cpp=(generated/'ppc_test_functions.cpp').read_text();default=(baseline/'ppc_test_functions.cpp').read_text();assert cpp[cpp.index('DEFINE_REX_FUNC('):]==default[default.index('DEFINE_REX_FUNC('):]
parts=re.split(r'DEFINE_REX_FUNC\((\w+)\)\s*\{',cpp);bodies={parts[i]:parts[i+1] for i in range(1,len(parts),2)};correspondence=[]
for p in meta:
    mapping={f'{p["mode"]}_{addr:X}':addr for addr in [p['entry'],p['helper']]}
    for start,end in [(p['entry'],p['helper']),(p['helper'],p['entry']+p['instructions']*4)]:
        rows=[r for r in p['rows'] if start<=r['pc']<end];name=f'{p["mode"]}_{start:X}'
        correspondence.append({'function':name,**cfg.reconcile(rows,bodies[name],function_map=mapping)})
header='#pragma once\n#include "runtime/horizon/generated.h"\n'
for p in meta:
    for pc in [p['entry'],p['helper']]:header+=f'REX_EXTERN({p["mode"]}_{pc:X});\n'
header+='namespace context_test {\nusing namespace brutal::horizon;\nstruct GeometrySpan {int64_t offset;uint32_t width;};\nenum class Mode {'+','.join(MODES)+'};\nstruct Program {Mode mode;uint32_t entry,helper;uint64_t module,source;std::array<Function,2> functions;bool relocatable;};\ninline const std::array<Program,5> programs{{\n'
for i,p in enumerate(meta):
    fs=','.join('{'+str(pc)+','+p['mode']+'_'+f'{pc:X}'+',"'+p['mode']+'_'+f'{pc:X}'+'"}' for pc in [p['entry'],p['helper']])
    header+='{Mode::'+p['mode']+','+str(p['entry'])+','+str(p['helper'])+','+str(0x4551554900+i)+',0x'+p['sha256'][:16]+',{{'+fs+'}},'+str(p['relocatable_contract']).lower()+'},\n'
header+='}};\n}\n';(w/'equivalence-data.h').write_text(header);tests=[]
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
    binary=w/('probe-'+profile)
    execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',ROOT/'tests/context-equivalence','-I',w,'-I',generated,ROOT/'tests/context-equivalence/probe.cpp',generated/'ppc_test_functions.cpp','-o',binary],'compile-'+profile)
    snapshot=w/('observations-'+profile+'.bin')
    r=json.loads(execute([binary,snapshot],'run-'+profile));assert r['refusals']==r['zero_entry_checks'] and r['completed_differential_runs']==24 and r['equivalent_pairs']==12 and r['counterexample_pairs']==10 and r['allocations']==r['leases']==0
    independent=json.loads(execute(['python3',ROOT/'tests/context-equivalence/verify.py',snapshot],'independent-'+profile));assert independent['equivalent_pairs']==r['equivalent_pairs'] and independent['counterexample_pairs']==r['counterexample_pairs'];tests.append({'profile':profile,**r,'independent':independent})
assert before=={n:sha(ROOT/n) for n in names} and hashes=={p.name:sha(p) for p in generated.iterdir()}
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records,'fixtures':meta,'static_correspondence':correspondence,'generated_sha256':hashes,'sources':before,'game_executed':False,'generated_unedited':True,'hardware_execution':'NOT TESTED'},indent=2)+'\n');print(json.dumps(tests))
