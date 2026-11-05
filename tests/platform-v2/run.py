#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import argparse
import json
from pathlib import Path
from generate import ROOT,prepare,run,sha
p=argparse.ArgumentParser();p.add_argument('--workdir',type=Path,required=True);p.add_argument('--toolchain',type=Path,required=True);a=p.parse_args();w=a.workdir.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:p.error('fresh external output required')
w.mkdir(parents=True,exist_ok=False);result=prepare(w,tc);
from assets import create as create_assets
create_assets(w/'assets');records=result['records'];tests=[]
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/profile;generated=w/'input/generated'
 cmd=['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',ROOT,'-isystem',tc/'include','-I',w,'-I',generated,ROOT/'tests/platform-v2/host.cpp',ROOT/'tests/platform-v2/demo.cpp',generated/'fixture_recomp.0.cpp',generated/'fixture_init.cpp',w/'hooks.cpp','-o',binary]
 for label,command in [('compile-'+profile,cmd),('run-'+profile,[binary,w/'assets'])]:
  row=run(list(map(str,command)),w,label,30);records.append(row)
  if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; see bounded diagnostics')
 data=json.loads((w/('run-'+profile+'.stdout')).read_text());assert data['failures']==0 and data['allocations']==data['leases']==data['resource_handles']==0
 tests.append(dict(profile=profile,**data));print(json.dumps(tests[-1]),flush=True)
result['tests']=tests;result['game_executed']=False;result['binary_hashes']={v:sha(w/v) for v in ['Debug','Release','Sanitized']};(w/'results.json').write_text(json.dumps(result,indent=2)+'\n')
