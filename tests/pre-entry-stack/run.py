#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Authoritative window tests; optional pinned production-generated PPC integration."""
from pathlib import Path
import argparse,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--workdir',type=Path,required=True);ap.add_argument('--toolchain',type=Path);ap.add_argument('--foundation',type=Path);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external outputs required')
if bool(a.toolchain)!=bool(a.foundation):ap.error('toolchain and generated foundation must be supplied together')
w.mkdir(parents=True,exist_ok=False);records=[];tests=[]
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def execute(cmd,label):
 r=run(list(map(str,cmd)),w,label,30);records.append(r)
 if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
 return (w/(label+'.stdout')).read_text()
sources=['runtime/horizon/space.h','runtime/horizon/guest.h','runtime/horizon/generated.h','tests/pre-entry-stack/window.cpp','tests/pre-entry-stack/entry.cpp'];before={n:sha(ROOT/n) for n in sources}
tc=a.toolchain.resolve() if a.toolchain else None;f=a.foundation.resolve() if a.foundation else None
if tc:
 pin=json.loads((tc/'prepared.json').read_text());assert pin['sdk_revision']=='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' and sha(tc/'rexglue')==pin['cli_sha256']
 result=json.loads((f/'results.json').read_text());generated=result['generation_sha256'];assert generated=={p.name:sha(p) for p in (f/'generated').iterdir()};assert result['static']['instructions']==30 and result['static']['functions']==3
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 common=['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',ROOT]
 binary=w/('window-'+profile);execute([*common,ROOT/'tests/pre-entry-stack/window.cpp','-o',binary],'compile-window-'+profile);row={'profile':profile,'window':json.loads(execute([binary],'window-'+profile))}
 if tc:
  inc=['-isystem',tc/'include','-I',f,'-I',f/'generated'];binary=w/('entry-'+profile)
  execute([*common,*inc,ROOT/'tests/pre-entry-stack/entry.cpp',f/'generated/ppc_test_functions.cpp','-o',binary],'compile-entry-'+profile);entry=json.loads(execute([binary],'entry-'+profile));assert entry['refusals']==entry['zero_entry_checks'] and entry['valid_leaf_entries']==7 and entry['stop_join_cycles']==3 and entry['allocations']==entry['leases']==0;row['generated_entry']=entry
  binary=w/('foundation-'+profile);execute([*common,*inc,ROOT/'tests/horizon-foundation/host.cpp',ROOT/'tests/horizon-foundation/probe.cpp',f/'generated/ppc_test_functions.cpp','-o',binary],'compile-foundation-'+profile);foundation=json.loads(execute([binary],'foundation-'+profile));assert foundation['passed']==108 and not foundation['failed'];row['foundation_replay']=foundation
 else:row['generated_entry']='NOT RUN without pinned SDK/verified production fixture; no mock used'
 assert row['window']['allocations']==row['window']['leases']==0;tests.append(row)
assert before=={n:sha(ROOT/n) for n in sources}
if tc:assert generated=={p.name:sha(p) for p in (f/'generated').iterdir()}
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records,'sources':before,'generated_output_unchanged':True if tc else None},indent=2)+'\n');print(json.dumps(tests))
