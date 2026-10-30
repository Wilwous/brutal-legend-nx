#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original DATA-only validator protocol fixtures. No title files or guest entry."""
from pathlib import Path
import argparse,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tests/ppc'));from watchdog import run
ap=argparse.ArgumentParser();ap.add_argument('--workdir',type=Path,required=True);a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False);records=[];tests=[]
source=b'original';(w/'source.bin').write_bytes(source);(w/'other.bin').write_bytes(b'different')
header=[1,0,1,1,4096,8192,4096,4096,0,32,64,0];row=[1,4096,4096,0,8,4088,4096,1,1,1,1]
def encode(h=header,r=row):return b'ND01'+b''.join(struct.pack('>Q',x) for x in h)+hashlib.sha256(source).digest()+bytes(96)+struct.pack('>Q',1)+b''.join(struct.pack('>Q',x) for x in r)
(w/'candidate.bin').write_bytes(encode());bad=list(header);bad[4]=1<<32;(w/'overflow.bin').write_bytes(encode(bad));badrow=list(row);badrow[-1]=2;(w/'boolean.bin').write_bytes(encode(r=badrow));(w/'truncated.bin').write_bytes(encode()[:-1]);(w/'magic.bin').write_bytes(b'BAD!'+encode()[4:])
def execute(cmd,label,expected=0):
 v=run(list(map(str,cmd)),w,label,30);v['expected_exit_code']=expected;records.append(v)
 if v['timeout'] or v['exit_code']!=expected:raise RuntimeError(label)
 return (w/(label+'.stdout')).read_text()
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
 binary=w/profile;execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',ROOT,ROOT/'tests/native-provenance/native_refusal.cpp','-o',binary],'compile-'+profile)
 for kind,match in [('source',True),('other',False)]:
  v=json.loads(execute([binary,w/'candidate.bin',w/(kind+'.bin')],kind+'-'+profile));assert v['source_identity_match']==match and v['expected_refusals']==2 and v['guest_images_constructed']==0
 for name,code in [('overflow',1),('boolean',1),('truncated',1),('magic',2)]:execute([binary,w/(name+'.bin'),w/'source.bin'],name+'-'+profile,code)
 tests.append({'profile':profile,'identity_controls':2,'native_validator_refusals':4,'malformed_protocol_refusals':4,'guest_instructions':0})
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records},indent=2)+'\n');print(json.dumps(tests))
