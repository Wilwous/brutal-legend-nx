#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded host-only performance baseline over original production output."""
import argparse,hashlib,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__)
for name in ['build','toolchain','workdir']:ap.add_argument('--'+name,type=Path,required=True)
a=ap.parse_args();w=a.workdir.resolve();build=a.build.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('private output required')
w.mkdir(parents=True,exist_ok=False);generated=build/'input/generated';receipt=json.loads((build/'results.json').read_text())
if receipt['stage']!=8 or any(r['failed'] for r in receipt['host']):raise ValueError('verified Stage 8 host output required')
for name,digest in receipt['generation_sha256'].items():
 if hashlib.sha256((generated/name).read_bytes()).hexdigest()!=digest:raise ValueError('generated output changed')
records=[]
commands=[(['clang++','-std=c++23','-pthread','-O2','-Wall','-Wextra','-Werror','-isystem',str(tc/'include'),'-iquote',str(ROOT),'-I',str(build),'-I',str(generated),str(ROOT/'tests/horizon-expansion/benchmark.cpp'),str(ROOT/'tests/horizon-xex/import.cpp'),str(ROOT/'tests/horizon-expansion/import.cpp'),str(generated/'fixture_recomp.0.cpp'),str(generated/'fixture_init.cpp'),str(build/'hooks.cpp'),'-o',str(w/'benchmark')],'compile'),([str(w/'benchmark')],'run')]
for command,label in commands:
 row=run(command,w,label,30);records.append(row)
 if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
result=json.loads((w/'run.stdout').read_text());result['process_records']=records
(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2))
