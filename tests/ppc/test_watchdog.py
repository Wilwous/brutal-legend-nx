#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Original supervision regressions. No SDK or game files required."""
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from watchdog import run

class WatchdogTests(unittest.TestCase):
    def test_explicit_exit(self):
        with tempfile.TemporaryDirectory() as d:
            r=run([sys.executable,'-c','raise SystemExit(7)'],d,'exit')
            self.assertEqual(r['exit_code'],7);self.assertFalse(r['timeout'])
    def test_signal(self):
        with tempfile.TemporaryDirectory() as d:
            r=run([sys.executable,'-c','import os,signal;os.kill(os.getpid(),signal.SIGTERM)'],d,'signal')
            self.assertEqual(r['exit_code'],-signal.SIGTERM);self.assertFalse(r['timeout'])
    def test_output_cap(self):
        with tempfile.TemporaryDirectory() as d:
            r=run([sys.executable,'-c','import os;os.write(1,b"x"*200000);os.write(2,b"y"*200000)'],d,'output')
            self.assertEqual(r['exit_code'],0)
            self.assertEqual(r['output_bytes'],{'.stdout':200000,'.stderr':200000})
            for suffix in ['.stdout','.stderr']:self.assertEqual((Path(d)/('output'+suffix)).stat().st_size,65536)
    def check_descendant(self, leader_exits):
        with tempfile.TemporaryDirectory() as d:
            code = ('import os,time,pathlib\n'
                    'pid=os.fork()\n'
                    'if pid==0:\n'
                    ' pathlib.Path("descendant.pid").write_text(str(os.getpid()))\n'
                    ' time.sleep(10)\n'
                    'else:\n'
                    ' while not pathlib.Path("descendant.pid").exists(): time.sleep(.001)\n'
                    ' if '+str(leader_exits)+': os._exit(0)\n'
                    ' time.sleep(10)\n')
            r=run([sys.executable,'-c',code],d,'group',.5)
            self.assertEqual(r['timeout'],not leader_exits)
            self.assertLess(r['seconds'],2)
            pid=int((Path(d)/'descendant.pid').read_text())
            for _ in range(20):
                state=subprocess.run(['ps','-o','stat=','-p',str(pid)],capture_output=True,text=True,timeout=1).stdout.strip()
                if not state or state.startswith('Z'):break
                time.sleep(.01)
            self.assertTrue(not state or state.startswith('Z'),state)
    def test_timeout_kills_process_group(self):self.check_descendant(False)
    def test_exited_leader_descendant_cleanup(self):self.check_descendant(True)
    def test_budget_rejection(self):
        for budget in [0,31]:
            with self.assertRaises(ValueError):run([],'.','invalid',budget)
if __name__=='__main__':unittest.main()
