#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original PPC fixtures with pinned real ReXGlue SDK implementations."""
import argparse
import hashlib
import json
from pathlib import Path
import signal
import subprocess
import struct
import sys
from watchdog import run
ROOT=Path(__file__).resolve().parents[2]
PINS={
 'src/system/xthread.cpp':'065605de5137129ce2c0f9ff4ede028219e35d5ec1f3f2e2d38226ef94d6ac61',
 'src/kernel/xboxkrnl/xboxkrnl_io.cpp':'1ec34917f7029cf9e8b6fb540962d77b2a5b29a819d44877c22676f24cc2ebca',
 'src/kernel/xboxkrnl/xboxkrnl_memory.cpp':'80a1a7a55b75a6c05d0e57406cfe2db11ce97afc42140683e92ba400e516cc07',
 'src/kernel/xboxkrnl/xboxkrnl_threading.cpp':'687c0c5c4477c61c3e7fc8666cd65a0cfa98e145d37ca74c295580aca5da81fc',
 'src/core/exception_handler_posix.cpp':'c615444b11c59377536e0616aff93b4500bc0052de47b74208f4d36a673b46ac',
 'src/codegen/builders/vector.cpp':'b3528ed4a7e0642e2f390cf3d745e18d6e496f14d1707afef6f7049bb84fb39d',
}

