#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original header lifecycle and central-memory regressions; no SDK or PPC needed."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--workdir',required=True,type=Path);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('new external outputs required')
w.mkdir(parents=True,exist_ok=False);rows=[];tests=[];commands=[]
header=bytes.fromhex('5845583200000001000000200000000000000000000000010002040112345678')
source=b'prefix01'+header+b'original-suffix!'
canonical=struct.pack('>6Q',0x4845414445523031,7,1,8,len(header),0x40000000)+hashlib.sha256(source).digest()+hashlib.sha256(header).digest()
(w/'source.bin').write_bytes(source);(w/'plan.bin').write_bytes(canonical);(w/'trusted.bin').write_bytes(hashlib.sha256(canonical).digest())
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/profile
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),str(ROOT/'tests/header-buffer/contract.cpp'),'-o',str(binary)],'compile-'+profile),([str(binary)],'run-'+profile)]:
  commands.append(cmd);r=run(cmd,w,label,30);rows.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
 result=json.loads((w/('run-'+profile+'.stdout')).read_text());assert result['header_registrations']==result['PPC_entries']==0
 reader=w/('data-'+profile)
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),str(ROOT/'tests/header-buffer/data.cpp'),'-o',str(reader)],'compile-data-'+profile),([str(reader),str(w/'source.bin'),str(w/'plan.bin'),str(w/'trusted.bin')],'data-'+profile)]:
  commands.append(cmd);r=run(cmd,w,label,30);rows.append(r)
  if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; diagnostics retained')
 data=json.loads((w/('data-'+profile+'.stdout')).read_text())
 assert data['header_sha256']==hashlib.sha256(header).hexdigest() and data['byte_comparisons']==len(header)
 assert data['allocations']==data['leases']==data['executable_images']==data['PPC_entries']==0
 tests.append({'profile':profile,**result,'reader':data})
(w/'COMMANDS.json').write_text(json.dumps(commands,indent=2)+'\n');(w/'results.json').write_text(json.dumps({'tests':tests,'records':rows},indent=2)+'\n');print(json.dumps(tests))
