#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
import argparse,json,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser();ap.add_argument('--workdir',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve()
if w==R or R in w.parents:ap.error('new external output required')
w.mkdir(parents=True,exist_ok=False);records=[];tests=[];commands=[]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 b=w/name
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(R),str(R/'tests/native-module/contract.cpp'),'-o',str(b)],'compile-'+name),([str(b)],'run-'+name)]:
  commands.append(cmd);x=run(cmd,w,label,30);records.append(x)
  if x['timeout'] or x['exit_code']:raise RuntimeError(label+' failed; logs retained')
 row=json.loads((w/('run-'+name+'.stdout')).read_text());assert row['allocations']==row['leases']==0;tests.append({'profile':name,**row})
(w/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n');(w/'results.json').write_text(json.dumps({'tests':tests,'records':records},indent=2)+'\n');print(json.dumps(tests))
