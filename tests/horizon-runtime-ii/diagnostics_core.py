#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Public bounded diagnostic regression; no SDK, device, or proprietary input."""
import argparse,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--workdir',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('private output required')
w.mkdir(parents=True,exist_ok=False);rows=[];records=[]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/name
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),str(ROOT/'tests/horizon-runtime-ii/diagnostics_core.cpp'),'-o',str(binary)],'compile-'+name),([str(binary)],'run-'+name)]:
  r=run(cmd,w,label,30);records.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed')
 row=json.loads((w/('run-'+name+'.stdout')).read_text());assert row=={'passed':22,'failed':0},row;rows.append({'profile':name,**row})
(w/'results.json').write_text(json.dumps({'tests':rows,'records':records},indent=2)+'\n');print(json.dumps(rows))
