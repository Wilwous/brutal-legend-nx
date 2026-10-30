#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Independent Python canonical oracle plus bounded C++ NativeImagePlan tests."""
import argparse,hashlib,json,struct,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser();ap.add_argument('--workdir',required=True,type=Path);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False)
source=bytes((i*17+3)%256 for i in range(8192));h=hashlib.sha256(source).digest();one=bytes([1])*32
regions=[[1,0x82000000,4096,0,4096,0,4096,5,1,1,1],[2,0x82001000,4096,4096,4096,0,4096,1,1,1,1],[3,0x82002000,8192,0,0,8192,4096,3,1,1,1]]
ints=[1,1,7,1,0x82000000,0x82005000,0x82000000,0x82001000,16,32,64,0]
canonical=b''.join(struct.pack('>Q',x) for x in ints)+h+one*3+struct.pack('>Q',len(regions))+b''.join(struct.pack('>Q',x) for row in regions for x in row)+struct.pack('>Q',0)
expected=hashlib.sha256(canonical).digest();(w/'oracle.h').write_text('#pragma once\ninline constexpr uint8_t expected_source[]{'+','.join(map(str,h))+'};\ninline constexpr uint8_t expected_plan[]{'+','.join(map(str,expected))+'};\n')
(w/'fixture-manifest.json').write_text(json.dumps({'source_sha256':h.hex(),'plan_sha256':expected.hex(),'canonical_hex':canonical.hex(),'regions':regions,'source_bytes':len(source),'original_synthetic':True},indent=2)+'\n')
records=[];tests=[]
for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 for cmd,label in [(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),'-I',str(w),str(Path(__file__).with_name('contract.cpp')),'-o',str(w/name)],'compile-'+name),([str(w/name)],'run-'+name)]:
  rec=run(cmd,w,label,30);records.append(rec)
  if rec['timeout'] or rec['exit_code']:raise RuntimeError(label+' failed; inspect diagnostics')
 tests.append({'profile':name,**json.loads((w/('run-'+name+'.stdout')).read_text())})
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records},indent=2)+'\n');print(json.dumps(tests))
