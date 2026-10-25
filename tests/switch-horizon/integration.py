#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original four-instruction PPC -> real pinned CLI -> Horizon compile-only probe.

No fake SDK API, platform spoofing, edited output, linking or guest execution.
The pinned desktop SDK is expected to reject Horizon; retain exact diagnostics.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/ppc'))
from watchdog import run
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ['workdir','sdk','rexglue']:ap.add_argument('--'+name,type=Path,required=True)
    a=ap.parse_args();work=a.workdir.resolve();sdk=a.sdk.resolve();cli=a.rexglue.resolve()
    if work==ROOT or ROOT in work.parents:ap.error('private output directory required')
    lock=json.loads((ROOT/'tests/switch-horizon/toolchain.json').read_text())
    metadata=json.loads((sdk/'fixture-sdk.json').read_text())
    if metadata['sdk_revision']!=lock['sdk_revision']:raise RuntimeError('SDK revision mismatch')
    if sha(sdk/'lib'/metadata['runtime_filename'])!=metadata['runtime_sha256']:raise RuntimeError('SDK stage changed')
    work.mkdir(parents=True,exist_ok=False)
    source=work/'input';source.mkdir();output=work/'generated';output.mkdir()
    (source/'leaf.bin').write_bytes(bytes.fromhex('8064000038630001906400004e800020'))
    (source/'leaf.map').write_text('00000000 T test_leaf\n')
    (source/'leaf.s').write_text('test_leaf:\n    lwz r3,0(r4)\n    addi r3,r3,1\n    stw r3,0(r4)\n    blr\n')
    generation=run([str(cli),'recompile-tests','--bin-dir',str(source),'--asm-dir',str(source),'--output',str(output)],work,'generation',30)
    if generation['timeout'] or generation['exit_code']:raise RuntimeError('actual CLI generation failed')
    cpp=output/'ppc_test_functions.cpp';pch=output/'ppc_config.h';body=cpp.read_text()
    counts={key:body.count(key) for key in ['DEFINE_REX_FUNC(','REX_LOAD_U32(','REX_STORE_U32(']}
    if list(counts.values())!=[1,1,1] or len(re.findall(r'^\t// ',body,re.M))!=4:
        raise RuntimeError('original emission census changed')
    if 'REX_CHECKED_ACCESS_HEADER' not in pch.read_text():raise RuntimeError('checked seam absent')
    before={p.name:sha(p) for p in [cpp,pch]}
    # Only public open-source headers, never desktop libraries or title output.
    shutil.copytree(sdk/'include',work/'sdk-include')
    name='brutal-ppc-compile-'+uuid.uuid4().hex[:12]
    cmd=['docker','run','--rm','--pull','never','--platform',lock['platform'],'--name',name,
         '--network','none','--cap-drop','ALL','--security-opt','no-new-privileges',
         '--pids-limit','128','--memory','1g','--cpus','2','--read-only','--tmpfs','/tmp:rw,size=64m',
         '--mount',f'type=bind,src={ROOT},dst=/src,readonly','--mount',f'type=bind,src={work},dst=/probe',
         '-w','/probe',lock['image'],'/opt/devkitpro/devkitA64/bin/aarch64-none-elf-g++',
         '-std=gnu++23','-O2','-march=armv8-a+crc+crypto','-mtune=cortex-a57','-mtp=soft','-fPIE',
         '-D__SWITCH__','-DSPDLOG_FMT_EXTERNAL','-DREX_CHECKED_ACCESS_HEADER="runtime/generated_access.h"',
         '-fmax-errors=8','-isystem','/opt/devkitpro/libnx/include','-isystem','/probe/sdk-include',
         '-I/probe/generated','-iquote','/src','-c','/probe/generated/ppc_test_functions.cpp','-o','/probe/leaf.o']
    try:compilation=run(cmd,work,'horizon-compile',30)
    finally:
        cleanup=subprocess.run(['docker','rm','-f',name],capture_output=True,text=True,timeout=10)
        if cleanup.returncode and 'No such container' not in cleanup.stderr:raise RuntimeError('container cleanup failed')
    if compilation['timeout']:raise RuntimeError('compile timed out; outcome unknown')
    if before!={p.name:sha(p) for p in [cpp,pch]}:raise RuntimeError('generated output changed')
    errors=(work/'horizon-compile.stderr').read_text()
    if compilation['exit_code']:
        if 'Unsupported target OS' not in errors:raise RuntimeError('new unclassified compile failure; inspect diagnostics')
        classification='BLOCKED: pinned SDK does not admit Horizon'
    else:classification='COMPILE VERIFIED; linking and runtime UNKNOWN'
    result={'classification':classification,'sdk_revision':metadata['sdk_revision'],'runtime_sha256':metadata['runtime_sha256'],
            'cli_sha256':sha(cli),'input_sha256':sha(source/'leaf.bin'),'generated_sha256':before,
            'original_instructions':4,'original_functions':1,'census':counts,
            'generation':generation,'compilation':compilation,'checked_header_selected':True,
            'linked':False,'executed':False,'game_used':False}
    (work/'results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ['generation','compilation']},indent=2))
    # A reproduced, classified portability blocker is a completed audit result.
    return 0
if __name__=='__main__':
    try:sys.exit(main())
    except (OSError,ValueError,RuntimeError,subprocess.TimeoutExpired) as e:print(str(e),file=sys.stderr);sys.exit(1)
