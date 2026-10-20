#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""External monotonic process-group watchdog; logs stay outside the checkout."""
import json
import os
from pathlib import Path
import resource
import signal
import subprocess
import sys
import time
import threading


def run(command, directory, label, limit=30, sample=False):
    if not 0 < limit <= 30:
        raise ValueError('child budget must be in (0, 30] seconds')
    directory = Path(directory)
    directory.mkdir(parents=True, exist_ok=True)
    def limits():
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    start = time.monotonic()
    deadline = start + limit
    snapshots = []
    sampler = None
    captured = {}
    def drain(pipe, suffix):
        total = 0
        with pipe, (directory / (label + suffix)).open('wb') as log:
            while True:
                data = pipe.read(8192)
                if not data: break
                if total < 65536: log.write(data[:65536-total])
                total += len(data)
        captured[suffix] = total
    child = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, cwd=directory,
                             start_new_session=True, preexec_fn=limits)
    readers = [threading.Thread(target=drain, args=(child.stdout, '.stdout'), daemon=True),
               threading.Thread(target=drain, args=(child.stderr, '.stderr'), daemon=True)]
    for reader in readers: reader.start()
    timed_out = False
    next_snapshot = start + 2
    while True:
        pid, status, usage = os.wait4(child.pid, os.WNOHANG)
        if pid:
            child.returncode = os.waitstatus_to_exitcode(status)
            break
        now = time.monotonic()
        if now >= deadline:
            timed_out = True
            # Do not wait for cooperative termination of a looping signal handler.
            try:
                os.killpg(child.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass
            _, status, usage = os.wait4(child.pid, 0)
            child.returncode = os.waitstatus_to_exitcode(status)
            break
        if now >= next_snapshot:
            try:
                p = subprocess.run(['ps', '-o', 'pid,pgid,state,rss,pcpu,time,comm', '-p', str(child.pid)],
                                   capture_output=True, text=True, timeout=min(1, deadline-now))
                snapshots.append(p.stdout)
                if sample and sampler is None and sys.platform == 'darwin':
                    sampler = subprocess.Popen(['/usr/bin/sample', str(child.pid), '1', '10', '-file',
                                                str(directory/(label+'.sample.txt'))],
                                               stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                               start_new_session=True)
            except subprocess.TimeoutExpired:
                pass
            next_snapshot = now + 5
        time.sleep(min(.02, max(0, deadline-time.monotonic())))
    elapsed = time.monotonic()-start
    # Also remove descendants when the group leader exited independently.
    if not timed_out:
        try:
            os.killpg(child.pid, signal.SIGKILL)
        except ProcessLookupError:
            pass
    for reader in readers: reader.join(timeout=1)
    if sampler is not None:
        try:
            sampler.wait(timeout=2)
        except subprocess.TimeoutExpired:
            os.killpg(sampler.pid, signal.SIGKILL)
            sampler.wait()
    rss = usage.ru_maxrss if sys.platform == 'darwin' else usage.ru_maxrss*1024
    record = {'label': label, 'exit_code': child.returncode, 'timeout': timed_out,
              'limit_seconds': limit, 'seconds': elapsed, 'peak_rss_bytes': rss,
              'user_seconds': usage.ru_utime, 'system_seconds': usage.ru_stime,
              'cpu_percent': 100*(usage.ru_utime+usage.ru_stime)/elapsed,
              'process_group': child.pid, 'thread_snapshots': snapshots,
              'output_bytes': captured, 'retained_bytes_per_stream': 65536}
    (directory/(label+'.json')).write_text(json.dumps(record, indent=2)+'\n')
    return record
