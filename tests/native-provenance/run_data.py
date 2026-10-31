#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original data-only contracts; optional real SDK-header synthetic context.

No code generation, title data or device access. Context test takes no files.
"""
from pathlib import Path
import argparse, hashlib, json, struct, sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--workdir',type=Path,required=True)
ap.add_argument('--toolchain',type=Path)
a=ap.parse_args();w=a.workdir.resolve()
if w==ROOT or ROOT in w.parents:ap.error('external output required')
w.mkdir(parents=True,exist_ok=False);records=[];tests=[]
def execute(cmd,label,expected=0):
    row=run(list(map(str,cmd)),w,label,30);row['expected_exit_code']=expected;records.append(row)
    if row['timeout'] or row['exit_code']!=expected:raise RuntimeError(label+' failed; logs retained')
    return (w/(label+'.stdout')).read_text()
source=bytes((i*7+3)%256 for i in range(64));(w/'source.bin').write_bytes(source)
regions=[(0x1000,0,16,1,1,1),(0x1020,32,12,2,3,1),(0x102c,44,4,0,0,1)]
canonical=b''.join(struct.pack('>Q',v) for v in [0x444154413031,1,0x1000,7,1])+bytes([2])*32+hashlib.sha256(source).digest()+struct.pack('>Q',3)
for r in regions:canonical+=b''.join(struct.pack('>Q',v) for v in r)+hashlib.sha256(source[r[1]:r[1]+r[2]]).digest()
(w/'plan.bin').write_bytes(canonical);(w/'trusted.bin').write_bytes(hashlib.sha256(canonical).digest());(w/'bad-source.bin').write_bytes(b'X'+source[1:]);(w/'truncated.bin').write_bytes(canonical[:-1]);bad=bytearray(canonical);bad[23]^=1;(w/'stale.bin').write_bytes(bad)
tc=a.toolchain.resolve() if a.toolchain else None
if tc:
    receipt=json.loads((tc/'prepared.json').read_text())
    if receipt['sdk_revision']!='f5337cdc947ff6d4c4196737e2c807a48f2a1fc2' or hashlib.sha256((tc/'rexglue').read_bytes()).hexdigest()!=receipt['cli_sha256']:raise ValueError('pinned SDK receipt mismatch')
for profile,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
    def compile(name,extra=()):
        binary=w/(name+'-'+profile)
        execute(['clang++','-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',ROOT,*extra,ROOT/'tests/native-provenance'/(name+'.cpp'),'-o',binary],'compile-'+name+'-'+profile)
        return binary
    contract=compile('data_contract');row=json.loads(execute([contract],'data-contract-'+profile));assert row['byte_comparisons']==32 and row['live_data']==row['guest_images']==row['guest_execution']==0
    reader=compile('data_reader');observed=json.loads(execute([reader,w/'plan.bin',w/'source.bin',w/'trusted.bin'],'data-reader-'+profile))
    oracle=b''.join(source[r[1]:r[1]+r[2]] for r in regions)
    assert observed['copy_sha256']==hashlib.sha256(oracle).hexdigest() and observed['copied_bytes']==32 and observed['expected_guest_refusals']==3 and observed['rollback_refusals']==1 and observed['live_buffers']==observed['guest_images']==observed['guest_execution']==0
    for name,metadata,src,reason in [('wrong-source','plan.bin','bad-source.bin','data source fingerprint'),('truncated','truncated.bin','source.bin','truncated data digest'),('stale','stale.bin','source.bin','stale or untrusted data plan')]:
        label=name+'-'+profile;execute([reader,w/metadata,w/src,w/'trusted.bin'],label,1)
        if (w/(label+'.stderr')).read_text().strip()!=reason:raise RuntimeError('unclassified data refusal: '+label)
    result={'profile':profile,'data':row,'reader':observed,'reader_negative_refusals':3}
    if tc:
        context=compile('context_contract',['-isystem',tc/'include']);ctx=json.loads(execute([context],'context-'+profile));assert ctx['live_allocations']==ctx['leases']==ctx['guest_entries']==0 and ctx['byte_comparisons']==32;result['context']=ctx
    else:result['context']='NOT RUN: pinned SDK headers not supplied; not replaced with mock context'
    tests.append(result)
(w/'results.json').write_text(json.dumps({'tests':tests,'records':records},indent=2)+'\n');print(json.dumps(tests))
