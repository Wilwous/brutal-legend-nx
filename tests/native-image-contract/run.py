#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded host-only native image model; no production loader or PPC execution."""
from pathlib import Path
import argparse,sys,json
from fixture import create
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser();ap.add_argument('--workdir',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False);image=create(w/'original-fixture');records=[];tests=[]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),str(Path(__file__).with_name('contract.cpp')),'-o',str(w/name)],'compile-'+name),([str(w/name),str(image)],'run-'+name)]:
  v=run(cmd,w,label,30);records.append(v)
  if v['timeout'] or v['exit_code']:raise RuntimeError(label)
 row=json.loads((w/('run-'+name+'.stdout')).read_text());tests.append({'profile':name,**row})
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records},indent=2)+'\n');print(json.dumps(tests))
