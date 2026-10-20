#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Optional hash-checked private XEX load; no generated game code compilation or execution."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--sdk',type=Path,required=True)
ap.add_argument('--game-root',type=Path,required=True)
ap.add_argument('--workdir',type=Path,required=True)
ap.add_argument('--sanitize',action='store_true')
ap.add_argument('--pre-entry',action='store_true',help='Require validated XDBF from the Phase 8 SDK')
a=ap.parse_args();work=a.workdir.resolve();sdk=a.sdk.resolve()
if ROOT==work or ROOT in work.parents:ap.error('output must be private')
if hashlib.sha256((a.game_root/'default.xex').read_bytes()).hexdigest()!='636a8036b630ae64acb582de4ac7303b8435de808f67e5343f1ddebc74d29456':ap.error('original XEX hash mismatch')
meta=json.loads((sdk/'fixture-sdk.json').read_text())
if hashlib.sha256((sdk/'lib'/meta['runtime_filename']).read_bytes()).hexdigest()!=meta['runtime_sha256']:ap.error('SDK hash mismatch')
for name in list(os.environ):
 if name.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[name]
os.environ['UBSAN_OPTIONS']='print_stacktrace=1'
work.mkdir(parents=True,exist_ok=False);binary=work/'loader'
flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-g','-I'+str(sdk/'include')]
if a.pre_entry:flags+=['-DREX_PRE_ENTRY=1']
if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all']
for command,label in [(['clang++',*flags,str(ROOT/'tests/unified/loader.cpp'),'-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',str(binary)],'build-loader'),([str(binary),str(a.game_root),str(work),str(sdk)],'loader')]:
 r=run(command,work,label,30,True)
 if r['exit_code'] or r['timeout']:print('private loader probe failed; inspect private diagnostics',file=sys.stderr);sys.exit(1)
rows=[json.loads(line) for line in (work/'loader.stdout').read_text().splitlines() if line.startswith('{')]
if len(rows)!=1 or rows[0].get('result')!='PASS':sys.exit(1)
print(json.dumps(rows[0]))
