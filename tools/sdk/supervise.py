#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Bounded build supervision, compact logs and wait4 resource accounting."""
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import threading
import time


def command(argv, work, label, budget=1200, env=None, cwd=None):
    work=Path(work);work.mkdir(parents=True,exist_ok=True)
    start=time.monotonic();calendar=time.time();capture={}
    def limit():resource.setrlimit(resource.RLIMIT_CORE,(0,0))
    child=subprocess.Popen(argv,cwd=cwd or work,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                           start_new_session=True,preexec_fn=limit)
    def drain(stream,suffix):
        first=bytearray();tail=bytearray();total=0
        with stream:
            while True:
                block=stream.read(8192)
                if not block:break
                first.extend(block[:max(0,65536-len(first))]);tail.extend(block);tail[:]=tail[-65536:];total+=len(block)
        output=bytes(first) if total<=65536 else bytes(first)+b'\n[... bounded log; final bytes follow ...]\n'+bytes(tail)
        (work/(label+suffix)).write_bytes(output);capture[suffix]=total
    threads=[threading.Thread(target=drain,args=(child.stdout,'.stdout')),threading.Thread(target=drain,args=(child.stderr,'.stderr'))]
    for thread in threads:thread.start()
    timed_out=False
    while True:
        pid,status,usage=os.wait4(child.pid,os.WNOHANG)
        if pid:break
        # Calendar is an additional ceiling; monotonic remains the primary deadline.
        if time.monotonic()-start>=budget or time.time()-calendar>=budget:
            timed_out=True
            try:os.killpg(child.pid,signal.SIGKILL)
            except ProcessLookupError:pass
            _,status,usage=os.wait4(child.pid,0);break
        time.sleep(.05)
    child.returncode=os.waitstatus_to_exitcode(status)
    if not timed_out:
        try:os.killpg(child.pid,signal.SIGKILL)
        except ProcessLookupError:pass
    for thread in threads:thread.join(timeout=2)
    result={'label':label,'exit_code':child.returncode,'timeout':timed_out,'budget_seconds':budget,
            'seconds':time.monotonic()-start,'calendar_seconds':time.time()-calendar,
            'peak_rss_bytes':usage.ru_maxrss*(1 if sys.platform=='darwin' else 1024),
            'user_seconds':usage.ru_utime,'system_seconds':usage.ru_stime,'output_bytes':capture}
    (work/(label+'.json')).write_text(json.dumps(result,indent=2)+'\n')
    if child.returncode or timed_out:raise RuntimeError(label+' failed; see private diagnostics')
    return result
