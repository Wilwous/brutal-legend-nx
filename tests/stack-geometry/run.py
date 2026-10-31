#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original PPC -> production generation -> owned stack admission, host only."""
from pathlib import Path
import argparse,hashlib,json,re,sys
from fixture import create,cfg
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--workdir',type=Path,required=True);ap.add_argument('--toolchain',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);records=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def execute(cmd,label):
 r=run(list(map(str,cmd)),w,label,30);records.append(r)
 if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
 return (w/(label+'.stdout')).read_text()
names=['runtime/horizon/'+n for n in ['space.h','guest.h','generated.h']]+['tests/stack-geometry/'+n for n in ['fixture.py','admission.h','probe.cpp','run.py','templates/test/ppc_config_h.inja','templates/test/ppc_test_functions_cpp.inja']]
source={n:sha(ROOT/n) for n in names}
pin=json.loads((tc/'prepared.json').read_text())
assert pin['sdk_revision']=='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' and sha(tc/'rexglue')==pin['cli_sha256']=='6f40ec768bba9ea985719f6df543c5e0bd099b19ceaec88aee9f9c7f7e6cc15c'
meta=create(w/'input');generated=w/'generated';baseline=w/'baseline';generated.mkdir();baseline.mkdir()
cli=[tc/'rexglue','recompile-tests','--bin-dir',w/'input','--asm-dir',w/'input']
execute([*cli,'--output',generated,'--templates',ROOT/'tests/stack-geometry/templates'],'production-generation')
execute([*cli,'--output',baseline],'production-default')
hashes={p.name:sha(p) for p in generated.iterdir()}
execute([*cli,'--output',generated,'--templates',ROOT/'tests/stack-geometry/templates'],'deterministic-regeneration')
assert hashes=={p.name:sha(p) for p in generated.iterdir()}
cpp=(generated/'ppc_test_functions.cpp').read_text();original=(baseline/'ppc_test_functions.cpp').read_text()
assert cpp[cpp.index('DEFINE_REX_FUNC('):]==original[original.index('DEFINE_REX_FUNC('):]
comments=[s.strip() for s in re.findall(r'^\s*// (.+)$',cpp,re.M) if not s.startswith(('Generated','DO NOT'))]
assert comments==[r['text'] for r in meta['rows']] and len(comments)==14
assert cpp.count('REX_STORE_U64(')==4 and cpp.count('REX_STORE_U32(')==2
assert 'REX_LOAD_' not in cpp
# Production bodies and helper call/LR anchor correspondence are checked without
# editing generated C++. Separate parser tests bind address expressions.
assert f'ctx.lr = 0x{meta["entry"]+12:X};' in cpp and 'REX_CALL_FUNC(geometry_'+f'{meta["helper"]:X})' in cpp
header='#pragma once\n#include "runtime/horizon/generated.h"\n'
for name in ['entry','helper']:header+=f'REX_EXTERN(geometry_{meta[name]:X});\ninline constexpr uint32_t geometry_{name}={meta[name]};\n'
header+='inline constexpr uint64_t geometry_module_id=0x535441434b3130;\ninline constexpr uint64_t geometry_source_tag=0x'+meta['raw_sha256'][:16]+';\n'
header+='struct GeometrySpan {int64_t offset;uint32_t width;bool operator==(const GeometrySpan&)const=default;};\ninline constexpr std::array<GeometrySpan,6> geometry_stores{{'+','.join('{'+str(s['offset'])+','+str(s['width'])+'}' for s in meta['footprint'])+'}};\n'
header+='inline const brutal::horizon::Function geometry_functions[]{'+','.join('{'+str(meta[n])+',geometry_'+f'{meta[n]:X}'+',"geometry_'+f'{meta[n]:X}'+'"}' for n in ['entry','helper'])+'};\n'
(w/'geometry-data.h').write_text(header);tests=[]
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/('host-'+profile)
 execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-isystem',tc/'include','-iquote',ROOT,'-I',ROOT/'tests/stack-geometry','-I',w,'-I',generated,ROOT/'tests/stack-geometry/probe.cpp',generated/'ppc_test_functions.cpp','-o',binary],'compile-'+profile)
 result=json.loads(execute([binary],'run-'+profile));assert result['allocations']==result['leases']==0 and result['refusals']==result['zero_entry_checks'];tests.append({'profile':profile,**result})
assert source=={n:sha(ROOT/n) for n in names} and hashes=={p.name:sha(p) for p in generated.iterdir()}
result={'tests':tests,'records':records,'fixture':meta,'generated_sha256':hashes,'sources':source,'generated_unedited':True,'production_bodies_equal_default':True,'hardware_execution':'NOT TESTED','game_executed':False}
(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(tests))
