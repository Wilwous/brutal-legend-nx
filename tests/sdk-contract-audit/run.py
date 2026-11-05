#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original contract audit; optional real pinned SDK/production corpus."""
import argparse,hashlib,json,sys
from pathlib import Path
R=Path(__file__).resolve().parents[2];sys.path.insert(0,str(R/'tests/ppc'));from watchdog import run
p=argparse.ArgumentParser();p.add_argument('--workdir',type=Path,required=True);p.add_argument('--toolchain',type=Path);p.add_argument('--acceptance-output',type=Path);a=p.parse_args();w=a.workdir.resolve()
if w==R or R in w.parents:p.error('fresh external output required')
if bool(a.toolchain)!=bool(a.acceptance_output):p.error('both pinned toolchain and verified acceptance output required')
w.mkdir(parents=True,exist_ok=False);tc=a.toolchain.resolve() if a.toolchain else None;old=a.acceptance_output.resolve() if a.acceptance_output else None;sha=lambda f:hashlib.sha256(f.read_bytes()).hexdigest();records=[];tests=[]
if tc:
 receipt=json.loads((tc/'prepared.json').read_text());assert sha(tc/'rexglue')==receipt['cli_sha256'];result=json.loads((old/'results.json').read_text());assert result['stage']==15 and all(t['failed']==0 for t in result['host'])
 for n,h in result['generation_sha256'].items():assert sha(old/'input/generated'/n)==h
 lock=json.loads((R/'tests/horizon-xex/sdk-lock.json').read_text());assert sha(old/'sdk-compare.inc')==lock['function_sha256']
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 for suite in ['transactions','edges']+(['lifetime','compare'] if tc else []):
  binary=w/(suite+'-'+profile);extra=[]
  if suite=='compare':extra=[R/n for n in ['tests/horizon-xex/import.cpp','tests/horizon-expansion/import.cpp','tests/horizon-runtime-ii/query.cpp','tests/horizon-acceptance/fill.cpp']]+[old/'input/generated/fixture_recomp.0.cpp',old/'input/generated/fixture_init.cpp',old/'hooks.cpp']
  command=['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',R,*(['-isystem',tc/'include','-I',old,'-I',old/'input/generated'] if tc else []),R/'tests/sdk-contract-audit'/(suite+'.cpp'),*extra,'-o',binary]
  for cmd,label in [(command,'compile-'+suite+'-'+profile),([binary],'run-'+suite+'-'+profile)]:
   r=run(list(map(str,cmd)),w,label,30);records.append(r)
   if r['exit_code'] or r['timeout']:raise RuntimeError(label+' failed; diagnostics retained')
  row=json.loads((w/('run-'+suite+'-'+profile+'.stdout')).read_text());tests.append({'profile':profile,'suite':suite,**row});print(json.dumps(tests[-1]),flush=True)
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records,'proprietary_execution':False,'hardware':'UNKNOWN'},indent=2)+'\n')
