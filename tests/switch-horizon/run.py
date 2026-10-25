#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded original host contracts; optional offline pinned-container NRO build.

Never executes an NRO. No device access, private SDK, game input or network.
Pull the digest-locked image explicitly before --switch-build.
"""
import argparse
import hashlib
import json
import os
import shutil
from pathlib import Path
import subprocess
import sys
import uuid

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--workdir',type=Path,required=True)
    ap.add_argument('--compiler',default='clang++')
    ap.add_argument('--switch-build',action='store_true')
    ap.add_argument('--icon',type=Path,help='Original JPEG to embed in the NRO')
    args=ap.parse_args(); work=args.workdir.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('output must be outside the checkout')
    work.mkdir(parents=True,exist_ok=False)
    records=[]; tests=[];ui_tests=[]
    def execute(cmd,label):
        r=run(cmd,work,label,30);records.append(r)
        if r['timeout'] or r['exit_code']:raise RuntimeError(label+' failed; inspect private logs')
        return (work/(label+'.stdout')).read_text()
    for name,flags in [('Debug',['-O0']),('Release',['-O2']),('Sanitized',['-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer'])]:
        binary=work/('host-'+name)
        execute([args.compiler,'-std=c++23','-pthread','-Wall','-Wextra','-Werror',*flags,'-iquote',str(ROOT),str(ROOT/'tests/switch-horizon/host.cpp'),str(ROOT/'tests/switch-horizon/probe.cpp'),'-o',str(binary)],'build-host-'+name)
        row=json.loads(execute([str(binary)],'run-host-'+name))
        if row!={'passed':21,'failed':0}:raise RuntimeError('host contract count/failure drift: '+str(row))
        tests.append({'profile':name,**row})
        ui=work/('ui-'+name)
        execute([args.compiler,'-std=c++23','-Wall','-Wextra','-Werror',*flags,str(ROOT/'tests/switch-horizon/ui.cpp'),'-o',str(ui)],'build-ui-'+name)
        row=json.loads(execute([str(ui)],'run-ui-'+name))
        if row!={'passed':6,'failed':0}:raise RuntimeError('UI policy failure/count drift')
        ui_tests.append({'profile':name,**row})
    summary={'host':tests,'ui_host':ui_tests,'records':records,'nro_executed':False,'game_executed':False}
    if args.switch_build:
        lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
        # No privileged container, networking, host devices or Docker socket mount.
        def container(out,command,label):
            name='brutal-switch-'+uuid.uuid4().hex[:12]
            cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,
                 '--network','none','--cap-drop','ALL','--security-opt','no-new-privileges',
                 '--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m',
                 '--mount',f'type=bind,src={ROOT},dst=/src,readonly','--mount',f'type=bind,src={out},dst=/build',
                 '-w','/build',lock['image'],*command]
            try:return execute(cmd,label)
            finally:
                # Killing the Docker client alone would not terminate a container.
                cleanup=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
                if cleanup.returncode and 'No such container' not in cleanup.stderr:
                    raise RuntimeError('container cleanup failed: '+cleanup.stderr)
        outputs=[]
        for i in range(2):
            out=work/('switch-'+str(i));out.mkdir()
            icon_flags=[]
            if args.icon:
                shutil.copyfile(args.icon,out/'icon.jpg');icon_flags=['ICON=/build/icon.jpg']
            container(out,['make','--no-print-directory','-f','/src/tests/switch-horizon/Makefile','REPO=/src',*icon_flags,'-j2'],'nro-build-'+str(i))
            container(out,['python3','/src/tests/switch-horizon/inspect_nro.py'],'nro-inspect-'+str(i))
            outputs.append({name:hashlib.sha256((out/name).read_bytes()).hexdigest() for name in ['probe.elf','probe.nro']})
        if outputs[0]!=outputs[1]:raise RuntimeError('two clean builds differ')
        summary['switch']={'classification':'BUILD VERIFIED / RUNTIME UNKNOWN','sha256':outputs[0],
                           'inspection':json.loads((work/'switch-0/inspection.json').read_text()),'deterministic_builds':2}
    summary['records']=records
    inputs=['tools/switch/sanitize_elf.py']+['runtime/'+n for n in ['checked_memory.h','checked_tls.h','cooperative_stop.h']]+['tests/switch-horizon/'+n for n in ['probe.h','probe.cpp','main.cpp','result_ui.h','ui.cpp','host.cpp','Makefile','toolchain.json','run.py','inspect_nro.py']]
    summary['build_sources_sha256']={n:hashlib.sha256((ROOT/n).read_bytes()).hexdigest() for n in inputs}
    (work/'results.json').write_text(json.dumps(summary,indent=2)+'\n')
    print(json.dumps({k:v for k,v in summary.items() if k!='records'},indent=2))
    return 0
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,ValueError,RuntimeError,subprocess.TimeoutExpired) as e:print(str(e),file=sys.stderr);sys.exit(1)
