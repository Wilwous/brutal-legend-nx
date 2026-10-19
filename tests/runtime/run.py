#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Build deterministic original component tests, optionally pinned SDK leaf probes.

All output must be outside the checkout. No game input, codegen or guest entry.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import resource
import signal

ROOT = Path(__file__).resolve().parents[2]
PINS = {
 'src/system/xthread.cpp':'065605de5137129ce2c0f9ff4ede028219e35d5ec1f3f2e2d38226ef94d6ac61',
 'src/kernel/xboxkrnl/xboxkrnl_io.cpp':'1ec34917f7029cf9e8b6fb540962d77b2a5b29a819d44877c22676f24cc2ebca',
 'src/kernel/xboxkrnl/xboxkrnl_memory.cpp':'80a1a7a55b75a6c05d0e57406cfe2db11ce97afc42140683e92ba400e516cc07',
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
    ap.add_argument('--compiler',default='clang++')
    ap.add_argument('--optimization',choices=['0','2'],default='0')
    ap.add_argument('--sanitize',action='store_true')
    ap.add_argument('--sdk-source-root',type=Path,help='Optional original pinned SDK source root; NEVER a game directory')
    ap.add_argument('--sdk-binary-root',type=Path,help='Optional prebuilt SDK include/lib root for a real memory-only probe')
    ap.add_argument('--probe-protection-fault',action='store_true',help='Opt in to a separate bounded deliberate read-only write child')
    args=ap.parse_args();work=args.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('Output must remain outside the checkout')
    work.mkdir(parents=True,exist_ok=True);results=[]
    def run(cmd,label,cwd=work):
        r=subprocess.run(cmd,cwd=cwd,text=True,capture_output=True,timeout=45)
        (work/(label+'.stdout')).write_text(r.stdout);(work/(label+'.stderr')).write_text(r.stderr)
        if r.returncode:raise RuntimeError(label+' exit '+str(r.returncode)+'\n'+r.stderr)
        return r
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-pthread','-O'+args.optimization]
    if args.sanitize:flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer']
    run([args.compiler,*flags,'-iquote',str(ROOT),str(ROOT/'tests/runtime/harness.cpp'),'-o',str(work/'foundation')],'build-foundation')
    r=run([str(work/'foundation')],'foundation')
    results.extend(json.loads(l) for l in r.stdout.splitlines() if l.startswith('{'))
    if args.sdk_source_root:
        original={}
        for path,sha in PINS.items():
            raw=(args.sdk_source_root/path).read_bytes()
            if hashlib.sha256(raw).hexdigest()!=sha:raise RuntimeError('SDK source fingerprint mismatch: '+path)
            original[path]=raw.decode();dest=work/'sdk'/path;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(raw)
        run(['git','apply','--check',str(ROOT/'patches/rexglue-sdk/0001-runtime-safety.patch')],'patch-check',work/'sdk')
        run(['git','apply',str(ROOT/'patches/rexglue-sdk/0001-runtime-safety.patch')],'patch-apply',work/'sdk')
        patched={path:(work/'sdk'/path).read_text() for path in PINS}
        for label,sources,klass in [('original',original,'OriginalThread'),('patched',patched,'PatchedThread')]:
            body=''.join(function(sources['src/system/xthread.cpp'],'bool XThread::'+name) for name in ['GetTLSValue','SetTLSValue'])
            (work/(label+'_tls.inc')).write_text(body.replace('XThread::',klass+'::'))
            io=sources['src/kernel/xboxkrnl/xboxkrnl_io.cpp']
            body=''.join(function(io,'u32 '+name+'_entry') for name in ['StfsCreateDevice','StfsControlDevice'])
            if label=='original':body=body.replace('StfsCreateDevice_entry','OriginalStfsCreateDevice_entry').replace('StfsControlDevice_entry','OriginalStfsControlDevice_entry')
            (work/(label+'_stfs.inc')).write_text(body)
        (work/'protect_flags.inc').write_text(function(original['src/kernel/xboxkrnl/xboxkrnl_memory.cpp'],'uint32_t FromXdkProtectFlags'))
        leaf_flags=[x for x in flags if x!='-Werror']+['-Wno-unused-parameter']
        run([args.compiler,*leaf_flags,'-I'+str(work),str(ROOT/'tests/runtime/sdk_leaf_adapter.cpp'),'-o',str(work/'sdk-leaf')],'build-sdk-leaf')
        r=run([str(work/'sdk-leaf')],'sdk-leaf');results.extend(json.loads(l) for l in r.stdout.splitlines() if l.startswith('{'))
    if args.sdk_binary_root:
        sdk=args.sdk_binary_root.resolve()
        run([args.compiler,'-std=c++23','-DSPDLOG_FMT_EXTERNAL','-O0','-I'+str(sdk/'include'),
             str(ROOT/'tests/runtime/sdk_memory_probe.cpp'),'-L'+str(sdk/'lib'),'-lrexruntime',
             '-Wl,-rpath,'+str(sdk/'lib'),'-o',str(work/'sdk-memory')],'build-sdk-memory')
        r=run([str(work/'sdk-memory')],'sdk-memory')
        results.extend(json.loads(l) for l in r.stdout.splitlines() if l.startswith('{'))
        if args.probe_protection_fault:
            def limits():
                resource.setrlimit(resource.RLIMIT_CORE,(0,0))
                resource.setrlimit(resource.RLIMIT_CPU,(30,30))
            try:
                r=subprocess.run([str(work/'sdk-memory'),'--fault-readonly'],cwd=work,capture_output=True,text=True,timeout=45,preexec_fn=limits)
                (work/'sdk-readonly-fault.stderr').write_text(r.stderr)
                if r.returncode not in [-signal.SIGSEGV,-signal.SIGBUS]:
                    raise RuntimeError('Native read-only write did not fault as expected: '+str(r.returncode))
                results.append({'test':'prebuilt_sdk_os_readonly_enforcement','result':'PASS','signal':-r.returncode})
            except subprocess.TimeoutExpired:
                results.append({'test':'prebuilt_sdk_os_readonly_enforcement','result':'UNKNOWN','reason':'bounded child timeout; killed and reaped'})
    summary={'compiler':run([args.compiler,'--version'],'compiler').stdout.splitlines()[0],
             'optimization':args.optimization,'sanitizers':args.sanitize,'sdk_leaf_tests':bool(args.sdk_source_root),'status':'PARTIAL' if any(r.get('result')=='UNKNOWN' for r in results) else 'PASS', 'results':results}
    (work/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps(summary,indent=2))


if __name__=='__main__':
    try:main()
    except (RuntimeError,subprocess.TimeoutExpired) as exc:
        print(str(exc),file=sys.stderr);sys.exit(1)
