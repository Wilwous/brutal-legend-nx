#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Run original unified-library fixtures with a 30-second child/group watchdog."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import signal
import struct
import subprocess
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--sdk',type=Path,required=True)
    ap.add_argument('--source',type=Path,required=True)
    ap.add_argument('--workdir',type=Path,required=True)
    ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--pre-entry',action='store_true',help='Validate hardened failure contracts; Phase 7 remains the default')
    a=ap.parse_args();sdk=a.sdk.resolve();source=a.source.resolve();work=a.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('outputs must stay private')
    work.mkdir(parents=True,exist_ok=False)
    metadata=json.loads((sdk/'fixture-sdk.json').read_text());config=metadata['config']
    runtime=sdk/'lib'/metadata['runtime_filename']
    if hashlib.sha256(runtime.read_bytes()).hexdigest()!=metadata['runtime_sha256']:raise RuntimeError('SDK library fingerprint mismatch')
    # Prevent environment overrides from injecting the old installed SDK.
    for name in list(os.environ):
        if name.startswith(('DYLD_','REXSDK','VULKAN_','VK_')):del os.environ[name]
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0' if config=='Debug' else '-O2','-g','-I'+str(sdk/'include')]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    libraries=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
    results=[];blocked=[]
    def execute(command,label,expect=0):
        rec=run(command,work,label,30,True);results.append(rec)
        if rec['exit_code']!=expect or rec['timeout']:raise RuntimeError(label+' unexpected exit/timeout; inspect private diagnostics')
        return rec
    def compile_file(path,label,extra=(),link=True):
        binary=work/label;cmd=['clang++',*flags,*extra,str(ROOT/path),*(libraries if link else ['-c']),'-o',str(binary)]
        execute(cmd,'build-'+label);return binary
    for label,path,args in [
      ('phase6','tests/ppc/execution.cpp',[str(work/'phase6-data'),'unified',str(sdk)]),
      ('abi','tests/unified/abi.cpp',[str(work/'abi-data')]),
      ('errors','tests/unified/errors.cpp',[str(work/'error-data')]+(['hardened'] if a.pre_entry else [])),
      ('phase5','tests/runtime/harness.cpp',[]),
      ('native-memory','tests/runtime/sdk_memory_probe.cpp',[])]:
        binary=compile_file(path,label,['-iquote',str(ROOT)] if label=='phase5' else [])
        rec=execute([str(binary),*args],label)
        rows=[json.loads(line) for line in (work/(label+'.stdout')).read_text().splitlines() if line.startswith('{')]
        rec['checks']=rows
        blocked.extend(row for row in rows if row.get('status')=='BLOCKED')
        if any(row.get('result')=='FAIL' or row.get('failed',0) for row in rows):raise RuntimeError(label+' contract failure')
    # An unsupported ABI feature is an explicit blocker, never a passing ABI contract.
    negative=run(['clang++',*flags,'-c',str(ROOT/'tests/unified/vector-negative.cpp'),'-o',str(work/'vector-negative.o')],work,'typed-vector-negative')
    if negative['exit_code']==0 or negative['timeout']:raise RuntimeError('typed vector support unexpectedly changed')
    negative_text=(work/'typed-vector-negative.stderr').read_text()
    if 'VRegister' not in negative_text or 'GetValue' not in negative_text:raise RuntimeError('unrelated negative compile failure')
    blocked.append({'finding':'typed_vector_marshalling_unsupported','status':'BLOCKED'})
    suffix='d' if config=='Debug' else ''
    emitter=compile_file('tests/unified/emitter.cpp','emitter',[
      '-I'+str(source/'src/codegen/builders'),'-I'+str(source/'thirdparty/disasm'),
      '-lrexcodegen'+suffix,'-ldisasm'+suffix,'-lfmt'+suffix,'-lxxhash'+suffix])
    generated=work/'original-vmx.cpp';execute([str(emitter),str(generated)],'emit-original-vmx')
    generated_binary=work/'original-vmx';execute(['clang++',*flags,str(generated),*libraries,'-o',str(generated_binary)],'build-original-vmx')
    rec=execute([str(generated_binary)],'original-vmx');rec['checks']=[json.loads(line) for line in (work/'original-vmx.stdout').read_text().splitlines() if line.startswith('{')]
    # Native fault instructions are assembly; surrounding C++ follows this profile.
    fault_binary=compile_file('tests/unified/faults.cpp','faults')
    modes={'mmio':0,'readonly':-signal.SIGBUS,'sigsegv':-signal.SIGSEGV,'invalid-guest':-signal.SIGBUS,'noaccess-read':-signal.SIGBUS,'nested':128+signal.SIGSEGV,'hole-probe':(-signal.SIGBUS if a.pre_entry else 79)}
    for mode,status in modes.items():
        rec=execute([str(fault_binary),mode],'fault-'+mode,status)
        raw=(work/('fault-'+mode+'.stderr')).read_bytes()
        if mode=='hole-probe' and not a.pre_entry:
            blocked.append({'finding':'unallocated_guest_hole_writable','status':'BLOCKED'})
        if mode!='mmio' and (mode!='hole-probe' or a.pre_entry):
            found=[]
            for offset in range(0,len(raw)-31,8):
                values=struct.unpack_from('=IiiIQQ',raw,offset)
                if values[0]==0x52455846:found.append({'signal':values[1],'code':values[2],'nested':values[3],'address':hex(values[4]),'pc':hex(values[5])})
            rec['fault_records']=found
            if not found:raise RuntimeError('missing compact signal record: '+mode)
    # Phase 6 claimed PC recovery through this same newly built library.
    old_fault=compile_file('tests/ppc/host_fault.cpp','phase6-fault')
    old_modes={'sdk-recover':0,'posix-capture':80,'sdk-decline':-signal.SIGBUS,'sdk-memory':-signal.SIGBUS}
    if not a.sanitize:old_modes['posix-default']=-signal.SIGBUS
    for mode,status in old_modes.items():execute([str(old_fault),mode],'phase6-'+mode,status)
    summary={'sdk':metadata,'sanitizers':a.sanitize,'records':results,'blocked_findings':blocked,
             'passed_groups':sum(sum(x.get('result')=='PASS' for x in r.get('checks',[])) for r in results),
             'fault_checks':len(modes)-(0 if a.pre_entry else 1)+len(old_modes),'unexpected_failures':0}
    (work/'results.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps({k:summary[k] for k in ['passed_groups','fault_checks','unexpected_failures','blocked_findings']}))
if __name__=='__main__':
    try:main()
    except (RuntimeError,OSError,subprocess.SubprocessError) as exc:print(str(exc),file=sys.stderr);sys.exit(1)
