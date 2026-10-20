#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Build the pinned runtime + codegen SDK privately. No game inputs; no global install."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import urllib.request
from supervise import command
from stage import stage
ROOT=Path(__file__).resolve().parents[2]
LOCK=json.loads((Path(__file__).parent/'lock.json').read_text())


def output(args,cwd):return subprocess.check_output(args,cwd=cwd,text=True,timeout=30).strip()


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--workspace',required=True,type=Path)
    ap.add_argument('--cmake',required=True,type=Path)
    ap.add_argument('--ninja',required=True,type=Path)
    ap.add_argument('--source-cache',type=Path,help='Optional explicit pinned git checkout for local clone and submodule object reuse')
    ap.add_argument('--jobs',type=int,choices=[1,2],default=2)
    ap.add_argument('--profiles',nargs='+',choices=['Debug','Release','Sanitized'],default=['Debug','Release','Sanitized'])
    ap.add_argument('--clean',action='store_true',help='Remove only owned build/staging output, retaining pinned source/cache')
    a=ap.parse_args();work=a.workspace.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('workspace must be outside public checkout')
    if sys.platform!='darwin':ap.error('this locked build profile currently targets macOS ARM64')
    if output(['uname','-m'],ROOT)!='arm64':ap.error('ARM64 host required for this profile')
    marker=work/'unified-sdk-workspace.json'
    if work.exists() and any(work.iterdir()) and not marker.exists():ap.error('nonempty workspace lacks ownership marker')
    if marker.exists() and json.loads(marker.read_text()).get('owner')!='brutal-legend-nx-phase7':ap.error('workspace owner mismatch')
    work.mkdir(parents=True,exist_ok=True);marker.write_text(json.dumps({'owner':'brutal-legend-nx-phase7','revision':LOCK['sdk_revision']})+'\n')
    for patch in LOCK['patches']:
        if hashlib.sha256((ROOT/patch['path']).read_bytes()).hexdigest()!=patch['sha256']:raise RuntimeError('patch hash mismatch: '+patch['path'])
    cmake,ninja=map(lambda p:str(p.resolve()),(a.cmake,a.ninja))
    versions={name:output([path,'--version'],work).splitlines()[0] for name,path in [('cmake',cmake),('ninja',ninja),('compiler','/usr/bin/clang++'),('git','/usr/bin/git')]}
    if LOCK['tools']['cmake']['version'] not in versions['cmake'] or versions['ninja']!=LOCK['tools']['ninja']['binary_version']:raise RuntimeError('locked build tool version mismatch')
    env={k:v for k,v in os.environ.items() if not k.startswith(('DYLD_','CMAKE_','REXSDK','VULKAN_','VK_','CPM_'))}
    env['PATH']=str(Path(ninja).parent)+':/usr/bin:/bin:/usr/sbin:/sbin';env['CPM_SOURCE_CACHE']=str(work/'cpm-cache')
    previous=work/'build-results.json'
    if previous.exists():
        history=work/'build-history';history.mkdir(exist_ok=True)
        digest=hashlib.sha256(previous.read_bytes()).hexdigest()[:16]
        shutil.copy2(previous,history/(digest+'.json'))
    source=work/'sdk';records=[]
    if not source.exists():
        origin=str(a.source_cache.resolve()) if a.source_cache else LOCK['sdk_url']
        records.append(command(['git','clone','--depth','1','--branch',LOCK['sdk_tag'],origin,str(source)],work,'clone',180,env))
        if output(['git','rev-parse','HEAD'],source)!=LOCK['sdk_revision']:raise RuntimeError('SDK revision mismatch')
        if a.source_cache:
            for module in LOCK['submodules']:
                command(['git','config','submodule.'+module['path']+'.url',str(a.source_cache.resolve()/module['path'])],work,'cache-'+Path(module['path']).name,30,env,cwd=source)
        records.append(command(['git','-c','protocol.file.allow=always','submodule','update','--init','--depth','1','--jobs','2'],work,'submodules',300,env,cwd=source))
        for patch in LOCK['patches']:
            command(['git','apply','--check',str(ROOT/patch['path'])],work,'check-'+Path(patch['path']).stem,30,env,cwd=source)
            command(['git','apply',str(ROOT/patch['path'])],work,'apply-'+Path(patch['path']).stem,30,env,cwd=source)
    if output(['git','rev-parse','HEAD'],source)!=LOCK['sdk_revision']:raise RuntimeError('SDK revision mismatch')
    for module in LOCK['submodules']:
        if output(['git','rev-parse','HEAD'],source/module['path'])!=module['commit']:raise RuntimeError('dependency mismatch: '+module['path'])
        if output(['git','status','--porcelain'],source/module['path']):raise RuntimeError('dirty dependency: '+module['path'])
    if output(['git','ls-files','--others','--exclude-standard'],source):raise RuntimeError('untracked source files')
    # Verify every patched file against a temporary pristine git archive plus ordered patches.
    verify=work/'verify-source';verify.mkdir(exist_ok=True)
    changed=output(['git','diff','--name-only'],source).splitlines()
    for name in changed:
        dest=verify/name;dest.parent.mkdir(parents=True,exist_ok=True)
        dest.write_bytes(subprocess.check_output(['git','show','HEAD:'+name],cwd=source))
    for patch in LOCK['patches']:command(['git','apply',str(ROOT/patch['path'])],work,'verify-'+Path(patch['path']).stem,30,env,cwd=verify)
    for name in changed:
        if (verify/name).read_bytes()!=(source/name).read_bytes():raise RuntimeError('unexpected source edit: '+name)
    shutil.rmtree(verify)
    build=work/'build';staged=work/'staged'
    if a.clean:
        for path in [build,staged,source/'out']:
            if path.exists():shutil.rmtree(path)
    env['SOURCE_DATE_EPOCH']=output(['git','show','-s','--format=%ct','HEAD'],source)
    san='-O1 -g -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer'
    bootstrap=work/'cpm-cache/cpm/CPM_0.40.8.cmake'
    if not bootstrap.exists():
        bootstrap.parent.mkdir(parents=True,exist_ok=True)
        with urllib.request.urlopen(LOCK['cpm_bootstrap']['url'],timeout=30) as response:raw=response.read()
        if hashlib.sha256(raw).hexdigest()!=LOCK['cpm_bootstrap']['sha256']:raise RuntimeError('CPM bootstrap hash mismatch')
        bootstrap.write_bytes(raw)
    if hashlib.sha256(bootstrap.read_bytes()).hexdigest()!=LOCK['cpm_bootstrap']['sha256']:raise RuntimeError('CPM bootstrap hash mismatch')
    args=[cmake,'-S',str(source),'-B',str(build),'-G','Ninja Multi-Config','-DCMAKE_MAKE_PROGRAM='+ninja,
          '-DCMAKE_C_COMPILER=/usr/bin/clang','-DCMAKE_CXX_COMPILER=/usr/bin/clang++','-DCMAKE_OBJC_COMPILER=/usr/bin/clang',
          '-DCMAKE_OBJCXX_COMPILER=/usr/bin/clang++','-DCMAKE_OSX_ARCHITECTURES=arm64',
          '-DCMAKE_CONFIGURATION_TYPES=Debug;Release;Sanitized','-DREXGLUE_ENABLE_TRACY=OFF','-DREXGLUE_ENABLE_PERF_COUNTERS=OFF',
          '-DCMAKE_INSTALL_PREFIX='+str(work/'unused-install'),'-DCPM_SOURCE_CACHE='+str(work/'cpm-cache'),
          '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON']
    for lang in ['C','CXX','OBJC','OBJCXX']:args+=['-DCMAKE_'+lang+'_FLAGS_SANITIZED='+san]
    for kind in ['EXE','SHARED','MODULE']:args+=['-DCMAKE_'+kind+'_LINKER_FLAGS_SANITIZED=-fsanitize=address,undefined']
    records.append(command(args,work,'configure',180,env))
    # These CPM source revisions are inherited from the pinned MoltenVK revision.
    cpm_sources={}
    for name,sha in LOCK['cpm'].items():
        matches=list((work/'cpm-cache').rglob(name+'/.git'))
        if len(matches)!=1:raise RuntimeError('ambiguous CPM source: '+name)
        directory=matches[0].parent
        if output(['git','rev-parse','HEAD'],directory)!=sha:raise RuntimeError('CPM revision mismatch: '+name)
        if output(['git','status','--porcelain'],directory):raise RuntimeError('dirty CPM dependency: '+name)
        cpm_sources[name]=sha
    profiles=[]
    for profile in a.profiles:
        rec=command([cmake,'--build',str(build),'--config',profile,'--target','rexruntime','rexcodegen','rexglue','--parallel',str(a.jobs)],work,'build-'+profile,1200,env)
        records.append(rec);profiles.append(stage(source,build,staged/profile,profile))
        print(json.dumps({'profile':profile,'seconds':rec['seconds'],'peak_rss_bytes':rec['peak_rss_bytes']}),flush=True)
    result={'revision':LOCK['sdk_revision'],'versions':versions,'submodules':LOCK['submodules'],'cpm':cpm_sources,
            'patches':LOCK['patches'],'records':records,'profiles':profiles,'clean':a.clean,
            'targets':['rexruntime','rexcodegen','rexglue'],'gpu_plugin_built':False,
            'uninstrumented':['system frameworks','handwritten assembly; sanitizer passes cannot instrument assembly']}
    (work/('clean-build-results.json' if a.clean else 'build-results.json')).write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':
    try:main()
    except (RuntimeError,OSError,subprocess.SubprocessError) as exc:print(str(exc),file=sys.stderr);sys.exit(1)
