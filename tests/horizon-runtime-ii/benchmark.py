#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded host-only performance baseline over original production output."""
import argparse,hashlib,json,sys,statistics
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__)
for name in ['build','toolchain','workdir']:ap.add_argument('--'+name,type=Path,required=True)
a=ap.parse_args();w=a.workdir.resolve();build=a.build.resolve();tc=a.toolchain.resolve()
if w==ROOT or ROOT in w.parents:ap.error('private output required')
w.mkdir(parents=True,exist_ok=False);generated=build/'input/generated';receipt=json.loads((build/'results.json').read_text())
if receipt['stage']!=11 or any(r['failed'] for r in receipt['host']):raise ValueError('verified Stage 11 host output required')
for name,digest in receipt['generation_sha256'].items():
 if hashlib.sha256((generated/name).read_bytes()).hexdigest()!=digest:raise ValueError('generated output changed')
records=[]
def execute(command,label):
 row=run(command,w,label,30);records.append(row)
 if row['timeout'] or row['exit_code']:raise RuntimeError(label+' failed; private diagnostics retained')
execute(['clang++','-std=c++23','-pthread','-O2','-Wall','-Wextra','-Werror','-isystem',str(tc/'include'),'-iquote',str(ROOT),'-I',str(build),'-I',str(generated),str(ROOT/'tests/horizon-runtime-ii/benchmark.cpp'),str(ROOT/'tests/horizon-xex/import.cpp'),str(ROOT/'tests/horizon-expansion/import.cpp'),str(ROOT/'tests/horizon-runtime-ii/query.cpp'),str(generated/'fixture_recomp.0.cpp'),str(generated/'fixture_init.cpp'),str(build/'hooks.cpp'),'-o',str(w/'benchmark')],'compile')
# Retain and validate the warm-up, but exclude its timings from the comparison.
runs=[]
for index in range(5):
 label='warmup' if not index else 'run-'+str(index)
 execute([str(w/'benchmark'),*(['--reverse'] if index%2 else [])],label)
 row=json.loads((w/(label+'.stdout')).read_text())
 if row['failed'] or row['remaining'] or row['leases'] or any(m['failed'] for m in row['modes']):raise ValueError('benchmark failure')
 if index:runs.append(row)
modes=[]
for tracing in [False,True]:
 selected=[next(m for m in r['modes'] if m['tracing']==tracing) for r in runs];measurements={}
 for name in selected[0]['measurements']:
  values=[m['measurements'][name] for m in selected];means=[v['mean_us'] for v in values]
  measurements[name]={'runs':4,'samples_per_run':values[0]['samples'],'operations_per_sample':values[0]['operations_per_sample'],'mean_us':statistics.mean(means),'min_us':min(v['min_us'] for v in values),'max_us':max(v['max_us'] for v in values),'between_run_stddev_us':statistics.pstdev(means)}
 modes.append({'tracing':tracing,'measurements':measurements,'failed':0})
result={'host_only':True,'units':'microseconds_per_operation','method':'one validated warm-up excluded; four sequential runs alternating mode order; steady_clock; O2; trace clear outside timings','failed':0,'remaining':0,'leases':0,'modes':modes,'runs':runs,'process_records':records,'build_fingerprint':receipt['build_fingerprint']}
(w/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items() if k not in ('runs','process_records')},indent=2))