def function(source, signature):
    start=source.index(signature);opening=source.index('{',start);depth=1;end=opening+1
    while depth:
        if source[end]=='{':depth+=1
        if source[end]=='}':depth-=1
        end+=1
    return source[start:end]+'\n'


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--workdir',type=Path,required=True)
    ap.add_argument('--sdk-source-root',type=Path,required=True)
    ap.add_argument('--sdk-binary-root',type=Path,required=True)
    ap.add_argument('--compiler',default='clang++')
    ap.add_argument('--optimization',choices=['0','2'],default='0')
    ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--faults',action='store_true',help='Run isolated POSIX and patched-dispatcher fault checks')
    ap.add_argument('--reproduce-timeouts',action='store_true',help='Explicit opt-in: two original SDK 30-second fault loops, once per diagnostic session')
    a=ap.parse_args();work=a.workdir.resolve();sdk=a.sdk_binary_root.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('outputs must be outside checkout')
    work.mkdir(parents=True,exist_ok=False)
    results=[]
    def checked(command,label,cwd=None):
        # Workaround for git patch working directory; all commands still have a group watchdog.
        r=run(command,cwd or work,label)
        if r['exit_code']!=0 or r['timeout']:
            raise RuntimeError(label+' failed; inspect private '+label+'.stderr')
        return r
    for name,sha in PINS.items():
        raw=(a.sdk_source_root/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=sha:raise RuntimeError('source fingerprint mismatch: '+name)
        dest=work/'sdk'/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
    for patch in ['0001-runtime-safety.patch','0002-host-fault-and-protect.patch']:
        checked(['git','apply','--check',str(ROOT/'patches/rexglue-sdk'/patch)],patch+'-check',work/'sdk')
        checked(['git','apply',str(ROOT/'patches/rexglue-sdk'/patch)],patch+'-apply',work/'sdk')
    source=(work/'sdk/src/system/xthread.cpp').read_text()
    # These are exact method bodies, using the real SDK class, real allocations and lifecycle.
    # Keep a single prebuilt XThread::Create/current-thread binding across Mach-O images.
    (work/'tls-methods.cpp').write_text('#include <rex/system/xthread.h>\n#include <rex/memory.h>\nnamespace rex::system {\n'+
        ''.join(function(source,'bool XThread::'+n) for n in ['GetTLSValue','SetTLSValue'])+'}\n')
    flags=['-std=c++23','-DSPDLOG_FMT_EXTERNAL','-pthread','-g','-O'+a.optimization,'-I'+str(sdk/'include')]
    if a.sanitize:flags+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    libs=['-L'+str(sdk/'lib'),'-lrexruntime','-Wl,-rpath,'+str(sdk/'lib')]
    objects=[]
    for source in [work/'tls-methods.cpp',*[work/'sdk/src/kernel/xboxkrnl'/name for name in ['xboxkrnl_io.cpp','xboxkrnl_memory.cpp','xboxkrnl_threading.cpp']]]:
        obj=work/(source.stem+'.o');checked([a.compiler,*flags,'-c',str(source),'-o',str(obj)],'build-'+source.stem);objects.append(str(obj))
    for patched in [False,True]:
        label='patched' if patched else 'baseline';binary=work/('execution-'+label)
        checked([a.compiler,*flags,str(ROOT/'tests/ppc/execution.cpp'),*(objects if patched else []),*libs,'-o',str(binary)],'build-'+label)
        r=run([str(binary),str(work/label),label],work,label)
        groups=[]
        for line in (work/(label+'.stdout')).read_text().splitlines():
            if line.startswith('{'):groups.append(json.loads(line))
        expected=7 if patched else 6
        r.update({'groups':groups,'passed':sum(g.get('result')=='PASS' for g in groups),
                  'failed':int(r['exit_code']!=0 or r['timeout'] or sum(g.get('result')=='PASS' for g in groups)!=expected)})
        results.append(r)
    if a.faults or a.reproduce_timeouts:
        # Fault checks intentionally use non-sanitized handlers: ASan owns signal handling.
        fault_flags=[f for f in flags if not f.startswith('-fsanitize') and not f.startswith('-fno-sanitize')]
        handler=work/'exception-handler.o'
        checked([a.compiler,*fault_flags,'-c',str(work/'sdk/src/core/exception_handler_posix.cpp'),'-o',str(handler)],'build-handler')
        for patched in [False,True]:
            binary=work/('fault-patched' if patched else 'fault-baseline')
            checked([a.compiler,*fault_flags,str(ROOT/'tests/ppc/host_fault.cpp'),*([str(handler)] if patched else []),*libs,'-o',str(binary)],'build-'+binary.name)
            modes=['posix-default','posix-capture','sdk-recover'] if not patched else ['sdk-decline','sdk-memory','sdk-recover']
            if not a.faults:modes=[]
            if a.reproduce_timeouts and not patched:modes+=['sdk-decline','sdk-memory']
            for mode in modes:
                label=binary.name+'-'+mode;r=run([str(binary),mode],work,label,30,True)
                expected_signal=r['exit_code'] in [-signal.SIGBUS,-signal.SIGSEGV]
                timeout_expected=not patched and mode in ['sdk-decline','sdk-memory']
                ok=r['timeout'] if timeout_expected else not r['timeout'] and (r['exit_code']==80 if mode=='posix-capture' else r['exit_code']==0 if mode=='sdk-recover' else expected_signal)
                outcome='timeout' if r['timeout'] else 'recovered_fault' if mode=='sdk-recover' and ok else 'expected_fault' if ok else 'unexpected_crash'
                r.update({'outcome':outcome,'passed':int(ok),'failed':int(not ok)})
                raw=(work/(label+'.stderr')).read_bytes()
                if mode=='posix-capture' and len(raw)==24:
                    number,code,address,pc=struct.unpack('=iiQQ',raw)
                    r['fault_record']={'signal':number,'si_code':code,'address':hex(address),'pc':hex(pc)}
                elif mode in ['sdk-decline','sdk-memory'] and len(raw)%24==0:
                    r['sdk_fault_records']=[{'address':hex(address),'pc':hex(pc),'operation':op}
                        for address,pc,op in struct.iter_unpack('=QQQ',raw)]
                results.append(r)
    summary={'optimization':a.optimization,'sanitizers':a.sanitize,'prebuilt_sdk_instrumented':False,
             'binary_fingerprints':{file.name:hashlib.sha256(file.read_bytes()).hexdigest()
                for file in sorted((sdk/'lib').glob('librexruntime.*')) if file.is_file()},
             'passed':sum(r['passed'] for r in results),'failed':sum(r['failed'] for r in results),'results':results}
    (work/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({'passed':summary['passed'],'failed':summary['failed']}))
    return bool(summary['failed'])
if __name__=='__main__':
    try:sys.exit(main())
    except (RuntimeError,OSError,subprocess.SubprocessError) as exc:
        print(str(exc),file=sys.stderr);sys.exit(1)
