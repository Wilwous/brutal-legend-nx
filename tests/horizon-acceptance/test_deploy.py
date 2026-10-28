#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (c) 2026 Wilwous
"""Mock SD tests only; never enumerate or access devices."""
import os,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2];sys.path.insert(0,str(ROOT/'tools/switch'));import deploy as usb
APP='brutal-legend-nx-acceptance'
class Separate(unittest.TestCase):
 def setUp(self):
  t=tempfile.TemporaryDirectory();self.addCleanup(t.cleanup);self.root=Path(t.name)
  for name in ['switch/brutal-legend-nx','switch/brutal-legend-nx-ppc','switch/brutal-legend-nx-runtime','switch/brutal-legend-nx-runtime-ii','atmosphere','bootloader']:(self.root/name).mkdir(parents=True)
  self.preserved=['switch/brutal-legend-nx/brutal-legend-nx.nro','switch/brutal-legend-nx-ppc/brutal-legend-nx-ppc.nro','switch/brutal-legend-nx-runtime/brutal-legend-nx-runtime.nro','switch/brutal-legend-nx-runtime-ii/brutal-legend-nx-runtime-ii.nro','atmosphere/package3','bootloader/hekate_ipl.ini']
  for name in self.preserved:(self.root/name).write_bytes(b'preserve')
 def tearDown(self):
  for name in self.preserved:self.assertEqual((self.root/name).read_bytes(),b'preserve')
 def deploy(self,replace=False):
  fd=usb.directory(self.root)
  try:return usb.deploy_fd(fd,b'original synthetic runtime',lambda:None,application=APP,allow_replace=replace)
  finally:os.close(fd)
 def test_separate(self):self.assertEqual(self.deploy()['status'],'COPIED_VERIFIED')
 def test_repeat(self):self.deploy();self.assertEqual(self.deploy()['status'],'ALREADY_CURRENT_VERIFIED')
 def test_refuse_replacement(self):
  folder=self.root/'switch'/APP;folder.mkdir();(folder/(APP+'.nro')).write_bytes(b'previous')
  with self.assertRaises(usb.Refused):self.deploy()
  self.assertEqual((folder/(APP+'.nro')).read_bytes(),b'previous')
 def test_selected_backup(self):
  folder=self.root/'switch'/APP;folder.mkdir();(folder/(APP+'.nro')).write_bytes(b'previous');row=self.deploy(True);self.assertEqual((folder/row['backup']).read_bytes(),b'previous')
 def test_symlink(self):
  (self.root/'switch'/APP).symlink_to(self.root/'switch/brutal-legend-nx',target_is_directory=True)
  with self.assertRaises(OSError):self.deploy()
 def test_foreign_name(self):
  with self.assertRaises(usb.Refused):usb.deployment_names('../runtime')
if __name__=='__main__':unittest.main()
