#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Private CLI relink: reuse pinned built objects, replace ONE patched command.

Reads existing Ninja commands, never runs Ninja build rules or modifies the SDK.
Alternatively apply horizon-leaf.patch to a fresh 19-patch SDK and build normally.
All subprocesses bounded externally. No game inputs.
"""
import argparse,hashlib,json,shlex,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run

def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for n in ['sdk-source','sdk-build','sdk-stage','ninja','workdir']:ap.add_argument('--'+n,type=Path,required=True)
    a=ap.parse_args();work=a.workdir.resolve();sdk=a.sdk_source.resolve();build=a.sdk_build.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output required')
    work.mkdir(parents=True,exist_ok=False)
    lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
    if subprocess.check_output(['git','rev-parse','HEAD'],cwd=sdk,text=True).strip()!=lock['sdk_revision']:raise ValueError('SDK revision mismatch')
    leaf_lock=json.loads((ROOT/'tests/horizon-runtime/lock.json').read_text())
    if sha(sdk/'out/mac-arm64/Release/rexglue')!=leaf_lock['original_cli_sha256']:raise ValueError('original CLI changed')
    if sha(sdk/'src/rexglue/commands/test_recompiler.cpp')!=leaf_lock['sdk_command_sha256']:raise ValueError('SDK command changed')
    if sha(ROOT/'patches/rexglue-sdk/horizon-leaf.patch')!=leaf_lock['patch_sha256']:raise ValueError('leaf patch lock changed')
    patches=json.loads((ROOT/lock['sdk_lock']).read_text())['patches']
    verification=work/'verified-sdk';verification.mkdir()
    touched=set()
    for patch in patches:
        if sha(ROOT/patch['path'])!=patch['sha256']:raise ValueError('patch lock changed')
        touched.update(line[6:] for line in (ROOT/patch['path']).read_text().splitlines() if line.startswith('+++ b/'))
    for name in sorted(touched):
        r=subprocess.run(['git','show','HEAD:'+name],cwd=sdk,capture_output=True,timeout=10)
        if r.returncode==0:
            dest=verification/name;dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(r.stdout)
    for patch in patches:
        subprocess.run(['git','apply',str(ROOT/patch['path'])],cwd=verification,check=True,capture_output=True,timeout=10)
    for name in sorted(touched):
        if sha(verification/name)!=sha(sdk/name):raise ValueError('patched SDK source mismatch: '+name)
    stage=a.sdk_stage.resolve();meta=json.loads((stage/'fixture-sdk.json').read_text())
    if meta['sdk_revision']!=lock['sdk_revision'] or sha(stage/'lib'/meta['runtime_filename'])!=meta['runtime_sha256']:raise ValueError('SDK stage mismatch')
    for name,expected in leaf_lock['sdk_context_headers_sha256'].items():
        if sha(sdk/'include'/name)!=expected or sha(stage/'include'/name)!=expected:raise ValueError('context header mismatch: '+name)
    overlay=work/'sdk';overlay.mkdir()
    # Only two open-source SDK files are patched, in a private copy.
    paths=['include/rex/platform.h','src/rexglue/commands/test_recompiler.cpp']
    before={p:sha(sdk/p) for p in paths}
    for p in paths:
        dest=overlay/p;dest.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(sdk/p,dest)
    shutil.copytree(sdk/'src/rexglue',overlay/'src/rexglue',dirs_exist_ok=True)
    subprocess.run(['git','apply',str(ROOT/'patches/rexglue-sdk/horizon-leaf.patch')],cwd=overlay,check=True,timeout=10)
    records=[]
    def execute(cmd,label):
        # watchdog sets private cwd; commands use absolute source/object paths.
        r=run(cmd,work,label,30);records.append(r)
        if r['exit_code'] or r['timeout']:raise RuntimeError(label+' failed; inspect private logs')
    obj='src/rexglue/CMakeFiles/rexglue.dir/Release/commands/test_recompiler.cpp.o'
    def command(target):
        text=subprocess.check_output([str(a.ninja.resolve()),'-f','build-Release.ninja','-t','commands',target],cwd=build,text=True,timeout=10).splitlines()[-1]
        return shlex.split(text)
    args=command(obj);source=str(sdk/'src/rexglue/commands/test_recompiler.cpp')
    for i,t in enumerate(args):
        if t==source:args[i]=str(overlay/'src/rexglue/commands/test_recompiler.cpp')
        if t==obj:args[i]=str(work/'test_recompiler.o')
        if t==obj+'.d':args[i]=str(work/'test_recompiler.o.d')
    execute(args,'compile-command')
    args=command('rexglue')
    if args[:2]!=[':','&&'] or args[-2:]!=['&&',':']:raise ValueError('unexpected Ninja link command')
    args=args[2:-2]
    for i,t in enumerate(args):
        if t==obj:args[i]=str(work/'test_recompiler.o')
        elif t.endswith('.o') and not t.startswith('/'):args[i]=str(build/t)
    output=args.index('-o')+1;args[output]=str(work/'rexglue')
    execute(args,'link-cli')
    shutil.copytree(stage/'include',work/'include')
    shutil.copyfile(overlay/'include/rex/platform.h',work/'include/rex/platform.h')
    if before!={p:sha(sdk/p) for p in paths}:raise ValueError('original SDK changed')
    receipt={'sdk_revision':meta['sdk_revision'],'sdk_runtime_sha256':meta['runtime_sha256'],
             'cli_sha256':sha(work/'rexglue'),'patch_sha256':sha(ROOT/'patches/rexglue-sdk/horizon-leaf.patch'),
             'sdk_files_before_after':before,'records':records}
    (work/'prepared.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='records'},indent=2))
if __name__=='__main__':main()
