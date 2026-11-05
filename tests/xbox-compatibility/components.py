#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded portable platform services tests; no SDK, title or device input."""
import argparse
import json
import sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
p=argparse.ArgumentParser();p.add_argument('--workdir',type=Path,required=True);p.add_argument('--sdk-include',type=Path);a=p.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:p.error('external fresh output required')
w.mkdir(parents=True,exist_ok=False);records=[];results=[]
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 for suite in (['words','services','formats'] if a.sdk_include else ['words','formats']):
  binary=w/(suite+'-'+profile)
  commands=[(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),*(['-isystem',str(a.sdk_include)] if a.sdk_include else []),str(ROOT/'tests/xbox-compatibility'/(suite+'.cpp')),'-o',str(binary)],'compile'),([str(binary)],'run')]
  for cmd,kind in commands:
   label=kind+'-'+suite+'-'+profile;r=run(cmd,w,label,30);records.append(r)
   if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; inspect retained diagnostics')
  data=json.loads((w/('run-'+suite+'-'+profile+'.stdout')).read_text());results.append({**data,'profile':profile,'suite':suite});print(json.dumps(results[-1]),flush=True)
(w/'results.json').write_text(json.dumps({'results':results,'records':records,'hardware':'UNKNOWN','proprietary_execution':False},indent=2)+'\n')
