#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Optional existing isolated SDK binary test. No build/download of the SDK or title."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
from fixture import cases, first_match_oracle
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--sdk-root',required=True,type=Path,help='existing staged root containing Debug/Release/Sanitized')
ap.add_argument('--workdir',required=True,type=Path)
ap.add_argument('--reference',type=Path,help='optional pinned XenonUtils header directory; no download')
a=ap.parse_args();w=a.workdir.resolve()
if a.reference:a.reference=a.reference.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external new output required')
w.mkdir(parents=True,exist_ok=False);records=[];results=[];commands=[]
REFERENCE_HASHES={'xex.h': 'd75db078add62416cb465f06550c89e470382895d098ee8e95ad49b6901fc9b8', 'xbox.h': 'eba4079e8379bdb138f4da77405a3432df60f8bcdf3f47cbc3ca1c8543cf5814', 'byteswap.h': 'e736c4e1386cbdf467cba3938dbd892b6488417700818a80e6ecf0a834e59bbd'}
if a.reference:
    for name,digest in REFERENCE_HASHES.items():
        if hashlib.sha256((a.reference/name).read_bytes()).hexdigest()!=digest:raise ValueError('reference source fingerprint')
for key in list(os.environ):
    if key.startswith(('DYLD_','REXSDK','VK_','VULKAN_')):del os.environ[key]
for name,blob in cases():(w/(name+'.bin')).write_bytes(blob)
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
    sdk=(a.sdk_root/profile).resolve();meta=json.loads((sdk/'fixture-sdk.json').read_text())
    assert meta['config']==profile and meta['sdk_revision']=='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2'
    library=sdk/'lib'/meta['runtime_filename'];digest=hashlib.sha256(library.read_bytes()).hexdigest();assert digest==meta['runtime_sha256']
    def execute(cmd,label):
        commands.append(list(map(str,cmd)));r=run(list(map(str,cmd)),w,label,30);records.append(r)
        if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
    binary=w/('sdk-'+profile)
    execute(['clang++','-std=c++23','-DSPDLOG_FMT_EXTERNAL',*flags,'-I'+str(sdk/'include'),ROOT/'tests/header-field/sdk.cpp','-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib'),'-o',binary],'compile-'+profile)
    execute([binary,w/('runtime-'+profile),*[w/(name+'.bin') for name,_ in cases()]],'run-'+profile)
    rows=[json.loads(line) for line in (w/('run-'+profile+'.stdout')).read_text().splitlines() if line.startswith('{')]
    assert len(rows)==9
    for row,(name,blob) in zip(rows[:-1],cases()):
        assert row['case']==name;expected=first_match_oracle(blob)
        assert row['helper']==row['result']==(row['base']+expected[0] if expected else 0)
        assert row['status']==(0 if expected else 0xc0000225)
    assert rows[-1]['outstanding_fixture_allocations']==rows[-1]['PPC_bodies']==0
    assert hashlib.sha256(library.read_bytes()).hexdigest()==digest
    reference = 'NOT RUN: no pinned separate host parser supplied'
    if a.reference:
        ref_binary=w/('reference-'+profile)
        execute(['clang++','-std=c++23',*flags,'-I'+str(a.reference),ROOT/'tests/header-field/reference.cpp','-o',ref_binary],'compile-reference-'+profile)
        execute([ref_binary,*[w/(name+'.bin') for name,_ in cases()]],'reference-'+profile)
        reference=[json.loads(line) for line in (w/('reference-'+profile+'.stdout')).read_text().splitlines()]
        assert len(reference)==8
        for row,(name,blob) in zip(reference,cases()):
            expected=first_match_oracle(blob)
            assert row['case']==name and row['offset']==(expected[0] if expected else -1)
    results.append({'profile':profile,'library_sha256':digest,'cases':rows[:-1],'summary':rows[-1],'oracle_agreements':8,'reference':reference,'historical_Xbox':'UNKNOWN'})
(w/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n');(w/'results.json').write_text(json.dumps({'tests':results,'records':records},indent=2)+'\n');print(json.dumps(results))
